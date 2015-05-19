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

/*Inject only once */
static int FLIPIT_SingleInjection = 0;
static int FLIPIT_State = 0;


/*fault injection count*/
static int FLIPIT_InjectionCount = 0;
static unsigned long FLIPIT_Attempts = 0;
static unsigned long FLIPIT_InjCountdown = 0;

/*Fault Injection Statistics*/
static unsigned long* FLIPIT_Histogram;
static int FLIPIT_MAX_LOC = 20000;

/* MPI injections */
static int FLIPIT_Rank = 0;                   
static int FLIPIT_RankInject = 0;

/* Selective Injections */
static int* FLIPIT_FaultSites = NULL;
static int FLIPIT_NumFaultSites = -1;


static unsigned long FLIPIT_Attempts;
static void (*FLIPIT_CustomLogger)(FILE*) = NULL;
static void (*FLIPIT_CountdownCustomLogger)(FILE*) = NULL;
static double (*FLIPIT_FaultProb)() = NULL;

static void flipit_parseArgs(int argc, char** argv);
static int flipit_shouldInject(int fault_index, int inject_once);
static void flipit_print_injectedErr(char* type, unsigned int bPos, int fault_index, double prob,
                                     double p);
static double flipit_countdown();
static void flipit_countdownLogger(FILE*);

/***********************************************************************************************/
/* The functions below are the functions that should be called by a user of FlipIt             */
/***********************************************************************************************/

