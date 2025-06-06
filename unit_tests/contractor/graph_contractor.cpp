#include "contractor/graph_contractor.hpp"
#include <contractor/contract_excludable_graph.hpp>

#include "../common/range_tools.hpp"
#include "helper.hpp"

#include <boost/test/unit_test.hpp>

#include <tbb/global_control.h>

using namespace osrm;
using namespace osrm::contractor;
using namespace osrm::unit_test;

BOOST_AUTO_TEST_SUITE(graph_contractor)

BOOST_AUTO_TEST_CASE(contract_exclude_graph)
{
    tbb::global_control scheduler(tbb::global_control::max_allowed_parallelism, 1);
    /* Edge 0 is labeled with toll,
     * no edge will be contracted
     *
     * toll
     * (0)  <--1--< (1)
     *  v             v
     *  |              \
     *  1               2
     *  |                \
     *  > (3) <--2--< (2) <
     */
    std::vector edges = {TestEdge{1, 0, 1},
                         TestEdge{0, 3, 1},

                         TestEdge{1, 2, 2},
                         TestEdge{2, 3, 2}};
    auto reference_graph = makeGraph(edges);

    auto contracted_graph = reference_graph;

    QueryGraph query_graph;
    std::vector<std::vector<bool>> edge_filters;
    std::tie(query_graph, edge_filters) =
        contractExcludableGraph(contracted_graph,
                                {{1}, {1}, {1}, {1}},
                                {{true, true, true, true}, {false, true, true, true}});
    REQUIRE_SIZE_RANGE(query_graph.GetAdjacentEdgeRange(0), 0);
    BOOST_CHECK(query_graph.FindEdge(0, 1) == SPECIAL_EDGEID);
    BOOST_CHECK(query_graph.FindEdge(0, 3) == SPECIAL_EDGEID);
    REQUIRE_SIZE_RANGE(query_graph.GetAdjacentEdgeRange(1), 2);
    BOOST_CHECK(query_graph.FindEdge(1, 0) != SPECIAL_EDGEID);
    BOOST_CHECK(query_graph.FindEdge(1, 2) != SPECIAL_EDGEID);
    REQUIRE_SIZE_RANGE(query_graph.GetAdjacentEdgeRange(2), 0);
    BOOST_CHECK(query_graph.FindEdge(2, 1) == SPECIAL_EDGEID);
    BOOST_CHECK(query_graph.FindEdge(2, 3) == SPECIAL_EDGEID);
    REQUIRE_SIZE_RANGE(query_graph.GetAdjacentEdgeRange(3), 2);
    BOOST_CHECK(query_graph.FindEdge(3, 0) != SPECIAL_EDGEID);
    BOOST_CHECK(query_graph.FindEdge(3, 2) != SPECIAL_EDGEID);

    auto reference_graph2 = makeGraph(edges);

    auto contracted_graph2 = reference_graph2;

    /* All edges are normal edges,
     * edge 2 will be contracted
     *
     * Deleted edge_based_edges 1 -> 2, 3 -> 2
     *
     * (0)  <--1--< (1)
     *  v             v
     *  |              \
     *  1               2
     *  |                \
     *  > (3) <--2--< (2) <
     */
    QueryGraph query_graph2;
    std::vector<std::vector<bool>> edge_filters2;
    std::tie(query_graph2, edge_filters2) =
        contractExcludableGraph(contracted_graph2,
                                {{1}, {1}, {1}, {1}},
                                {{true, true, true, true}, {true, true, true, true}});

    REQUIRE_SIZE_RANGE(query_graph2.GetAdjacentEdgeRange(0), 0);
    BOOST_CHECK(query_graph2.FindEdge(0, 1) == SPECIAL_EDGEID);
    BOOST_CHECK(query_graph2.FindEdge(0, 3) == SPECIAL_EDGEID);
    REQUIRE_SIZE_RANGE(query_graph2.GetAdjacentEdgeRange(1), 1);
    BOOST_CHECK(query_graph2.FindEdge(1, 0) != SPECIAL_EDGEID);
    BOOST_CHECK(query_graph2.FindEdge(1, 2) == SPECIAL_EDGEID);
    REQUIRE_SIZE_RANGE(query_graph2.GetAdjacentEdgeRange(2), 2);
    BOOST_CHECK(query_graph2.FindEdge(2, 1) != SPECIAL_EDGEID);
    BOOST_CHECK(query_graph2.FindEdge(2, 3) != SPECIAL_EDGEID);
    REQUIRE_SIZE_RANGE(query_graph2.GetAdjacentEdgeRange(3), 1);
    BOOST_CHECK(query_graph2.FindEdge(3, 0) != SPECIAL_EDGEID);
    BOOST_CHECK(query_graph2.FindEdge(3, 2) == SPECIAL_EDGEID);
}

