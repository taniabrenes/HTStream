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
is discarded. 
*/

void find_longest(std::string& str_read, Counter& counters){
    std::string result_read; // will be the longest string from read one
    int minimumLength = 24; // was this arbitrarily chose?
    BOOST_FOREACH(std::string token, tokenizer(str_read, sep))
    {
        // don't store anything just keep the longest
        if (token.length() > result_read.length() && token.length())
        {
            result_read.assign(token);
        }
    }
    // increase counts if sequences were replaced
    // this does not differentiate between read one and read two
    if (result_read.length() < str_read.length() && \
        result_read.length() > minimumLength)
    {
        ++counters["Replaced"];
        ++counters["HasN"];
    }  
    else if (result_read == str_read )
    {
        ++counters["Kept"];

    } 
    else
    {
        // discard it
        ++counters["Discarded"];
    }      
    str_read = result_read;
}