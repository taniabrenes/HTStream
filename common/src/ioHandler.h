#ifndef IOHANDLER_H
#define IOHANDLER_H

#include <istream>
#include <fstream>
#include <memory>
#include <utility>

#include "read.h"
#include <boost/iostreams/concepts.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/device/mapped_file.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem.hpp>

#include <iostream>
#include <string>

namespace bf = boost::filesystem;
namespace bi = boost::iostreams;

int check_open_r(const std::string& filename) ;
int check_exists(const std::string& filename, bool force, bool gzip, bool std_out) ;
std::string get_fasta_seq(std::istream &inFasta);
Read fasta_to_read(std::string fasta_file);

class HtsOfstream {
private:
    std::string filename;
    bool force;
    bool gzip;
    bool std_out;
    std::shared_ptr<std::ostream> out = nullptr;

    void create_out() {
        out.reset(new bi::stream<bi::file_descriptor_sink> {check_exists(filename, force, gzip, std_out), bi::close_handle});
    }

public:
    ~HtsOfstream() {
        if (out) {
            std::flush(*out);
        }
    }
    
    HtsOfstream(std::string filename_, bool force_, bool gzip_, bool stdout_) : filename(filename_), force(force_), gzip(gzip_), std_out(stdout_)  { }
    
    HtsOfstream(std::shared_ptr<std::ostream> out_) : out(out_) { }

    template<class T>
    HtsOfstream& operator<< (T s) {
        if (!out) {
            create_out();
        }
        *out << s;
        return *this;
    } 

    void flush() {
        if (out) {
            std::flush(*out);
        }
    }
};


// ### input ###
template <class T, class Impl>
class InputReader : Impl {
public:
    typedef std::unique_ptr<T> value_type;
    using Impl::Impl;
    
    bool has_next();
    value_type next();
};

class InputFastq {
protected:
    Read load_read(std::istream *input);
    
    std::string id, seq, id2, qual;
};

class InputFasta {
protected:
    Read load_read(std::istream *input);
    std::string id, seq;
    std::string tmpSeq;
};

class FastaReadImpl : public InputFasta {
public:
    FastaReadImpl(std::istream& input_) : input(&input_) {}
protected:
    std::istream* input = 0;
};

class SingleEndReadFastqImpl : public InputFastq{
public:
    SingleEndReadFastqImpl(std::istream& in) : input(&in) {}
    
protected:
    std::istream* input = 0;
};

class PairedEndReadFastqImpl : public InputFastq {
public:
    PairedEndReadFastqImpl(std::istream& in1_, std::istream& in2_) : in1(&in1_), in2(&in2_) {}
  
protected:
    std::istream* in1, * in2 = 0;
};

class TabReadImpl : public InputFastq {
public:
    TabReadImpl(std::istream& in1_) : in1(&in1_) {}
    std::vector<Read> load_read(std::istream *input);
protected:
    std::istream* in1;
    //to read the line
    std::string tabLine;
};

class InterReadImpl : public InputFastq {
public:
    InterReadImpl(std::istream& in1_) : in1(&in1_) {}
protected:
    std::istream *in1;
};

class OutputWriter {
public:
    virtual ~OutputWriter() {  }
    virtual void write(const PairedEndRead& ) { throw std::runtime_error("No PE implementation of write (Probably a SE read)"); }
    virtual void write(const SingleEndRead& ) { throw std::runtime_error("No SE implementaiton of write (Probably a PE read)"); }
    virtual void write_read(const Read &, bool ) { throw std::runtime_error("No write_read class, only accessable with SE"); } //only SE
    virtual void write(const ReadBase &) { throw std::runtime_error("No ReadBase class, only accessable with tab"); } //maybe typecase eventually 
};

class SingleEndReadOutFastq : public OutputWriter {
public:
    SingleEndReadOutFastq(std::shared_ptr<HtsOfstream> &out_) : output(out_) { }
    ~SingleEndReadOutFastq() { output->flush(); }
    void write(const SingleEndRead &read) { format_writer(read.get_read()); }
    void write_read(const Read &read, bool rc) { if (rc) { format_writer_rc(read); } else { format_writer(read); } }
    void write(const ReadBase &read) {
        const SingleEndRead *ser = dynamic_cast<const SingleEndRead*>(&read);
        if (ser) {
            write(*ser);
        } else {
            throw std::runtime_error("PairedEndRead passed in SingleEndReadOutFastq::write");
        }
    }        
protected:
    std::shared_ptr<HtsOfstream> output = nullptr;
    
