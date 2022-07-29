#ifndef __HDF5FILE_H__
#define __HDF5FILE_H__

#include <memory>
#include <highfive/H5File.hpp>

namespace h5 = HighFive;
class HDF5File
{
    public:
    HDF5File(std::string filename, std::string open_flag) : _file(nullptr) {
        if(open_flag == "w") {
            _file = std::make_unique<h5::File>(filename, h5::File::ReadWrite | h5::File::Create | h5::File::Truncate );
        }
        else if(open_flag == "r") {
            _file = std::make_unique<h5::File>(filename, h5::File::ReadWrite | h5::File::Create | h5::File::Truncate );
        }
        else {
            assert(false and "Invalid open flag");
        }
    }
    template<typename T>
    void write(const std::vector<T>& vec, std::string datapath) {
        auto dataset = _file->createDataSet<T>(datapath, h5::DataSpace::From(vec));
        dataset.write(vec);
    }

    template<typename Partitionable, 
        std::enable_if_t<std::is_integral_v<decltype(std::declval<Partitionable>.num_partitions()) >, int> = 0>
    void write(const Partitionable& object, std::string datapath) {
        auto num_parts = object.num_partitions();
        for(std::size_t i = 0; i < num_parts; ++i) {
            auto part = object.partition(i);
        }
    }

    private:

    std::unique_ptr<h5::File> _file;
};

#endif // __HDF5FILE_H__ 