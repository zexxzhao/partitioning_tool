#ifndef __MESH_PARTITIONER_H__
#define __MESH_PARTITIONER_H__
#include "CSRList.hpp"
#include "Reorder.hpp"
#include "ElementSpace.hpp"


template<typename DerivedClass> struct MeshPartitioner {};

template<template <int> typename DerivedClass, int D>
struct MeshPartitioner<DerivedClass<D>>
{
	using Derived = DerivedClass<D>;
    MeshPartitioner(const Derived& mesh): _mesh(&mesh) {}


	const CSRList<std::size_t>& part(const std::string& mode = "e") const {
		if(mode == "e") {
			return this->_element_attribution;
		}
		else if(mode == "n"){
			return this->_node_attribution;
		}
		assert(mode == "e" or mode == "n");
		return this->_node_attribution;
	}
	std::pair<std::vector<std::size_t>, std::vector<std::size_t>>
		part(std::size_t rank) const {
		auto element_attribution = this->_element_attribution[rank];
		auto node_attribution = this->_node_attribution[rank];
		return {element_attribution, node_attribution};
	}
    std::vector<std::size_t> part(std::size_t rank, const std::string& mode = "e") const {
        if(mode == "e"){
            return _element_attribution[rank];
        }
        else if(mode == "n") {
            return _node_attribution[rank];
        }
		return {};
    }
	
	auto local_mesh_data(std::size_t rank) const {
		return _build_local_mesh(rank);
	}
    void metis(idx_t num_parts = 4) {
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
		std::vector<std::vector<idx_t>> _element_partitioning;
		std::vector<std::vector<idx_t>> _node_partitioning;
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

		// store partitioning results in CSRList
		std::for_each(_element_partitioning.begin(), _element_partitioning.end(),
				[this](const auto& e) {
					this->_element_attribution.push_back(e);
				});
		std::for_each(_node_partitioning.begin(), _node_partitioning.end(),
				[this](const auto& e) {
					this->_node_attribution.push_back(e);
				});

    }
	// renumbering
    private:
	std::vector<std::size_t> _collect_nodes(std::size_t rank) const {
		auto local_elements = this->_element_attribution[rank];
		auto local_nodes = this->_node_attribution[rank];

		std::vector<std::size_t> all_local_nodes;
		all_local_nodes.reserve(_mesh->elements().first.size() * 4);
		auto element_local_to_global = this->_element_attribution[rank];
		auto elements = _mesh->elements(D).first;
		for(auto ielem : element_local_to_global) {
			auto element_vertices = elements[ielem];
			all_local_nodes.insert(
					all_local_nodes.end(), 
					element_vertices.begin(), 
					element_vertices.end());
		}
		std::sort(all_local_nodes.begin(), all_local_nodes.end());
		all_local_nodes.erase(std::unique(all_local_nodes.begin(), all_local_nodes.end()), all_local_nodes.end());
		return all_local_nodes;
	}

	typedef short ghosted_type;
	std::vector<ghosted_type> _find_ghosted_node(size_t rank, const std::vector<std::size_t>& nodes) const {

		auto sorted = std::all_of(nodes.cbegin(), nodes.cend() - 1, 
				[](const std::size_t& n) {
					return n < *(&n+1);
				});
		assert(sorted);

		std::vector<ghosted_type> ghosted(nodes.size(), 0);
		auto owned_local_nodes = this->_node_attribution[rank];

		std::sort(owned_local_nodes.begin(), owned_local_nodes.end());
		
		for(auto it = nodes.cbegin(), it_owned = owned_local_nodes.cbegin(); it != nodes.cend(); ++it) {
			it_owned = std::lower_bound(it_owned, owned_local_nodes.cend(), *it);
			ghosted[std::distance(nodes.cbegin(), it)] = *it_owned != *it;
		}

		return ghosted;
	}

