#ifndef __MESH_IO_H__
#define __MESH_IO_H__
#include <fstream>
#include <iostream>
#include <string>
#include <map>
#include <numeric>

#include "Mesh.hpp"

struct MeshIO
{

    enum class MshGenerator : int {
        GMSH = 0, 
        ANSA = 1
    };

    template<int D>
    static int read(Mesh<D>& mesh, const std::string& filename, MshGenerator type = MshGenerator::GMSH) {
        // get the extension
        auto ext = filename.substr(filename.find_last_of('.') + 1);
        if(ext == "msh" or ext == "gmsh") {
            return _read_gmsh(mesh, filename, type);
        }
        return -1;
    }

	template<int D>
	static int write(const MeshPartitioner<D>& mesh, const std::string& filename) {
        // get the extension
        auto ext = filename.substr(filename.find_last_of('.') + 1);
		if(ext == "h5" or ext == ".hdf5") {
			_write_h5(mesh);
		}
		return -1;
	}

    private:
    template<typename T> 
    static const char* str_parsing(const char* str, T& value) {
        char* endptr = nullptr;
        if constexpr(std::is_integral_v<T>) {
            value = static_cast<T>(std::strtol(str, &endptr, 10));
        }
        if constexpr(std::is_floating_point_v<T>) {
            value = static_cast<T>(std::strtod(str, &endptr));
        }
        return endptr;

        if constexpr(std::is_same_v<T, void>) {
            char* tmp0, *tmp1;
            std::strtol(str, &tmp0, 10);
            std::strtod(str, &tmp1);
            if(tmp0 != tmp1) {
                //floating point conversion
                tmp0 = tmp1;
            }
            return tmp0;
        }

    }
    template<int D>
	static int _read_gmsh(Mesh<D>& mesh, const std::string& filename, MshGenerator type) {

        static_assert(D == std::decay_t<decltype(mesh)>::dim());
        mesh.nodes().clear();

		std::ifstream fmesh(filename);
        assert(fmesh.good());

		std::string line;

		std::getline(fmesh, line); // "#MeshFormat"
		std::getline(fmesh, line); // Version info

		double version;
		int tmp[2];
		std::sscanf(line.c_str(), "%lf %d %d", &version, tmp, tmp + 1);
		// find version gmsh2.2
		if(std::abs(version - 2.2) < 1e-6) { 
			return _read_gmsh22(mesh, fmesh, type);
		}
		// find version gmsh4.0
		else if(std::abs(version - 4.0) < 1e-6) {
			return _read_gmsh40(mesh, fmesh, type);
		}
		// find version gmsh4.1
		else if(std::abs(version - 4.1) < 1e-6) {
			return _read_gmsh41(mesh, fmesh, type);
		}
		else {
			std::cerr << "Unknown gmsh format: " << version << std::endl;
			return -1;
		}
        fmesh.close();
        return 0;
	}
    template<int D>
	static int _read_gmsh22(Mesh<D>& mesh, std::ifstream& fmesh, MshGenerator type = MshGenerator::GMSH) {
        std::string line;
        std::getline(fmesh, line); // "$EndMeshFormat"
        std::getline(fmesh, line); // "$Nodes"
		// read nodes
        std::getline(fmesh, line); // Number of nodes
        std::size_t nnodes;
        //sscanf(line.c_str(), "%zu\n", &nnodes);
        str_parsing(line.c_str(), nnodes);

        std::vector<double>& node_coordinates = mesh.nodes(); 
        node_coordinates.reserve(nnodes * mesh.dim());

        int tmp;
        double x[D];
        for(std::size_t i = 0; i < nnodes; ++i) {
            std::getline(fmesh, line);
            const char* p = line.c_str();

            p = str_parsing(p, tmp);
            if constexpr(D >= 1) p = str_parsing(p, x[0]);
            if constexpr(D >= 2) p = str_parsing(p, x[1]);
            if constexpr(D >= 3) p = str_parsing(p, x[2]);
            node_coordinates.insert(node_coordinates.end(), x, x + D);
            //node_csrlist.push_back(std::vector<typename std::decay_t<decltype(node_csrlist)>::data_type>(x, x + D));
        }

        // read elements
        // initialize element map
        std::map<FiniteElementType, typename Mesh<D>::MeshElementInfo> _element_all;
        {
            for(auto type : ElementSpace<D>().all_element_types()) {
                auto index = static_cast<int>(type);
                _element_all[type];
            }
        }

        std::getline(fmesh, line); // "$EndNodes"
        std::getline(fmesh, line); // "$Elements"
        std::getline(fmesh, line); // number of Elements
        // fill vertex element
        {
            auto& [csrlist, element_id] = _element_all.at(FiniteElementType::Vertex);
            csrlist.data().resize(nnodes);
            csrlist.offset().resize(nnodes + 1);

            std::iota(csrlist.data().begin(), csrlist.data().end(), 0);
            std::iota(csrlist.offset().begin(), csrlist.offset().end(), 0);

            element_id.resize(nnodes);
        }
        std::size_t nelements;
        str_parsing(line.c_str(), nelements);

        for(std::size_t i = 0; i < nelements; ++i) {
            /*
             * Line content:
             * ElementID(1-based), ElementType, Number of tags, <tags...>, <node list...>
             */
            std::getline(fmesh, line);
            const char* p = line.c_str();
            // ElementID
            p = str_parsing(p, tmp);
            // ElementType
            p = str_parsing(p, tmp);
            auto element_type = static_cast<typename ElementSpace<D>::Type>(tmp);
            int num_vertices = _num_vertices<D>(element_type);
            auto& info = _element_all[element_type];
            // Number of tags (==2)
            int num_tags = 2;
            p = str_parsing(p, num_tags);
            int ids[2];
            for(std::size_t j = 0; j < num_tags; ++j) {
                p = str_parsing(p, ids[j]);
            }
            auto& element_node_list = std::get<0>(info);
            auto& element_ID = std::get<1>(info);

            element_ID.push_back(ids[static_cast<int>(type)]);

            std::vector<typename std::decay_t<decltype(element_node_list)>::data_type> node_list(num_vertices);
            for(int j = 0; j < num_vertices; j++) {
                p = str_parsing(p, node_list[j]);
                node_list[j]--;
            }

			// for Pyramid element, renumbering by swap node_list[2] and node_list[3]
			if (element_type == FiniteElementType::Pyramid) {
				std::swap(node_list[2], node_list[3]);
			}
            element_node_list.push_back(node_list);
        }

        // immigrate data from _element_all to mesh.elements()
        {
            auto& type_offset = mesh.type_offset();
            auto& [element_info, element_ID] = mesh.elements();
            for(const auto& [key, value] : _element_all) {
                element_info += std::get<0>(value);
                element_ID.insert(element_ID.end(), std::get<1>(value).begin(), std::get<1>(value).end());
                type_offset.push_back(element_info.size());
                //std::cout << "type_offset: " << type_offset.size() << std::endl;
                //printf("type_offset: %zu %zu\n, ", type_offset.size(), type_offset.back());
                //printf("size = %zu\n", [key].first.size());
            }

        }

		return 1;
	}

