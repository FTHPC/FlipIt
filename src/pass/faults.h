/***********************************************************************************************/
/* This file is licensed under the University of Illinois/NCSA Open Source License.            */
/* See LICENSE.TXT for details.                                                                */
/***********************************************************************************************/

/***********************************************************************************************/
/*                                                                                             */
/* Name: faults.h                                                                              */
/*                                                                                             */
/* Description: Header file for the LLVM IR compiler pass. This pass add calls to corrupting   */
/*              functions that will corrupt (single bit-flip) the result of LLVM instructions  */
/*              with a certain probability.                                                    */
/*                                                                                             */
/***********************************************************************************************/

#ifndef FAULTS_H
#define FAULTS_H

#include <sstream>
#include <algorithm>
#include <iterator>
#include <string>
    using std::string;
#include <vector>
    using std::vector;
#include <assert.h>
#include <stdlib.h>
#include <iostream>
#include <map>
#include <fstream>
    using std::ifstream;
    using std::ofstream;
    using std::ios;
#include <cxxabi.h>
#include <sys/file.h>
#include <stdio.h>
#include <unistd.h>

#include "FlipIt/pass/Logger.h"

#include <llvm/Pass.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/User.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/Statistic.h>
 
#include <llvm/Support/CommandLine.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/PassManager.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Assembly/PrintModulePass.h>
#include <llvm/DebugInfo.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/TypeBuilder.h>

using namespace llvm;


#ifdef COMPILE_PASS
static cl::opt<string> funcList("funcList", cl::desc("Name(s) of the function(s) to be targeted"), cl::value_desc("func1 func2 func3"), cl::init(""), cl::ValueRequired);
static cl::opt<string> configPath("config", cl::desc("Path to the FlipIt Config file"), cl::value_desc("/path/to/FlipIt.config"), cl::init("FlipIt.config"));
static cl::opt<double> siteProb("prob", cl::desc("Probability that instrution is faulty"), cl::value_desc("Any value [0,1)"), cl::init(1e-8), cl::ValueRequired);
static cl::opt<int> byte_val("byte", cl::desc("Which byte to consider for fault injection"), cl::value_desc("-1-7"), cl::init(-1), cl::ValueRequired);
static cl::opt<int> singleInj("singleInj", cl::desc("Inject Error Only Once"), cl::value_desc("0/1"), cl::init(1), cl::ValueRequired);
static cl::opt<bool> arith_err("arith", cl::desc("Inject Faults Into Arithmetic Instructions"), cl::value_desc("0/1"), cl::init(1), cl::ValueRequired);
static cl::opt<bool> ctrl_err("ctrl", cl::desc("Inject Faults Into Control Instructions"), cl::value_desc("0/1"), cl::init(1), cl::ValueRequired);
static cl::opt<bool> ptr_err("ptr", cl::desc("Inject Faults Into Pointer Instructions"), cl::value_desc("0/1"), cl::init(1), cl::ValueRequired);
static cl::opt<string> srcFile("srcFile", cl::desc("Name of the source file being compiled"), cl::value_desc("e.g. foo.c, foo.cpp, or foo.f90"), cl::init("UNKNOWN"), cl::ValueRequired);
static cl::opt<string> stateFile("stateFile", cl::desc("Name of the state file being updated when compiled. Used to provide unique fault site indexes."), cl::value_desc("FlipItState"), cl::init("FlipItState"), cl::ValueRequired);
#endif




