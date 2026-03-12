#include "contractor/graph_contractor.hpp"
#include "contractor/contracted_edge_container.hpp"
#include "contractor/contractor_graph.hpp"
#include "contractor/contractor_search.hpp"
#include "contractor/graph_contractor_adaptors.hpp"
#include "contractor/query_edge.hpp"
#include "contractor/query_graph.hpp"
#include "util/integer_range.hpp"
#include "util/log.hpp"
#include "util/percent.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>

#include <cstddef>
#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/concurrent_priority_queue.h>
#include <oneapi/tbb/concurrent_vector.h>
#include <oneapi/tbb/enumerable_thread_specific.h>
#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/parallel_for_each.h>
#include <oneapi/tbb/parallel_invoke.h>
#include <oneapi/tbb/parallel_sort.h>

#include <algorithm>
#include <limits>
#include <set>
#include <vector>

// The algorithm here implemented is described in the paper:
// Contraction Hierarchies: Faster and Simpler Hierarchical Routing in Road Networks
// Robert Geisberger, Peter Sanders, Dominik Schultes, and Daniel Delling
// https://turing.iem.thm.de/routeplanning/hwy/contract.pdf

namespace osrm::contractor
{
namespace
{

/** New edges produced by node contractions. */
tbb::concurrent_vector<ContractorEdge> global_inserted_edges;

struct ContractorNodeData
{
    using NodeDepth = int;
    using NodePriority = float;
    using NodeLevel = float;

    ContractorNodeData(std::size_t number_of_nodes,
                       std::vector<bool> uncontracted_nodes_,
                       std::vector<bool> contractible_,
                       std::vector<EdgeWeight> weights_)
        : is_core(std::move(uncontracted_nodes_)), is_contractible(std::move(contractible_)),
          priorities(number_of_nodes), weights(std::move(weights_)), depths(number_of_nodes, 0)
    {
        if (is_contractible.empty())
        {
            is_contractible.resize(number_of_nodes, true);
        }
        if (is_core.empty())
        {
            is_core.resize(number_of_nodes, true);
        }
    }

