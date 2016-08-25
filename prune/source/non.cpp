

#include "non.h"

void non::outputStats(FILE *f) {
    fprintf(f, "PE_Reads\tPE_Kept\tAvg_PE_Left_Trim\tAvg_PE_Right_Trim\tR1_Discarded\tR2_Discarded\tSE_Reads\tSE_Kept\tAvg_SE_Left_Trim\tAvg_SE_Right_Trim\tRun_Time\n");
    fprintf(f, "%lf\t%lf\t%.2lf\t%.2lf\t%lf\t%lf\t%lf\t%lf\t%.2lf\t%.2lf\t%u\n",
                PE_reads,
                    PE_kept,
                        PE_3trim/PE_kept,
                            PE_5trim/PE_kept,
                                R1_discarded,
                                    R2_discarded,
                                        SE_reads,
                                            SE_kept,
                                                SE_3trim/(SE_kept),
                                                    SE_5trim/(SE_kept),
                                                        time_end-time_start);
}





void non::helperTrim(readInfo **R1, readInfo **R2) {

    /*SE logic*/
    if (!(*R2)) {
        SE_reads++;
        uint16_t front_cut, back_cut;
        trimNs((*R1)->getSeq(), (*R1)->getLength(), &front_cut, &back_cut);



        if ((*R1)->changeSeq(front_cut, back_cut, minLength)) {
            delete (*R1);
            (*R1) = NULL;
        }

        if ((*R1)) {
            SE_3trim += front_cut;
            SE_5trim += back_cut;
            SE_kept += 1;
        }
        

    /*PE logic*/
    } else if (*R1 && *R2)  {
        PE_reads++;
        
        uint16_t R1_front_cut, R1_back_cut;
        uint16_t R2_front_cut, R2_back_cut;

        trimNs((*R1)->getSeq(), (*R1)->getLength(), &R1_front_cut, &R1_back_cut);

        if (!(*R1)->changeSeq(R1_front_cut, R1_back_cut, minLength)) {
            delete (*R1);
            (*R1) = NULL;
        }

        trimNs((*R2)->getSeq(), (*R2)->getLength(), &R2_front_cut, &R2_back_cut);
        
        if (!(*R2)->changeSeq(R2_front_cut, R2_back_cut, minLength)) {
            delete (*R2);
            (*R2) = NULL;
        }

        if ((*R1) && (*R2)) {
            PE_kept++;
            PE_3trim += R1_front_cut + R2_front_cut;
            PE_5trim += R2_back_cut - (*R2)->getLength() +  R1_back_cut - (*R1)->getLength();
        } else if ((*R2)) {
            SE_reads++;
            SE_kept++;
            R1_discarded++;
            SE_3trim += R2_front_cut;
            SE_5trim += R2_back_cut - (*R2)->getLength();
            if (!(*R1)) {
                (*R2)->RC();
            }
        } else if ((*R1)) {
            R2_discarded++;
            SE_reads++;
            SE_kept++;
            SE_3trim += R1_front_cut;
            SE_5trim += R1_back_cut - (*R1)->getLength();
        } else {
            R1_discarded++;
            R2_discarded++;
        }
        

    } else {
        fprintf(stderr, "This makes no sense, Error in non.cpp in function helperTrim(...)\n");
        fprintf(stderr, "R1 and R2 both should never be NULL\n");
        exit(30);
    }

}



void non::trimNs(char *r, uint16_t len, uint16_t *front_cut, uint16_t *back_cut) {
    uint16_t max_end = 1;
    uint16_t max_start = 0;
    uint16_t current_end = 0;
    uint16_t current_start = 0;

    uint16_t i = 0;
    
    while (r[i] != '\0' ) {
        if (r[i] == 'N') {
            /*If no len has been established*/
            if (i - current_start + 1> max_end - max_start) {
                max_end = i;
                max_start = current_start;
                current_start = i + 1;
            } else if (!i) {
                max_start =  1;
                max_end = 1;
                current_start =  1;
            }
        }

        i++;
    }
    
    if (i - current_start > max_end - max_start) {
        max_end = i;
        max_start = current_start;
    }

    *front_cut = max_start;
    *back_cut = max_end;
}