void FLIPIT_Init(int myRank, int argc, char** argv, unsigned long long seed) {
    char* home, *path;
    FILE* infile;
    FLIPIT_Rank = myRank;
    int ammount;
    flipit_parseArgs(argc, argv);

    if (FLIPIT_Rank == 0)
        printf("Fault injector seed: %llu\n", seed+myRank);
    
    home = getenv("HOME");
    path = (char*) malloc(strlen(home) + strlen("/.FlipItState"));
    strcpy(path, home);
    strcat(path, "/.FlipItState");
    infile = fopen(path, "r");
    if (infile) 
        ammount = fscanf(infile, "%d", &FLIPIT_MAX_LOC);
    fclose(infile);
    FLIPIT_Histogram = (unsigned long*) calloc(FLIPIT_MAX_LOC, sizeof(unsigned long));

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


static void flipit_parseArgs(int argc, char** argv) {
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
            FLIPIT_FaultSites = (int*) malloc( sizeof(int) * FLIPIT_NumFaultSites);
            for(j = 0; j < FLIPIT_NumFaultSites; j++) 
                FLIPIT_FaultSites[j] = atoi(argv[i + j + 1]);
            i += j;
        }
    }

    if (numFaulty == -1){
        FLIPIT_RankInject = 1;
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

static int flipit_shouldInject(int fault_index, int inject_once) {    
    int i;
    int inject = (FLIPIT_State && FLIPIT_RankInject) ? 1 : 0;

    if (!inject)
        return 0;

    /* check fault index site */
    inject = 0;
    if (FLIPIT_FaultSites != NULL) {
        for (i = 0; i < FLIPIT_NumFaultSites; i++)
            if (FLIPIT_FaultSites[i] == fault_index) {
                inject = 1;
                break;
            }
    }
    else
        inject = 1;

    /* inject only once */
    if(inject_once == 1)
        FLIPIT_SingleInjection = 1;
    if(FLIPIT_SingleInjection == 1 && FLIPIT_InjectionCount > 0)
    {
        inject = 0;
        FLIPIT_RankInject = 0;
    }
    FLIPIT_Attempts += inject;
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

char corruptIntData_8bit(int fault_index, int inject_once, double prob, int byte_val,
                         char inst_data) {
    unsigned int bPos;
    double p;

    FLIPIT_Histogram[fault_index]++;
    if (!flipit_shouldInject(fault_index, inject_once))
        return inst_data;

    p = FLIPIT_FaultProb();
    if (p > prob)
        return inst_data;
    
    bPos = rand()%8;

    FLIPIT_InjectionCount++;
    flipit_print_injectedErr("8-bit Integer Data", bPos, fault_index, prob, p);
    FLIPIT_Attempts = 0;
    return inst_data ^ ((char) 0x1 << bPos);
}


short corruptIntData_16bit(int fault_index, int inject_once, double prob, int byte_val, short inst_data) {
    unsigned int bPos;
    double p;
    
    FLIPIT_Histogram[fault_index]++;
    if (!flipit_shouldInject(fault_index, inject_once))
        return inst_data;
    p = FLIPIT_FaultProb();
    if (p > prob)
        return inst_data;

    if (byte_val == -1 || byte_val > 1)
        byte_val = rand()%2;
    bPos=(8*byte_val)+rand()%8;

    FLIPIT_InjectionCount++;
    flipit_print_injectedErr("16-bit Integer Data", bPos, fault_index, prob, p);
    FLIPIT_Attempts = 0;
    return inst_data ^ ((short) 0x1 << bPos);
}


int corruptIntData_32bit(int fault_index, int inject_once, double prob, int byte_val, int inst_data) {
   unsigned int bPos;
    double p;

    FLIPIT_Histogram[fault_index]++;
    if (!flipit_shouldInject(fault_index, inject_once))
        return inst_data;
    p = FLIPIT_FaultProb();
    if (p > prob)
        return inst_data;

    if (byte_val == -1 || byte_val > 3)
        byte_val = rand()%4;
    bPos=(8*byte_val)+rand()%8;
    
    FLIPIT_InjectionCount++;
flipit_print_injectedErr("32-bit Integer Data", bPos, fault_index, prob, p);
    FLIPIT_Attempts = 0;
    return inst_data ^ ((int) 0x1 << bPos);  
}

float corruptFloatData_32bit(int fault_index, int inject_once, double prob, int byte_val, float inst_data) {
    unsigned int bPos;
    double p;
    
    FLIPIT_Histogram[fault_index]++;
    if (!flipit_shouldInject(fault_index, inject_once))
        return inst_data;
    p = FLIPIT_FaultProb();
    if (p > prob)
        return inst_data;

    if (byte_val == -1 || byte_val > 3)
        byte_val = rand()%4;
    bPos=(8*byte_val)+rand()%8;

    FLIPIT_InjectionCount++;
    flipit_print_injectedErr("32-bit IEEE Float Data", bPos, fault_index, prob, p);
    FLIPIT_Attempts = 0;

	int* ptr = (int* ) &inst_data;
    int tmp  = (*ptr ^ (0x1 << bPos));
    float*pf = (float*)&tmp;
    return *pf;
 
}

long long corruptIntData_64bit(int fault_index, int inject_once, double prob,  int byte_val, long long inst_data) {
    unsigned int bPos;
    double p;
    
    FLIPIT_Histogram[fault_index]++;
    if (!flipit_shouldInject(fault_index, inject_once))
        return inst_data;
    p = FLIPIT_FaultProb();
    if (p > prob)
        return inst_data;

    if (byte_val == -1)
        byte_val = rand()%8;
    bPos=(8*byte_val)+rand()%8;
    FLIPIT_InjectionCount++;
    flipit_print_injectedErr("64-bit Integer Data", bPos, fault_index, prob, p);
    FLIPIT_Attempts = 0;
    return inst_data ^ ((long long) 0x1L << bPos);
}

long long corruptPtr2Int_64bit(int fault_index, int inject_once, double prob,  int byte_val, long long inst_data) {
    unsigned int bPos;
    double p;
    
    FLIPIT_Histogram[fault_index]++;
    if (!flipit_shouldInject(fault_index, inject_once))
        return inst_data;
    p = FLIPIT_FaultProb();
    if (p > prob)
        return inst_data;

    if (byte_val == -1)
        byte_val = rand()%8;
    bPos=(8*byte_val)+rand()%8;
    FLIPIT_InjectionCount++;
    flipit_print_injectedErr("Converted Pointer", bPos, fault_index, prob, p);
    FLIPIT_Attempts = 0;
    return inst_data ^ ((long long) 0x1L << bPos);
}

double corruptFloatData_64bit(int fault_index, int inject_once, double prob,  int byte_val, double inst_data) {
    unsigned int bPos;
    double p;

    FLIPIT_Histogram[fault_index]++;
    if (!flipit_shouldInject(fault_index, inject_once))
        return inst_data;
    p = FLIPIT_FaultProb();
    if (p > prob)
        return inst_data;

    if (byte_val == -1)
        byte_val = rand()%8;
    bPos=(8*byte_val)+rand()%8;
    FLIPIT_InjectionCount++;
    flipit_print_injectedErr("64-bit IEEE Float Data", bPos, fault_index, prob, p);
    FLIPIT_Attempts = 0;

	long long* ptr = (long long* ) &inst_data;
    long long tmp  = (*ptr ^ (0x1L << bPos));
    double *pf = (double*)&tmp;
    return *pf;
}

char* corruptIntAdr_8bit(int fault_index, int inject_once, double prob,  int byte_val, char* inst_add) {
    unsigned int bPos;
    double p;

    FLIPIT_Histogram[fault_index]++;
    if (!flipit_shouldInject(fault_index, inject_once))
        return inst_add;
    p = FLIPIT_FaultProb();
    if (p > prob)
        return inst_add;

    if (byte_val == -1)
        byte_val = rand()%8;
    bPos=(8*byte_val)+rand()%8;
    FLIPIT_InjectionCount++;
    flipit_print_injectedErr("8-bit Integer pointer", bPos, fault_index, prob, p);
    FLIPIT_Attempts = 0;
    return (char*) ((long long) inst_add ^ ((long long) 0x1L << bPos));
}

short* corruptIntAdr_16bit(int fault_index, int inject_once, double prob,  int byte_val, short* inst_add) {
    unsigned int bPos;
    double p;

    FLIPIT_Histogram[fault_index]++;
    if (!flipit_shouldInject(fault_index, inject_once))
        return inst_add;
    p = FLIPIT_FaultProb();
    if (p > prob)
        return inst_add;

    if (byte_val == -1)
        byte_val = rand()%8;
    bPos=(8*byte_val)+rand()%8;
    FLIPIT_InjectionCount++;
    flipit_print_injectedErr("16-bit Integer pointer", bPos, fault_index, prob, p);
    FLIPIT_Attempts = 0;
    return (short*) ((long long) inst_add ^ ((long long) 0x1L << bPos));
}

int* corruptIntAdr_32bit(int fault_index, int inject_once, double prob,  int byte_val, int* inst_add) {
    unsigned int bPos;
    double p;

    FLIPIT_Histogram[fault_index]++;
    if (!flipit_shouldInject(fault_index, inject_once))
        return inst_add;
    p = FLIPIT_FaultProb();
    if (p > prob)
        return inst_add;

    if (byte_val == -1)
        byte_val = rand()%8;
    bPos=(8*byte_val)+rand()%8;
    FLIPIT_InjectionCount++;
    flipit_print_injectedErr("32-bit Integer pointer", bPos, fault_index, prob, p);
    FLIPIT_Attempts = 0;
    return (int*) ((long long) inst_add ^ ((long long) 0x1L << bPos));
}

long long* corruptIntAdr_64bit(int fault_index, int inject_once, double prob,  int byte_val, long long* inst_add) {
    unsigned int bPos;
    double p;

    FLIPIT_Histogram[fault_index]++;
    if (!flipit_shouldInject(fault_index, inject_once))
        return inst_add;
    p = FLIPIT_FaultProb();
    if (p > prob)
        return inst_add;  

   if (byte_val == -1)
        byte_val = rand()%8;
    bPos=(8*byte_val)+rand()%8;
    FLIPIT_InjectionCount++;
    flipit_print_injectedErr("64-bit Integer pointer", bPos, fault_index, prob, p);
    FLIPIT_Attempts = 0;
    return (long long*) ((long long) inst_add ^ ((long long) 0x1L << bPos));
}

float* corruptFloatAdr_32bit(int fault_index, int inject_once, double prob,  int byte_val, float* inst_add) {
    unsigned int bPos;
    double p;

    FLIPIT_Histogram[fault_index]++;
    if (!flipit_shouldInject(fault_index, inject_once))
        return inst_add;
    p = FLIPIT_FaultProb();
    if (p > prob)
        return inst_add;

    if (byte_val == -1)
        byte_val = rand()%8;
    bPos=(8*byte_val)+rand()%8;
    FLIPIT_InjectionCount++;
    flipit_print_injectedErr("32-bit IEEE Float pointer", bPos, fault_index, prob, p);
    FLIPIT_Attempts = 0;
    return (float*) ((long long) inst_add ^ ((long long) 0x1L << bPos));
}

double* corruptFloatAdr_64bit(int fault_index, int inject_once, double prob,  int byte_val, double* inst_add) {
    unsigned int bPos;
    double p;

    FLIPIT_Histogram[fault_index]++;
    if (!flipit_shouldInject(fault_index, inject_once))
        return inst_add;
    p = FLIPIT_FaultProb();
    if (p > prob)
        return inst_add;

    if (byte_val == -1)
        byte_val = rand()%8;
    bPos=(8*byte_val)+rand()%8;
    FLIPIT_InjectionCount++;
    flipit_print_injectedErr("64-bit IEEE Float pointer", bPos, fault_index, prob, p);
    FLIPIT_Attempts = 0;
    return (double *) ((long long) inst_add ^ ((long long) 0x1L << bPos)); 
}
