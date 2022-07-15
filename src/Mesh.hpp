#ifndef __MESH_H__
#define __MESH_H__

#include <iostream>
#include <vector>
#include <array>
#include <algorithm>
#include <numeric>
#include <metis.h>
#include "CSRList.hpp"
#include "ElementSpace.hpp"

template<int D>
struct Mesh
{
    using MeshElementInfo = std::pair<CSRList<std::size_t>, std::vector<std::size_t>>;
    using FiniteElementType = typename ElementSpace<D>::Type;

    Mesh() {
        nodes().clear();
        type_offset().clear();
        ElementSpace<3> space;
        type_offset().reserve(ElementSpace<D>().all_element_types().size());
        type_offset().push_back(0);
    }

    static constexpr int dim() { 
        return D;
    }

    const Mesh& operator=(const Mesh&) = delete;

    const std::vector<double>& nodes() const noexcept { return _nodes; }
    std::vector<double>& nodes() noexcept { return _nodes; }

    MeshElementInfo& elements() noexcept { return _elements; }
    const MeshElementInfo& elements() const noexcept { return _elements; }

    MeshElementInfo elements(FiniteElementType type) const noexcept {
            auto [begin_index, end_index] = type_offset(type);
            const auto& [element_connectivity, element_ID] = elements();
            CSRList<std::size_t> local_element_connectivity(
                element_connectivity.begin() + begin_index, 
                element_connectivity.begin() + end_index);
            std::vector<std::size_t> local_element_ID(
                element_ID.begin() + begin_index, 
                element_ID.begin() + end_index);
            return {local_element_connectivity, local_element_ID};
    }
    std::pair<std::size_t, std::size_t> type_offset(FiniteElementType type) const { 
        auto i = static_cast<std::size_t>(type);
        return {type_offset()[i], type_offset()[i + 1]};
    }

    const std::vector<std::size_t>& type_offset() const noexcept {
        return _element_type_offset;
    }

    std::vector<std::size_t>& type_offset() noexcept {
        return _element_type_offset;
    }

    private:

    private:
    std::vector<double> _nodes;
    MeshElementInfo _elements;
    std::vector<std::size_t> _element_type_offset;

    friend class MeshIO;
    //friend class MeshConnectivity;

};

// not using CRTP because MeshConnectivity<> workd for Mesh<> only
template<int D>
struct MeshConnectivity
{
    MeshConnectivity(Mesh<D>& mesh) : _mesh(&mesh), _element_aggregations(D+1) {
        for (std::size_t i = 0; i <= D; ++i) {
            _collect_mesh_entities(i);
        }
        _build_all_connectivity();
        _build_vertex_adjacency_list();
		_build_orientation_of_subentities();
    }

    const CSRList<std::size_t>& connectivity(std::size_t dim0, std::size_t dim1) const { 
        return _connectivity.at({dim0, dim1}); 
    }

    const CSRList<std::size_t>& adjacent_vertices() const {
        return _adjacent_vertices;
    }

	const CSRList<std::size_t>& element_collections(int dim) const {
		return this->_element_aggregations[dim];
	}

	const CSRList<std::size_t>& orientation() const {
		return this->_orientation;
	}

