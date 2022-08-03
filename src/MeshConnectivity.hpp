#ifndef __MESH_CONNECTIVITY_H__
#define __MESH_CONNECTIVITY_H__

#include "CSRList.hpp"

template <typename Derived> struct MeshConnectivity {};

template <template <int> typename DerivedClass, int D>
struct MeshConnectivity<DerivedClass<D>> {

    using Derived = DerivedClass<D>;
    MeshConnectivity(const Derived &mesh)
        : _mesh(&mesh), _element_aggregations(D + 1) {}

    void init() {
        for (std::size_t i = 0; i <= D; ++i) {
            _collect_mesh_entities(i);
        }
        _build_all_connectivity();
        _build_vertex_adjacency_list();
        _build_orientation_of_subentities();
    }

    const CSRList<std::size_t> &connectivity(std::size_t dim0,
                                             std::size_t dim1) const {
        return _connectivity.at({dim0, dim1});
    }

    const CSRList<std::size_t> &adjacent_vertices() const {
        return _adjacent_vertices;
    }

    const CSRList<std::size_t> &element_collections(int dim) const {
        return this->_element_aggregations[dim];
    }

    const CSRList<std::size_t> &orientation() const {
        return this->_orientation;
    }

private:
    void _collect_mesh_entities(std::size_t dim) {
        /*
    const auto prime_element_type = ElementSpace<D>().prime_element_types();
    auto num_prime_elements = std::accumulate(prime_element_type.begin(),
    prime_element_type.end(),
    static_cast<std::size_t>(0),
    [this](std::size_t init, decltype(*prime_element_type.begin()) type) {
        return init + _mesh->elements(type).second.size();
    });
    const auto secondary_element_type =
    ElementSpace<D>().secondary_element_types(); auto num_secondary_elements =
    std::accumulate(secondary_element_type.begin(),
    secondary_element_type.end(),
    static_cast<std::size_t>(0),
    [this](std::size_t init, decltype(*prime_element_type.begin()) type) {
        return init + _mesh->elements(type).second.size();
    });
    auto num_vertices = _mesh->nodes().size() / D;
    const auto & element_types = ElementSpace<D>().all_element_types();
    if(0 <= dim and dim <= D){
    std::for_each(element_types.begin(), element_types.end(),
        [dim, this](FiniteElementType type) {
            auto topologic_dim = ElementSpace<D>().topologic_dim(type);
            if(topologic_dim == dim) {
                this->_element_aggregations[dim] +=
    std::move(this->_mesh->elements(type).first);
            }
        });
    }
        */
        this->_element_aggregations[dim] = this->_mesh->elements(dim).first;
    }

    // {dim0, dim1} already exists, now construct the reverse mappping {dim1,
    // dim0}
    void _construct_reverse_map(std::size_t dim0, std::size_t dim1) {
        auto size0 = this->_connectivity[{dim0, dim1}].size();
        auto size1 = this->_connectivity[{dim1, dim0}].size();
        if (size0 > 0 and size1 == 0) {
            this->_connectivity[{dim1, dim0}] =
                this->_connectivity[{dim0, dim1}].reverse();
            auto max_begin = this->_connectivity[{dim1, dim0}].size();
            auto max_end = this->_element_aggregations[dim1].size();
            for (std::size_t i = max_begin; i < max_end; ++i) {
                this->_connectivity[{dim1, dim0}].push_back(
                    std::vector<std::size_t>());
            }
        }
    }

    void _build_connectivity_pair(std::size_t dim0, std::size_t dim1,
                                  int depth) {

        auto size0 = this->_connectivity[{dim0, dim1}].size();
        // auto size1 = this->_connectivity[{dim1, dim0}].size();
        if (dim0 == dim1     // Connectivity to self alwasy exsits
            or size0 != 0) { // Connectivity has been constructed
                             // return;
        } else if (dim0 == 0 or dim1 == 0) {
            // if {dim0+dim1, 0} is empty, then construct it
            if (this->_connectivity[{dim0 + dim1, 0}].size() == 0) {
                this->_connectivity[{dim0 + dim1, 0}] =
                    this->_element_aggregations[dim1 + dim0];
            }
            // when (0 == dim0), construct {0, dim1}
            // reverse the CSRList and fill to # of elements
            if (dim0 < dim1) {
                _construct_reverse_map(dim1, dim0);
            }
        } else if (dim0 > dim1) {
            _build_connectivity_pair(dim1, dim0, depth + 1);
            _construct_reverse_map(dim1, dim0);
        } else {
            _build_connectivity_pair(dim0, 0, depth + 1);
            _build_connectivity_pair(0, dim1, depth + 1);
            const auto &entity_dim0_to_vertex = this->_connectivity[{dim0, 0}];
            const auto &entity_vertex_to_dim1 = this->_connectivity[{0, dim1}];
            if (entity_dim0_to_vertex.size() != 0 and
                entity_vertex_to_dim1.size() != 0) {
                for (auto dim0_to_vertex_list : entity_dim0_to_vertex) {
                    // Find: who appears n times, where n is number of vertices
                    // in the entity;
                    std::map<std::size_t, std::size_t> counter;
                    for (const auto ivtx : dim0_to_vertex_list.data()) {
                        for (const auto &ientity :
                             entity_vertex_to_dim1[ivtx]) {
                            counter[ientity]++;
                        }
                    }

                    std::vector<std::size_t> connected_entities;
                    std::for_each(counter.begin(), counter.end(),
                                  [&](decltype(counter)::value_type &pair) {
                                      if (pair.second ==
                                          dim0_to_vertex_list.data().size()) {
                                          connected_entities.push_back(
                                              pair.first);
                                      }
                                  });
                    this->_connectivity[{dim0, dim1}].push_back(
                        std::move(connected_entities));
                }
            }
        }
    }

