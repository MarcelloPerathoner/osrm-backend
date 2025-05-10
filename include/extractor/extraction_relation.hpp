#ifndef EXTRACTION_RELATION_HPP
#define EXTRACTION_RELATION_HPP

#include "util/exception.hpp"

#include <boost/assert.hpp>
#include <boost/flyweight.hpp>
#include <boost/flyweight/no_tracking.hpp>

#include <osmium/osm/area.hpp>
#include <osmium/osm/item_type.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/object.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/osm/types.hpp>
#include <osmium/osm/way.hpp>

#include <oneapi/tbb/concurrent_map.h>
#include <oneapi/tbb/mutex.h>

#include <osmium/storage/item_stash.hpp>
#include <string>
#include <vector>

namespace osrm::extractor
{

// It contains data of all parsed relations for each node/way element
class ExtractionRelationContainer
{
  public:
    using rel_id_type = osmium::object_id_type;
    using handle_type = osmium::ItemStash::handle_type;
    using mutex_type = tbb::mutex;

    using RelationIDList = std::vector<rel_id_type>;
    using id_and_type = std::pair<osmium::object_id_type, osmium::item_type>;

    using RelationHandleMap = tbb::concurrent_map<rel_id_type, handle_type>;
    using MemberRelationMap = tbb::concurrent_map<id_and_type, RelationIDList>;

    ExtractionRelationContainer() = default;
    ExtractionRelationContainer(ExtractionRelationContainer &&) = delete;
    ExtractionRelationContainer(const ExtractionRelationContainer &) = delete;

    void AddRelation(const osmium::Relation &rel)
    {
        BOOST_ASSERT(handles.find(rel.id()) == handles.end());
        for (auto const &m : rel.members())
        {
            AddRelationMember(rel.id(), m.ref(), m.type());
        }

        handle_type handle;
        {
            mutex_type::scoped_lock lock(mutex);
            handle = stash.add_item(rel);
        }
        handles.emplace(rel.id(), handle);
    }

    void
    AddRelationMember(rel_id_type relation_id, osmium::object_id_type id, osmium::item_type type)
    {
        parents[id_and_type{id, type}].push_back(relation_id);
    }

    std::size_t GetRelationsNum() const { return handles.size(); }

    const RelationIDList &_GetRelationsFor(osmium::object_id_type id, osmium::item_type type) const
    {
        if (type == osmium::item_type::area)
        {
            type = (id & 1) ? osmium::item_type::relation : osmium::item_type::way;
            id /= 2;
        }
        auto it = parents.find(id_and_type{id, type});
        if (it != parents.end())
            return it->second;

        return empty_rel_list;
    }

    const RelationIDList &GetRelationsFor(const osmium::OSMObject &o) const
    {
        return _GetRelationsFor(o.id(), o.type());
    }

    // Note: non-const because SOL somehow chokes on const.
    osmium::Relation &GetRelation(rel_id_type rel_id) const
    {
        auto it = handles.find(rel_id);
        if (it == handles.end())
            throw osrm::util::exception("Can't find relation data for " + std::to_string(rel_id));

        return stash.get<osmium::Relation>(it->second);
    }

    void reset() { iter = handles.begin(); }

    osmium::memory::Buffer read()
    {
        osmium::memory::Buffer buffer(16 * 1024, osmium::memory::Buffer::auto_grow::yes);
        auto end = handles.end();
        if (iter == end)
            return osmium::memory::Buffer();
        while (iter != end && buffer.written() < 12 * 1024)
        {
            buffer.add_item(stash.get_item(iter->second));
            buffer.commit();
            ++iter;
        }
        return buffer;
    };

  private:
    osmium::ItemStash stash;
    RelationIDList empty_rel_list;
    RelationHandleMap handles;
    RelationHandleMap::const_iterator iter;
    MemberRelationMap parents;

    mutex_type mutex;
};

/**
 * @brief A copiable RelationMember
 */
class RelationMember
{
    osmium::object_id_type m_ref;
    osmium::item_type m_type;
    std::string m_role;

  public:
    RelationMember(const osmium::RelationMember &o)
        : m_ref{o.ref()}, m_type{o.type()}, m_role(o.role()){};
    osmium::object_id_type ref() const noexcept { return m_ref; }
    osmium::item_type type() const noexcept { return m_type; }
    const std::string &role() const noexcept { return m_role; }
};

} // namespace osrm::extractor

#endif // EXTRACTION_RELATION_HPP