    void format_writer_rc(const Read &read) { 
       *output << "@" << read.get_id() << '\n' << read.get_seq_rc() << "\n+\n" << read.get_qual_rc() << '\n'; 
    }
    void format_writer(const Read &read) { 
       *output << "@" << read.get_id() << '\n' << read.get_sub_seq() << "\n+\n" << read.get_sub_qual() << '\n'; 
    }
    
};

class PairedEndReadOutFastq : public OutputWriter {
public:
    PairedEndReadOutFastq(std::shared_ptr<HtsOfstream> &out1_, std::shared_ptr<HtsOfstream> &out2_) : out1(out1_), out2(out2_) { }
    ~PairedEndReadOutFastq() { out1->flush(); out2->flush(); }
    void write(const PairedEndRead &read) { format_writer(read.get_read_one(), read.get_read_two());  }
    void write(const ReadBase &read) {
        const PairedEndRead *per = dynamic_cast<const PairedEndRead*>(&read);
        if (per) {
            write(*per);
        } else {
            throw std::runtime_error("SingleEndRead passed in PairedEndReadOutFastq::write");
        }
    }        
    
        
protected:
    std::shared_ptr<HtsOfstream> out1 = nullptr;
    std::shared_ptr<HtsOfstream> out2 = nullptr;
    
    void format_writer(const Read &read1, const Read &read2) { 
        *out1 << "@" << read1.get_id() << '\n' << read1.get_sub_seq() << "\n+\n" << read1.get_sub_qual() << '\n'; 
        *out2 << "@" << read2.get_id() << '\n' << read2.get_sub_seq() << "\n+\n" << read2.get_sub_qual() << '\n'; 
    }
};

class PairedEndReadOutInter : public OutputWriter {
public:
    PairedEndReadOutInter(std::shared_ptr<HtsOfstream> &out_) : out1(out_) { }
    ~PairedEndReadOutInter() { out1->flush(); }
    void write(const PairedEndRead &read) { format_writer(read.get_read_one(), read.get_read_two()); }
    void write(const ReadBase &read) {
        const PairedEndRead *per = dynamic_cast<const PairedEndRead*>(&read);
        if (per) {
            write(*per);
        } else {
            throw std::runtime_error("SingleEndRead called in PairedEndReadOutInter::write");
        }
    }        
protected:
    std::shared_ptr<HtsOfstream> out1 = nullptr;
    void format_writer(const Read &read1, const Read &read2) { 
        *out1 << "@" << read1.get_id() << '\n' << read1.get_sub_seq() << "\n+\n" << read1.get_sub_qual() << '\n';
        *out1 << "@" << read2.get_id() << '\n' << read2.get_sub_seq() << "\n+\n" << read2.get_sub_qual() << '\n'; 
    }
};

/*Unmapped reads*/
class ReadBaseOutUnmapped : public OutputWriter {
public:
    ReadBaseOutUnmapped(std::shared_ptr<HtsOfstream> &out_) : output(out_) { }
    ~ReadBaseOutUnmapped() { output->flush(); }
    void write(const PairedEndRead &read) { format_writer(read.get_read_one(), read.get_read_two()); }
    void write(const SingleEndRead &read) { format_writer(read.get_read()); }
    void write_read(const Read &read, bool rc) { if (rc) { format_writer_rc(read); } else { format_writer(read); } }
    
    void write(const ReadBase &read) {  
        const PairedEndRead *per = dynamic_cast<const PairedEndRead*>(&read);
        if (per) {
            format_writer(per->get_read_one(), per->get_read_two());
        } else {
            const SingleEndRead *ser = dynamic_cast<const SingleEndRead*>(&read);
            if (ser == NULL) {
                throw std::runtime_error("ReadBaseOutTab::write could not cast read as SE or PE read");
            }
            format_writer(ser->get_read());
        }    
    }
protected:
    std::shared_ptr<HtsOfstream> output = nullptr;
    
