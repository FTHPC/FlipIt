/***********************************************************************************************/
/* This file is licensed under the University of Illinois/NCSA Open Source License.            */
/* See LICENSE.TXT for details.                                                                */
/***********************************************************************************************/

/***********************************************************************************************/
/*                                                                                             */
/* Name: corrupt.h                                                                             */
/*                                                                                             */
/* Description: Header file for the user-callable FlipIt function as well as the definition of */
/*              the functions inserted by the compiler pass (faults.cpp). These corruption     */
/*              functions corrupt a single bit in the supplied value if we are to inject.      */
/*                                                                                             */
/***********************************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif 

#ifndef CORRUPTH
#define CORRUPTH


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>                                                                                                                     
#include <stdint.h> 
#include <limits.h>

#define FLIPIT_ON 1
#define FLIPIT_OFF 0


/* setting up and house keeping */
void FLIPIT_Init(uint32_t myRank, uint32_t argc, char** argv, uint64_t seed);
void FLIPIT_Finalize(char* filename);
void FLIPIT_SetInjector(int state);
void FLIPIT_SetRankInject(int state);
void FLIPIT_SetFaultProbability(double(faultProb)());
void FLIPIT_SetCustomLogger(void (customLogger)(FILE*));
void FLIPIT_CountdownTimer(unsigned long numInstructions);
unsigned long long FLIPIT_GetExecutedInstructionCount();
int FLIPIT_GetInjectionCount();

/* FORTRAN VERSIONS (ex: CALL flipit_init_ftn(myrank, argc, argv, seed) */
int flipit_init_ftn_(int* myRank, int* argc, char*** argv, unsigned long long* seed);
int flipit_finalize_ftn_(char** filename);
int flipit_setinjector_ftn_(int* state);
int flipit_setrankinject_ftn_(int* state);
/* void flipit_setfaultprobability_ftn_(double(faultProb)()); */
/* void flipit_setcustomlogger_ftn_(void (customLogger)(FILE*)); */
int flipit_countdowntimer_ftn_(unsigned long* numInstructions);

/* corrupt the data */
float      corruptFloatData_32bit (uint32_t parameter, double prob, float inst_data);
uint64_t corruptIntData_64bit   (uint32_t parameter, double prob, uint64_t inst_data);
double     corruptFloatData_64bit (uint32_t parameter, double prob, double inst_data);
uint64_t corruptPtr2Int_64bit   (uint32_t parameter, double prob, uint64_t inst_data);
/*
char       corruptIntData_8bit    (int fault_index, int inject_once, double prob, int byte_val, char inst_data);
short      corruptIntData_16bit   (int fault_index, int inject_once, double prob, int byte_val, short inst_data);
int        corruptIntData_32bit   (int fault_index, int inject_once, double prob, int byte_val, int inst_data);
float      corruptFloatData_32bit (int fault_index, int inject_once, double prob, int byte_val, float inst_data);
long long  corruptIntData_64bit   (int fault_index, int inject_once, double prob, int byte_val, long long inst_data);
long long  corruptPtr2Int_64bit   (int fault_index, int inject_once, double prob, int byte_val, long long inst_data);
double     corruptFloatData_64bit (int fault_index, int inject_once, double prob, int byte_val, double inst_data);

char*       corruptIntAdr_8bit    (int fault_index, int inject_once, double prob, int byte_val, char* inst_data);
short*     corruptIntAdr_16bit    (int fault_index, int inject_once, double prob, int byte_val, short* inst_data);
int*       corruptIntAdr_32bit    (int fault_index, int inject_once, double prob, int byte_val, int* inst_add);
long long* corruptIntAdr_64bit    (int fault_index, int inject_once, double prob, int byte_val, long long* inst_add);
float*     corruptFloatAdr_32bit  (int fault_index, int inject_once, double prob, int byte_val, float* inst_add);
double*    corruptFloatAdr_64bit  (int fault_index, int inject_once, double prob, int byte_val, double* inst_add);
*/
#endif

#ifdef __cplusplus
}
#endif
