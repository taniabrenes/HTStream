#include <vector>
#include <fstream>
#include "ioHandler.h"
#include <unordered_map>

typedef std::unordered_map<std::string, size_t> Counter;

template <class T, class Impl>
void qualit_trimmer(InputReader<T, Impl> &reader, Counter& counters, int& min_quality_threshold) {

    while (reader.has_next()) {
        ++counters["TotalRecords"];
        auto i = reader.next();
        if (i->avg_q_score >= min_quality_threshold){
            //write out and count
        }else{
            //discard and cound
        }
    }
}