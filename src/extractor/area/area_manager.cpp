#include "extractor/area/area_manager.hpp"

#include "util/log.hpp"

#include <osmium/area/assembler.hpp>

namespace osrm::extractor::area
{

/**
 * @brief Registers the given closed way for area-building.
 *
 * This function is thread-safe.
 *
 * This member function is named way() so the manager can
 * be used as a handler for the first pass through a data file.
 *
 * @param way Way we want to build.
 */
void AreaManager::way(const osmium::Way &way)
{
    // the way must be closed
    if (!way.ends_have_same_id())
    {
        return;
    }
    // the way must have at least 5 nodes (first and last node are the same, a triangle
    // doesn't need meshing)
    if (way.nodes().size() < 5)
    {
        return;
    }
    util::Log(logDEBUG) << "Registering way: " << way.get_value_by_key("name", "noname")
                        << " id: " << way.id();
    registered_closed_ways.push_back(way.id());
    ++number_of_ways;
}

/**
 * @brief Registers the given relation for area-building.
 *
 * This function is thread-safe.
 *
 * We are interested only in relations tagged with type=multipolygon.
 *
 * This member function is named relation() so the manager can
 * be used as a handler for the first pass through a data file.
 *
 * @param relation Relation we want to build.
 */
void AreaManager::relation(const osmium::Relation &relation)
{
    const char *type = relation.get_value_by_key("type");

    // the relation must be a multipolygon
    if (type == nullptr || std::strcmp(type, "multipolygon") != 0)
    {
        return;
    }

    util::Log(logDEBUG) << "Registering relation: " << relation.get_value_by_key("name", "noname")
                        << " id: " << relation.id();

    // osmium is not thread-safe
    MutexType::scoped_lock lock(mutex);
    auto rel_handle = relations_database().add(relation);

    std::size_t n = 0;
    for (auto &member : rel_handle->members())
    {
        switch (member.type())
        {
        case osmium::item_type::node:
            member_database(member.type()).track(rel_handle, member.ref(), n);
            m_node_relation[member.ref()] = relation.id();
            break;
        case osmium::item_type::way:
            member_database(member.type()).track(rel_handle, member.ref(), n);
            m_way_relation[member.ref()] = relation.id();
            break;
        default:
            member.set_ref(0); // set member id to zero to indicate we are not interested
        }
        ++n;
    }
    ++number_of_relations;
}

/**
 * @brief Sort the members databases to prepare them for reading.
 *
 * Usually this is called between the first and second pass reading through an OSM data
 * file.
 */
void AreaManager::prepare_for_lookup(extractor::NodeLocationsForWays &nlw)
{
    nlw.prepare_for_lookup();
    node_locations_for_ways = &nlw;
    RelationsManager::prepare_for_lookup();
    std::sort(registered_closed_ways.begin(), registered_closed_ways.end());
}

inline bool AreaManager::is_registered_closed_way(osmium::object_id_type osm_id) const
{
    return std::binary_search(registered_closed_ways.begin(), registered_closed_ways.end(), osm_id);
}

/**
 * @brief Return the registered relations for the given way.
 *
 * Call this from the LUA {@code process_way} function to find out if the way is a member of
 * any relation that you registered in the LUA {@code process_relation} function.
 *
 * @param way The given way
 * @return A list of relations
 */
AreaManager::relation_ids AreaManager::get_relations_for_way(const osmium::Way &way) const
{
    relation_ids result;
    auto found = m_way_relation.find(way.id());
    if (found != m_way_relation.end())
        result.push_back(found->second);
    return result;
}

/**
 * @brief Return the registered relations for the given node.
 *
 * Call this from the LUA {@code process_node} function to find out if the node is a member of
 * any relation that you registered in the LUA {@code process_relation} function.
 *
 * Note: not overloaded to avoid a hideous cast when registering on sol.
 *
 * @param node The given node
 * @return A list of relations
 */
AreaManager::relation_ids AreaManager::get_relations_for_node(const osmium::Node &node) const
{
    relation_ids result;
    auto found = m_node_relation.find(node.id());
    if (found != m_node_relation.end())
        result.push_back(found->second);
    return result;
}

/**
 * @brief Second-pass handler for ways.
 */
void AreaManager::after_way(const osmium::Way &way)
{
    if (is_registered_closed_way(way.id()))
    {
        const char *name = way.get_value_by_key("name", "noname");
        util::Log(logDEBUG) << "Completing way: " << name << " id: " << way.id();

        node_locations_for_ways->way(way);
        osmium::area::Assembler assembler{m_assembler_config};
        assembler(way, this->buffer());
    }
}

/**
 * Second-pass handler for relations.
 *
 * This is called when a relation is complete, ie. all members
 * were found in the input.
 */
void AreaManager::complete_relation(const osmium::Relation &relation)
{
    const char *name = relation.get_value_by_key("name", "noname");
    util::Log(logDEBUG) << "Completing relation: " << name << " id: " << relation.id();

    std::vector<const osmium::Way *> ways;
    ways.reserve(relation.members().size());
    for (const auto &member : relation.members())
    {
        if (member.ref() != 0)
        {
            const osmium::Way *way = get_member_way(member.ref());
            assert(way != nullptr);
            node_locations_for_ways->way(*way);
            ways.push_back(way);
        }
    }

    try
    {
        osmium::area::Assembler assembler{m_assembler_config};
        assembler(relation, ways, this->buffer());
    }
    catch (const osmium::invalid_location &)
    {
        // XXX ignore
    }
}

} // namespace osrm::extractor::area
