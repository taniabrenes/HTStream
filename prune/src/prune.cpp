//  this is so we can implment hash function for dynamic_bitset
#define BOOST_DYNAMIC_BITSET_DONT_USE_FRIENDS

#include <iostream>
#include <string>
#include <boost/program_options.hpp>
#include <vector>
#include <fstream>
#include "ioHandler.h"
#include <unordered_map>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem.hpp>
#include <boost/functional/hash.hpp>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>

namespace
{
    const size_t SUCCESS = 0;
    const size_t ERROR_IN_COMMAND_LINE = 1;
    const size_t ERROR_UNHANDLED_EXCEPTION = 2;

} // namespace

typedef std::unordered_map<std::string, size_t> Counter;

typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
boost::char_separator<char> sep(
  "N", // dropped delimiters
  "",  // kept delimiters
  boost::keep_empty_tokens); // empty token policy

/*
In constructing this template I initially had a test for if
paired or if single read end. The true/false conditions
would not set at compile time. The simplest fix is partial 
template specialization but I didn't know how to do that
so I just created separate objects.
*/
/*
N remover loops through the entire read to find the 
longest string with no N's. If this non-n chunk
is greater than the minimum length requirement it is kept. If it isn't it
is discarded. If it is a PE and only R1 is discarded, and the stranded
option is set, R2 will become a SE RC read. Otherwise, the stand alone 
read will stay the same if its mate is discarded. 
*/

template <class T, class Impl>
void find_longest_paired(InputReader<T, Impl> &reader, Counter& counters) {
    // we want to keep the longest continuous sequence without N's
    while(reader.has_next()) { // iterate over file
        ++counters["TotalRecords"];
        auto i = reader.next();
        std::string str_read_one = i->get_read_one().get_seq(); // paired end one
        std::string str_read_two = i->get_read_two().get_seq(); // paired end two
        std::string result_read_one; // will be the longest string from read one
        std::string result_read_two; // will be the longest string from read two
        int minimumLength = 20; // was this arbitrarily chose?
        // tokenize the string and keep the longest token
        BOOST_FOREACH(std::string token, tokenizer(str_read_one, sep))
        {
            // don't store anything just keep the longest
            if (token.length() > result_read_one.length() && token.length())
            {
                result_read_one.assign(token);
            }
        }
        BOOST_FOREACH(std::string token, tokenizer(str_read_two, sep))
        {
            // don't store anything just keep the longest
            if (token.length() > result_read_one.length() && token.length())
            {
                result_read_two.assign(token);
            }
        }
        // increase counts if sequences were replaced
        if (result_read_one.length() < str_read_one.length() && \
            result_read_one.length() > minimumLength)
        {
            ++counters["Replaced"];
            ++counters["HasN"];
            // output the result
            std::cout << result_read_one << std::endl;
            // ouput an empty line
            std::cout << std::endl;
            // write output to a file. 

        } else
        {
            // discard it
        }
    }
}

template <class T, class Impl>
void find_longest_single(InputReader<T, Impl> &reader, Counter& counters) {
    // we want to keep the longest continuous sequence without N's
    while(reader.has_next()) { // iterate over file
        ++counters["TotalRecords"];
        auto i = reader.next();
        std::string str_read = i->get_read().get_seq(); // paired end
        std::string result;
        BOOST_FOREACH(std::string token, tokenizer(str_read, sep))
        {
            // don't store anything just keep the longest
            if (token.length() > result.length())
            {
                result.assign(token);
            }
        }
        // increase counts if sequences were replaced
        if (result_read_one.length() < str_read_one.length() && \
            result_read_one.length() > minimumLength)
        {
            ++counters["Replaced"];
            ++counters["HasN"];
            // output the result
            std::cout << result << std::endl;
            // ouput an empty line
            std::cout << std::endl; 
        }
   }
}

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

                for(size_t i = 0; i < read1_files.size(); ++i) {
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

    std::cerr << "TotalRecords:" << counters["TotalRecords"] 
            << "\tReplaced:" << counters["Replaced"]
            //<< "\tKept:" << read_map.size() 
            //<< "\tRemoved:" << counters["TotalRecords"] - read_map.size()
            << "\tHasN:" << counters["HasN"] << std::endl;
    return SUCCESS;

}