BOOST_AUTO_TEST_CASE(contract_graph)
{
    tbb::global_control scheduler(tbb::global_control::max_allowed_parallelism, 1);
    /*
     *                 <--1--<
     * (0) >--3--> (1) >--3--> (3)
     *  v          ^  v          ^
     *   \        /    \         |
     *    1      1      1        1
     *     \    ^        \       /
     *      >(5)          > (4) >
     */
    std::vector<TestEdge> edges = {TestEdge{0, 1, 3},
                                   TestEdge{0, 5, 1},
                                   TestEdge{1, 3, 3},
                                   TestEdge{1, 4, 1},
                                   TestEdge{3, 1, 1},
                                   TestEdge{4, 3, 1},
                                   TestEdge{5, 1, 1}};
    auto reference_graph = makeGraph(edges);

    auto contracted_graph = reference_graph;
    std::vector<bool> core = contractGraph(contracted_graph, {{1}, {1}, {1}, {1}, {1}, {1}});

    // This contraction order is dependent on the priority caculation in the contractor
    // but deterministic for the same graph.
    CHECK_EQUAL_RANGE(core, false, false, false, false, false, false);

    /* After contracting 0 and 2:
     *
     * Deltes edges 5 -> 0, 1 -> 0
     *
     *                 <--1--<
     * (0) ---3--> (1) >--3--> (3)
     *  \          ^  v          ^
     *   \        /    \         |
     *    1      1      1        1
     *     \    ^        \       /
     *      >(5)          > (4) >
     */
    reference_graph.DeleteEdgesTo(5, 0);
    reference_graph.DeleteEdgesTo(1, 0);

    /* After contracting 5:
     *
     * Deletes edges 1 -> 5
     *
     *                 <--1--<
     * (0) ---3--> (1) >--3--> (3)
     *  \          ^  v          ^
     *   \        /    \         |
     *    1      1      1        1
     *     \    /        \       /
     *      >(5)          > (4) >
     */
    reference_graph.DeleteEdgesTo(5, 0);
    reference_graph.DeleteEdgesTo(1, 0);

    /* After contracting 3:
     *
     * Deletes edges 1 -> 3
     * Deletes edges 4 -> 3
     * Insert edge 4 -> 1
     *
     *                 <--1---
     * (0) ---3--> (1) >--3--- (3)
     *  \          ^  v ^        |
     *   \        /    \ \       |
     *    1      1      1 2      1
     *     \    /        \ \     /
     *      >(5)          > (4) >
     */
    reference_graph.DeleteEdgesTo(1, 3);
    reference_graph.DeleteEdgesTo(4, 3);
    // Insert shortcut
    reference_graph.InsertEdge(4, 1, {{2}, {4}, {1.0}, 3, 0, true, true, false});

    /* After contracting 4:
     *
     * Delete edges 1 -> 4
     *
     *                 <--1---
     * (0) ---3--> (1) >--3--- (3)
     *  \          ^  v ^        |
     *   \        /    \ \       |
     *    1      1      1 2      1
     *     \    /        \ \     /
     *      >(5)          \ (4) >
     */
    reference_graph.DeleteEdgesTo(1, 4);

    /* After contracting 1:
     *
     * Delete no edges.
     *
     *                 <--1---
     * (0) ---3--> (1) >--3--- (3)
     *  \          ^  v ^        |
     *   \        /    \ \       |
     *    1      1      1 2      1
     *     \    /        \ \     /
     *      >(5)          \ (4) >
     */

    REQUIRE_SIZE_RANGE(contracted_graph.GetAdjacentEdgeRange(0), 2);
    BOOST_CHECK(contracted_graph.FindEdge(0, 1) != SPECIAL_EDGEID);
    BOOST_CHECK(contracted_graph.FindEdge(0, 5) != SPECIAL_EDGEID);
    REQUIRE_SIZE_RANGE(contracted_graph.GetAdjacentEdgeRange(1), 0);
    REQUIRE_SIZE_RANGE(contracted_graph.GetAdjacentEdgeRange(2), 0);
    REQUIRE_SIZE_RANGE(contracted_graph.GetAdjacentEdgeRange(3), 3);
    BOOST_CHECK(contracted_graph.FindEdge(3, 1) != SPECIAL_EDGEID);
    BOOST_CHECK(contracted_graph.FindEdge(3, 4) != SPECIAL_EDGEID);
    REQUIRE_SIZE_RANGE(contracted_graph.GetAdjacentEdgeRange(4), 2);
    BOOST_CHECK(contracted_graph.FindEdge(4, 1) != SPECIAL_EDGEID);
    REQUIRE_SIZE_RANGE(contracted_graph.GetAdjacentEdgeRange(5), 1);
    BOOST_CHECK(contracted_graph.FindEdge(5, 1) != SPECIAL_EDGEID);
}

BOOST_AUTO_TEST_SUITE_END()
