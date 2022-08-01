#ifndef __HDF5FILE_H__
#define __HDF5FILE_H__

#include <memory>
#include <highfive/H5File.hpp>

#include "Mesh.hpp"

namespace h5 = HighFive;
class HDF5File
{
public:
    /**
     * @brief Construct a new HDF5File object
     *  The HDF5File is based HighFive and HDF5. It provides read/write access to common stl templates.
     *  It also has interface to Mesh<int>
     * @param filename
     * @param open_flag: "r" (Read) or "w" (Write)
     */
    HDF5File(std::string filename, std::string open_flag) : _file(nullptr)
    {
        if (open_flag == "w")
        {
            _file = std::make_unique<h5::File>(filename, h5::File::ReadWrite | h5::File::Create | h5::File::Truncate);
        }
        else if (open_flag == "r")
        {
            _file = std::make_unique<h5::File>(filename, h5::File::ReadOnly | h5::File::Create);
        }
        else
        {
            assert(false and "Invalid open flag");
        }
    }

    /**
     * @brief write std::vector 
     * The vector is stored at "datapath/vector/0"
     * @tparam T typename in std::vector
     * @param vec: std::vector to write
     * @param datapath: path to data 
     */
    template <typename T>
    void write(const std::vector<T> &vec, std::string datapath)
    {
        // auto dataset = _file->createDataSet<T>(datapath, h5::DataSpace::From(vec));
        // dataset.write(vec);
        _write_vector(vec, datapath);
    }

    /**
     * @brief write a scalar (including integer or floating point)
     * 
     * @tparam T scalar type
     * @tparam  Anonymous typename 
     * @param data: data to write
     * @param datapath: path to data
     */
    template <typename T, typename = std::enable_if_t<std::is_scalar_v<T>>>
    void write(const T &data, std::string datapath)
    {
        //std::vector<T> vec({data});
        //write(vec, datapath);
        _write_scalar(data, datapath);
    }

    /**
     * @brief write a tuple
     * 
     * @tparam Ts 
     * @param pack: data to write
     * @param datapath: path to data
     */
    template <typename... Ts>
    void write(const std::tuple<Ts...> &pack, std::string datapath)
    {
        _write_tuple<0, Ts...>(pack, datapath);
    }

    /**
     * @brief write a CSRList
     * 
     * @tparam T 
     * @tparam U 
     * @tparam is_directed 
     * @param csrlist 
     * @param datapath 
     */
    template <typename T, typename U, bool is_directed>
    void write(const CSRList<T, U, std::bool_constant<is_directed>>& csrlist, std::string datapath) {
        _write_csrlist(csrlist, datapath);
    }
    /**
     * @brief write a mesh
     * 
     * @tparam D: integer, dimension
     * @param mesh: mesh to write 
     * @param datapath: path to data
     */
    template <int D>
    void write(const Mesh<D> &mesh, std::string datapath)
    {
        static_assert(0 <= D and D <= 3, "D must be between 0 and 3");
        // write global data, including nodal coordinates, element, and element ID
        write(mesh.nodes(), "node");
        if constexpr(D == 3) {
            // D == 2
            {
                auto info = mesh.elements(2);
                write(info.first, "secondary/element");
                write(info.second, "secondary/ID");
            }
            // D == 3
            {
                auto info = mesh.elements(3);
                write(info.first, "prime/element");
                write(info.second, "prime/ID");
            }
        }
        else if constexpr (D == 2) {}
        else if constexpr (D == 1) {}
        else if constexpr (D == 0) {}
        else {}

        // write local data
        auto num_parts = mesh.num_partitions();
        for (std::size_t i = 0; i < num_parts; ++i)
        {
            auto part = mesh.local_mesh_data(i);
            //write(part, datapath);
        }
    }

private:
    /**
     * @brief add "/" in the path if it does not exist
     * 
     * @param path 
     */
    static void regulerize_path(std::string &path)
    {
        if (path.back() != '/')
        {
            path += '/';
        }
    }

    /**
     * @brief 
     * 
     * @tparam Iterator 
     * @tparam std::iterator_traits<Iterator>::value_type 
     * @param begin 
     * @param end 
     * @param datapath 
     */
    template <typename Iterator, typename ValueType = typename std::iterator_traits<Iterator>::value_type>
    void _write_1D_array(Iterator begin, Iterator end, std::string datapath)
    {
        auto dim = std::distance(begin, end);
        auto dataspace = h5::DataSpace(dim);
        auto dataset = _file->createDataSet<ValueType>(datapath, dataspace);
        dataset.write_raw(&(*begin));
    }