    /** All these are keyed by NodeID */
    std::vector<bool> is_core;
    std::vector<bool> is_contractible;
    std::vector<NodePriority> priorities;
    std::vector<EdgeWeight> weights;
    std::vector<NodeDepth> depths;
};

struct ContractionStats
{
    int edges_deleted_count;
    int edges_added_count;
    int original_edges_deleted_count;
    int original_edges_added_count;
    ContractionStats()
        : edges_deleted_count(0), edges_added_count(0), original_edges_deleted_count(0),
          original_edges_added_count(0)
    {
    }
};

struct NodeAndPriority
{
    NodeAndPriority() = default;
    NodeAndPriority(NodeID id, float priority) : id{id}, priority{priority} {}
    NodeID id;
    float priority;
    bool operator<(const NodeAndPriority &other) const
    {
        return priority < other.priority || (priority == other.priority && id < other.id);
    }
};

template <bool RUNSIMULATION>
void ContractNode(const ContractorGraph &graph,
                  const NodeID node,
                  ContractorHeap &heap,
                  std::vector<EdgeWeight> &node_weights,
                  const std::vector<bool> &contractible,
                  ContractionStats *stats = nullptr)
{
    std::vector<ContractorEdge> inserted_edges;
    inserted_edges.reserve(32);
    constexpr bool SHORTCUT_ARC = true;
    constexpr bool FORWARD_DIRECTION_ENABLED = true;
    constexpr bool FORWARD_DIRECTION_DISABLED = false;
    constexpr bool REVERSE_DIRECTION_ENABLED = true;
    constexpr bool REVERSE_DIRECTION_DISABLED = false;

    for (auto in_edge : graph.GetAdjacentEdgeRange(node))
    {
        const ContractorEdgeData &in_data = graph.GetEdgeData(in_edge);
        const NodeID source = graph.GetTarget(in_edge);
        if (source == node)
            continue;

        if (RUNSIMULATION)
        {
            BOOST_ASSERT(stats != nullptr);
            ++stats->edges_deleted_count;
            stats->original_edges_deleted_count += in_data.originalEdges;
        }
        if (!in_data.backward)
        {
            continue;
        }

        heap.Clear();
        heap.Insert(source, {0}, ContractorHeapData{});
        EdgeWeight max_weight = {0};
        unsigned number_of_targets = 0;

        for (auto out_edge : graph.GetAdjacentEdgeRange(node))
        {
            const ContractorEdgeData &out_data = graph.GetEdgeData(out_edge);
            if (!out_data.forward)
            {
                continue;
            }
            const NodeID target = graph.GetTarget(out_edge);
            if (node == target)
            {
                continue;
            }

            const EdgeWeight path_weight = in_data.weight + out_data.weight;
            if (target == source)
            {
                if (path_weight < node_weights[node])
                {
                    if (RUNSIMULATION)
                    {
                        // make sure to prune better, but keep inserting this loop if it should
                        // still be the best
                        // CAREFUL: This only works due to the independent node-setting. This
                        // guarantees that source is not connected to another node that is
                        // contracted
                        node_weights[source] = path_weight + EdgeWeight{1};
                        BOOST_ASSERT(stats != nullptr);
                        stats->edges_added_count += 2;
                        stats->original_edges_added_count +=
                            2 * (out_data.originalEdges + in_data.originalEdges);
                    }
                    else
                    {
                        // CAREFUL: This only works due to the independent node-setting. This
                        // guarantees that source is not connected to another node that is
                        // contracted
                        node_weights[source] = path_weight; // make sure to prune better
                        inserted_edges.emplace_back(source,
                                                    target,
                                                    path_weight,
                                                    in_data.duration + out_data.duration,
                                                    in_data.distance + out_data.distance,
                                                    out_data.originalEdges + in_data.originalEdges,
                                                    node,
                                                    SHORTCUT_ARC,
                                                    FORWARD_DIRECTION_ENABLED,
                                                    REVERSE_DIRECTION_DISABLED);

                        inserted_edges.emplace_back(target,
                                                    source,
                                                    path_weight,
                                                    in_data.duration + out_data.duration,
                                                    in_data.distance + out_data.distance,
                                                    out_data.originalEdges + in_data.originalEdges,
                                                    node,
                                                    SHORTCUT_ARC,
                                                    FORWARD_DIRECTION_DISABLED,
                                                    REVERSE_DIRECTION_ENABLED);
                    }
                }
                continue;
            }
            max_weight = std::max(max_weight, path_weight);
            if (!heap.WasInserted(target))
            {
                heap.Insert(target, INVALID_EDGE_WEIGHT, ContractorHeapData{0, true});
                ++number_of_targets;
            }
        }

        if (RUNSIMULATION)
        {
            const int constexpr SIMULATION_SEARCH_SPACE_SIZE = 1000;
            search(heap,
                   graph,
                   contractible,
                   number_of_targets,
                   SIMULATION_SEARCH_SPACE_SIZE,
                   max_weight,
                   node);
        }
        else
        {
            const int constexpr FULL_SEARCH_SPACE_SIZE = 2000;
            search(heap,
                   graph,
                   contractible,
                   number_of_targets,
                   FULL_SEARCH_SPACE_SIZE,
                   max_weight,
                   node);
        }
        for (auto out_edge : graph.GetAdjacentEdgeRange(node))
        {
            const ContractorEdgeData &out_data = graph.GetEdgeData(out_edge);
            if (!out_data.forward)
            {
                continue;
            }
            const NodeID target = graph.GetTarget(out_edge);
            if (target == node)
                continue;

            const EdgeWeight path_weight = in_data.weight + out_data.weight;
            const EdgeWeight weight = heap.GetKey(target);
            if (path_weight < weight)
            {
                if (RUNSIMULATION)
                {
                    BOOST_ASSERT(stats != nullptr);
                    stats->edges_added_count += 2;
                    stats->original_edges_added_count +=
                        2 * (out_data.originalEdges + in_data.originalEdges);
                }
                else
                {
                    inserted_edges.emplace_back(source,
                                                target,
                                                path_weight,
                                                in_data.duration + out_data.duration,
                                                in_data.distance + out_data.distance,
                                                out_data.originalEdges + in_data.originalEdges,
                                                node,
                                                SHORTCUT_ARC,
                                                FORWARD_DIRECTION_ENABLED,
                                                REVERSE_DIRECTION_DISABLED);

                    inserted_edges.emplace_back(target,
                                                source,
                                                path_weight,
                                                in_data.duration + out_data.duration,
                                                in_data.distance + out_data.distance,
                                                out_data.originalEdges + in_data.originalEdges,
                                                node,
                                                SHORTCUT_ARC,
                                                FORWARD_DIRECTION_DISABLED,
                                                REVERSE_DIRECTION_ENABLED);
                }
            }
        }
    }

    // Merge forward and backward one-way edges.
    // Output edges
    if (!RUNSIMULATION)
    {
        const std::size_t iend = inserted_edges.size();

        // loops over all edges inserted by this procedure call
        for (std::size_t i = 0; i < iend; ++i)
        {
            const ContractorEdge &ei = inserted_edges[i];
            bool keep = true;
            // compares each edge with every other edge
            for (std::size_t j = i + 1; j < iend; ++j)
            {
                ContractorEdge &ej = inserted_edges[j];
                if (ej.source == ei.source && ej.target == ei.target &&
                    ej.data.weight == ei.data.weight && ej.data.shortcut == ei.data.shortcut)
                {
                    // merge the two edges into one
                    ej.data.forward |= ei.data.forward;
                    ej.data.backward |= ei.data.backward;
                    keep = false;
                    break;
                }
            }
            if (keep)
            {
                global_inserted_edges.emplace_back(ei);
            }
        }
    }
}

/**
 * @brief Calculate a node's priority. Lower priority gets contracted first.
 *
 * @param stats The statistics obtained from a simulated contraction.
 * @param node_depth The node's depth.
 * @return float The priority
 */
float EvaluateNodePriority(const ContractionStats &stats,
                           const ContractorNodeData::NodeDepth node_depth)
{
    // Result will contain the priority
    float result;
    if (0 == (stats.edges_deleted_count * stats.original_edges_deleted_count))
    {
        result = 1.f * node_depth;
    }
    else
    {
        result =
            2.f * (((float)stats.edges_added_count) / stats.edges_deleted_count) +
            4.f * (((float)stats.original_edges_added_count) / stats.original_edges_deleted_count) +
            1.f * node_depth;
    }
    BOOST_ASSERT(result >= 0);
    return result;
}

/**
 * @brief Get all neighbours of a node. Duplicates removed.
 *
 * @param graph The graph
 * @param v The node
 * @return std::set<NodeID> The neighbours
 */
inline std::set<NodeID> GetNeighbours(const ContractorGraph &graph, const NodeID v)
{
    std::set<NodeID> neighbours;
    for (auto e : graph.GetAdjacentEdgeRange(v))
    {
        const NodeID u = graph.GetTarget(e);
        if (u != v)
        {
            neighbours.insert(u);
        }
    }
    return neighbours;
}

void DeleteIncomingEdges(ContractorGraph &graph, const NodeID node)
{
    for (const NodeID u : GetNeighbours(graph, node))
    {
        graph.DeleteEdgesTo(u, node);
    }
}

/**
 * @brief Recalculate the priorities of all neighbouring nodes.
 *
 * @param graph
 * @param v The node id
 * @param data
 * @param node_data
 */
void UpdateNodeNeighbours(const ContractorGraph &graph,
                          const NodeID node,
                          ContractorHeap &heap,
                          ContractorNodeData &node_data)
{
    for (const NodeID v : GetNeighbours(graph, node))
    {
        node_data.depths[v] = std::max(node_data.depths[node] + 1, node_data.depths[v]);
        if (node_data.is_contractible[v])
        {
            ContractionStats stats;
            ContractNode<true>(
                graph, v, heap, node_data.weights, node_data.is_contractible, &stats);
            node_data.priorities[v] = EvaluateNodePriority(stats, node_data.depths[v]);
        }
    }
}

inline bool bias(const NodeID a, const NodeID b) { return a < b; }

/**
 * @brief Test if a node is independent.
 *
 * A node is independent if there is no node with lower priority 2 hops away from it.
 * In case of equal priorities the node id is used as tie breaker.
 *
 * @param graph
 * @param node the node to test
 * @param priorities
 * @return bool true if the node is independent.
 */
bool IsNodeIndependent(const ContractorGraph &graph,
                       const NodeID node,
                       const std::vector<float> &priorities)
{
    const float priority = priorities[node];
    BOOST_ASSERT(priority >= 0);

    for (const NodeID hop1 : GetNeighbours(graph, node))
    {
        // 1 hop away
        const float target_priority = priorities[hop1];
        BOOST_ASSERT(target_priority >= 0);

        if (priority > target_priority || (priority == target_priority && bias(node, hop1)))
        {
            return false;
        }

        for (auto e : graph.GetAdjacentEdgeRange(hop1))
        {
            // 2 hops away
            const NodeID hop2 = graph.GetTarget(e);
            // it is cheaper to evaluate a node twice than to do an expensive test here
            if (hop2 == node)
                continue;
            const float target_priority = priorities[hop2];
            BOOST_ASSERT(target_priority >= 0);

            if (priority > target_priority || (priority == target_priority && bias(node, hop2)))
            {
                return false;
            }
        }
    }
    return true;
}

} // namespace

// Initially all nodes are in `remaining_nodes`.
//
// All independent nodes are moved from `remaining_nodes` into a priority queue, ordered
// by "priority" and contracted. Lowest priority nodes get contracted first.
// After a node is contracted its incoming edges are deleted, the new edges are inserted
// into the graph, and the priorities of all neighbouring nodes are updated.
//
// This step is repeated until a sufficient percentile of all nodes is contracted.

std::vector<bool> contractGraph(ContractorGraph &graph,
                                std::vector<bool> node_is_uncontracted_,
                                std::vector<bool> node_is_contractible_,
                                std::vector<EdgeWeight> node_weights_,
                                double core_factor)
{
    BOOST_ASSERT(node_weights_.size() == graph.GetNumberOfNodes());

    /** A heap kept in thread-local storage to avoid multiple recreation of it. */
    ContractorHeap heap_exemplar(8000);
    tbb::enumerable_thread_specific<ContractorHeap> thread_data(heap_exemplar);

    std::size_t number_of_contracted_nodes = 0;

    const NodeID number_of_nodes = graph.GetNumberOfNodes();
    ContractorNodeData node_data{number_of_nodes,
                                 std::move(node_is_uncontracted_),
                                 std::move(node_is_contractible_),
                                 std::move(node_weights_)};

    /** Nodes still waiting for contraction. Not all of them will be contracted. */
    std::vector<NodeID> remaining_nodes;
    remaining_nodes.reserve(number_of_nodes);

    // initialization

    for (auto node : util::irange<NodeID>(0, number_of_nodes))
    {
        if (node_data.is_core[node])
        {
            if (node_data.is_contractible[node])
            {
                remaining_nodes.emplace_back(node);
            }
            else
            {
                node_data.priorities[node] =
                    std::numeric_limits<ContractorNodeData::NodePriority>::max();
            }
        }
    }

    {
        util::UnbufferedLog log;
        log << "initializing node priorities...";
        tbb::parallel_for_each(remaining_nodes,
                               [&](NodeID v)
                               {
                                   BOOST_ASSERT(node_data.is_contractible[v]);
                                   ContractionStats stats;
                                   ContractNode<true>(graph,
                                                      v,
                                                      thread_data.local(),
                                                      node_data.weights,
                                                      node_data.is_contractible,
                                                      &stats);
                                   node_data.priorities[v] =
                                       EvaluateNodePriority(stats, node_data.depths[v]);
                               });
        log << " ok.";
    }

    auto number_of_core_nodes = std::max<std::size_t>(0, (1 - core_factor) * number_of_nodes);
    auto number_of_nodes_to_contract = remaining_nodes.size() - number_of_core_nodes;
    util::Log() << "preprocessing " << number_of_nodes_to_contract << " ("
                << (number_of_nodes_to_contract / (float)number_of_nodes * 100.) << "%) nodes...";

    util::UnbufferedLog log;
    util::Percent p(log, remaining_nodes.size());

    // contract a chunk of nodes until enough nodes are contracted

    while (remaining_nodes.size() > number_of_core_nodes)
    {
        tbb::concurrent_vector<NodeAndPriority> independent_nodes;

        // push the independent nodes into `independent_nodes` and delete them from
        // `remaining_nodes`
        tbb::parallel_for(std::size_t(0),
                          remaining_nodes.size(),
                          [&](const std::size_t idx)
                          {
                              const NodeID v = remaining_nodes[idx];
                              if (IsNodeIndependent(graph, v, node_data.priorities))
                              {
                                  independent_nodes.emplace_back(v, node_data.priorities[v]);
                                  remaining_nodes[idx] = SPECIAL_NODEID; // mark for removal
                              }
                          });

        const auto new_end =
            std::remove(remaining_nodes.begin(), remaining_nodes.end(), SPECIAL_NODEID);
        remaining_nodes.resize(std::distance(remaining_nodes.begin(), new_end));

        // contract the independent nodes in order of priority
        tbb::parallel_sort(independent_nodes.begin(), independent_nodes.end());

        tbb::parallel_for_each(independent_nodes,
                               [&](const NodeAndPriority &nap)
                               {
                                   ContractNode<false>(graph,
                                                       nap.id,
                                                       thread_data.local(),
                                                       node_data.weights,
                                                       node_data.is_contractible);
                               });

        // core flags need to be set in serial since vector<bool> is not thread safe
        for (auto p = independent_nodes.begin(); p < independent_nodes.end(); ++p)
        {
            node_data.is_core[p->id] = false;
        }

        tbb::parallel_for_each(independent_nodes,
                               [&](const NodeAndPriority &nap)
                               { DeleteIncomingEdges(graph, nap.id); });

        // insert new edges into graph
        // since the graph is not thread-safe this must happen in a single thread
        for (const ContractorEdge &edge : global_inserted_edges)
        {
            const EdgeID current_edge_ID = graph.FindEdge(edge.source, edge.target);
            if (current_edge_ID != SPECIAL_EDGEID)
            {
                auto &current_data = graph.GetEdgeData(current_edge_ID);
                if (current_data.shortcut && edge.data.forward == current_data.forward &&
                    edge.data.backward == current_data.backward)
                {
                    // found a duplicate edge with smaller weight, update it.
                    if (edge.data.weight < current_data.weight)
                    {
                        current_data = edge.data;
                    }
                    // don't insert duplicates
                    continue;
                }
            }
            graph.InsertEdge(edge.source, edge.target, edge.data);
        }
        global_inserted_edges.clear();

        // recalculate the neighbours' priorities
        tbb::parallel_for_each(
            independent_nodes,
            [&](const NodeAndPriority &nap)
            { UpdateNodeNeighbours(graph, nap.id, thread_data.local(), node_data); });

        number_of_contracted_nodes += independent_nodes.size();
        p.PrintStatus(number_of_contracted_nodes);
    }

    // no permutation happens here but the edge list is compressed
    graph.Renumber(std::vector<NodeID>());

    return std::move(node_data.is_core);
}

using GraphAndFilter = std::tuple<QueryGraph, std::vector<std::vector<bool>>>;

GraphAndFilter contractFullGraph(ContractorGraph contractor_graph,
                                 std::vector<EdgeWeight> node_weights)
{
    auto num_nodes = contractor_graph.GetNumberOfNodes();
    contractGraph(contractor_graph, std::move(node_weights));

    auto edges = toEdges<QueryEdge>(std::move(contractor_graph));
    std::vector<bool> edge_filter(edges.size(), true);

    return GraphAndFilter{QueryGraph{num_nodes, edges}, {std::move(edge_filter)}};
}

GraphAndFilter contractExcludableGraph(ContractorGraph contractor_graph_,
                                       std::vector<EdgeWeight> node_weights,
                                       const std::vector<std::vector<bool>> &filters)
{
    if (filters.size() == 1)
    {
        if (std::all_of(filters.front().begin(), filters.front().end(), [](auto v) { return v; }))
        {
            return contractFullGraph(std::move(contractor_graph_), std::move(node_weights));
        }
    }

    auto num_nodes = contractor_graph_.GetNumberOfNodes();
    ContractedEdgeContainer edge_container;
    ContractorGraph shared_core_graph;
    std::vector<bool> is_shared_core;
    {
        ContractorGraph contractor_graph = std::move(contractor_graph_);
        std::vector<bool> always_allowed(num_nodes, true);
        for (const auto &filter : filters)
        {
            for (const auto node : util::irange<NodeID>(0, num_nodes))
            {
                always_allowed[node] = always_allowed[node] && filter[node];
            }
        }

        // By not contracting all contractible nodes we avoid creating
        // a very dense core. This increases the overall graph sizes a little bit
        // but increases the final CH quality and contraction speed.
        constexpr float BASE_CORE = 0.9f;
        is_shared_core =
            contractGraph(contractor_graph, std::move(always_allowed), node_weights, BASE_CORE);

        // Add all non-core edges to container
        {
            auto non_core_edges = toEdges<QueryEdge>(contractor_graph);
            auto new_end = std::remove_if(non_core_edges.begin(),
                                          non_core_edges.end(),
                                          [&](const auto &edge) {
                                              return is_shared_core[edge.source] &&
                                                     is_shared_core[edge.target];
                                          });
            non_core_edges.resize(new_end - non_core_edges.begin());
            edge_container.Insert(std::move(non_core_edges));

            for (const auto filter_index : util::irange<std::size_t>(0, filters.size()))
            {
                edge_container.Filter(filters[filter_index], filter_index);
            }
        }

        // Extract core graph for further contraction
        shared_core_graph = contractor_graph.Filter([&is_shared_core](const NodeID node)
                                                    { return is_shared_core[node]; });
    }

    for (const auto &filter : filters)
    {
        auto filtered_core_graph =
            shared_core_graph.Filter([&filter](const NodeID node) { return filter[node]; });

        contractGraph(filtered_core_graph, is_shared_core, is_shared_core, node_weights);

        edge_container.Merge(toEdges<QueryEdge>(std::move(filtered_core_graph)));
    }

    return GraphAndFilter{QueryGraph{num_nodes, edge_container.edges},
                          edge_container.MakeEdgeFilters()};
}

} // namespace osrm::contractor
