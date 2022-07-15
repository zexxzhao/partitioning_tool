#ifndef __ELEMENT_SPACE_H__
#define __ELEMENT_SPACE_H__

#include <type_traits>
#include <vector>

enum class FiniteElementType : int {
    Vertex = 0,
    Line = 1,
    Triangle = 2,
    Quadrangle = 3,
    Tetrahedron = 4,
    Hexahedron = 5,
    Prism = 6,
    Pyramid = 7,
    IGA2 = 8,
    All = 99
};

class ElementNumbering
{
	public:
	static std::vector<std::size_t> subentity_indices(FiniteElementType type, std::size_t i) {
		if(type == FiniteElementType::Line) {
			switch(i) {
				case 0: 
					return {0};
				case 1: 
					return {1};
				default: 
					return {};
			}
		}
		else if(type == FiniteElementType::Triangle) {
			switch(i) {
				case 0:
					return {1, 2};
				case 1:
					return {2, 0};
				case 2:
					return {0, 1};
				default:
					return {};
			}
		}
		else if(type == FiniteElementType::Quadrangle) {
			switch(i) {
				case 0:
					return {0, 1};
				case 1:
					return {0, 2};
				case 2:
					return {1, 3};
				case 3:
					return {2, 3};
				default:
					return {};
			}
		}
		else if(type == FiniteElementType::Tetrahedron) {
			switch(i) {
				case 0:
					return {1, 2, 3};
				case 1:
					return {0, 2, 3};
				case 2:
					return {0, 1, 3};
				case 3:
					return {0, 1, 2};
				default:
					return {};
			}
		}
		else if(type == FiniteElementType::Prism) {
			switch(i) {
				case 0:
					return {0, 1, 2};
				case 1:
					return {0, 1, 3, 4};
				case 2:
					return {0, 2, 3, 5};
				case 3:
					return {1, 2, 4, 5};
				case 4:
					return {3, 4, 5};
				default:
					return {};
			}
		}
		else if(type == FiniteElementType::Pyramid) {
			switch(i) {
				case 0:
					return {0, 1, 2, 3};
				case 1:
					return {0, 1, 4};
				case 2:
					return {0, 2, 4};
				case 3:
					return {1, 3, 4};
				case 4:
					return {2, 3, 4};
				default:
					return {};
			}
		}
		else if(type == FiniteElementType::Hexahedron) {
			switch(i) {
				case 0:
					return {0, 1, 2, 3};
				case 1:
					return {0, 1, 4, 5};
				case 2:
					return {0, 2, 4, 6};
				case 3:
					return {1, 3, 5, 7};
				case 4:
					return {2, 3, 6, 7};
				case 5:
					return {4, 5, 6, 7};
				default:
					return {};
			}
		}
		return {};
	}

	static std::size_t subentity_indices(FiniteElementType type, const std::vector<std::size_t>& indices) {
		if(type == FiniteElementType::Line) {
			assert(indices.size() == 1);
			return indices[0];
		}
		else if(type == FiniteElementType::Triangle) {
			assert(indices.size() == 2);
			return 3 - indices[0] - indices[1];
		}
		else if(type == FiniteElementType::Quadrangle) {
			assert(indices.size() == 2);
			return (indices[0] + indices[1]) >> 1;
		}
		else if(type == FiniteElementType::Tetrahedron) {
			assert(indices.size() == 3);
			return 6 - indices[0] - indices[1] - indices[2];
		}
		else if(type == FiniteElementType::Prism) {
			assert(indices.size() == 3 or indices.size() == 4);

			if(indices.size() == 3) {
				return (indices[0] + indices[1] + indices[2] == 3 ? 0 : 4);
			}
			else { // indices.size() == 4
				auto sum = indices[0] + indices[1] + indices[2] + indices[3];
				return (sum - 6) >> 1;
			}
			return -1;
		}
		else if(type == FiniteElementType::Pyramid) {
			assert(indices.size() == 3 or indices.size() == 4);
			if(indices.size() == 4) {
				return 0;
			}
			else {
				auto sum = indices[0] + indices[1] + indices[2];
				return sum - (sum >= 8 ? 5 : 4);
			}
		}
		else if(type == FiniteElementType::Hexahedron) {
			assert(indices.size() == 4);
			const int mapping[] = {0, -1, 1, 2, -1, 3, 4, -1, 5};
			auto sum = indices[0] + indices[1] + indices[2] + indices[3];
			return mapping[(sum - 6) >> 1];
		}
		return -1;
	}	
};