    /*sam format specs spaces are for readability 
     * id \t bitwise flag \t rname \t pos \t mapQ \t CIGAR \t RNEXT \t PNEXT \t TLEN \t SEQ \t QUAL\n
     *
     * id = id
     * bitwas flag
     * SE - 68
     * PE R1 - 69
     * PE R2 - 133
     * RNAME - *
     * POS - 0
     * MAPQ - 255
     * CIGAR - *
     * RNEXT - *
     * PNEXT - *
     * TLEN - SEQ.length
     * SEQ - seq
     * QUAL - qual */
    const size_t se_bitwise = 68;
    const size_t pe1_bitwise = 69;
    const size_t pe2_bitwise = 133;

    void samout(const Read &read, size_t bitwiseflag) {
        *output << read.get_id() << '\t'
            << bitwiseflag << '\t'
            << "*\t" /*RNAME*/
            << "0\t" /*POS*/
            << "255\t" /*MAPQ*/
            << "*\t" /*CIGAR*/
            << "0\t" /*RNEXT*/
            << "0\t" /*PNEXT*/
            << read.getLength() << "\t"
            << read.get_sub_seq() << "\t"
            << read.get_sub_qual() << "\n";
    }

    void samout_rc(const Read &read, size_t bitwiseflag) {
        *output << read.get_id() << '\t'
            << bitwiseflag << '\t'
            << "*\t" /*RNAME*/
            << "0\t" /*POS*/
            << "255\t" /*MAPQ*/
            << "*\t" /*CIGAR*/
            << "0\t" /*RNEXT*/
            << "0\t" /*PNEXT*/
            << read.getLength() << "\t"
            << read.get_seq_rc() << "\t"
            << read.get_qual_rc() << "\n";
    }

    /*Unmapped specs for SE reads*/
    void format_writer(const Read &read) { 
        samout(read, se_bitwise);
    }

    void format_writer(const Read &read1, const Read &read2) {
        samout(read1, pe1_bitwise);
        samout(read2, pe2_bitwise);
    }
   
    void format_writer_rc(const Read &read) { 
       samout_rc(read, se_bitwise);
    } 
};


class ReadBaseOutTab : public OutputWriter {
public:
    ReadBaseOutTab(std::shared_ptr<HtsOfstream> &out_) : output(out_) { }
    ~ReadBaseOutTab() { output->flush(); }
    void write(const PairedEndRead &read) { format_writer(read.get_read_one(), read.get_read_two()); }
    void write(const SingleEndRead &read) { format_writer(read.get_read()); }
    void write_read(const Read &read, bool rc) { if (rc) { format_writer_rc(read); } else { format_writer(read); } }
    
    void write(const ReadBase &read) {  
        const PairedEndRead *per = dynamic_cast<const PairedEndRead*>(&read);
        if (per) {
            format_writer(per->get_read_one(), per->get_read_two());
        } else {
            const SingleEndRead *ser = dynamic_cast<const SingleEndRead*>(&read);
            if (ser == NULL) {
                throw std::runtime_error("ReadBaseOutTab::write could not cast read as SE or PE read");
            }
            format_writer(ser->get_read());
        }    
    }
protected:
    std::shared_ptr<HtsOfstream> output = nullptr;
    
    void format_writer(const Read &read) { 
        *output << read.get_id() << '\t' << read.get_sub_seq() << '\t' << read.get_sub_qual() << '\n'; 
    }

    void format_writer(const Read &read1, const Read &read2) {
        *output << read1.get_id() << '\t' << read1.get_sub_seq() << '\t' << read1.get_sub_qual() << '\t' << read2.get_sub_seq() << '\t' << read2.get_sub_qual() << '\n';
    }
   
    void format_writer_rc(const Read &read) { 
       *output <<  read.get_id() << '\t' << read.get_seq_rc() << "\t" << read.get_qual_rc() << '\n'; 
    } 
};

void writer_helper(ReadBase *r, std::shared_ptr<OutputWriter> pe, std::shared_ptr<OutputWriter> se, bool stranded, Counter &c, bool no_orphans = false);

#endif
