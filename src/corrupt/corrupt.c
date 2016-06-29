/***********************************************************************************************/
/* This file is licensed under the University of Illinois/NCSA Open Source License.            */
/* See LICENSE.TXT for details.                                                                */
/***********************************************************************************************/

/***********************************************************************************************/
/*                                                                                             */
/* Name: corrupt.c                                                                             */
/*                                                                                             */
/* Description: User-callable FlipIt function as well as the definition of the functions       */
/*              inserted by the compiler pass (faults.cpp). These corruption functions corrupt */
/*              a single bit in the supplied value if we are to inject.                        */
/*                                                                                             */
/***********************************************************************************************/

#include "corrupt.h"

#define FAULT_IDX_MASK 0x00FFFFFF

static uint32_t FLIPIT_MaxInjections = 1;
static uint32_t FLIPIT_State = 0;


/*fault injection count*/
static uint32_t FLIPIT_InjectionCount = 0;
static uint64_t FLIPIT_Attempts = 0;
static uint64_t FLIPIT_InjCountdown = 0;
static uint64_t FLIPIT_TotalInsts = 0;

/*Fault Injection Statistics*/
static uint64_t* FLIPIT_Histogram;
static uint32_t FLIPIT_MAX_LOC = 20000;
static char* FLIPIT_StateFile = NULL;

static uint32_t FLIPIT_MAX_INJECT_LINES = 33554432;
static uint32_t FLIPIT_REMAIN_INJECT_COUNT = 1;  
static uint32_t FLIPIT_char_type_size = sizeof(char);

/* MPI injections */
static uint32_t FLIPIT_Rank = 0;                   
static uint32_t FLIPIT_RankInject = 0;

/* Selective Injections */
static uint32_t* FLIPIT_FaultSites = NULL;
static int32_t FLIPIT_NumFaultSites = -1;


static void (*FLIPIT_CustomLogger)(FILE*) = NULL;
static void (*FLIPIT_CountdownCustomLogger)(FILE*) = NULL;
static double (*FLIPIT_FaultProb)() = NULL;

static void flipit_parseArgs(uint32_t argc, char** argv);
static uint8_t flipit_shouldInjectNoCheck(); 
static uint8_t flipit_checkActiveFaultSite(uint32_t fault_index);
static void flipit_print_injectedErr(char* type, unsigned int bPos, int fault_index, double prob,
                                     double p);
static double flipit_countdown();
static void flipit_countdownLogger(FILE*);

/***********************************************************************************************/
/* The functions below are the functions that should be called by a user of FlipIt             */
/***********************************************************************************************/

void FLIPIT_Init(uint32_t myRank, uint32_t argc, char** argv, uint64_t seed) {
    FILE* infile;
    FLIPIT_Rank = myRank;
    int amount;
    flipit_parseArgs(argc, argv);

    if (FLIPIT_Rank == 0)
        printf("Fault injector seed: %llu\n", (unsigned long long)seed+myRank);
    
    infile = fopen(FLIPIT_StateFile, "r");
    
    if (infile) {
        int ret = fscanf(infile, "%d", &amount);
        if (amount > FLIPIT_MAX_LOC)
            FLIPIT_MAX_LOC = amount;
        fclose(infile);
    }
#ifdef FLIPIT_HISTOGRAM
    FLIPIT_Histogram = (uint64_t*) calloc(FLIPIT_MAX_LOC, sizeof(uint64_t));
#endif
#ifdef FLIPIT_DEBUG
    printf("Rank %d alloced an FLIPIT_Histogram of length: %d\n", FLIPIT_Rank, FLIPIT_MAX_LOC);
#endif
    FLIPIT_State = FLIPIT_ON;
    srand(seed + myRank);
    srand48(seed + myRank);
    FLIPIT_SetFaultProbability(drand48);
}

