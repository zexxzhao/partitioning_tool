#ifndef __MESH_H__
#define __MESH_H__

#include <iostream>
#include <vector>
#include <array>
#include <algorithm>
#include <numeric>
#include <metis.h>

#include "MeshConnectivity.hpp"
#include "MeshPartitioner.hpp"

template<int D>
struct Mesh : public MeshConnectivity<Mesh<D>>, public MeshPartitioner<Mesh<D>>
{
    using MeshElementInfo = std::pair<CSRList<std::size_t>, std::vector<std::size_t>>;
    using FiniteElementType = typename ElementSpace<D>::Type;

	//using MeshConnectivity<Mesh<D>>::MeshConnectivity;
	//using MeshPartitioner<Mesh<D>>::MeshPartitioner;
    Mesh() : MeshConnectivity<Mesh<D>>(*this), MeshPartitioner<Mesh<D>>(*this) {
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

	MeshElementInfo elements(int dim) const noexcept {
		assert(0 <= dim && dim <= D);
        /*
        const auto prime_element_type = ElementSpace<D>().prime_element_types();
        auto num_prime_elements = std::accumulate(prime_element_type.begin(),
            prime_element_type.end(),
            static_cast<std::size_t>(0),
            [this](std::size_t init, decltype(*prime_element_type.begin()) type) {
                return init + this->elements(type).second.size();
            });
        const auto secondary_element_type = ElementSpace<D>().secondary_element_types();
        auto num_secondary_elements = std::accumulate(secondary_element_type.begin(),
            secondary_element_type.end(),
            static_cast<std::size_t>(0),
            [this](std::size_t init, decltype(*prime_element_type.begin()) type) {
                return init + this->elements(type).second.size();
            });
        auto num_vertices = this->nodes().size() / D;
        */
        const auto & element_types = ElementSpace<D>().all_element_types();
        std::size_t begin_index, end_index;

        switch (dim) {
            case 0:
                begin_index = type_offset(FiniteElementType::Vertex).first;
                end_index = type_offset(FiniteElementType::Vertex).second;
                break;
            case 1:
                begin_index = type_offset(FiniteElementType::Line).first;
                end_index = type_offset(FiniteElementType::Line).second;
                break;
            case 2:
                begin_index = type_offset(FiniteElementType::Triangle).first;
                end_index = type_offset(FiniteElementType::Quadrangle).second;
                break;
            case 3:
                begin_index = type_offset(FiniteElementType::Tetrahedron).first;
                end_index = type_offset(FiniteElementType::IGA2).second;
                break;
            default:
                begin_index = 0;
                end_index = 0;
                break;
        }

        const auto& [element_connectivity, element_ID] = elements();
        CSRList<std::size_t> local_element_connectivity(
            element_connectivity.begin() + begin_index, 
            element_connectivity.begin() + end_index);
        std::vector<std::size_t> local_element_ID(
            element_ID.begin() + begin_index, 
            element_ID.begin() + end_index);
        return {local_element_connectivity, local_element_ID};
        /*
		std::for_each(element_types.begin(), element_types.end(),
		[dim, this, &local_element_connectivity, &local_element_ID](FiniteElementType type) {
			auto topologic_dim = ElementSpace<D>().topologic_dim(type);
			if(topologic_dim == dim) {
                auto info = this->elements(type);
				local_element_connectivity += std::move(info.first);
                local_element_ID.insert(local_element_ID.end(),
                    info.second.begin(), info.second.end());
			}
		});
        */
		//return {local_element_connectivity, local_element_ID};
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

	void init() {
		MeshConnectivity<Mesh<D>>::init();
	}
    private:

    private:
    std::vector<double> _nodes;
    MeshElementInfo _elements;
    std::vector<std::size_t> _element_type_offset;

    friend class MeshIO;

};


#endif // __MESH_H__
