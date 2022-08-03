#ifndef __PARAMETER_PARSER_H__
#define __PARAMETER_PARSER_H__

#include <iostream>
#include <string>
#include <vector>

#include <boost/program_options.hpp>
namespace po = boost::program_options;
class ParameterParser {
public:
    ParameterParser(int argc, const char *const *argv)
        : _argc(argc), _argv(argv), _description("Allowed options:") {
        resolve_cmd_line_args();
    };

    template <typename T> T eval(std::string key) const {
        return _arg_map[key].as<T>();
    }

private:
    void resolve_cmd_line_args() {
        std::string input_fmt;
        std::string output_fmt;
        std::string periodic_bc_file;
        _description.add_options()("help", "print help message")(
            "input,i", po::value<std::string>(),
            "the input mesh file (gmsh only)")(
            "input_fmt",
            po::value<std::string>(&input_fmt)->default_value("msh"),
            "format of the input file (gmsh only)")(
            "num,n", po::value<int>(), "parititon the mesh into #n parts")(
            "periodic,p", po::value<std::string>(),
            "the file on nodal mapping about periodic BC")(
            "output,o", po::value<std::string>(), "the output mesh file")(
            "output_fmt",
            po::value<std::string>(&output_fmt)->default_value("h5"),
            "format of the output mesh file");

        po::positional_options_description p_desc;
        p_desc.add("input", -1);

        po::store(po::command_line_parser(_argc, _argv)
                      .options(_description)
                      .positional(p_desc)
                      .run(),
                  _arg_map);

        po::notify(_arg_map);
    }

    friend std::ostream &operator<<(std::ostream &os, const ParameterParser &p);

private:
    po::variables_map _arg_map;
    po::options_description _description;
    int _argc;
    const char *const *_argv;
};

inline std::ostream &operator<<(std::ostream &os, const ParameterParser &p) {
    std::vector<std::string> keys = {"help",     "input",  "input_fmt", "num",
                                     "periodic", "output", "output_fmt"};
    const auto &vm = p._arg_map;

    os << "ARGV[" << p._argc << "]: ";
    for (int i = 0; i < p._argc; ++i) {
        os << p._argv[i] << (i == p._argc - 1 ? "\n" : ", ");
    }
    if (vm.count("help")) {
        os << p._description;
        return os;
    }
    for (auto key : keys) {
        auto found = vm.count(key);
        if (found) {
            os << key << "[" << found << "]"
               << ": ";
            if (key == "num")
                os << vm[key].as<int>() << "\n";
            else {
                os << vm[key].as<std::string>() << "\n";
            }
        } else {
            os << key << "[" << found << "]"
               << ": "
               << "NULL\n";
        }
    }
    return os;
}
#endif // __PARAMETER_PARSER_H__