void FLIPIT_Finalize(char* fname) {
    int i;
    FILE* outfile;
#ifdef FLIPIT_HISTOGRAM
    if (fname != NULL) {
        char filename[500];
        char tmp[15];
        strcpy(filename, fname);
        strcat(filename, "_");
        sprintf(tmp, "%d", FLIPIT_Rank);
        strcat(filename, tmp);

        outfile = fopen(filename, "w");
        for (i = 0; i < FLIPIT_MAX_LOC; i++)
            fprintf(outfile, "Location %i: %lu\n", i, FLIPIT_Histogram[i]);
        fclose(outfile);
    }
    
    free(FLIPIT_Histogram);
#endif
    if (FLIPIT_FaultSites != NULL)
        free(FLIPIT_FaultSites);
}

void FLIPIT_SetInjector(int state) {
    if (state == FLIPIT_ON || state == FLIPIT_OFF)
        FLIPIT_State = state;
}


void FLIPIT_SetRankInject(int state) {
    if (state == FLIPIT_ON || state == FLIPIT_OFF)
        FLIPIT_RankInject = state;
}


void FLIPIT_SetFaultProbability(double (prob)()) {
    FLIPIT_FaultProb = prob;
}


void FLIPIT_SetCustomLogger(void (logger)(FILE*)) {
    if (FLIPIT_InjCountdown != 0) {
        FLIPIT_CountdownCustomLogger = logger;
        FLIPIT_CustomLogger = flipit_countdownLogger;
    }
    else {
        FLIPIT_CustomLogger = logger;
    }
}

void FLIPIT_CountdownTimer(unsigned long numInstructions) {
    FLIPIT_InjCountdown = numInstructions;
    FLIPIT_SetFaultProbability(flipit_countdown);
    
    FLIPIT_CountdownCustomLogger = FLIPIT_CustomLogger;
    FLIPIT_CustomLogger = flipit_countdownLogger;
}

unsigned long long FLIPIT_GetExecutedInstructionCount() {
    return FLIPIT_TotalInsts;
}

int FLIPIT_GetInjectionCount() {
    return FLIPIT_InjectionCount;
}

void FLIPIT_SetMaxInjections(int n)
{
    if (n < 0)
        printf("Warning: Attempting to set Max Injections to negative value %d. Defaulting to 1.\n", n);
        n  = 1;

    // set max number of injections for this rank;
    // then calculate the remaing number of injections if any
    FLIPIT_MaxInjections = n;
    if (FLIPIT_MaxInjections > FLIPIT_InjectionCount)
        FLIPIT_REMAIN_INJECT_COUNT = FLIPIT_MaxInjections - FLIPIT_InjectionCount;
    else
        FLIPIT_REMAIN_INJECT_COUNT = 0;

    assert(FLIPIT_REMAIN_INJECT_COUNT >= 0
        && "ERROR: NEGATIVE NUMBER OF REMAINING INJECTIONS!!!");  
}

int FLIPIT_GetMaxInjections()
{
    return FLIPIT_MaxInjections;
}
/***********************************************************************************************/
/* User callable function for FORTRAN wrapper                                              */
/***********************************************************************************************/
int flipit_init_ftn_(int* myRank, int* argc, char*** argv, unsigned long long* seed) {
    if (argv == NULL)
        FLIPIT_Init(*myRank, 0, NULL, *seed); 
    else
        FLIPIT_Init(*myRank, *argc, *argv, *seed);
    
    return 0;
}

int flipit_finalize_ftn_(char** filename) {
    if (filename != NULL)
        FLIPIT_Finalize(*filename);
    else
        FLIPIT_Finalize(NULL);

    return 0;
}

int flipit_setinjector_ftn_(int* state) {
    FLIPIT_SetInjector(*state);
    
    return 0;
}

int flipit_setrankinject_ftn_(int* state) {
    FLIPIT_SetRankInject(*state);
    
    return 0;
}

int flipit_countdowntimer_ftn_(unsigned long* numInstructions) {
    FLIPIT_CountdownTimer(*numInstructions);
    
    return 0;
}


/***********************************************************************************************/
/* The functions below this are used internally by FlipIt                                      */
/***********************************************************************************************/