template <int D> struct ElementSpace
{

    using Type = FiniteElementType;
    /*
    static constexpr Type type_values[9] = {
        Type::Vertex, 
        Type::Line, 
        Type::Triangle, 
        Type::Quadrangle, 
        Type::Tetrahedron, 
        Type::Hexahedron, 
        Type::Prism, 
        Type::Pyramid, 
        Type::IGA2
    };
    */



    template<Type type> struct is_compatible : std::integral_constant<bool, 
        (D == 1 and type <= Type::Line)
        or (D == 2 and type <= Type::Quadrangle)
        or (D == 3)> {};
    template<Type type> inline static constexpr bool is_compatible_v = is_compatible<type>::value;

    static std::vector<Type> prime_element_types() {
        if constexpr (D == 0) {
            return {Type::Vertex};
        }
        else if constexpr (D == 1) {
            return {Type::Line};
        }
        else if constexpr (D == 2) {
            return {Type::Triangle, Type::Quadrangle};
        }
        else if constexpr (D == 3) {
            return {Type::Tetrahedron, Type::Hexahedron, Type::Prism, Type::Pyramid, Type::IGA2};
        }
        else {
            return {};
        }
    }

    static std::vector<Type> secondary_element_types() {
        if constexpr (D == 1) {
            return {Type::Vertex};
        }
        else if constexpr (D == 2) {
            return {Type::Line};
        }
        else if constexpr (D == 3) {
            return {Type::Triangle, Type::Quadrangle};
        }
        else {
            return {};
        }
    }

    static std::vector<Type> all_element_types() {
        if constexpr (D == 0) {
            return {Type::Vertex};
        }
        else if constexpr (D == 1) {
            return {Type::Vertex, Type::Line};
        }
        else if constexpr (D == 2) {
            return {Type::Vertex, Type::Line, Type::Triangle, Type::Quadrangle};
        }
        else if constexpr (D == 3) {
            return {Type::Vertex, 
                Type::Line, 
                Type::Triangle, Type::Quadrangle, 
                Type::Tetrahedron, Type::Hexahedron, Type::Prism, Type::Pyramid, Type::IGA2};
        }
        else {
            return {};
        }

    }
    static int topologic_dim(Type type) {
        if(type == Type::Vertex) {
            return 0;
        }
        else if(type == Type::Line) {
            return 1;
        }
        else if(type == Type::Triangle or type == Type::Quadrangle) {
            return 2;
        }
        else if(type == Type::Tetrahedron 
            or type == Type::Hexahedron 
            or type == Type::Prism 
            or type == Type::Pyramid) {
            return 3;
        }
        return -1;
    }
	static Type element_type(int nvtx, int dim = D) {
		if(dim == 0) {
			return Type::Vertex;
		}
		else if(dim == 1) {
			return Type::Line;
		}
		else if(dim == 2) {
			return (nvtx == 3 ? Type::Triangle : Type::Quadrangle);
		}
		else if(dim == 3) {
			if(nvtx == 4)
				return Type::Tetrahedron;
			else if(nvtx == 5)
				return Type::Pyramid;
			else if(nvtx == 6)
				return Type::Prism;
			else if(nvtx == 8)
				return Type::Hexahedron;
			else if(nvtx == 27)
				return Type::IGA2;
		}
		return Type::All;
	}
    template<Type type, typename std::enable_if_t<is_compatible_v<type>>* = nullptr> struct Element
    { 

        static constexpr int dim() {
            return D;
        };

        static constexpr int geometric_dim() {
            return dim();
        }

        static constexpr int topologic_dim() {
            if constexpr(type == Type::Vertex) {
                return 0;
            }
            else if constexpr(type == Type::Line) {
                return 1;
            }
            else if constexpr(type == Type::Triangle or type == Type::Quadrangle) {
                return 2;
            }
            else if constexpr(type == Type::Tetrahedron 
                or type == Type::Hexahedron 
                or type == Type::Prism 
                or type == Type::Pyramid) {
                return 3;
            }
            else {
                return -1;
            }
        }

        static constexpr int num_vertices() {
            if constexpr(type == Type::Vertex) {
                return 1;
            }
            else if constexpr(type == Type::Line) {
                return 2;
            }
            else if constexpr(type == Type::Triangle) {
                return 3;
            }
            else if constexpr(type == Type::Quadrangle) {
                return 4;
            }
            else if constexpr(type == Type::Tetrahedron) {
                return 4;
            }
            else if constexpr(type == Type::Hexahedron) {
                return 8;
            }
            else if constexpr(type == Type::Prism) {
                return 6;
            }
            else if constexpr(type == Type::Pyramid) {
                return 5;
            }
            else if constexpr(type == Type::IGA2) {
                return 27;
            }
            else{
                return -1;
            }
        }
    };
};


#endif // __ELEMENT_SPACE_H__
