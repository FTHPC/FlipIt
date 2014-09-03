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
#include <llvm/Pass.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/IR/Function.h> 
#include <llvm/IR/Module.h> 
#include <llvm/IR/User.h> 
#include <llvm/IR/IRBuilder.h>  
#include <llvm/IR/Instructions.h> 
#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/CodeGen/MachineOperand.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/PassManager.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Assembly/PrintModulePass.h>
#include <llvm/DebugInfo.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/TypeBuilder.h>

using namespace llvm;

static cl::opt<string> func_list("funcList", cl::desc("Name(s) of the function(s) to be targeted"), cl::value_desc("func1 func2 func3"), cl::init(""), cl::ValueRequired);
static cl::opt<string> configPath("config", cl::desc("Path to the FlipIt Config file"), cl::value_desc("/path/to/FlipIt.config"), cl::init("FlipIt.config"));
static cl::opt<double> siteProb("prob", cl::desc("Probability that instrution is faulty"), cl::value_desc("Any value [0,1)"), cl::init(1e-8), cl::ValueRequired);
static cl::opt<int> byte_val("byte", cl::desc("Which byte to consider for fault injection"), cl::value_desc("-1-7"), cl::init(-1), cl::ValueRequired);
static cl::opt<int> ijo("singleInj", cl::desc("Inject Error Only Once"), cl::value_desc("0/1"), cl::init(1), cl::ValueRequired);
static cl::opt<bool> arith_err("arith", cl::desc("Inject Faults Into Arithmetic Instructions"), cl::value_desc("0/1"), cl::init(1), cl::ValueRequired);
static cl::opt<bool> ctrl_err("ctrl", cl::desc("Inject Faults Into Control Instructions"), cl::value_desc("0/1"), cl::init(1), cl::ValueRequired);
static cl::opt<bool> ptr_err("ptr", cl::desc("Inject Faults Into Pointer Instructions"), cl::value_desc("0/1"), cl::init(1), cl::ValueRequired);





/*Dynamic Fault Injection LLVM Pass*/
namespace {
    class DynamicFaults : public ModulePass {
        public:
            static char ID; 
            DynamicFaults(); 
            virtual bool runOnModule(Module &M);


		private:

            void init(unsigned int& faultIdx, unsigned int& displayIdx);
            void finalize(unsigned int& faultIdx, unsigned int& displayIdx);
			std::vector<std::string> splitAtSpace(std::string spltStr);
            int selectArgument(CallInst* callInst);
            void readConfig(string path);
            double getInstProb(Instruction* I);
            string demangle(string name);

            bool injectControl(Instruction* I, int faultIndex);
            bool injectArithmetic(Instruction* I, int faultIndex);
            bool injectPointer(Instruction* I, int faultIndex);
            bool injectCall(Instruction* I, int faultIndex);

			bool inject_Store_Data(Instruction* I, std::vector<Value*> args, CallInst* CallI);
            bool inject_Compare(Instruction* I, std::vector<Value*> args, CallInst* CallI);
            bool inject_Generic(Instruction* I, std::vector<Value*> args, CallInst* CallI,  BasicBlock* BB);

            bool inject_Store_Ptr(Instruction* I, std::vector<Value*> args, CallInst* CallI);
            bool inject_Load_Ptr(Instruction* I, std::vector<Value*> args, CallInst* CallI, BasicBlock::iterator BI, BasicBlock* BB);
            bool inject_Alloc_Ptr(Instruction* I, std::vector<Value*> args, CallInst* CallI, BasicBlock::iterator BI, BasicBlock* BB);
            bool inject_Call(Instruction* I, std::vector<Value*> args, CallInst* CallI, BasicBlock::iterator BI, BasicBlock* BB);
            bool inject_GetElementPtr_Ptr(Instruction* I, std::vector<Value*> args, CallInst* CallI, BasicBlock::iterator BI, BasicBlock* BB);

            void cacheFunctions(Module::FunctionListType &functionList);
            void enumerateSites(std::vector<Instruction*>& ilist, Function *F, unsigned& displayIdx);
            void injectFaults(std::vector<Instruction*>& ilist, unsigned& faultIdx);

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
            std::map<string, double> funcProbs;
            std::map<string, double> instProbs;
            string comment;
            string injectionType;
            std::stringstream strStream;

    };/*end class definition*/
}/*end namespace*/
char DynamicFaults::ID = 0;
static RegisterPass<DynamicFaults> F0("DynamicFaults", "Dynamic Fault Injection emulating transient hardware error behavior");