static void flipit_parseArgs(uint32_t argc, char** argv) {
    int i, j;
    int numFaulty = -1;
    
    for (i = 1; i < argc; i++){
        if (strcmp("--numberFaulty", argv[i]) == 0 || strcmp("-nF", argv[i]) == 0)
            numFaulty = atoi(argv[++i]);
        else if (strcmp("--faulty", argv[i]) == 0 || strcmp("-f", argv[i]) == 0) {
            for(j = 0; j < numFaulty; j++)
                if (atoi(argv[i + j + 1]) == FLIPIT_Rank)
                    FLIPIT_RankInject = 1;
            i += j;
        }

        else if (strcmp("--numberFaultLoc", argv[i]) == 0 || strcmp("-nLOC", argv[i]) == 0)
            FLIPIT_NumFaultSites = atoi(argv[++i]);
        else if (strcmp("--faultyLoc", argv[i]) == 0 || strcmp("-fLOC", argv[i]) == 0) {
            FLIPIT_FaultSites = (uint32_t*) malloc( sizeof(uint32_t) * FLIPIT_NumFaultSites);
            for(j = 0; j < FLIPIT_NumFaultSites; j++) 
                FLIPIT_FaultSites[j] = atoi(argv[i + j + 1]);
            i += j;
        }
        else if (strcmp("--stateFile", argv[i]) == 0 || strcmp("-sF", argv[i]) == 0) {
            int len = strlen(argv[i]) + 1;
            FLIPIT_StateFile = (char*) malloc(sizeof(char)*len);
            strcpy(FLIPIT_StateFile, argv[i]);
        }
    }

    // verify variables have valid values
    if (numFaulty == -1){
        FLIPIT_RankInject = 1;
    }
    if (FLIPIT_StateFile == NULL) {
        char* home = getenv("HOME");
        FLIPIT_StateFile = (char*) malloc(strlen(home) + strlen("/.FlipItState")+1);
        strcpy(FLIPIT_StateFile, home);
        strcat(FLIPIT_StateFile, "/.FlipItState");

    }
#ifdef FLIPIT_DEBUG
    if(FLIPIT_Rank == 0)
    {
        for (i=0; i<argc; i++)
            printf("Arg[%d] = %s\n", i, argv[i]);
        if (FLIPIT_FaultSites == NULL)
            printf("NULL\n");
        else
            printf("Faulty sites(%d):\n", FLIPIT_NumFaultSites);
        for (i = 0; i < FLIPIT_NumFaultSites; i++)
            printf("%d\n", FLIPIT_FaultSites[i] );
        printf("Num faulty = %d\n", numFaulty);
    } 
#endif
}

static uint8_t flipit_shouldInjectNoCheck() {
    FLIPIT_TotalInsts++;                                    //CS
    if ((0 == FLIPIT_State)
        || (0 == FLIPIT_RankInject)
        || (0 == FLIPIT_REMAIN_INJECT_COUNT))  //CS
        return 0;
    FLIPIT_Attempts++;                                      //CS
    return 1;   
}


static uint8_t flipit_checkActiveFaultSite(uint32_t fault_index) {
/*
    FLIPIT_TotalInsts++;                                    //CS
    if ((0 == FLIPIT_State) || (0 == FLIPIT_RankInject) || (0 == FLIPIT_REMAIN_INJECT_COUNT)) //CS
        return 0;

    uint32_t index = fault_index / FLIPIT_char_type_size;
    uint8_t bit = fault_index % FLIPIT_char_type_size;
    uint8_t inject = (FLIPIT_FaultSites[index] & (0x1 << (bit))) >> (bit);
    FLIPIT_Attempts += inject;                              //CS
    return inject;
*/
/*
    int inject_once = FLIPIT_MaxInjections == 1;
    int i;
    int inject = flipit_shouldInjectNoCheck();
    FLIPIT_TotalInsts++;
    if (!inject)
        return 0;
*/
    /* check fault index site */
    int inject = 0;
    int i = 0;
    if (FLIPIT_FaultSites != NULL) {
        for (i = 0; i < FLIPIT_NumFaultSites; i++)
            if (FLIPIT_FaultSites[i] == fault_index) {
                inject = 1;
                break;
            }
    }
    else
        inject = 1;
    
    //FLIPIT_Attempts += inject;
    return inject;
}