    private:
    void _collect_mesh_entities(std::size_t dim) {
        const auto prime_element_type = ElementSpace<D>().prime_element_types();
        auto num_prime_elements = std::accumulate(prime_element_type.begin(),
            prime_element_type.end(),
            static_cast<std::size_t>(0),
            [this](std::size_t init, decltype(*prime_element_type.begin()) type) {
                return init + _mesh->elements(type).second.size();
            });
        const auto secondary_element_type = ElementSpace<D>().secondary_element_types();
        auto num_secondary_elements = std::accumulate(secondary_element_type.begin(),
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
                        this->_element_aggregations[dim] += std::move(this->_mesh->elements(type).first);
                    }
                });
        }
    }

    // {dim0, dim1} already exists, now construct the reverse mappping {dim1, dim0}
    void _construct_reverse_map(std::size_t dim0, std::size_t dim1) {
        auto size0 = this->_connectivity[{dim0, dim1}].size();
        auto size1 = this->_connectivity[{dim1, dim0}].size();
        if( size0 > 0 and size1 == 0) {
            this->_connectivity[{dim1, dim0}] = this->_connectivity[{dim0, dim1}].reverse();
            auto max_begin = this->_connectivity[{dim1, dim0}].size();
            auto max_end = this->_element_aggregations[dim1].size();
            for(std::size_t i = max_begin; i < max_end; ++i) {
                this->_connectivity[{dim1, dim0}].push_back(std::vector<std::size_t>());
            }
            
        }
    }

    void _build_connectivity_pair(std::size_t dim0, std::size_t dim1, int depth) {

        auto size0 = this->_connectivity[{dim0, dim1}].size();
        //auto size1 = this->_connectivity[{dim1, dim0}].size();
        if(dim0 == dim1 // Connectivity to self alwasy exsits
            or size0 != 0) { // Connectivity has been constructed
            //return;
        }
        else if(dim0 == 0 or dim1 == 0) {
            // if {dim0+dim1, 0} is empty, then construct it
            if (this->_connectivity[{dim0 + dim1, 0}].size() == 0) { 
                this->_connectivity[{dim0 + dim1, 0}] = this->_element_aggregations[dim1 + dim0];
            }
            // when (0 == dim0), construct {0, dim1}
            // reverse the CSRList and fill to # of elements
            if(dim0 < dim1) {
                _construct_reverse_map(dim1, dim0);
            }
        }
        else if(dim0 > dim1){
            _build_connectivity_pair(dim1, dim0, depth + 1);
            _construct_reverse_map(dim1, dim0);
        }
        else {
            _build_connectivity_pair(dim0, 0, depth + 1);
            _build_connectivity_pair(0, dim1, depth + 1);
            const auto& entity_dim0_to_vertex = this->_connectivity[{dim0, 0}];
            const auto& entity_vertex_to_dim1 = this->_connectivity[{0, dim1}];
            if(entity_dim0_to_vertex.size() != 0 and entity_vertex_to_dim1.size() != 0) {
                for(auto dim0_to_vertex_list: entity_dim0_to_vertex) {
                    // Find: who appears n times, where n is number of vertices in the entity;
                    std::map<std::size_t, std::size_t> counter;
                    for(const auto ivtx: dim0_to_vertex_list.data()) {
                        for (const auto& ientity: entity_vertex_to_dim1[ivtx]){
                            counter[ientity]++;
                        }
                    }

                    std::vector<std::size_t> connected_entities;
                    std::for_each(counter.begin(), counter.end(),
                        [&](decltype(counter)::value_type& pair){
                            if(pair.second == dim0_to_vertex_list.data().size()) {
                                connected_entities.push_back(pair.first);
                            }
                        }
                    );
                    this->_connectivity[{dim0, dim1}].push_back(std::move(connected_entities));
                }
            }

        }

    }

    void _build_all_connectivity() {
        for(std::size_t i = 0; i <= D; ++i) {
            for(std::size_t j = 0; j <= D; ++j) {
                _build_connectivity_pair(j, i, 0);
            }
        }
    }

    void _build_vertex_adjacency_list() {
        const auto prime_element_list = _element_aggregations[D];
        auto nnode = _mesh->nodes().size() / D;
		std::size_t expected_bandwidth = 64;
        std::vector<std::vector<std::size_t>> adjacency(nnode);
		for(auto& cache: adjacency) {
			cache.reserve(expected_bandwidth);
		}

        for(auto cell : prime_element_list) {
            const auto& vertex_list = cell.data();
            std::for_each(
                vertex_list.begin(), 
                vertex_list.end(),
                [&](std::size_t ivtx){
                    adjacency[ivtx].insert(adjacency[ivtx].end(), vertex_list.begin(), vertex_list.end());
                }
            );
        }

		for(auto& cache: adjacency) {
			// remove duplicated entries
			std::sort(cache.begin(), cache.end());
			cache.erase(std::unique(cache.begin(), cache.end()), cache.end());
			this->_adjacent_vertices.push_back(std::move(cache));
		}
    }

	static std::vector<std::size_t> _get_child_indices_in_parent(
			const std::vector<std::size_t>& child,
			const std::vector<std::size_t>& parent) {
		std::vector<std::size_t> indices;
		indices.reserve(child.size());
		for(std::size_t i = 0; indices.size() < child.size() and i < child.size(); ++i) {
			for(std::size_t j = 0; j < parent.size(); ++j) {
				if(child[i] == parent[j]) {
					indices.push_back(j);
					break;
				}
			}
		}
		return (indices.size() == child.size() ? indices : std::vector<std::size_t>());
	}

	void _build_orientation_of_subentities() {
		const auto& prime_element_list = element_collections(D);
		const auto& secondary_element_list = element_collections(D - 1);
		const auto& subentity_to_entity = connectivity(D - 1, D);
		for(std::size_t i = 0; i < subentity_to_entity.size(); ++i) {
			auto subentity_global_indices = secondary_element_list[i];
			auto base_entity_ID = subentity_to_entity[i];
			std::vector<std::size_t> subentity_orientation;
			subentity_orientation.reserve(1);
			for(auto idx : base_entity_ID) {
				const auto& base_entity_global_list = prime_element_list[idx];
				const auto& subentity_local_indices = _get_child_indices_in_parent(
						subentity_global_indices,
						base_entity_global_list
					);
				assert(not subentity_local_indices.empty());
				auto type = ElementSpace<D>::element_type(base_entity_global_list.size());
				auto orientation = ElementNumbering::subentity_indices(type, subentity_local_indices);
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
    struct Cmp{
        bool operator()(const Key& first, const Key& second) const {
            return first[0] * 10 + first[1] < second[0] * 10 + second[1];
        }
    };
    std::map<Key, CSRList<std::size_t>, Cmp> _connectivity;
    CSRList<std::size_t> _adjacent_vertices;
	CSRList<std::size_t> _orientation;
    Mesh<D> * _mesh;

};

template<int D>
struct MeshPartitioner
{
    MeshPartitioner(const Mesh<D>& mesh): _mesh(&mesh) {}


    const std::vector<idx_t>& partition(std::size_t rank, const std::string& mode = "e") const {
        if(mode == "e"){
            return _element_partitioning[rank];
        }
        else {
            return _node_partitioning[rank];
        }
    }

    void metis(idx_t num_parts = 4){
		_num_parts = num_parts;
		// calculate numbers of nodes and elements
		idx_t num_nodes = _mesh->nodes().size() / D;
		const auto& elements = _mesh->elements();
		const auto prime_element_type = ElementSpace<D>().prime_element_types();
		CSRList<std::size_t> prime_element_list;
		std::for_each(prime_element_type.begin(), prime_element_type.end(),
			[&](FiniteElementType type) {
				prime_element_list += std::move(this->_mesh->elements(type).first);
			}
		);
		idx_t num_elements = prime_element_list.size();
        _element_partitioning.resize(_num_parts);
        _node_partitioning.resize(_num_parts);

		if(_num_parts >= 2) {
			std::vector<idx_t> element_array(prime_element_list.data().begin(), prime_element_list.data().end());
			std::vector<idx_t> element_offset(prime_element_list.offset().begin(), prime_element_list.offset().end());


			idx_t* vwgt = nullptr;
			idx_t* vsize = nullptr;
			idx_t ncommon = 1;

			real_t* tpwgts = nullptr;
			idx_t objval = 1;

			idx_t options[METIS_NOPTIONS];
			METIS_SetDefaultOptions(options);
			options[METIS_OPTION_PTYPE]   = METIS_PTYPE_KWAY;
			options[METIS_OPTION_OBJTYPE] = METIS_OBJTYPE_CUT;
			options[METIS_OPTION_CTYPE]   = METIS_CTYPE_SHEM;
			options[METIS_OPTION_IPTYPE]  = METIS_IPTYPE_GROW;
			options[METIS_OPTION_RTYPE]   = -1;
			options[METIS_OPTION_DBGLVL]  = 0;
			options[METIS_OPTION_UFACTOR] = -1;
			options[METIS_OPTION_MINCONN] = 0;
			options[METIS_OPTION_CONTIG]  = 0;
			options[METIS_OPTION_SEED]    = -1;
			options[METIS_OPTION_NITER]   = 10;
			options[METIS_OPTION_NCUTS]   = 1;
			// buffer for element and node attributions
			std::vector<idx_t> epart(num_elements), npart(num_nodes);
			auto status = METIS_PartMeshDual(&num_elements, &num_nodes, 
				//mesh.eptr.data(), mesh.eind.data(), 
				element_offset.data(), element_array.data(),
				vwgt, vsize, &ncommon, &num_parts,
				tpwgts, options, &objval,
				epart.data(), npart.data()
			);
			assert(status == METIS_OK);
			for(std::size_t i = 0; i < epart.size(); ++i) {
				auto rank = epart[i];
				_element_partitioning[rank].push_back(i);
			}
			for(std::size_t i = 0; i < npart.size(); ++i) {
				auto rank = npart[i];
				_node_partitioning[rank].push_back(i);
			}
		}
		else {
			std::iota(_element_partitioning[0].begin(), _element_partitioning[0].end(), 0);
			std::iota(_node_partitioning[0].begin(), _node_partitioning[0].end(), 0);
		}
    }
    private:
    const Mesh<D>* _mesh;
	std::size_t _num_parts;
    std::vector<std::vector<idx_t>> _element_partitioning;
    std::vector<std::vector<idx_t>> _node_partitioning;
};

#endif // __MESH_H__
