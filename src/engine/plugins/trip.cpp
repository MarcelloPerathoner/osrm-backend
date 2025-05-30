#include "engine/plugins/trip.hpp"

#include "engine/api/trip_api.hpp"
#include "engine/api/trip_parameters.hpp"
#include "engine/trip/trip_brute_force.hpp"
#include "engine/trip/trip_farthest_insertion.hpp"
#include "util/dist_table_wrapper.hpp" // to access the dist table more easily

#include <boost/assert.hpp>

#include <algorithm>
#include <utility>
#include <vector>

namespace osrm::engine::plugins
{

bool IsStronglyConnectedComponent(const util::DistTableWrapper<EdgeDuration> &result_table)
{
    return std::find(std::begin(result_table), std::end(result_table), INVALID_EDGE_DURATION) ==
           std::end(result_table);
}

bool IsSupportedParameterCombination(const bool fixed_start,
                                     const bool fixed_end,
                                     const bool roundtrip)
{
    return roundtrip || fixed_start || fixed_end;
}

// given the node order in which to visit, compute the actual route (with geometry, travel time and
// so on) and return the result
InternalRouteResult
TripPlugin::ComputeRoute(const RoutingAlgorithmsInterface &algorithms,
                         const std::vector<PhantomNodeCandidates> &waypoint_candidates,
                         const std::vector<NodeID> &trip,
                         const bool roundtrip) const
{

    // TODO make a more efficient solution that doesn't require copying all the waypoints vectors.
    std::vector<PhantomNodeCandidates> trip_candidates;
    std::transform(trip.begin(),
                   trip.end(),
                   std::back_inserter(trip_candidates),
                   [&](const auto &node) { return waypoint_candidates[node]; });
    // return back to the first node if it is a round trip
    if (roundtrip)
    {
        trip_candidates.push_back(waypoint_candidates[trip.front()]);
        // trip comes out to be something like 0 1 4 3 2 0
        BOOST_ASSERT(trip_candidates.size() == trip.size() + 1);
    }
    else
    {
        // trip comes out to be something like 0 1 4 3 2
        BOOST_ASSERT(trip_candidates.size() == trip.size());
    }

    auto min_route = algorithms.ShortestPathSearch(trip_candidates, {false});
    BOOST_ASSERT_MSG(min_route.shortest_path_weight < INVALID_EDGE_WEIGHT, "unroutable route");
    return min_route;
}

void ManipulateTableForFSE(const std::size_t source_id,
                           const std::size_t destination_id,
                           util::DistTableWrapper<EdgeDuration> &result_table)
{
    // ****************** Change Table *************************
    // The following code manipulates the table and produces the new table for
    // Trip with Fixed Start and End (TFSE). In the example the source is a
    // and destination is c. The new table forces the roundtrip to start at
    // source and end at destination by virtually squashing them together.
    // This way the brute force and the farthest insertion algorithms don't
    // have to be modified, and instead we can just pass a modified table to
    // return a non-roundtrip "optimal" route from a start node to an end node.

    // Original Table           // New Table
    //   a  b  c  d  e          //   a        b         c        d         e
    // a 0  15 36 34 30         // a 0        15        10000    34        30
    // b 15 0  25 30 34         // b 10000    0         25       30        34
    // c 36 25 0  18 32         // c 0        10000     0        10000     10000
    // d 34 30 18 0  15         // d 10000    30        18       0         15
    // e 30 34 32 15 0          // e 10000    34        32       15        0

    // change parameters.source column
    // set any node to source to impossibly high numbers so it will never
    // try to use any node->source in the middle of the "optimal path"
    for (std::size_t i = 0; i < result_table.GetNumberOfNodes(); i++)
    {
        if (i == source_id)
            continue;
        result_table.SetValue(i, source_id, INVALID_EDGE_DURATION);
    }

    // change parameters.destination row
    // set destination to anywhere else to impossibly high numbers so it will
    // never try to use destination->any node in the middle of the "optimal path"
    for (std::size_t i = 0; i < result_table.GetNumberOfNodes(); i++)
    {
        if (i == destination_id)
            continue;
        result_table.SetValue(destination_id, i, INVALID_EDGE_DURATION);
    }

    // set destination->source to zero so roundtrip treats source and
    // destination as one location
    result_table.SetValue(destination_id, source_id, {0});

    // set source->destination as very high number so algorithm is forced
    // to find another path to get to destination
    result_table.SetValue(source_id, destination_id, INVALID_EDGE_DURATION);

    //*********  End of changes to table  *************************************
}

void ManipulateTableForNonRoundtripFS(const std::size_t source_id,
                                      util::DistTableWrapper<EdgeDuration> &result_table)
{
    // We can use the round-trip calculation to simulate non-round-trip fixed start
    // by making all paths to the source location zero. Effectively finding an 'optimal'
    // round-trip path that ignores the cost of getting back from any destination to the
    // source.
    for (const auto i : util::irange<size_t>(0, result_table.GetNumberOfNodes()))
    {
        result_table.SetValue(i, source_id, {0});
    }
}

void ManipulateTableForNonRoundtripFE(const std::size_t destination_id,
                                      util::DistTableWrapper<EdgeDuration> &result_table)
{
    // We can use the round-trip calculation to simulate non-round-trip fixed end
    // by making all paths from the destination to other locations zero.
    // Effectively, finding an 'optimal' round-trip path that ignores the cost of getting
    // from the destination to any source.
    for (const auto i : util::irange<size_t>(0, result_table.GetNumberOfNodes()))
    {
        result_table.SetValue(destination_id, i, {0});
    }
}

Status TripPlugin::HandleRequest(const RoutingAlgorithmsInterface &algorithms,
                                 const api::TripParameters &parameters,
                                 osrm::engine::api::ResultT &result) const
{
    if (!algorithms.HasShortestPathSearch())
    {
        return Error("NotImplemented",
                     "Shortest path search is not implemented for the chosen search algorithm.",
                     result);
    }
    if (!algorithms.HasManyToManySearch())
    {
        return Error("NotImplemented",
                     "Many to many search is not implemented for the chosen search algorithm.",
                     result);
    }

    BOOST_ASSERT(parameters.IsValid());
    const auto number_of_locations = parameters.coordinates.size();

    std::size_t source_id = INVALID_INDEX;
    std::size_t destination_id = INVALID_INDEX;
    if (parameters.source == api::TripParameters::SourceType::First)
    {
        source_id = 0;
    }
    if (parameters.destination == api::TripParameters::DestinationType::Last)
    {
        BOOST_ASSERT(number_of_locations > 0);
        destination_id = number_of_locations - 1;
    }
    bool fixed_start = (source_id == 0);
    bool fixed_end = (destination_id == number_of_locations - 1);
    if (!IsSupportedParameterCombination(fixed_start, fixed_end, parameters.roundtrip))
    {
        return Error("NotImplemented", "This request is not supported", result);
    }

    // enforce maximum number of locations for performance reasons
    if (max_locations_trip > 0 && static_cast<int>(number_of_locations) > max_locations_trip)
    {
        return Error("TooBig", "Too many trip coordinates", result);
    }

    if (!CheckAllCoordinates(parameters.coordinates))
    {
        return Error("InvalidValue", "Invalid coordinate value.", result);
    }

    if (!CheckAlgorithms(parameters, algorithms, result))
        return Status::Error;

    const auto &facade = algorithms.GetFacade();
    auto phantom_node_pairs = GetPhantomNodes(facade, parameters);
    if (phantom_node_pairs.size() != number_of_locations)
    {
        return Error("NoSegment",
                     MissingPhantomErrorMessage(phantom_node_pairs, parameters.coordinates),
                     result);
    }
    BOOST_ASSERT(phantom_node_pairs.size() == number_of_locations);

    if (fixed_start && fixed_end &&
        (source_id >= parameters.coordinates.size() ||
         destination_id >= parameters.coordinates.size()))
    {
        return Error("InvalidValue", "Invalid source or destination value.", result);
    }

    auto snapped_phantoms = SnapPhantomNodes(std::move(phantom_node_pairs));

    BOOST_ASSERT(snapped_phantoms.size() == number_of_locations);

    // compute the duration table of all phantom nodes
    auto result_duration_table = util::DistTableWrapper<EdgeDuration>(
        algorithms.ManyToManySearch(snapped_phantoms, {}, {}, /*requestDistance*/ false).first,
        number_of_locations);

    if (result_duration_table.size() == 0)
    {
        return Status::Error;
    }

    const constexpr std::size_t BF_MAX_FEASIBLE = 10;
    BOOST_ASSERT_MSG(result_duration_table.size() == number_of_locations * number_of_locations,
                     "Distance Table has wrong size");

    if (!IsStronglyConnectedComponent(result_duration_table))
    {
        return Error("NoTrips", "No trip visiting all destinations possible.", result);
    }

    if (fixed_start && fixed_end)
    {
        ManipulateTableForFSE(source_id, destination_id, result_duration_table);
    }
    else if (!parameters.roundtrip && fixed_start)
    {
        ManipulateTableForNonRoundtripFS(source_id, result_duration_table);
    }
    else if (!parameters.roundtrip && fixed_end)
    {
        ManipulateTableForNonRoundtripFE(destination_id, result_duration_table);
    }

    std::vector<NodeID> duration_trip;
    duration_trip.reserve(number_of_locations);
    // get an optimized order in which the destinations should be visited
    if (number_of_locations < BF_MAX_FEASIBLE)
    {
        duration_trip = trip::BruteForceTrip(number_of_locations, result_duration_table);
    }
    else
    {
        duration_trip = trip::FarthestInsertionTrip(number_of_locations, result_duration_table);
    }

    if (!fixed_end || fixed_start)
    {
        // rotate result such that trip starts at node with index 0
        auto desired_start_index = std::find(std::begin(duration_trip), std::end(duration_trip), 0);
        BOOST_ASSERT(desired_start_index != std::end(duration_trip));
        std::rotate(std::begin(duration_trip), desired_start_index, std::end(duration_trip));
    }
    else
    { // fixed_end
        auto destination_index =
            std::find(std::begin(duration_trip), std::end(duration_trip), destination_id);
        BOOST_ASSERT(destination_index != std::end(duration_trip));
        if (!parameters.roundtrip)
        {
            // We want the location after destination to be at the front
            std::advance(destination_index, 1);
            if (destination_index == std::end(duration_trip))
            {
                destination_index = std::begin(duration_trip);
            }
        }
        std::rotate(std::begin(duration_trip), destination_index, std::end(duration_trip));
    }

    // get the route when visiting all destinations in optimized order
    InternalRouteResult route =
        ComputeRoute(algorithms, snapped_phantoms, duration_trip, parameters.roundtrip);

    // get api response
    const std::vector<std::vector<NodeID>> trips = {duration_trip};
    const std::vector<InternalRouteResult> routes = {route};
    api::TripAPI trip_api{facade, parameters};
    trip_api.MakeResponse(trips, routes, snapped_phantoms, result);

    return Status::Ok;
}
} // namespace osrm::engine::plugins