static void flipit_print_injectedErr(char* type, unsigned int bPos, int fault_index, double prob,
                                     double p) {
    printf("\n/*********************************Start**************************************/\n"
            "\nSuccessfully injected %s error!!\nRank: %d\n"
            "Total # faults injected: %d\n" 
            "Bit position is: %u\n"
            "Index of the fault site: %d\n"
            "Fault site probability: %e\n"
            "Chosen random probability is: %e\n" 
            "Attempts since last injection: %lu\n", type, FLIPIT_Rank, FLIPIT_InjectionCount,
                                                    bPos, fault_index, 
            prob, p, FLIPIT_Attempts);   
    if (FLIPIT_CustomLogger != NULL)
        FLIPIT_CustomLogger(stdout);
    printf("\n/*********************************End**************************************/\n");
}

static double flipit_countdown() {
    return (double) --FLIPIT_InjCountdown;
}

static void flipit_countdownLogger(FILE* outfile) {
    if (FLIPIT_CountdownCustomLogger != NULL)
        FLIPIT_CountdownCustomLogger(outfile);
    FLIPIT_InjCountdown = FLIPIT_Attempts;
}

/***********************************************************************************************/
/* The functions below this are inserted by the compiler pass to flip a bit                    */
/***********************************************************************************************/

uint64_t corruptIntData_64bit(uint32_t parameter, double prob, uint64_t inst_data)
{

#ifdef FLIPIT_HISTOGRAM
    // extract fault_index, byte_val from parameter
    uint32_t fault_index = (uint32_t) (parameter & FAULT_IDX_MASK);
    FLIPIT_Histogram[fault_index]++;
#endif

    // verify that it is the correct time to inject
    if (0 == flipit_shouldInjectNoCheck()) return inst_data;
    float p = FLIPIT_FaultProb();
    if (p > prob) return inst_data;
#ifndef FLIPIT_HISTOGRAM
    uint32_t fault_index = (uint32_t) (parameter & FAULT_IDX_MASK);
#endif
    if (0 == flipit_checkActiveFaultSite(fault_index)) return inst_data;

    // determine which bit & byte should be flipped
    char bit = ((parameter >> 24) & 0xF); 
    char byte = ((parameter >> 28) & 0xF);
    //printf("###Byte = %d, Bit = %d\n", byte, bit);            

    if (bit == 0xF) bit = rand() % 8; //get correct bit
    if (byte > 7) byte = rand() % (16 - byte); 

    //printf("Byte = %d, Bit = %d\n", byte, bit);            
    FLIPIT_InjectionCount++;
    FLIPIT_REMAIN_INJECT_COUNT--;
    if (FLIPIT_REMAIN_INJECT_COUNT == 0) FLIPIT_RankInject = 0; 
    
    flipit_print_injectedErr("Integer Data", byte*8 + bit, fault_index, prob, p);
    FLIPIT_Attempts = 0;
    return inst_data ^ ((uint64_t) 0x1L << (byte*8 + bit)); //TODO: correctly wrap for 32, 16, and 8 bit integers
}

