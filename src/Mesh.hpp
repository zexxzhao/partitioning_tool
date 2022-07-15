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
