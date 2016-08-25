#ifndef SOURCE_NON_H_
#define SOURCE_NON_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>


#include "readInfo.h"
#include "fileWriter.h"


class non {

    private:

        /*Tree Stats*/
        uint32_t time_start;
        uint32_t time_end;

        double PE_reads;
        double R1_discarded;
        double R2_discarded;

        double PE_kept;
        double PE_3trim;
        double PE_5trim;

        double SE_reads;
        double SE_3trim;
        double SE_5trim;
        double SE_kept;

        uint16_t minLength;
    
    public:
        
        non() {
            time_start = time(NULL);
            minLength = 20;

            PE_reads = 0;
            R1_discarded = 0;
            R2_discarded = 0;
            PE_kept = 0;
            PE_3trim = 0;
            PE_5trim = 0;
            SE_reads = 0;
            SE_5trim = 0;
            SE_5trim = 0;
            SE_kept = 0;
        }

        void setLength(uint16_t l_) {minLength = l_;};
        void stopTime() {time_end = time(NULL);};

                        
        void helperTrim(readInfo **R1, readInfo **R2);
        void trimNs(char *r, uint16_t len,  uint16_t *front_cut, uint16_t *back_cut);

        void outputStats(FILE *f);

};




#endif

