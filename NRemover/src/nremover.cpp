#include "nremover.h"
#include <iostream>
#include <string>
#include <boost/program_options.hpp>
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

namespace
{
    const size_t SUCCESS = 0;
    const size_t ERROR_IN_COMMAND_LINE = 1;
    const size_t ERROR_UNHANDLED_EXCEPTION = 2;

} // namespace

namespace bi = boost::iostreams;

int main(int argc, char** argv)
{
    Counter counters;
    counters["TotalRecords"] = 0;
    counters["Replaced"] = 0;
    counters["HasN"] = 0;
    counters["Discarded"] = 0;
    size_t start = 0, length = 0;
    std::string prefix;
    std::vector<std::string> default_outfiles = {"PE1", "PE2", "SE"};

    bool fastq_out;
    bool tab_out;
    bool std_out;
    bool std_in;
    bool gzip_out;
    bool interleaved_out;
    bool force;

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
            ("stdin-input,S", po::bool_switch(&std_in)->default_value(false), "STDIN input <MUST BE TAB DELIMITED INPUT>")
            ("start,s", po::value<size_t>(&start)->default_value(10),  "Start location for unique ID <int>")
            ("length,l", po::value<size_t>(&length)->default_value(10), "Length of unique ID <int>")
            ("quality-check-off,q",        "Quality Checking Off First Duplicate seen will be kept")
            ("gzip-output,g", po::bool_switch(&gzip_out)->default_value(false),  "Output gzipped")
            ("interleaved-output,i", po::bool_switch(&interleaved_out)->default_value(false),     "Output to interleaved")
            ("fastq-output,f", po::bool_switch(&fastq_out)->default_value(false), "Fastq format output")
            ("force,F", po::bool_switch(&force)->default_value(false),         "Forces overwrite of files")
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
                std::cout << "NRemover" << std::endl
                          << desc << std::endl;
                return SUCCESS;
            }

            po::notify(vm); // throws on error, so do after help in case
            //Index 1 start location (making it more human friendly)
            start--;

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
                    while(ifp.has_next()) { // iterate over data
                        ++counters["TotalRecords"];
                        auto i = ifp.next();
                        std::string read_one = i->get_read_one().get_seq(); // paired end one
                        std::string read_two = i->get_read_two().get_seq(); // paired end one
                        find_longest(read_one, counters);
                        find_longest(read_two, counters);
                        }
                    }
            }

            if(vm.count("singleend-input")) {
                auto read_files = vm["singleend-input"].as<std::vector<std::string> >();
                for (auto file : read_files) {
                    bi::stream<bi::file_descriptor_source> se{ check_open_r(file), bi::close_handle};
                    InputReader<SingleEndRead, SingleEndReadFastqImpl> ifs(se);
                    //load_map(ifs, counters, read_map, start, length);
                }
            }
            
            if(vm.count("tab-input")) {
                auto read_files = vm["tab-input"].as<std::vector<std::string> > ();
                for (auto file : read_files) {
                    bi::stream<bi::file_descriptor_source> tabin{ check_open_r(file), bi::close_handle};
                    InputReader<ReadBase, TabReadImpl> ift(tabin);
                    //load_map(ift, counters, read_map, start, length);
                }
            }
            
            if (vm.count("interleaved-input")) {
                auto read_files = vm["interleaved-input"].as<std::vector<std::string > >();
                for (auto file : read_files) {
                    bi::stream<bi::file_descriptor_source> inter{ check_open_r(file), bi::close_handle};
                    InputReader<PairedEndRead, InterReadImpl> ifp(inter);
                    //load_map(ifp, counters, read_map, start, length);
                }
            }
            
            if (std_in) {
                bi::stream<bi::file_descriptor_source> tabin {fileno(stdin), bi::close_handle};
                InputReader<ReadBase, TabReadImpl> ift(tabin);
                //load_map(ift, counters, read_map, start, length);
            }

            std::shared_ptr<HtsOfstream> out_1 = nullptr;
            std::shared_ptr<HtsOfstream> out_2 = nullptr;
            std::shared_ptr<HtsOfstream> out_3 = nullptr;

            std::unique_ptr<OutputWriter> pe = nullptr;
            std::unique_ptr<OutputWriter> se = nullptr;
            
            if (fastq_out || (! std_out && ! tab_out) ) {
                for (auto& outfile: default_outfiles) {
                    outfile = prefix + outfile + ".fastq";
                }
               
                out_1.reset(new HtsOfstream(default_outfiles[0], force, gzip_out, false));
                out_2.reset(new HtsOfstream(default_outfiles[1], force, gzip_out, false));
                out_3.reset(new HtsOfstream(default_outfiles[2], force, gzip_out, false));
                pe.reset(new PairedEndReadOutFastq(out_1, out_2));
                se.reset(new SingleEndReadOutFastq(out_3));

            } else if (interleaved_out)  {
                for (auto& outfile: default_outfiles) {
                    outfile = prefix + "INTER" + ".fastq";
                }
                
                out_1.reset(new HtsOfstream(default_outfiles[0], force, gzip_out, false));
                out_3.reset(new HtsOfstream(default_outfiles[1], force, gzip_out, false));

                pe.reset(new PairedEndReadOutInter(out_1));
                se.reset(new SingleEndReadOutFastq(out_3));
            } else if (tab_out || std_out) {
                for (auto& outfile: default_outfiles) {
                    outfile = prefix + "tab" + ".tastq";
                }
                out_1.reset(new HtsOfstream(default_outfiles[0], force, gzip_out, std_out));

                pe.reset(new ReadBaseOutTab(out_1));
                se.reset(new ReadBaseOutTab(out_1));
            }

            //Crashes here after going to the deconstructor of HtsOfstream of tmp_1
        } catch(po::error& e) {
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

    std::cerr << "TotalRecords:" << counters["TotalRecords"] << "\tReplaced:" << counters["Replaced"]
              << "\tHasN:" << counters["HasN"] << "\tDiscarded:" << counters["Discarded"] << std::endl;
    return SUCCESS;

}
