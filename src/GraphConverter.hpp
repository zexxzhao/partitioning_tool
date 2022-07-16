#ifndef __GRAPH_CONVERTER_H__
#define __GRAPH_CONVERTER_H__

#ifdef HAS_BOOST_GRAPH
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/edge_list.hpp>
#endif // HAS_BOOST_CRAPH


template <typename>  struct is_direct {};

template <typename Graph> 
struct is_direct<Graph> : 
std::enable_if_t<
	std::is_same_v<boost::adjacency_list, Graph>
	or std::is_same_v<boost::adjacency_matrix, Graph>
	or std::is_same_v<boost::edge_list, Graph>, typename Graph::directed_category> {}; 

template <typename DirectedGraph> 
struct is_direct<Directed> : std::enable_if_t<std::is_same_v<DirectedGraph, boost::undirectedS>, std::false_type> {}; 

template <typename DirectedGraph> constexpr inline boo is_directed_v =  is_direct<DirectedGraph>::value;


#endif // HAS_BOOST_GRAPH

template <typename From, typename To, bool directed_graph = >
struct GraphConverter
{
	constexpr static auto = (is_direct)
	typedef To<>
	GraphConverter(const From& graph_src) {}

	private:
	To _graph_dst;
};

#endif // __GRAPH_CONVERTER_H__
