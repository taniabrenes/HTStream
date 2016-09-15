#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include "ioHandler.h"
#include <unordered_map>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>

typedef std::unordered_map<std::string, size_t> Counter;

typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
boost::char_separator<char> sep(
  "N", // dropped delimiters
  "",  // kept delimiters
  boost::keep_empty_tokens); // empty token policy

/*
Loops through the entire read to find the 
longest string with no N's. If this non-n chunk
is greater than the minimum length requirement it is kept. If it isn't it
is discarded. If it is a PE and only R1 is discarded, and the stranded
option is set, R2 will become a SE RC read. Otherwise, the stand alone 
read will stay the same if its mate is discarded. 
TODO: Implement stranded option
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
        int minimumLength = 24; // was this arbitrarily chose?
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

        } else
        {
            // discard it
            ++counters["Discarded"];
        }
    }
}

template <class T, class Impl>
void find_longest_single(InputReader<T, Impl> &reader, Counter& counters) {
    // we want to keep the longest continuous sequence without N's
    int minimumLength = 24;
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
        if (result.length() < str_read.length() && \
            result.length() > minimumLength)
        {
            ++counters["Replaced"];
            ++counters["HasN"];
            // output the result
            std::cout << result << std::endl;
            // ouput an empty line
            std::cout << std::endl; 
        }else {
            // discard it
            ++counters["Discarded"];
        }
   }
}