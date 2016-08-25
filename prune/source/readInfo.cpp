#include "readInfo.h"


bool readInfo::changeSeq(int16_t start, uint16_t end, uint16_t minLength) {
    int length = end - start;
    if (length > minLength) {
        sprintf(seq, "%.*s", end-start, seq+start);
        sprintf(qual, "%.*s", end-start, qual+start);
        return true;
    } else {
        return false;
    }

}

void readInfo::RC() {

    char *tmpSeq = strdup(seq);
    char *tmpQual = strdup(qual);
    uint16_t loc = 0;

    for (int i = len - 1; i >= 0; i--) {
        if (tmpSeq[i] == 'A') {
            seq[loc] = 'T';
        } else if (tmpSeq[i] == 'T') {
            seq[loc] = 'A';
        } else if (tmpSeq[i] == 'C') {
            seq[loc] = 'G';
        } else if (tmpSeq[i] == 'G') {
            seq[loc] = 'C';
        } else if (tmpSeq[i] == 'N') {
            seq[loc] = 'N';
        } else {
            fprintf(stderr, "Error in readInfo.cpp in function RC - illegal char '%c'\n", tmpSeq[i]);
            fprintf(stderr, "Seq string = '%s'\n", tmpSeq);
            exit(29);
        }
        qual[loc] = tmpQual[i];
        loc++;
    }

    free(tmpQual);
    free(tmpSeq);
}


readInfo::readInfo(char *head_, char *seq_, char *qual_, bool optimized_) {
    header = strdup(head_);
    seq = strdup(seq_);
    qual = strdup(qual_);
    len = strlen(seq);
    useq = NULL;
    useq = NULL;
    optimized = optimized_;
}
