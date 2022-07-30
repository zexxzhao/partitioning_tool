#ifndef __HDF5FILE_H__
#define __HDF5FILE_H__

#include <memory>
#include <highfive/H5File.hpp>

#include "Mesh.hpp"

namespace h5 = HighFive;
class HDF5File
{
public:
    HDF5File(std::string filename, std::string open_flag) : _file(nullptr)
    {
        if (open_flag == "w")
        {
            _file = std::make_unique<h5::File>(filename, h5::File::ReadWrite | h5::File::Create | h5::File::Truncate);
        }
        else if (open_flag == "r")
        {
            _file = std::make_unique<h5::File>(filename, h5::File::ReadWrite | h5::File::Create | h5::File::Truncate);
        }
        else
        {
            assert(false and "Invalid open flag");
        }
    }
    template <typename T>
    void write(const std::vector<T> &vec, std::string datapath)
    {
        auto dataset = _file->createDataSet<T>(datapath, h5::DataSpace::From(vec));
        dataset.write(vec);
    }

    template <typename T>
    void write(const T &data, std::string datapath)
    {
        std::vector<T> vec({data});
        write(vec, datapath);
    }

    template <typename... Ts>
    void write(const std::tuple<Ts...> &pack, std::string datapath)
    {
        _write_tuple<0, Ts...>(pack, datapath);
    }

    template <int D>
    void write(const Mesh<D> &object, std::string datapath)
    {
        auto num_parts = object.num_partitions();
        for (std::size_t i = 0; i < num_parts; ++i)
        {
            auto& part = object.local_mesh_data(i);
            write(part, datapath);
        }
    }

private:
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
        localpath += "tuple" + std::to_string(N);

        write(data, localpath);
        if constexpr(N < sizeof...(Ts) - 1)
        {
            _write_tuple<N + 1, Ts...>(pack, datapath);
        }
    }

private:
    std::unique_ptr<h5::File> _file;
};

#endif // __HDF5FILE_H__