	CSRList<std::size_t, std::size_t, std::false_type> _local_vertex_connectivity(const CSRList<std::size_t>& elements) const {
		// elements should use local node ID
		auto nnode = *std::max_element(elements.data().cbegin(), elements.data().cend()) + 1;
		std::size_t expected_bandwidth = 24;

        std::vector<std::vector<std::size_t>> adjacency(nnode);
		for(auto& cache: adjacency) {
			cache.reserve(expected_bandwidth);
		}

        for(auto cell : elements) {
            const auto& vertex_list = cell.data();
            std::for_each(
                vertex_list.cbegin(), 
                vertex_list.cend(),
                [&](std::size_t ivtx){
                    adjacency.at(ivtx).insert(adjacency.at(ivtx).end(), vertex_list.cbegin(), vertex_list.cend());
                }
            );
        }
		
		CSRList<std::size_t, std::size_t, std::false_type> local_graph;
		for(auto& cache: adjacency) {
			// remove duplicated entries
			std::sort(cache.begin(), cache.end());
			cache.erase(std::unique(cache.begin(), cache.end()), cache.end());
			local_graph.push_back(std::move(cache));
		}
		return local_graph;
	}

	auto _build_local_mesh(std::size_t rank) const {
		//
		// global to local mapping
		//
		auto nodal_local_to_global = _collect_nodes(rank);
		std::unordered_map<std::size_t, std::size_t> g2l;
		g2l.reserve(nodal_local_to_global.size());
		for(std::size_t inode = 0; inode < nodal_local_to_global.size(); ++inode) {
			g2l.insert({nodal_local_to_global[inode], inode});
		}

		//
		// elements using local nodal ID
		//
		CSRList<std::size_t> local_elements;
		auto element_local_to_global = this->_element_attribution[rank];
		auto elements = _mesh->elements(D).first;
		for(auto ielem : element_local_to_global) {
			auto vertices = elements[ielem];
			local_elements.push_back(vertices);
		}
		//return std::make_tuple(0, 0, 0);

		std::for_each(local_elements.data().begin(), local_elements.data().end(),
				[&](std::size_t& a){ a = g2l[a]; });
		//
		//	vertex connectivity
		//
		auto nodal_connectivity = _local_vertex_connectivity(local_elements);
		auto mapping = reordering::BandwidthReduction(nodal_connectivity)(); 

		// permute to ensure ghosted nodes come last
		auto ghosted = _find_ghosted_node(rank, nodal_local_to_global);
		auto num_ghosted_nodes = std::accumulate(ghosted.cbegin(), ghosted.cend(), static_cast<std::size_t>(0));		
		std::vector<std::size_t> mapping_next(ghosted.size());
		std::iota(mapping_next.begin(), mapping_next.end(), 0);
		for(std::size_t i = 0, j = num_ghosted_nodes; i < mapping.size() - num_ghosted_nodes; ++i) {
			if(not ghosted[i]) continue;
			
			while(ghosted[j]) ++j;
			using std::swap;
			// i --> ghosted node; j --> unghosted node
			swap(mapping_next[i], mapping_next[j]);
			swap(ghosted[i], ghosted[j]);
		}
		{
			auto it = std::upper_bound(ghosted.cbegin(), ghosted.cend(), 0);
			assert(std::all_of(it, ghosted.cend(), [](const auto & a) { return a; } ));
		}
		//
		// permute vertices
		//
		std::vector<std::size_t> nodal_local_to_global_tmp(nodal_local_to_global.size());
		for(std::size_t inode = 0; inode != nodal_local_to_global.size(); ++inode) {
			auto index = mapping_next[mapping[inode]];
			nodal_local_to_global_tmp[index] = nodal_local_to_global[inode];
		}
		nodal_local_to_global = nodal_local_to_global_tmp;
		//
		// permute elements
		//
		std::for_each(local_elements.data().begin(), local_elements.data().end(),
				[&](std::size_t& a) { a = mapping[a]; a = mapping_next[a]; });

		//
		// vertex adjacency
		//
		
		CSRList<std::size_t, std::size_t, std::false_type> local_adjacency;
		auto graph = this->_mesh->adjacent_vertices();
		for(std::size_t i = 0; i != nodal_local_to_global.size(); ++i) {
			local_adjacency.push_back(graph[nodal_local_to_global[i]]);
		}
		return std::make_tuple(nodal_local_to_global, local_elements, local_adjacency);
	}


    const Derived* _mesh;
	std::size_t _num_parts;
	CSRList<std::size_t> _element_attribution;
	CSRList<std::size_t> _node_attribution;
};
#endif // __MESH_PARTITIONER_H__
