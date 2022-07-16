#ifndef __REORDER_H__
#define __REORDER_H__
#include <cassert>
#include <CSRList.hpp>
#include <set>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/cuthill_mckee_ordering.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/bandwidth.hpp>

namespace reordering
{
    // a helper function of `partial_reverse_cuthill_mckee`
    // to check the connectivity of the given vertices in `vertices_to_be_reordered` wrt the graph
    template <typename T = std::size_t>
    bool compact(
        const CSRList<T>& undirected_graph, 
        const std::vector<T>& vertices_to_be_reordered
    ) {
        // sorted vertices to speed up exsistence checking
        std::vector<T> sorted_vertices;
        if(vertices_to_be_reordered.size() != 0) {
            sorted_vertices = vertices_to_be_reordered;
            std::sort(sorted_vertices.begin(), sorted_vertices.end());
        }
        // if `vertices_to_be_reordered` specifies no vertices, then check all vertices in the graph
        else {
            std::set<T> vtx(undirected_graph.data().begin(), undirected_graph.data().end());
            sorted_vertices.assign(vtx.begin(), vtx.end()); // naturally sorted
        }

        // an array to store the accessibility of each vertice.
        // 0 - has not been accessed
        // 1 - has been accessed
        std::vector<int> color(vertices_to_be_reordered.size(), 0);

        std::vector<T> vertex_queue;
        vertex_queue.reserve(sorted_vertices.size());
        
        // choose the first vertex
        vertex_queue.push_back(sorted_vertices[0]);
        
        for(std::size_t head = 0; head != vertices.size(); ++head) {
            auto head_vtx = vertex_queue[head];
            // find adjacency
            auto adjacency = undirected_graph[head_vtx];
            for(auto adj_vtx: adjacency) {
                auto it = std::lower_bound(sorted_vertices.begin(), sorted_vertices.end(), adj_vtx);
                auto index = std::distance(sorted_vertices.begin(), it);
                if(*it == adj_vtx and color[index] == 0) {
                    vertex_queue.push_back(adj_vtx);
                    color[index] = 1;
                }
            }
        }
        return std::all_of(color.cbegin(), color.cend(), [](int a) { return a > 0; });
    }
    template <typename T = std::size_t>
    T find_peripheral_vertex(
        const CSRList<T>& undirected_graph, 
        const std::vector<T>& vertices_to_be_reordered
    ) {
        auto is_compact = compact(undirected_graph, vertices_to_be_reordered);
        assert(is_compact);
        // sorted vertices to speed up exsistence checking
        std::vector<T> sorted_vertices;
        if(vertices_to_be_reordered.size() != 0) {
            sorted_vertices = vertices_to_be_reordered;
            std::sort(sorted_vertices.begin(), sorted_vertices.end());
        }
        // if `vertices_to_be_reordered` specifies no vertices, then check all vertices in the graph
        else {
            std::set<T> vtx(undirected_graph.data().begin(), undirected_graph.data().end());
            sorted_vertices.assign(vtx.begin(), vtx.end()); // naturally sorted
        }
        T peripheral_vertex {};
        auto min_degree = std::numeric_limits<std::size_t>::max();
        for(auto vtx : sorted_vertices) {
            auto adjacency = undirected_graph[vtx];
            auto degree = std::accumulate(
                adjacency.begin(), 
                adjacency.end(), 
                static_cast<std::size_t>(0),
                [](std::size_t a, std::size_t adj_vtx){
                    auto it = std::lower_bound(sorted_vertices.begin(), sorted_vertices.end(), adj_vtx);
                    return a + (*it == adj_vtx ? 1 : 0);
                });
            if(degree < min_degree) {
                min_degree = degree;
                peripheral_vertex = vtx;
            }
        }

        return peripheral_vertex;
    }

    template <typename T = std::size_t, typename Callable = void>
    struct BandwidthReduction
    {
        typedef boost::adjacency_list<
            boost::vecS, 
            boost::vecS, 
            boost::undirectedS, 
            boost::property<
                vertex_color_t, 
                default_color_type, 
                boost::property<vertex_degree_t,int> 
                >
            > Graph;
        typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
        typedef boost::graph_traits<Graph>::vertices_size_type size_type;

        BandwidthReduction(const CSRList<T>& undirected_graph) 
            : _undirected_graph(undirected_graph.size()) {
            // insert edges into the graph
        }

        private:
        Graph _undirected_graph;
    };
}

#endif // __REORDER_H__