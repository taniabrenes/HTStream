//  this is so we can implment hash function for dynamic_bitset
#define BOOST_DYNAMIC_BITSET_DONT_USE_FRIENDS

#include <iostream>
#include <string>
#include <boost/program_options.hpp>
#include <vector>
#include <fstream>
#include "ioHandler.h"
#include <map>
#include <unordered_map>
#include <boost/dynamic_bitset.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem.hpp>
#include <boost/functional/hash.hpp>
#include <boost/variant.hpp>
namespace
{
    const size_t SUCCESS = 0;
    const size_t ERROR_IN_COMMAND_LINE = 1;
    const size_t ERROR_UNHANDLED_EXCEPTION = 2;

} // namespace

typedef std::unordered_map<std::string, size_t> Counter;

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
/*
struct test : public boost::static_visitor<> {
    
    template <class T, class Impl>
    test(OutputWriter<T, Impl> **_out) { out = *_out; } 
    void operator()(Impl &data ) const {
        out->write(data);
    }
};*/

template <class Reader, class Writer>
void do_write_helper(Reader& reader, Writer &writer) {
    while(reader.has_next()) {
        auto read = reader.next();
        writer->write(*read);
    }
}

template <class Reader, class Writer>
void do_write(Reader& reader, Writer &writer) {
    switch (writer.which()) {
    case 0:
    {
        auto w = boost::get<OutputWriter<PairedEndRead, PairedEndReadOutFastq> *>(writer);
        do_write_helper(reader, w);
    }
        break;
    case 1:
    {
        auto w = boost::get<OutputWriter<PairedEndRead, PairedEndReadOutInter> *>(writer);
        do_write_helper(reader, w);
    }
        break;
    case 2:
    {
        auto w = boost::get<OutputWriter<ReadBase, ReadBaseOutTab> *>(writer);
        do_write_helper(reader, w);
    }
    break;
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

    bool fastq_out;
    bool tab_out;
    bool std_out;
    bool gzip_out;
    bool interleaved_out;
    
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
            ("gzip-output,g", po::bool_switch(&gzip_out)->default_value(false),  "Output gzipped")
            ("interleaved-output, i", po::bool_switch(&interleaved_out)->default_value(false),     "Output to interleaved")
            ("fastq-output,f", po::bool_switch(&fastq_out)->default_value(false), "Fastq format output")
            ("force,F", po::bool_switch()->default_value(true),         "Forces overwrite of files")
            ("tab-output,t", po::bool_switch(&tab_out)->default_value(false),   "Tab-delimited output")
            ("to-stdout,O", po::bool_switch(&std_out)->default_value(false),    "Prints to STDOUT in Tab Delimited")
            ("prefix,p", po::value<std::string>(&prefix)->default_value("output_"),
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
                std::cout << "Super-Deduper" << std::endl
                          << desc << std::endl;
                return SUCCESS;
            }

            po::notify(vm); // throws on error, so do after help in case
            //OutputWriter<ReadBase, InputFastq> pofs;
            //OutputWriter<ReadBase, InputFastq> sofs;
            //typedef boost::variant<OutputWriter<PairedEndRead, PairedEndReadOutFastq>, OutputWriter<PairedEndRead, PairedEndReadOutInter>, OutputWriter<PairedEndRead, ReadBaseOutTab>> PE_Writer;
            typedef boost::variant<OutputWriter<PairedEndRead, PairedEndReadOutFastq> *, OutputWriter<PairedEndRead, PairedEndReadOutInter> *, OutputWriter<ReadBase, ReadBaseOutTab> *> PE_Writer;
            //typedef boost::variant<OutputWriter<SingleEndRead, SingleEndReadOutFastq> *> SE_Writer;
            PE_Writer writer;
            //PE_Writer pofs;
            //SE_Writer sofs;
            //void (*p)(ReadBase &);
            //Output *pe;

            if (fastq_out || (! std_out && ! tab_out) ) {
                for (auto& outfile: default_outfiles) {
                    outfile = prefix + outfile + ".fastq";
                }
                
                if (gzip_out) {
                    bi::stream<bi::file_descriptor_sink> out1{fileno(popen(("gzip > " + default_outfiles[0] + ".gz").c_str(), "w")), bi::close_handle};
                    bi::stream<bi::file_descriptor_sink> out2{fileno(popen(("gzip > " + default_outfiles[1] + ".gz").c_str(), "w")), bi::close_handle};
                    bi::stream<bi::file_descriptor_sink> out3{fileno(popen(("gzip > " + default_outfiles[2] + ".gz").c_str(), "w")), bi::close_handle};
                } else {
                    // note: mapped file is faster but uses more memory
                    std::ofstream out1(default_outfiles[0], std::ofstream::out);
                    std::ofstream out2(default_outfiles[1], std::ofstream::out);
                    std::ofstream out3(default_outfiles[2], std::ofstream::out);
                    OutputWriter<PairedEndRead, PairedEndReadOutFastq> pnew =  OutputWriter<PairedEndRead, PairedEndReadOutFastq>(out1, out2); 
                    writer = new OutputWriter<PairedEndRead, PairedEndReadOutFastq>(out1, out2);
                    //p = new  OutputWriter<PairedEndRead, PairedEndReadOutFastq>(out1, out2)::write;
                    //pe = new PairedEndReadOutFastq(out1, out2);
                    
                    //p =  (OutputWriter::write)
                    //PairedEndReadOutFastq *per = new PairedEndReadOutFastq(out1, out2);
                    //OutputWriter<ReadBase, Output> *pofs;
                    //pofs = new OutputWriter<PairedEndRead, per>;
                    //sofs = new  OutputWriter<SingleEndRead, SingleEndReadOutFastq>(out3);
                }
            } else if (interleaved_out)  {
                for (auto& outfile: default_outfiles) {
                    outfile = prefix + "INTER" + ".fastq";
                }

                if (gzip_out) {
                    bi::stream<bi::file_descriptor_sink> out1{fileno(popen(("gzip > " + default_outfiles[0] + ".gz").c_str(), "w")), bi::close_handle};
                    bi::stream<bi::file_descriptor_sink> out3{fileno(popen(("gzip > " + default_outfiles[2] + ".gz").c_str(), "w")), bi::close_handle};
                    //pofs = OutputWriter<PairedEndRead, PairedEndReadOutInter> (out1);
                    //sofs = OutputWriter<SingleEndRead, SingleEndReadOutFastq>(out3);
                } else {
                    std::ofstream out1(default_outfiles[0], std::ofstream::out);
                    std::ofstream out3(default_outfiles[2], std::ofstream::out);
                    /*OutputWriter<PairedEndRead, PairedEndReadOutInter> pofs(out1);
                    OutputWriter<SingleEndRead, SingleEndReadOutFastq> sofs(out3);*/
                }
            } else if (tab_out) {
                for (auto& outfile: default_outfiles) {
                    outfile = prefix + "tab" + ".tastq";
                }
                
                if (gzip_out) {
                    bi::stream<bi::file_descriptor_sink> out1{fileno(popen(("gzip > " + default_outfiles[0] + ".gz").c_str(), "w")), bi::close_handle};
                    //writer = new OutputWriter<ReadBase, ReadBaseOutTab>(out1);
                } else {
                    std::ofstream out1(default_outfiles[0], std::ofstream::out);
                    //writer = new OutputWriter<ReadBase, ReadBaseOutTab>(out1);
                    //OutputWriter<ReadBase, Output> pofs = OutputWriter<ReadBase, Output>(out1);
                    //OutputWriter<ReadBase, ReadBaseOutTab> pofs(out1);
                }
            }
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
                    do_write(ifp, writer);
                }
            }
            if(vm.count("singleend-input")) {
                auto read_files = vm["singleend-input"].as<std::vector<std::string> >();
                for (auto file : read_files) {
                    std::ifstream read1(file, std::ifstream::in);
                    InputReader<SingleEndRead, SingleEndReadFastqImpl> ifs(read1);
                    //sofs(ifs);
                }
            }
            
            if(vm.count("tab-input")) {
                auto read_files = vm["tab-input"].as<std::vector<std::string> > ();
                for (auto file : read_files) {
                    std::ifstream tabReads(file, std::ifstream::in);
                    InputReader<ReadBase, TabReadImpl> ift(tabReads);
                    /*This takes care of SE as well*/
                    //pofs(ift);        
                }
            }
            
            if (vm.count("interleaved-input")) {
                auto read_files = vm["interleaved-input"].as<std::vector<std::string > >();
                for (auto file : read_files) {
                    std::ifstream interReads(file, std::ifstream::in);
                    InputReader<PairedEndRead, InterReadImpl> ifp(interReads);
                    //pofs(ifp);
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

    /*std::cerr << "TotalRecords:" << counters["TotalRecords"] << "\tReplaced:" << counters["Replaced"]
              << "\tKept:" << read_map.size() << "\tRemoved:" << counters["TotalRecords"] - read_map.size()
              << "\tHasN:" << counters["HasN"] << std::endl;*/
    return SUCCESS;

}
