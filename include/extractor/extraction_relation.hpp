#ifndef EXTRACTION_RELATION_HPP
#define EXTRACTION_RELATION_HPP

#include "util/exception.hpp"

#include <boost/assert.hpp>
#include <boost/flyweight.hpp>
#include <boost/flyweight/flyweight.hpp>
#include <boost/flyweight/flyweight_fwd.hpp>
#include <boost/flyweight/no_tracking.hpp>

#include <oneapi/tbb/concurrent_map.h>

#include <osmium/osm/area.hpp>
#include <osmium/osm/item_type.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/object.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/osm/tag.hpp>
#include <osmium/osm/types.hpp>
#include <osmium/osm/way.hpp>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "util/log.hpp"

namespace osrm::extractor
{

namespace
{
using string = boost::flyweights::flyweight<std::string, boost::flyweights::no_tracking>;
}

/**
 * @brief A copiable RelationMember
 */
class RelationMember
{
    osmium::object_id_type m_ref;
    osmium::item_type m_type;
    string m_role;

  public:
    RelationMember(osmium::object_id_type id, osmium::item_type type) : m_ref{id}, m_type{type} {};
    RelationMember(const osmium::RelationMember &o)
        : m_ref{o.ref()}, m_type{o.type()}, m_role(o.role()){};
    osmium::object_id_type ref() const noexcept { return m_ref; }
    osmium::item_type type() const noexcept { return m_type; }
    const string &role() const noexcept { return m_role; }

    friend bool operator==(const RelationMember &m, const RelationMember &o)
    {
        return m.ref() == o.ref() && m.type() == o.type();
    }
    friend bool operator<(const RelationMember &m, const RelationMember &o)
    {
        return m.ref() < o.ref() || (m.ref() == o.ref() && m.type() < m.type());
    }
};

/**
 * @brief A copiable Relation
 *
 * Motivation: osmium entities are almost impossible to use with LUA because they have
 * no copy constructors. Eg. SOL, some 15 template invocations down, wants to
 * copy-construct an entity before giving it to LUA. This implementation duck-types an
 * `osmium::Relation` for SOL/LUA.
 *
 * Care has been taken to minimize memory consumption. The alternative to use an
 * `osmium::stash` was found to bee too heavy on memory.
 */
class Relation
{
    using members_t = std::vector<RelationMember>;
    osmium::object_id_type m_id;
    osmium::object_version_type m_version;
    std::map<string, string> m_tags;
    members_t m_members;
    bool sorted{false};

  public:
    Relation(const osmium::Relation &o) : m_id{o.id()}, m_version(o.version())
    {
        for (const osmium::Tag &tag : o.tags())
        {
            m_tags.emplace(tag.key(), tag.value());
        }
        for (const osmium::RelationMember &member : o.cmembers())
        {
            m_members.emplace_back(member);
        }
    };

    osmium::object_id_type id() const noexcept { return m_id; };
    osmium::item_type type() const noexcept { return osmium::item_type::relation; };
    osmium::object_version_type version() const noexcept { return m_version; };

    const char *get_value_by_key_default(const char *key, const char *default_value) const
    {
        // Nitpick: if somebody searches for "higway", it will be added to the flyweights
        auto found = m_tags.find(string(key));
        if (found != m_tags.end())
            return found->second->c_str();
        return default_value;
    };

    const char *get_value_by_key(const char *key) const
    {
        return get_value_by_key_default(key, nullptr);
    }

    template <class T> const char *get_member_role(const T &o)
    {
        return get_member_role(RelationMember(o.id(), o.type()));
    }

    const char *get_member_role(const RelationMember &m)
    {
        if (!sorted)
        {
            std::sort(m_members.begin(), m_members.end());
            sorted = true;
        }
        auto it = std::lower_bound(m_members.begin(), m_members.end(), m);
        if (it != m_members.end() && *it == m)
        {
            return it->role()->c_str();
        }
        return nullptr;
    }

    const members_t &members() { return m_members; };
};

// It contains data of all parsed relations for each node/way element
class ExtractionRelationContainer
{
    using rel_id_type = osmium::object_id_type;
    using member_id_type = osmium::object_id_type;

    using rel_ids_t = std::vector<rel_id_type>;
    using parent_map_t = tbb::concurrent_map<member_id_type, rel_ids_t>;

    tbb::concurrent_map<rel_id_type, Relation> relations;
    std::vector<parent_map_t> parents;

    rel_ids_t empty_rel_list{};

    parent_map_t &p(osmium::item_type t) { return parents[(size_t)t]; }
    const parent_map_t &p(osmium::item_type t) const { return parents[(size_t)t]; }

  public:
    ExtractionRelationContainer(ExtractionRelationContainer &&) = delete;
    ExtractionRelationContainer(const ExtractionRelationContainer &) = delete;
    ExtractionRelationContainer() : parents(4){};

    void AddRelation(const osmium::Relation &rel)
    {
        BOOST_ASSERT(!relations.contains(rel.id()));
        for (auto const &m : rel.members())
        {
            AddRelationMember(rel.id(), m.ref(), m.type());
        }
        relations.emplace(rel.id(), rel);
    }

    void
    AddRelationMember(rel_id_type relation_id, osmium::object_id_type id, osmium::item_type type)
    {
        p(type)[id].push_back(relation_id);
    }

    std::size_t GetRelationsNum() const { return relations.size(); }

    /**
     * @brief Returns the relations that the given object is a member of.
     *
     * @param id The id of the object
     * @param type The type of the object
     * @return const rel_ids_t& The list of relation ids
     */
    const rel_ids_t &_GetRelationsFor(osmium::object_id_type id, osmium::item_type type) const
    {
        if (type == osmium::item_type::area)
        {
            type = (id & 1) ? osmium::item_type::relation : osmium::item_type::way;
            id /= 2;
        }
        assert(type == osmium::item_type::relation || type == osmium::item_type::way ||
               type == osmium::item_type::node);

        const parent_map_t &parents = p(type);

        util::Log(logINFO) << "size of parents map: " << parents.size();

        auto it = parents.find(id);
        if (it != parents.end())
            return it->second;

        return empty_rel_list;
    }

    template <class T> const rel_ids_t &GetRelationsFor(const T &o) const
    {
        return _GetRelationsFor(o.id(), o.type());
    }

    const Relation &GetRelation(rel_id_type rel_id) const
    {
        auto it = relations.find(rel_id);
        if (it == relations.end())
            throw osrm::util::exception("Can't find relation data for " + std::to_string(rel_id));

        return it->second;
    }
};

} // namespace osrm::extractor

#endif // EXTRACTION_RELATION_HPP
