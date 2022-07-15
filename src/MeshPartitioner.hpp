#ifndef __MESH_PARTITIONER_H__
#define __MESH_PARTITIONER_H__
#include "CSRList.hpp"
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
		return {};
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
    private:
    const Derived* _mesh;
	std::size_t _num_parts;
	CSRList<std::size_t> _element_attribution;
	CSRList<std::size_t> _node_attribution;
};
#endif // __MESH_PARTITIONER_H__