    void _build_all_connectivity() {
        for (std::size_t i = 0; i <= D; ++i) {
            for (std::size_t j = 0; j <= D; ++j) {
                _build_connectivity_pair(j, i, 0);
            }
        }
    }

    void _build_vertex_adjacency_list() {
        const auto prime_element_list = _element_aggregations[D];
        auto nnode = _mesh->nodes().size() / D;
        std::size_t expected_bandwidth = 64;
        std::vector<std::vector<std::size_t>> adjacency(nnode);
        for (auto &cache : adjacency) {
            cache.reserve(expected_bandwidth);
        }

        for (auto cell : prime_element_list) {
            const auto &vertex_list = cell.data();
            std::for_each(vertex_list.begin(), vertex_list.end(),
                          [&](std::size_t ivtx) {
                              adjacency[ivtx].insert(adjacency[ivtx].end(),
                                                     vertex_list.begin(),
                                                     vertex_list.end());
                          });
        }

        for (auto &cache : adjacency) {
            // remove duplicated entries
            std::sort(cache.begin(), cache.end());
            cache.erase(std::unique(cache.begin(), cache.end()), cache.end());
            this->_adjacent_vertices.push_back(std::move(cache));
        }
    }

    static std::vector<std::size_t>
    _get_child_indices_in_parent(const std::vector<std::size_t> &child,
                                 const std::vector<std::size_t> &parent) {
        std::vector<std::size_t> indices;
        indices.reserve(child.size());
        for (std::size_t i = 0;
             indices.size() < child.size() and i < child.size(); ++i) {
            for (std::size_t j = 0; j < parent.size(); ++j) {
                if (child[i] == parent[j]) {
                    indices.push_back(j);
                    break;
                }
            }
        }
        return (indices.size() == child.size() ? indices
                                               : std::vector<std::size_t>());
    }

    void _build_orientation_of_subentities() {
        const auto &prime_element_list = element_collections(D);
        const auto &secondary_element_list = element_collections(D - 1);
        const auto &subentity_to_entity = connectivity(D - 1, D);
        for (std::size_t i = 0; i < subentity_to_entity.size(); ++i) {
            auto subentity_global_indices = secondary_element_list[i];
            auto base_entity_ID = subentity_to_entity[i];
            std::vector<std::size_t> subentity_orientation;
            subentity_orientation.reserve(1);
            for (auto idx : base_entity_ID) {
                const auto &base_entity_global_list = prime_element_list[idx];
                const auto &subentity_local_indices =
                    _get_child_indices_in_parent(subentity_global_indices,
                                                 base_entity_global_list);
                assert(not subentity_local_indices.empty());
                auto type = ElementSpace<D>::element_type(
                    base_entity_global_list.size());
                auto orientation = ElementNumbering::subentity_indices(
                    type, subentity_local_indices);
                assert(orientation < 8);
                subentity_orientation.push_back(orientation);
            }

            this->_orientation.push_back(subentity_orientation);
        }
    }

private:
public:
    std::vector<CSRList<std::size_t>> _element_aggregations;
    using Key = std::array<std::size_t, 2>;
    struct Cmp {
        bool operator()(const Key &first, const Key &second) const {
            return first[0] * 10 + first[1] < second[0] * 10 + second[1];
        }
    };
    std::map<Key, CSRList<std::size_t>, Cmp> _connectivity;
    CSRList<std::size_t> _adjacent_vertices;
    CSRList<std::size_t> _orientation;
    const Derived *_mesh;
};

#endif // __MESH_CONNECTIVITY_H__
