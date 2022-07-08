#ifndef __MESH_H__
#define __MESH_H__

#include <vector>
//#include <map>
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
    //auto& _element_info() noexcept { return _element; }



    private:
    //CSRList<double> _nodes;
    std::vector<double> _nodes;
    //std::map<FiniteElementType, MeshElementInfo> _elements;
    MeshElementInfo _elements;
    std::vector<std::size_t> _element_type_offset;

    friend class MeshIO;
    //friend class MeshConnectivity;

};

/*
template<int D>
struct MeshConnectivity
{
    void collect_mesh_entities(int dim) {
        const auto prime_element_type = ElementSpace<D>::prime_element_type();
        const auto secondary_element_type = ElementSpace<D>::secondary_element_type();

    }
    MeshConnectivity(Derived& mesh) : _mesh(&mesh) {}

    void build_connectivity_pair(int dim0, int dim1) {
        collect_mesh_entities(dim0);
        collect_mesh_entities(dim1);

        if(dim0 == dim1) {
            // Connectivity to self alwasy exsits
            return;
        }
        else if (dim0 == 0 or dim1 == 0) {
            build_connectivity_pair(0, dim0 + dim1);
        }
        else {
            build_connectivity_pair(0, dim0);
            build_connectivity_pair(0, dim1);

            if constexpr (dim0 > dim1) {
                build_connectivity_pair(dim1, dim0);
            }
            else {

            }
        }

    }

    void build_all_connectivity() {
    }

    private:
    std::vector<int, CSRList<std::size_t>> _element_aggregations;
    std::vector<std::vector<CSRList<std::size_t>>> _connectivity;
    Mesh<D> * _mesh;
};
*/
#endif // __MESH_H__