    template<int D>
	static int _read_gmsh40(Mesh<D>& mesh, std::ifstream& fmesh, MshGenerator type = MshGenerator::GMSH) {
		std::cerr << "Not implemented: " << __func__ << std::endl;
		return -2;
	}

    template <int D>
	static int _read_gmsh41(Mesh<D>& mesh, std::ifstream& fmesh, MshGenerator type = MshGenerator::GMSH) {
		std::cerr << "Not implemented: " << __func__ << std::endl;
		return -2;
	}
	template<int D>
	static int _write_h5(const MeshPartitioner<D>& mesh) {
		return -1;
	}
    template <int D>
    static constexpr int _num_vertices(typename ElementSpace<D>::Type element_type) {
        typename ElementSpace<D>::template Element<ElementSpace<D>::Type::Vertex> E;
        switch(element_type) {
            case ElementSpace<D>::Type::Vertex:
                return ElementSpace<D>::template Element<ElementSpace<D>::Type::Vertex>::num_vertices();
            case ElementSpace<D>::Type::Line:
                return ElementSpace<D>::template Element<ElementSpace<D>::Type::Line>::num_vertices();
            case ElementSpace<D>::Type::Triangle:
                return ElementSpace<D>::template Element<ElementSpace<D>::Type::Triangle>::num_vertices();
            case ElementSpace<D>::Type::Quadrangle:
                return ElementSpace<D>::template Element<ElementSpace<D>::Type::Quadrangle>::num_vertices();
            case ElementSpace<D>::Type::Tetrahedron:
                return ElementSpace<D>::template Element<ElementSpace<D>::Type::Tetrahedron>::num_vertices();
            case ElementSpace<D>::Type::Hexahedron:
                return ElementSpace<D>::template Element<ElementSpace<D>::Type::Hexahedron>::num_vertices();
            case ElementSpace<D>::Type::Prism:
                return ElementSpace<D>::template Element<ElementSpace<D>::Type::Prism>::num_vertices();
            case ElementSpace<D>::Type::Pyramid:
                return ElementSpace<D>::template Element<ElementSpace<D>::Type::Pyramid>::num_vertices();
            case ElementSpace<D>::Type::IGA2:
                return ElementSpace<D>::template Element<ElementSpace<D>::Type::IGA2>::num_vertices();
            default:
                return -1;
        }
    }
};
#endif // __MESH_IO_H__
