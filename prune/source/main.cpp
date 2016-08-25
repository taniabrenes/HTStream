#include "argCollector.h"
#include "non.h"
#include "readInfo.h"
#include "fileHelper.h"


void writerHelper(readInfo *r1, readInfo *r2, FileWriter *R1, FileWriter *R2, FileWriter *SE) {

    if (r1 && r2) {
        if (!R2) {
            R1->writeData(r1, r2, NULL);
        } else {
            R1->writeData(r1, NULL, NULL);
            R2->writeData(r2, NULL, NULL);
        }
    } else if (r1) {
        if (!SE) {
            R1->writeData(r1, NULL, NULL);
        } else {
            SE->writeData(r1, NULL, NULL);
        }
    } else if (r2) {
        if (!SE) {
            R1->writeData(r2, NULL, NULL);
        } else {
            SE->writeData(r2, NULL, NULL);
        }
    }
    delete r1;
    delete r2;

}

int main(int argc, char *argv[]) {
        
    non *no = new non();
    argCollector *args = new argCollector(argc, argv, &no);

    /*Error checking to make sure R1 and R2 exist in arrCollector*/
    if (args->R1_In && args->R2_In) {
        FileHelper *R1 = args->R1_In;
        FileHelper *R2 = args->R2_In;
        readInfo *r1, *r2;

        /*Yeah, I hate this too, I did try and do while((r1 = R1-readData) != NULL && (r2 = R2->readData....
         * however, it would check the R1->getData, receive NULL, and then exit if statment
         * this means r2 wouldn't also be set to null to check the size of the file R2
         * that is why I stuck with always true with the break*/
        while(1) {
            R1->readData(&r1);
            R2->readData(&r2);
            if (!r1 || !r2) {
                break;
            }
            /*The only reason I'm doing this is for error checking 
             * I need to make sure the files are the same size*/
            /*Add trimming here*/
            
            no->helperTrim(&r1, &r2);
            writerHelper(r1, r2, args->R1_Out, args->R2_Out, args->SE_Out);
            //front_cut = no->three_prime_cut(r1->getSeq(), r1->getLength());
            //back_cut = no->five_prime_cut(r2->getSeq(), r2->getLength());

        }
        /*Means the files are different lengths*/
        if (!r1 && r2) {
            fprintf(stderr, "File R1 is shorter than File R2\n");
            exit(16);
        } else if (r1 && !r2) {
            fprintf(stderr, "File R2 is shorter than File R1\n");
            exit(17);
        }
    }




    if (args->SE_In) {
        FileHelper *SE = args->SE_In;
        readInfo *se;

        while(1) {
            SE->readData(&se);
            if (se) {
                /*Single end reads R1 is !null R2 is null*/
                /*Single end trim here*/
                no->helperTrim(&se, NULL);
                writerHelper(se, NULL, args->R1_Out, args->R2_Out, args->SE_Out);

            } else {
                break;
            }
        }      

    }

    if (args->INTER_In) {
        FileHelper *INTER = args->INTER_In;
        readInfo *r1, *r2;

        while (1) {
            INTER->readData(&r1, &r2);
            if (r1 && r2) {
                no->helperTrim(&r1, &r2);
                writerHelper(r1, r2, args->R1_Out, args->R2_Out, args->SE_Out);

            } else {
                break;
            }    
        }
    }
    /*Inter file checking is wihtin fileHelper read*/



    if (args->TAB_In) {
        FileHelper *TAB = args->TAB_In;
        readInfo *r1, *r2;
        while (1) {
            TAB->readData(&r1, &r2);
            if (r1) {
                no->helperTrim(&r1, &r2);
            } else { 
                break;
            }
            writerHelper(r1, r2, args->R1_Out, args->R2_Out, args->SE_Out);
        }
    }

    if (args->STDIN_In) {
        FileHelper *STDIN = args->STDIN_In;
        readInfo *r1, *r2;
        while (1) {
            STDIN->readData(&r1, &r2);
            if (r1) {
                no->helperTrim(&r1, &r2);
            } else { 
                break;
            }
            writerHelper(r1, r2, args->R1_Out, args->R2_Out, args->SE_Out);

        }
    }
   
    no->stopTime();
    no->outputStats(args->log);

}