/*Dynamic Fault Injection LLVM Pass*/
namespace FlipIt {
#ifdef COMPILE_PASS
    class DynamicFaults : public ModulePass {
#else
    class DynamicFaults {
            std::string funcList;
            std::string configPath;
            double siteProb;
            int byte_val;
            int singleInj;
            bool arith_err;
            bool ctrl_err;
            bool ptr_err;
            std::string srcFile;
            std::string stateFile;
#endif
        public:
            static char ID; 

#ifdef COMPILE_PASS
            DynamicFaults(); 
            DynamicFaults(string funcList, string configPath, double siteProb, int byte_val, int singleInj, bool arith_err, bool ctrl_err, bool ptr_err, std::string srcFile, std::string stateFile, Module* M); 
#else
            DynamicFaults(Module* M); 
            DynamicFaults(string funcList, string configPath, double siteProb, int byte_val, int singleInj, bool arith_err, bool ctrl_err, bool ptr_err, std::string srcFile, std::string stateFile, Module* M); 

#endif
            virtual ~DynamicFaults()
                { finalize(); }
            virtual bool runOnModule(Module &M);
            bool corruptInstruction(Instruction* I);

		private:

            void init();
            bool finalize();
			//std::vector<std::string> splitAtSpace(std::string spltStr);
			void splitAtSpace();
            int selectArgument(CallInst* callInst);
            void readConfig(string path);
            Value* getInstProb(Instruction* I);
            std::string demangle(std::string name);
            bool viableFunction(std::string name, std::vector<std::string>& flist);
            unsigned long updateStateFile(const char* stateFile, unsigned long sum);

            bool injectControl(Instruction* I);
            bool injectArithmetic(Instruction* I);
            bool injectPointer(Instruction* I);
            bool injectCall(Instruction* I);
            
            bool injectControl_NEW(Instruction* I);
            bool injectArithmetic_NEW(Instruction* I);
            bool injectPointer_NEW(Instruction* I);
            bool injectCall_NEW(Instruction* I);
            bool injectResult(Instruction* I);
			bool injectInOperand(Instruction* I, int operand);
            
            bool inject_Store_Data(Instruction* I,  CallInst* CallI);
            bool inject_Compare(Instruction* I, CallInst* CallI);
            bool inject_Generic(Instruction* I, CallInst* CallI,  BasicBlock* BB);

            bool inject_Store_Ptr(Instruction* I, CallInst* CallI);
            bool inject_Load_Ptr(Instruction* I, CallInst* CallI, BasicBlock* BB);
            bool inject_Alloc_Ptr(Instruction* I, CallInst* CallI, BasicBlock* BB);
            bool inject_Call(Instruction* I, CallInst* CallI, BasicBlock* BB);
            bool inject_GetElementPtr_Ptr(Instruction* I, CallInst* CallI, BasicBlock* BB);

            unsigned long cacheFunctions();
            bool injectFault(Instruction* I);

            Module* M;
            LogFile* logfile;
 
            Value* func_corruptIntData_8bit;
            Value* func_corruptIntData_16bit;
            Value* func_corruptIntData_32bit;
            Value* func_corruptIntData_64bit;
            Value* func_corruptPtr2Int_64bit; 
            Value* func_corruptFloatData_32bit;
            Value* func_corruptFloatData_64bit;
            Value* func_corruptIntAdr_8bit;
            Value* func_corruptIntAdr_16bit;
            Value* func_corruptIntAdr_32bit;
            Value* func_corruptIntAdr_64bit;
            Value* func_corruptFloatAdr_32bit;
            Value* func_corruptFloatAdr_64bit;

            // used for display and analysis
            Type* i64Ty;
            std::vector<Value*> args;
            std::map<int, Value*> byteVal;
            std::map<std::string, Value*> funcProbs;
            std::map<std::string, Value*> instProbs;
            int comment;
            int injectionType;
            std::stringstream strStream;
            unsigned int oldFaultIdx;
            unsigned int faultIdx;
            unsigned int displayIdx;
            std::vector<std::string> flist;

    };/*end class definition*/
}/*end namespace*/
            
#ifdef COMPILE_PASS
char FlipIt::DynamicFaults::ID = 0;
static RegisterPass<FlipIt::DynamicFaults> F0("FlipIt", "Dynamic Fault Injection emulating transient hardware error behavior");
#endif

#endif
