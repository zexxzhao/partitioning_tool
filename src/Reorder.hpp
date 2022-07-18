#ifndef __REORDER_H__
#define __REORDER_H__

#include <iostream>
#include <cassert>
#include <vector>

#include <boost/config.hpp>
#include <boost/graph/cuthill_mckee_ordering.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/bandwidth.hpp>

#include "GraphConverter.hpp"

namespace reordering
{

    template <typename T = std::size_t>
    struct BandwidthReduction
    {
        typedef boost::adjacency_list<
            boost::vecS, 
            boost::vecS, 
            boost::undirectedS, 
            boost::property<
                boost::vertex_color_t, 
                boost::default_color_type, 
                boost::property<boost::vertex_degree_t,int> 
                >
            > Graph;
        typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
        typedef boost::graph_traits<Graph>::vertices_size_type size_type;
        typedef CSRList<T, T, std::false_type> CustomizedGraph;
        BandwidthReduction(const CustomizedGraph& undirected_graph) {
                GraphConverter c(undirected_graph);
                _undirected_graph = c.template convert<Graph>();
            }
        
        std::vector<T> operator()() {

            auto& G = _undirected_graph;
            std::vector<Vertex> inv_perm(num_vertices(G));

            boost::cuthill_mckee_ordering(
                G,
                inv_perm.rbegin(),
                boost::get(boost::vertex_color, G),
                boost::make_degree_map(G)
                );
            return std::vector<T>(inv_perm.begin(), inv_perm.end());

        }


        private:
        Graph _undirected_graph;
    };


}

#endif // __REORDER_H__