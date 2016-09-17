/*
In constructing the templates I initially had a test for if
paired or if single read end. The true/false conditions
would not set at compile time. The simplest fix is partial 
template specialization but I didn't know how to do that
so I just created separate objects.

-ChrisM

I used the Super-Deduper.cpp as the codebase.
I removed the unneccessry libraries for the 
unordered map since I won't be using those. I removed the
basic workhorse template function and added the
counters that I needed for the meta data.
*/
#include "nremover.h"
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include "ioHandler.h"
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>


namespace
{
    const size_t SUCCESS = 0;
    const size_t ERROR_IN_COMMAND_LINE = 1;
    const size_t ERROR_UNHANDLED_EXCEPTION = 2;

} // namespace

namespace bi = boost::iostreams;
namespace bf = boost::filesystem;

int check_open_r(const std::string& filename) {
    bf::path p(filename);
    if (!bf::exists(p)) {
        throw std::runtime_error("File " + filename + " was not found.");
    }
    
    if (p.extension() == ".gz") {
        return fileno(popen(("gunzip -c " + filename).c_str(), "r"));
    } else {
        return fileno(fopen(filename.c_str(), "r"));
    }
}

int main(int argc, char** argv)
{

    Counter counters;
    counters["TotalRecords"] = 0;
    counters["Replaced"] = 0;
    counters["HasN"] = 0;
    counters["Kept"] = 0;
    counters["Discarded"] = 0;
    std::string prefix;
    std::vector<std::string> default_outfiles = {"PE1", "PE2", "SE"};
    bool fastq_out = false;
    bool tab_out = false;
    bool std_out = false;
    bool gzip_out = false;
    
    try
    {
        /** Define and parse the program options
         */
        namespace po = boost::program_options;
        po::options_description desc("Options");
        desc.add_options()
            ("version,v",                  "Version print")
            ("read1-input,1", po::value< std::vector<std::string> >(),
                                           "Read 1 input <comma sep for multiple files>") 
            ("read2-input,2", po::value< std::vector<std::string> >(), 
                                           "Read 2 input <comma sep for multiple files>")
            ("singleend-input,U", po::value< std::vector<std::string> >(),
                                           "Single end read input <comma sep for multiple files>")
            ("tab-input,T", po::value< std::vector<std::string> >(),
                                           "Tab input <comma sep for multiple files>")
            ("interleaved-input,I", po::value< std::vector<std::string> >(),
                                           "Interleaved input I <comma sep for multiple files>")
            ("stdin-input,S",              "STDIN input <MUST BE TAB DELIMITED INPUT>")
            ("quality-check-off,q",        "Quality Checking Off First Duplicate seen will be kept")
            ("gzip-output,g", po::bool_switch(&gzip_out)->default_value(false),  "Output gzipped")
            ("interleaved-output, i",      "Output to interleaved")
            ("fastq-output,f", po::bool_switch(&fastq_out)->default_value(false), "Fastq format output")
            ("force,F", po::bool_switch()->default_value(true),         "Forces overwrite of files")
            ("tab-output,t", po::bool_switch(&tab_out)->default_value(false),   "Tab-delimited output")
            ("to-stdout,O", po::bool_switch(&std_out)->default_value(false),    "Prints to STDOUT in Tab Delimited")
            ("prefix,p", po::value<std::string>(&prefix)->default_value("output_nodup_"),
                                           "Prefix for outputted files")
            ("log-file,L",                 "Output-Logfile")
            ("no-log,N",                   "No logfile <outputs to stderr>")
            ("help,h",                     "Prints help.");

        po::variables_map vm;
        try
        {
            po::store(po::parse_command_line(argc, argv, desc),
                      vm); // can throw

            /** --help option
             */
            if ( vm.count("help")  || vm.size() == 0)
            {
                std::cout << "Prune" << std::endl
                          << desc << std::endl;
                return SUCCESS;
            }

            po::notify(vm); // throws on error, so do after help in case
            // there are any problems
            if(vm.count("read1-input")) {
                if (!vm.count("read2-input")) {
                    throw std::runtime_error("must specify both read1 and read2 input files.");
                } else if (vm.count("read2-input") != vm.count("read1-input")) {
                    throw std::runtime_error("must have same number of input files for read1 and read2");
                }
                auto read1_files = vm["read1-input"].as<std::vector<std::string> >();
                auto read2_files = vm["read2-input"].as<std::vector<std::string> >();

                for(size_t i = 0; i < read1_files.size(); ++i) { // JIC there are many input files

                    bi::stream<bi::file_descriptor_source> is1{check_open_r(read1_files[i]), bi::close_handle};
                    bi::stream<bi::file_descriptor_source> is2{check_open_r(read2_files[i]), bi::close_handle};
                    
                    InputReader<PairedEndRead, PairedEndReadFastqImpl> ifp(is1, is2);
                    find_longest_paired(ifp, counters);
                }
            }
            if(vm.count("singleend-input")) {
                auto read_files = vm["singleend-input"].as<std::vector<std::string> >();
                for (auto file : read_files) {
                    std::ifstream read1(file, std::ifstream::in);
                    InputReader<SingleEndRead, SingleEndReadFastqImpl> ifs(read1);
                    find_longest_single(ifs, counters);
                }
            }            
        }
        catch(po::error& e)
        {
            std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
            std::cerr << desc << std::endl;
            return ERROR_IN_COMMAND_LINE;
        }

    }
    catch(std::exception& e)
    {
        std::cerr << "\n\tUnhandled Exception: "
                  << e.what() << std::endl;
        return ERROR_UNHANDLED_EXCEPTION;

    }

    std::cerr << "TotalRecords" << "\tReplaced" << "\tHasN" 
            << "\t " << counters["TotalRecords"] << "\t " << counters["Replaced"] << "\t " << counters["HasN"] 
            << std::endl;
    return SUCCESS;

}
