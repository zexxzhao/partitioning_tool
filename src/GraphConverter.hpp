#ifndef __GRAPH_CONVERTER_H__
#define __GRAPH_CONVERTER_H__

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/edge_list.hpp>
#include <type_traits>

namespace detail {
template <typename, template <typename...> typename>
struct is_instance : std::false_type {};

template <typename... Ts, template <typename...> typename Prototype>
struct is_instance<Prototype<Ts...>, Prototype> : std::true_type {};

template <typename Instance, template <typename...> typename Prototype>
constexpr inline bool is_instance_v = is_instance<Instance, Prototype>::value;

template <typename T> constexpr inline auto generalized_is_directed() {
    if constexpr (std::is_same_v<T, std::true_type> or
                  std::is_same_v<T, std::false_type>) {
        return T{};
    } else if constexpr (std::is_same_v<T, boost::directed_tag>) {
        return std::true_type{};
    } else {
        return std::false_type{};
    }
}

template <typename Graph>
struct is_directed_graph
    : decltype(generalized_is_directed<typename Graph::directed_category>()) {};

template <typename Graph>
constexpr inline bool is_directed_graph_v = is_directed_graph<Graph>::value;
}; // namespace detail

// template <typename Graph>
// struct is_directed_graph<Graph, decltype(typename Graph::directed_graph()())>
// : Graph::directed_category {};

// template <typename Graph>
// struct is_directed_graph<Graph, typename Graph::directed_category> :
// std::is_same<typename Graph::directed_category, boost::directed_tag> {};

// template <typename Graph> constexpr inline bool is_directed_graph_v =
// is_directed_graph<Graph>::value;

template <typename Graph>
struct is_boost_graph
    : boost::mpl::if_<
          // std::is_same_v<Graph, boost::adjacency_list>
          // or std::is_same_v<Graph, boost::adjacency_matrix>
          // or std::is_same_v<Graph, boost::edge_list>,
          std::integral_constant<
              bool, detail::is_instance_v<Graph, boost::adjacency_list> or
                        detail::is_instance_v<Graph, boost::adjacency_matrix> or
                        detail::is_instance_v<Graph, boost::edge_list>>,
          std::true_type, std::false_type>::type {};

template <typename Graph>
constexpr inline bool is_boost_graph_v = is_boost_graph<Graph>::value;

template <typename From> struct GraphConverter {
    typedef std::integral_constant<bool, detail::is_directed_graph<From>::value>
        directed_category;

    GraphConverter(const From &graph_src) : _graph_src(&graph_src) {}

    template <typename To> To convert() {
        static_assert(detail::is_directed_graph<From>::value ==
                      detail::is_directed_graph<To>::value);
        To _graph_dst;
        const auto &graph_src = *_graph_src;
        if constexpr (std::is_same_v<From, To>) {
            _graph_dst = graph_src;
        } else if constexpr (is_boost_graph_v<From>) {
            typename boost::graph_traits<From>::vertex_iterator vtx_begin,
                vtx_end;
            std::tie(vtx_begin, vtx_end) = boost::vertices(graph_src);
            auto num_vertices = *std::max_element(vtx_begin, vtx_end);

            std::vector<std::vector<std::size_t>> graph_tmp(num_vertices);
            for (auto &v : graph_tmp) {
                v.reserve(32);
            }
            for (std::tie(vtx_begin, vtx_end) = boost::vertices(graph_src);
                 vtx_begin != vtx_end; ++vtx_begin) {
                auto neighbors =
                    boost::adjacent_vertices(*vtx_begin, graph_src);
                for (auto it = neighbors.first; it != neighbors.second; ++it) {
                    graph_tmp[*vtx_begin].push_back(*it);
                }
            }
            for (auto &cache : graph_tmp) {
                std::sort(cache.begin(), cache.end());
                cache.erase(std::unique(cache.begin(), cache.end()),
                            cache.end());
                _graph_dst.push_back(cache);
            }
        } else if constexpr (is_boost_graph_v<To>) {
            auto num_vertices = *std::max_element(graph_src.data().begin(),
                                                  graph_src.data().end());
            _graph_dst = To(num_vertices);
            for (auto it = graph_src.begin(); it != graph_src.end(); ++it) {
                for (auto n : it->data()) {
                    boost::add_edge(it->index(), n, _graph_dst);
                }
            }
        }

        return _graph_dst;
    }

private:
    const From *_graph_src;
};

#endif // __GRAPH_CONVERTER_H__