    /**
     * @brief 
     * 
     * @tparam T 
     * @tparam typename 
     * @param value 
     * @param datapath 
     */
    template <typename T, typename = std::enable_if_t<std::is_scalar_v<T>>>
    void _write_scalar(T value, std::string datapath)
    {
        auto localpath = datapath;
        regulerize_path(localpath);
        localpath += "scalar/0";
        _write_1D_array(&value, &value + 1, localpath);
    }

    /**
     * @brief 
     * 
     * @tparam T 
     * @param vec 
     * @param datapath 
     */
    template <typename T>
    void _write_vector(const std::vector<T> &vec, std::string datapath)
    {
        auto localpath = datapath;
        regulerize_path(localpath);
        localpath += "vector/0";
        // h5::DataSet dataset = _file->createDataSet<T>(localpath, h5::DataSpace::From(vec));
        // dataset.write(vec);
        _write_1D_array(vec.cbegin(), vec.cend(), localpath);
    }

    /**
     * @brief (not tested)
     * 
     * @tparam T 
     * @tparam N 
     * @param arr 
     * @param datapath 
     */
    template <typename T, int N>
    void _write_array(const std::array<T, N> &arr, std::string datapath)
    {
        auto localpath = datapath;
        regulerize_path(localpath);
        localpath += "array/0";

        _write_1D_array(arr.cbegin(), arr.cend(), datapath);
    }

    /**
     * @brief (not tested)
     * 
     * @tparam MapType 
     * @tparam KeyType 
     * @tparam ValueType 
     * @tparam std::enable_if_t<true or std::declval<MapType<KeyType, ValueType> >().size()> 
     * @param map 
     * @param datapath 
     */
    template <template <typename, typename> typename MapType, typename KeyType, typename ValueType, typename = std::enable_if_t<true or std::declval<MapType<KeyType, ValueType> >().size()> >
    void _write_map(const MapType<KeyType, ValueType> &map, std::string datapath)
    {

        std::vector<KeyType> keys;
        std::vector<ValueType> values;
        for (auto [key, value] : map) {
            keys.push_back(key);
            values.push_back(value);
        }

        auto localpath = datapath;
        regulerize_path(localpath);
        _write_vector(keys, localpath + "map/0");
        _write_vector(values, localpath + "map/1");
        //_write_1D_array(keys.begin(), keys.end(), localpath + "map/key");
        //_write_1D_array(values.begin(), values.end(), localpath + "map/value");
    }

    /**
     * @brief (not tested)
     * 
     * @tparam PairType 
     * @tparam KeyType 
     * @tparam ValueType 
     * @tparam std::enable_if_t<true or std::declval<PairType<KeyType, ValueType> >().first> 
     * @param pair 
     * @param datapath 
     */
    template <template <typename, typename> typename PairType, typename KeyType, typename ValueType, typename = std::enable_if_t<true or std::declval<PairType<KeyType, ValueType> >().first> >
    void _write_pair(const PairType<KeyType, ValueType> &pair, std::string datapath)
    {
        auto first = pair.first;
        auto second = pair.second;
        auto localpath = datapath;
        regulerize_path(localpath);
        _write_scalar(first, localpath + "pair/0");
        _write_scalar(second, localpath + "pair/1");
    }

    /**
     * @brief 
     * 
     * @tparam N 
     * @tparam Ts 
     * @param pack 
     * @param datapath 
     */
    template <int N, typename... Ts>
    void _write_tuple(const std::tuple<Ts...> &pack, std::string datapath)
    {
        static_assert(N < sizeof...(Ts), "Out of bounds");
        auto data = std::get<N>(pack);

        auto localpath = datapath;
        if (localpath.back() != '/')
        {
            localpath += '/';
        }
        localpath += "tuple/" + std::to_string(N);

        write(data, localpath);
        if constexpr (N < sizeof...(Ts) - 1)
        {
            _write_tuple<N + 1, Ts...>(pack, datapath);
        }
    }

    /**
     * @brief 
     * 
     * @tparam T 
     * @tparam U 
     * @tparam is_directed 
     * @param list 
     * @param datapath 
     */
    template <typename T, typename U, bool is_directed>
    void _write_csrlist(const CSRList<T, U, std::bool_constant<is_directed>>& list, std::string datapath) {
        auto localpath = datapath;
        if (localpath.back() != '/')
        {
            localpath += '/';
        }
        localpath += "csrlist/";
        write(list.data(), localpath + "data");
        write(list.offset(), localpath + "offset");
    }
private:
    std::unique_ptr<h5::File> _file;
};

#endif // __HDF5FILE_H__