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