////////////////////////////////////////////////////////////////////////////////////////////////////
float corruptFloatData_32bit(uint32_t parameter, double prob, float inst_data)
{

#ifdef FLIPIT_HISTOGRAM
    // extract fault_index, byte_val from parameter
    uint32_t fault_index = (uint32_t) (parameter & FAULT_IDX_MASK);
    FLIPIT_Histogram[fault_index]++;
#endif

    //TODO: add support for CHECK()
    if (0 == flipit_shouldInjectNoCheck()) return inst_data;
    float p = FLIPIT_FaultProb();
    if (p > prob) return inst_data;
#ifndef FLIPIT_HISTOGRAM
    uint32_t fault_index = (uint32_t) (parameter & FAULT_IDX_MASK);
#endif
    if (0 == flipit_checkActiveFaultSite(fault_index)) return inst_data;


    // determine which bit & byte should be flipped
    char bit = ((parameter >> 24) & 0xF); 
    char byte = ((parameter >> 28) & 0xF);

    if (bit == 0xF)
        bit = rand() % 8; 
    else
        bit = bit % 32;
    if (byte > 7)
        byte = rand() % 4; // random byte (we know the size)
    else
        byte = byte % 4; // wrap fixed byte to sizeof(float) 
            
    FLIPIT_InjectionCount++;
    FLIPIT_REMAIN_INJECT_COUNT--;
    if (FLIPIT_REMAIN_INJECT_COUNT == 0) FLIPIT_RankInject = 0; 
    
    flipit_print_injectedErr("32-bit IEEE Float Data", byte*8 + bit, fault_index, prob, p);
    FLIPIT_Attempts = 0;
    
    int* ptr = (int* ) &inst_data;
    int tmp  = (*ptr ^ (0x1 << (bit + byte * 8)));
    float*pf = (float*)&tmp;
    return *pf;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
double corruptFloatData_64bit(uint32_t parameter, double prob, double inst_data)
{

#ifdef FLIPIT_HISTOGRAM
    // extract fault_index, byte_val from parameter
    uint32_t fault_index = (uint32_t) (parameter & FAULT_IDX_MASK);
    FLIPIT_Histogram[fault_index]++;
#endif

    //TODO: add support for CHECK()
    if (0 == flipit_shouldInjectNoCheck()) return inst_data;
    float p = FLIPIT_FaultProb();
    if (p > prob) return inst_data;
#ifndef FLIPIT_HISTOGRAM
    uint32_t fault_index = (uint32_t) (parameter & FAULT_IDX_MASK);
#endif
    if (0 == flipit_checkActiveFaultSite(fault_index)) return inst_data;

    // determine which bit & byte should be flipped
    char bit = ((parameter >> 24) & 0xF); //TODO: VERIFY
    char byte = ((parameter >> 28) & 0xF);

    if (bit == 0xF)
        bit = rand() % 8; //TODO VEIFY TODO wrap bit
    if (byte > 7)
        byte = rand() % 8;
            
    FLIPIT_InjectionCount++;
    FLIPIT_REMAIN_INJECT_COUNT--;
    if (FLIPIT_REMAIN_INJECT_COUNT == 0) FLIPIT_RankInject = 0; 
    
    flipit_print_injectedErr("64-bit IEEE Float Data", byte*8 + bit, fault_index, prob, p);
    FLIPIT_Attempts = 0;
    
	long long* ptr = (long long* ) &inst_data;
    long long tmp  = (*ptr ^ (0x1L << (byte*8 + bit)));
    double *pf = (double*)&tmp;
    return *pf;
}

uint64_t corruptPtr2Int_64bit(uint32_t parameter, double prob, uint64_t inst_data)
{

#ifdef FLIPIT_HISTOGRAM
    // extract fault_index, byte_val from parameter
    uint32_t fault_index = (uint32_t) (parameter & FAULT_IDX_MASK);
    FLIPIT_Histogram[fault_index]++;
#endif

    //TODO: add support for CHECK()
    if (0 == flipit_shouldInjectNoCheck()) return inst_data;
    float p = FLIPIT_FaultProb();
    if (p > prob) return inst_data;
#ifndef FLIPIT_HISTOGRAM
    uint32_t fault_index = (uint32_t) (parameter & FAULT_IDX_MASK);
#endif
    if (0 == flipit_checkActiveFaultSite(fault_index)) return inst_data;


    // determine which bit & byte should be flipped
    char bit = ((parameter >> 24) & 0xF); //TODO: VERIFY
    char byte = ((parameter >> 28) & 0xF);

    if (bit == 0xF) bit = rand() % 8; //get correct bit
    if (byte > 7) byte = rand() % 8; 
            
    FLIPIT_InjectionCount++;
    FLIPIT_REMAIN_INJECT_COUNT--;
    if (FLIPIT_REMAIN_INJECT_COUNT == 0) FLIPIT_RankInject = 0; 
    
    flipit_print_injectedErr("Converted Pointer", byte*8 + bit, fault_index, prob, p);
    FLIPIT_Attempts = 0;
    return inst_data ^ ((uint64_t) 0x1L << (byte*8 + bit)); 
}


