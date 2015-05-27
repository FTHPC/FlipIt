/***********************************************************************************************/
/* This file is licensed under the University of Illinois/NCSA Open Source License.            */
/* See LICENSE.TXT for details.                                                                */
/***********************************************************************************************/


/***********************************************************************************************/
/*                                                                                             */
/* Name: faults.cpp                                                                            */
/*                                                                                             */
/* Description: LLVM IR compiler pass. This pass add calls to corrupting functions that will   */
/*              corrupt (single bit-flip) the result of LLVM instructions with a certain       */
/*              probability.                                                                   */
/*                                                                                             */
/***********************************************************************************************/

#include "./faults.h"
#include <algorithm>
#include <vector>
#include <string>

#ifdef COMPILE_PASS
FlipIt::DynamicFaults::DynamicFaults() : ModulePass(FlipIt::DynamicFaults::ID) {
#else
FlipIt::DynamicFaults::DynamicFaults() {
    funcList = "";
    configPath = "FlipIt.config";
    siteProb = 1e-8;
    byte_val = -1;
    singleInj = 1;
    arith_err = true;
    ctrl_err = true;
    ptr_err = true;
#endif
    func_corruptIntData_8bit = NULL;
    func_corruptIntData_16bit = NULL;
    func_corruptIntData_32bit = NULL;
    func_corruptIntData_64bit = NULL;
    func_corruptPtr2Int_64bit = NULL;
    func_corruptFloatData_32bit = NULL;
    func_corruptFloatData_64bit = NULL;
    func_corruptIntAdr_8bit = NULL;
    func_corruptIntAdr_16bit = NULL;
    func_corruptIntAdr_32bit = NULL;
    func_corruptIntAdr_64bit = NULL;
    func_corruptFloatAdr_32bit = NULL;
    func_corruptFloatAdr_64bit = NULL;
}
#ifdef COMPILE_PASS
FlipIt::DynamicFaults::DynamicFaults(string _funcList, string _configPath, double _siteProb, 
                            int _byte_val, int _singleInj, bool _arith_err, 
                            bool _ctrl_err, bool _ptr_err): ModulePass(ID)
#else

FlipIt::DynamicFaults::DynamicFaults(string _funcList, string _configPath, double _siteProb, 
                            int _byte_val, int _singleInj, bool _arith_err, 
                            bool _ctrl_err, bool _ptr_err)
#endif
{
    funcList = _funcList;
    configPath =  _configPath;
    siteProb = _siteProb;
    byte_val = _byte_val;
    singleInj = _singleInj;
    arith_err = _arith_err;
    ctrl_err = _ctrl_err;
    ptr_err = _ptr_err;

    func_corruptIntData_8bit = NULL;
    func_corruptIntData_16bit = NULL;
    func_corruptIntData_32bit = NULL;
    func_corruptIntData_64bit = NULL;
    func_corruptPtr2Int_64bit = NULL;
    func_corruptFloatData_32bit = NULL;
    func_corruptFloatData_64bit = NULL;
    func_corruptIntAdr_8bit = NULL;
    func_corruptIntAdr_16bit = NULL;
    func_corruptIntAdr_32bit = NULL;
    func_corruptIntAdr_64bit = NULL;
    func_corruptFloatAdr_32bit = NULL;
    func_corruptFloatAdr_64bit = NULL;
}


bool FlipIt::DynamicFaults::runOnModule(Module &M) {
    return runOnModuleCustom(M, NULL);
}

bool FlipIt::DynamicFaults::runOnModuleCustom(Module &M, std::vector<Instruction*>* selectInsts) {
    srand(time(NULL));
    if (byte_val < -1 || byte_val > 7)
        byte_val = rand() % 8;

    /* Check for assertion violation(s) */
    assert(byte_val <= 7 && byte_val >= -1);
    assert(siteProb >= 0. && siteProb < 1.);
    assert(singleInj == 1 || singleInj == 0);
    assert(ptr_err == 1 || ptr_err == 0);
    assert(arith_err == 1 || arith_err == 0);
    assert(ptr_err == 1 || ptr_err == 0);


    Module::FunctionListType &functionList = M.getFunctionList();
    vector<std::string> flist = splitAtSpace(funcList);


    init();

    /*Cache function references of the function defined in Corrupt.c to all inserting of
     *call instructions to them */
    cacheFunctions(functionList);

    /*Corrupt all instruction in vaible functions or in selectedList */
    for (Module::iterator F = functionList.begin(); F != functionList.end(); ++F) {
        /* extract the pure function name, i.e. demangle if using c++*/
        std::string cstr = demangle(F->getName().str());
        if (!viableFunction(cstr, flist))
            continue;
        if (F->begin() == F->end())
            continue; 

        inst_iterator I, E, Inext;
        if (selectInsts == NULL) {
            errs() << "\n\nFunction Name: " << cstr
                << "\n------------------------------------------------------------------------------\n";
            I = inst_begin(F);
            E = inst_end(F);
        }
        /*else {
            I = selectInsts->begin();
            E = selectInsts->end();
        } */
        for ( ; I != E;) {
        //for ( ; I != E; I++) {
            Inext = I;
            Inext++;
            Value *in = &(*I);
            if (in == NULL)
                continue;
            //errs()  << *I; 
            if ( (isa<StoreInst>(in) || isa<LoadInst>(in) || isa<BinaryOperator>(in) || isa<CmpInst>(in)
                || isa<CallInst>(in) || isa<AllocaInst>(in) || isa<GetElementPtrInst>(in)) ) {
                injectFault(&(*I));
                //    I++;
                //errs()  << "; Fault Index: " << displayIdx++;
            }
            //errs() <<'\n';
            while (I != Inext && I != E) { I++; }
        }
    }/*end for*/

    return finalize();
}

std::string FlipIt::DynamicFaults::demangle(std::string name)
{
    int status;
    std::string demangled;
    char* tmp = abi::__cxa_demangle(name.c_str(), NULL, NULL, &status);
    if (tmp == NULL)
        return name;
    
    demangled = tmp;
    free(tmp);
    /* drop the parameter list as we only need the function name */
    return demangled.find("(") == string::npos ? demangled : demangled.substr(0, demangled.find("("));
}

bool FlipIt::DynamicFaults::viableFunction(std::string func, std::vector<std::string> flist) {
   
    /* verify func isn't a flipit runtime or a user specified
    non-inject function */
     
    if (func.find("corruptIntData_8bit") != std::string::npos  ||
        func.find("corruptIntData_16bit") != std::string::npos   ||
        func.find("corruptIntData_32bit") != std::string::npos   ||
        func.find("corruptIntData_64bit") != std::string::npos   ||
        func.find("corruptPtr2Int_64bit") != std::string::npos   ||
        func.find("corruptFloatData_32bit") != std::string::npos ||
        func.find("corruptFloatData_64bit") != std::string::npos ||
        func.find("corruptIntAdr_8bit") != std::string::npos    ||
        func.find("corruptIntAdr_16bit") != std::string::npos    ||
        func.find("corruptIntAdr_32bit") != std::string::npos    ||
        func.find("corruptIntAdr_64bit") != std::string::npos    ||
        func.find("corruptFloatAdr_32bit") != std::string::npos  ||
        func.find("corruptFloatAdr_64bit") != std::string::npos  ||
        !func.compare("main"))
        return false; 

     if (funcProbs.find(func) != funcProbs.end())
        if (funcProbs[func] == 0)
            return false;
    
    if (funcList.length() == 0 || std::find(flist.begin(), flist.end(), func) != flist.end()) 
        return true;
    return false;
}
void  FlipIt::DynamicFaults::init() {
    faultIdx = 0;
    displayIdx = 0;
    ifstream infile;
    string path(getenv("HOME"));
    path += "/.FlipItState";

    infile.open(path.c_str());
    if (infile.is_open()) {
        infile >> faultIdx >> displayIdx;
        oldFaultIdx = faultIdx;
    } else {
        faultIdx = 0;
        displayIdx = 0;
    }
    infile.close();

    readConfig(configPath);
}

void FlipIt::DynamicFaults::readConfig(string path) {
    ifstream infile;
    string line;
    infile.open(path.c_str());

    // read inst probs
    getline(infile, line); // INSTRUCTIONS:
    getline(infile, line);
    while (line != "FUNCTIONS:" && infile) {
        unsigned long found = line.find("=");

        if (found != string::npos)
            instProbs[line.substr(0, found)] = atof(line.substr(found+1).c_str());
        getline(infile, line);
    }

    // read func probs
    while (infile) {
        unsigned long found = line.find("=");

        if (found != string::npos && line[0] != '#')
            funcProbs[line.substr(0, found)] = atof(line.substr(found+1).c_str());
        getline(infile, line);
    }
    infile.close();
}


bool  FlipIt::DynamicFaults::finalize() {
    ofstream outfile;
    string path(getenv("HOME"));
    path += "/.FlipItState";

    outfile.open(path.c_str() , ios::trunc);
    if (!outfile.is_open()) {
        errs() << "WARNING: unable to update injector state file at path: " << path << "\n";
    } else {
        outfile << faultIdx << " " << displayIdx;
    }
    outfile.close();

    return oldFaultIdx != faultIdx;
}

vector<string> FlipIt::DynamicFaults::splitAtSpace(string spltStr) {
    std::vector<std::string> strLst;
    std::istringstream isstr(spltStr);
    copy(std::istream_iterator<std::string>(isstr), std::istream_iterator<std::string>(),
	   std::back_inserter<std::vector<std::string> >(strLst));

    return strLst;
}

double FlipIt::DynamicFaults::getInstProb(Instruction* I) {
    /*First check if it is a call to a function listed in the config file*/
    if (CallInst *callInst = dyn_cast<CallInst>(I)) {
        if (callInst->getCalledFunction() == NULL) /* function pointers will be null */
            return 0;

        string funcName = callInst->getCalledFunction()->getName().str();
        if (funcProbs.find(funcName) != funcProbs.end())
            return funcProbs[funcName];
    }

    /* Get the probability from the instruction's type from the config
    file or the default probabilty given as a command line argument */
    string type = I->getOpcodeName();
    return instProbs.find(type) != instProbs.end() ? instProbs[type] : siteProb;
}

bool FlipIt::DynamicFaults::injectControl(Instruction* I) {
    if (I == NULL)
        return false;

    /* Locate the instruction I in the basic block BB */
    BasicBlock *BB = I->getParent();
    BasicBlock::iterator BI;
    for (BI = BB->begin(); BI != BB->end(); BI++)
        if (BI == *I)
            break;

    /* Build argument list before calling Corrupt function */
    CallInst* CallI = NULL;
    std::vector<Value*> args;
    args.push_back(ConstantInt::get(IntegerType::getInt32Ty(getGlobalContext()), faultIdx));
    args.push_back(ConstantInt::get(IntegerType::getInt32Ty(getGlobalContext()), singleInj));
    args.push_back(ConstantFP::get(Type::getDoubleTy(getGlobalContext()), getInstProb(I)));
    args.push_back(ConstantInt::get(IntegerType::getInt32Ty(getGlobalContext()), byte_val));


    /* Choose a fault site in CmpInst and insert Corrupt function call */
    if (isa<CmpInst>(I))
    {
        injectionType = "Control-Branch";
        return inject_Compare(I, args, CallI);
    }

    /* check to see if instruction modifies a looping variable such as i++
        if so we need to inject into it and mark the injection type 'control' */
    if (!isa<CmpInst>(I) && !isa<StoreInst>(I))
        if (I->getName().str().find("indvars") == 0
            || I->getName().str().substr(0, 3) == "inc")
        {
            injectionType = "Control-Loop";
            return inject_Generic(I, args, CallI, BB);
        }
    return false;
}


bool FlipIt::DynamicFaults::injectArithmetic(Instruction* I) {
    if (I == NULL)
        return false;

    /* Locate the instruction I in the basic block BB */
    BasicBlock *BB = I->getParent();
    BasicBlock::iterator BI;
    for (BI = BB->begin(); BI != BB->end(); BI++)
        if (BI == *I)
            break;

    /* Build argument list before calling Corrupt function */
    CallInst* CallI = NULL;
    std::vector<Value*> args;
    args.push_back(ConstantInt::get(IntegerType::getInt32Ty(getGlobalContext()), faultIdx));
    args.push_back(ConstantInt::get(IntegerType::getInt32Ty(getGlobalContext()), singleInj));
    args.push_back(ConstantFP::get(Type::getDoubleTy(getGlobalContext()), getInstProb(I)));
    args.push_back(ConstantInt::get(IntegerType::getInt32Ty(getGlobalContext()), byte_val));


    if (isa<CallInst>(I))
        return false;
    /* store instruction required differnt injection logic than binary operators
    *   (add mul div) and loads */
    if (isa<StoreInst>(I))
        return inject_Store_Data(I, args, CallI);

    if (!isa<CmpInst>(I) && !isa<StoreInst>(I)) {
        return inject_Generic(I, args, CallI, BB);
    }
    return false;
}


bool FlipIt::DynamicFaults::injectPointer(Instruction* I) {
    if (I == NULL)
        return false;

    CallInst* CallI = NULL;
    /*Build argument list before calling Corrupt function*/
    std::vector<Value*> args;
    args.push_back(ConstantInt::get(IntegerType::getInt32Ty(getGlobalContext()), faultIdx));
    args.push_back(ConstantInt::get(IntegerType::getInt32Ty(getGlobalContext()), singleInj));
    args.push_back(ConstantFP::get(Type::getDoubleTy(getGlobalContext()), getInstProb(I)));
    args.push_back(ConstantInt::get(IntegerType::getInt32Ty(getGlobalContext()), byte_val));

    /*Locate the instruction I in the basic block BB*/
    BasicBlock *BB = I->getParent();
    BasicBlock::iterator BI, BINext;
    for (BI = BB->begin(); BI != BB->end(); BI++)
        if (BI == *I)
            break;

    if (isa<StoreInst>(I))
        return inject_Store_Ptr(I, args, CallI);

    if (isa<LoadInst>(I))
        return inject_Load_Ptr(I, args, CallI, BI, BB);

    if (isa<AllocaInst>(I))
       return inject_Alloc_Ptr(I, args, CallI, BI, BB);

    if (isa<GetElementPtrInst>(I))
        return inject_GetElementPtr_Ptr(I, args, CallI, BI, BB);

    return false;
}

bool FlipIt::DynamicFaults::injectCall(Instruction* I) {
    if (I == NULL)
        return false;

    CallInst* CallI = NULL;
    /*Build argument list before calling Corrupt function*/
    std::vector<Value*> args;
    args.push_back(ConstantInt::get(IntegerType::getInt32Ty(getGlobalContext()), faultIdx));
    args.push_back(ConstantInt::get(IntegerType::getInt32Ty(getGlobalContext()), singleInj));
    args.push_back(ConstantFP::get(Type::getDoubleTy(getGlobalContext()), getInstProb(I)));
    args.push_back(ConstantInt::get(IntegerType::getInt32Ty(getGlobalContext()), byte_val));

    /*Locate the instruction I in the basic block BB*/
    BasicBlock *BB = I->getParent();
    BasicBlock::iterator BI, BINext;
    for (BI = BB->begin(); BI != BB->end(); BI++)
        if (BI == *I)
            break;

    if (isa<CallInst>(I))
        return inject_Call(I, args, CallI, BI, BB);


    return false;
}

bool FlipIt::DynamicFaults::inject_Store_Data(Instruction* I, std::vector<Value*> args, CallInst* CallI) {
    args.push_back(I->getOperand(0)); // value stored

    /*Integer Data*/
    if (I->getOperand(0)->getType()->isIntegerTy(8)) {
        CallI = CallInst::Create(func_corruptIntData_8bit, args, "call_corruptIntData_8bit", I);
        assert(CallI);
        CallI->setCallingConv(CallingConv::C);
    } else if (I->getOperand(0)->getType()->isIntegerTy(16)) {
        CallI = CallInst::Create(func_corruptIntData_16bit, args, "call_corruptIntData_16bit", I);
        assert(CallI);
        CallI->setCallingConv(CallingConv::C);
    } else if (I->getOperand(0)->getType()->isIntegerTy(32)) {
        CallI = CallInst::Create(func_corruptIntData_32bit, args, "call_corruptIntData_32bit", I);
        assert(CallI);
        CallI->setCallingConv(CallingConv::C);
    } else if (I->getOperand(0)->getType()->isIntegerTy(64)) {
        CallI = CallInst::Create(func_corruptIntData_64bit, args, "call_corruptIntData_64bit", I);
        assert(CallI);
        CallI->setCallingConv(CallingConv::C);
    } else if (I->getOperand(0)->getType()->isFloatTy()) {
    /*Float Data*/
        CallI = CallInst::Create(func_corruptFloatData_32bit, args,
                                 "call_corruptFloatData_32bit", I);
        assert(CallI);
        CallI->setCallingConv(CallingConv::C);
    } else if (I->getOperand(0)->getType()->isDoubleTy()) {
      CallI = CallInst::Create(func_corruptFloatData_64bit, args,
                               "call_corruptFloatData_64bit", I);
        assert(CallI);
        CallI->setCallingConv(CallingConv::C);
    } else {
        return false;
    }
    if (CallI) {
        Value* corruptVal = &(*CallI);
        I->setOperand(0, corruptVal);
        comment = "Value";
        skipAmount = 0;
    }
    return true;
}

bool FlipIt::DynamicFaults::inject_Compare(Instruction* I, std::vector<Value*> args, CallInst* CallI) {
    /* select a random arg to corrupt because corrupting the result will yeild a
    50% chance of branching incorrectly */
    unsigned int opPos = rand() % 2;
    PtrToIntInst* p2iI = NULL;
    IntToPtrInst* i2pI = NULL;
    Value* corruptVal = NULL;
    Type* i64Ty = Type::getInt64Ty(I->getContext());

    /* LLVM doesn't like attempting to corrupt NULL */
    if (I->getOperand(opPos) == NULL)
    	opPos = (opPos+1) % 2;

    args.push_back(I->getOperand(opPos));


    /*integer data*/
    if (I->getOperand(opPos)->getType()->isIntegerTy(8)) {
        CallI = CallInst::Create(func_corruptIntData_8bit, args, "call_corruptIntData_8bit", I);
        assert(CallI);
        CallI->setCallingConv(CallingConv::C);
    } else if (I->getOperand(opPos)->getType()->isIntegerTy(16)) {
        CallI = CallInst::Create(func_corruptIntData_16bit, args, "call_corruptIntData_16bit", I);
        assert(CallI);
        CallI->setCallingConv(CallingConv::C);
    } else if (I->getOperand(opPos)->getType()->isIntegerTy(32)) {
        CallI = CallInst::Create(func_corruptIntData_32bit, args, "call_corruptIntData_32bit", I);
        assert(CallI);
        CallI->setCallingConv(CallingConv::C);
    } else if (I->getOperand(opPos)->getType()->isIntegerTy(64)) {
        CallI = CallInst::Create(func_corruptIntData_64bit, args, "call_corruptIntData_64bit", I);
        assert(CallI);
        CallI->setCallingConv(CallingConv::C);
    } else if (I->getOperand(opPos)->getType()->isFloatTy()) {
    /*Float Data*/
        CallI = CallInst::Create(func_corruptFloatData_32bit, args,
                                 "call_corruptFloatData_32bit", I);
        assert(CallI);
        CallI->setCallingConv(CallingConv::C);
    } else if (I->getOperand(opPos)->getType()->isDoubleTy()) {
        CallI = CallInst::Create(func_corruptFloatData_64bit, args,
                                 "call_corruptFloatData_64bit", I);
        assert(CallI);
        CallI->setCallingConv(CallingConv::C);
    } else {
       /* We are comparing non-scalar data a.k.a pointers. Let's convert the pointer to a 64-bit
        * integer corrupt that and cast back to the pointer type. */
        /* Value to corrupt is no longer correct */
        args.pop_back();

        /* Convert ptr to int64 */
        p2iI = new PtrToIntInst(I->getOperand(opPos), i64Ty, "convert_ptr2i64", I);
        assert(p2iI);

        /* Corrupt */
        args.push_back(p2iI);
        CallI = CallInst::Create(func_corruptPtr2Int_64bit, args, "call_corruptPtr2Int_64bit", I);
        assert(CallI);
        CallI->setCallingConv(CallingConv::C);
        corruptVal = &(*CallI);

        /* convert int64 to ptr */
        i2pI = new IntToPtrInst(corruptVal, I->getOperand(opPos)->getType(), "convert_i642ptr", I);
        assert(i2pI);
    }

    /* make sure everyone is using corrupt value */
    if (CallI) {
        if (i2pI == NULL)
            corruptVal = &(*CallI);
        else
            corruptVal = &(*i2pI);
        I->setOperand(opPos, corruptVal);

        strStream << "Operand # " << opPos;
        comment = strStream.str();
        strStream.str("");
        skipAmount = 0;
    }
    return true;
}


bool FlipIt::DynamicFaults::inject_Generic(Instruction* I, std::vector<Value*> args, CallInst* CallI,
                                   BasicBlock* BB) {
    Instruction* INext = NULL;
    BasicBlock::iterator BI = *I;
    args.push_back(&(*I));
    /* Corrupt result of instruction I */
    if (BI == BB->end()) {

        /*Integer Data*/
        if (I->getType()->isIntegerTy(8)) {
            CallI = CallInst::Create(func_corruptIntData_8bit, args,
                                     "call_corruptIntData_8bit", BB);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
        } else if (I->getType()->isIntegerTy(16)) {
            CallI = CallInst::Create(func_corruptIntData_16bit, args,
                                     "call_corruptIntData_16bit", BB);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
        } else if (I->getType()->isIntegerTy(32)) {
            CallI = CallInst::Create(func_corruptIntData_32bit, args,
                                     "call_corruptIntData_32bit", BB);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
        } else if (I->getType()->isIntegerTy(64)) {
            CallI = CallInst::Create(func_corruptIntData_64bit, args,
                                     "call_corruptIntData_64bit", BB);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
        } else if (I->getType()->isFloatTy()) {
        /*Float Data*/
            CallI = CallInst::Create(func_corruptFloatData_32bit, args,
                                     "call_corruptFloatData_32bit", BB);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
        } else if (I->getType()->isDoubleTy()) {
            CallI = CallInst::Create(func_corruptFloatData_64bit, args,
                                     "call_corruptFloatData_64bit", BB);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
        } else {
            return false;
        }
    } else {
        BasicBlock::iterator BINext = BI;
        BINext++;
        INext = &*BINext;
        assert(INext);

        /*Integer Data*/
        if (I->getType()->isIntegerTy(8)) {
            CallI = CallInst::Create(func_corruptIntData_8bit, args,
                                     "call_corruptIntData_8bit", INext);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
        } else if (I->getType()->isIntegerTy(16)) {
            CallI = CallInst::Create(func_corruptIntData_16bit, args,
                                      "call_corruptIntData_16bit", INext);
         assert(CallI);
         CallI->setCallingConv(CallingConv::C);
        } else if (I->getType()->isIntegerTy(32)) {
            CallI = CallInst::Create(func_corruptIntData_32bit, args,
                                   "call_corruptIntData_32bit", INext);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
        } else if (I->getType()->isIntegerTy(64)) {
            CallI = CallInst::Create(func_corruptIntData_64bit, args,
                                   "call_corruptIntData_64bit", INext);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
        } else if (I->getType()->isFloatTy()) {
        /*Float Data*/
            CallI = CallInst::Create(func_corruptFloatData_32bit, args,
                                     "call_corruptFloatData_32bit", INext);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
        } else if (I->getType()->isDoubleTy()) {
            CallI = CallInst::Create(func_corruptFloatData_64bit, args,
                                     "call_corruptFloatData_64bit", INext);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
        } else {
            return false;
        }
    }

    if (CallI) {
        Value* corruptVal = &(*CallI);
        I->replaceAllUsesWith(corruptVal);

        /* Because of the preceeding method invocation, we messed up last argument in the call instruction.
            We need to manually set this value to the result of Insturction I */
        BasicBlock::iterator BINext = BI;
        BINext++;
        INext = &*BINext;
        INext->setOperand(4, I); // hard coded. If like others, it says we have an extra argument
        comment = "Result";
        skipAmount = 1;
    }
    return true;
}


bool FlipIt::DynamicFaults::inject_Store_Ptr(Instruction* I, std::vector<Value*> args, CallInst* CallI) {

    PtrToIntInst* p2iI = NULL;
    IntToPtrInst* i2pI = NULL;
    Value* corruptVal = NULL;
    Type* i64Ty = Type::getInt64Ty(I->getContext());
    /* Make sure we corrupt an pointer. First attempt to corupt the value being stored,
    * but if that isn't a pointer let's inject into the address.*/
    int opNum = 0;
    comment  = "Value";
    if (!I->getType()->isPointerTy()) {
        comment = "Address";
        opNum = 1;
    }

    /*Corrupt operand*/
    args.push_back(I->getOperand(opNum));

    if (I->getOperand(opNum)->getType()->isIntegerTy(8)) {
        CallI = CallInst::Create(func_corruptIntAdr_8bit, args, "call_corruptIntAdr_8bit", I);
        assert(CallI);
        CallI->setCallingConv(CallingConv::C);
    } else if (I->getOperand(opNum)->getType()->isIntegerTy(16)) {
        CallI = CallInst::Create(func_corruptIntAdr_16bit, args, "call_corruptIntAdr_16bit", I);
        assert(CallI);
        CallI->setCallingConv(CallingConv::C);
    } else if (I->getOperand(opNum)->getType()->isIntegerTy(32)) {
        CallI = CallInst::Create(func_corruptIntAdr_32bit, args, "call_corruptIntAdr_32bit", I);
        assert(CallI);
        CallI->setCallingConv(CallingConv::C);
    } else if (I->getOperand(opNum)->getType()->isIntegerTy(64)) {
        CallI = CallInst::Create(func_corruptIntAdr_64bit, args, "call_corruptIntAdr_64bit", I);
        assert(CallI);
        CallI->setCallingConv(CallingConv::C);
    } else if (I->getOperand(opNum)->getType()->isFloatTy()) {
        CallI = CallInst::Create(func_corruptFloatAdr_32bit, args, "call_corruptFloatAdr_32bit", I);
        assert(CallI);
        CallI->setCallingConv(CallingConv::C);
    } else if (I->getOperand(opNum)->getType()->isDoubleTy()){
        CallI = CallInst::Create(func_corruptFloatAdr_64bit, args, "call_corruptFloatAdr_64bit", I);
        assert(CallI);
        CallI->setCallingConv(CallingConv::C);
    } else {
        args.pop_back();

        /* Convert ptr to int64 */
        p2iI = new PtrToIntInst(I->getOperand(opNum), i64Ty, "convert_ptr2i64", I);
        assert(p2iI);

        /* Corrupt */
        args.push_back(p2iI);
        CallI = CallInst::Create(func_corruptPtr2Int_64bit, args, "call_corruptPtr2Int_64bit", I);
        assert(CallI);
        CallI->setCallingConv(CallingConv::C);
        corruptVal = &(*CallI);

        /* convert int64 to ptr */
        i2pI = new IntToPtrInst(corruptVal, I->getOperand(opNum)->getType(), "convert_i642ptr", I);
        assert(p2iI);

    }

    /* make sure everyone is using corrupt value */
    if (CallI) {
        if (i2pI == NULL)
            corruptVal = &(*CallI);
        else
            corruptVal = &(*i2pI);
        I->setOperand(opNum, corruptVal);

        strStream << "Operand # " << opNum;
        comment = strStream.str();
        strStream.str("");
    }
    return true;
}

bool FlipIt::DynamicFaults::inject_Load_Ptr(Instruction* I, std::vector<Value*> args, CallInst* CallI, BasicBlock::iterator BI,  BasicBlock* BB) {

    Instruction* INext;
    PtrToIntInst* p2iI = NULL;
    IntToPtrInst* i2pI = NULL;
    Value* corruptVal = NULL;
    Type* i64Ty = Type::getInt64Ty(I->getContext());

    /* Make sure we corrupt an pointer. First attempt to corupt the value being loaded,
    * but if that isn't a pointer let's inject into the address */
    Value* ptr =  &(*I);
    comment  = "Value";
    if (!I->getType()->isPointerTy()) {
        ptr = dyn_cast<LoadInst>(I)->getPointerOperand();
        comment = "Address";
    }
    args.push_back(ptr);


    if (ptr->getType()->getPointerElementType()->isVectorTy() || ptr->getType()->getPointerElementType()->isIntegerTy(1))
        return false;

    /* Depending on if we are injecting into the result of an operand we need to handle things differenlty */
    if (comment == "Value") {
        if (BI == BB->end()) {
            if (ptr->getType()->isIntegerTy(8)) {
                CallI = CallInst::Create(func_corruptIntAdr_8bit, args, "call_corruptIntAdr_8bit", BB);
                assert(CallI);
                CallI->setCallingConv(CallingConv::C);
            } else if (ptr->getType()->isIntegerTy(16)) {
                CallI = CallInst::Create(func_corruptIntAdr_16bit, args, "call_corruptIntAdr_16bit", BB);
                assert(CallI);
                CallI->setCallingConv(CallingConv::C);
            } else if (ptr->getType()->isIntegerTy(32)) {
                CallI = CallInst::Create(func_corruptIntAdr_32bit, args, "call_corruptIntAdr_32bit", BB);
                assert(CallI);
                CallI->setCallingConv(CallingConv::C);
            } else if (ptr->getType()->isIntegerTy(64)) {
                CallI = CallInst::Create(func_corruptIntAdr_64bit, args, "call_corruptIntAdr_64bit", BB);
                assert(CallI);
                CallI->setCallingConv(CallingConv::C);
            } else if (ptr->getType()->isFloatTy()) {
                CallI = CallInst::Create(func_corruptFloatAdr_32bit, args, "call_corruptFloatAdr_32bit", BB);
                assert(CallI);
                CallI->setCallingConv(CallingConv::C);
            } else if (ptr->getType()->isDoubleTy()) {
                CallI = CallInst::Create(func_corruptFloatAdr_64bit, args, "call_corruptFloatAdr_64bit", BB);
                assert(CallI);
                CallI->setCallingConv(CallingConv::C);
            } else {
                args.pop_back();
                /* Convert ptr to int64 */
                p2iI = new PtrToIntInst(ptr, i64Ty, "convert_ptr2i64", BB);
                assert(p2iI);

                /* Corrupt */
                args.push_back(p2iI);
                CallI = CallInst::Create(func_corruptPtr2Int_64bit, args, "call_corruptPtr2Int_64bit", BB);
                assert(CallI);
                CallI->setCallingConv(CallingConv::C);
                corruptVal = &(*CallI);

                /* convert int64 to ptr */
                i2pI = new IntToPtrInst(corruptVal, ptr->getType(), "convert_i642ptr", BB);
                assert(i2pI);
            }
        } else {
            BasicBlock::iterator BINext = BI;
            BINext++;
            INext = &*BINext;
            assert(INext);

            if (ptr->getType()->isIntegerTy(8)) {
                CallI = CallInst::Create(func_corruptIntAdr_8bit, args, "call_corruptIntAdr_8bit", INext);
                assert(CallI);
                CallI->setCallingConv(CallingConv::C);
            } else if (ptr->getType()->isIntegerTy(16)) {
                CallI = CallInst::Create(func_corruptIntAdr_16bit, args, "call_corruptIntAdr_16bit", INext);
                assert(CallI);
                CallI->setCallingConv(CallingConv::C);
            } else if (ptr->getType()->isIntegerTy(32)) {
                CallI = CallInst::Create(func_corruptIntAdr_32bit, args, "call_corruptIntAdr_32bit", INext);
                assert(CallI);
                CallI->setCallingConv(CallingConv::C);
            } else if (ptr->getType()->isIntegerTy(64)) {
                CallI = CallInst::Create(func_corruptIntAdr_64bit, args, "call_corruptIntAdr_64bit", INext);
                assert(CallI);
                CallI->setCallingConv(CallingConv::C);
            } else if (ptr->getType()->isFloatTy()) {
                CallI = CallInst::Create(func_corruptFloatAdr_32bit, args, "call_corruptFloatAdr_32bit", INext);
                assert(CallI);
                CallI->setCallingConv(CallingConv::C);
            } else if (ptr->getType()->isDoubleTy()) {
                CallI = CallInst::Create(func_corruptFloatAdr_64bit, args, "call_corruptFloatAdr_64bit", INext);
                assert(CallI);
                CallI->setCallingConv(CallingConv::C);
            } else {
                args.pop_back();
                /* Convert ptr to int64 */
                p2iI = new PtrToIntInst(ptr, i64Ty, "convert_ptr2i64", INext);
                assert(p2iI);

                /* Corrupt */
                args.push_back(p2iI);
                CallI = CallInst::Create(func_corruptPtr2Int_64bit, args, "call_corruptPtr2Int_64bit", INext);
                assert(CallI);
                CallI->setCallingConv(CallingConv::C);
                corruptVal = &(*CallI);

                /* convert int64 to ptr */
                i2pI = new IntToPtrInst(corruptVal, ptr->getType(), "convert_i642ptr", INext);
                assert(i2pI);
            }
        }
    } else {
      /* if (comment == "Address") */
        if (ptr->getType()->isIntegerTy(8)) {
            CallI = CallInst::Create(func_corruptIntAdr_8bit, args, "call_corruptIntAdr_8bit", I);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
        } else if (ptr->getType()->isIntegerTy(16)) {
            CallI = CallInst::Create(func_corruptIntAdr_16bit, args, "call_corruptIntAdr_16bit", I);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
        } else if (ptr->getType()->isIntegerTy(32)) {
            CallI = CallInst::Create(func_corruptIntAdr_32bit, args, "call_corruptIntAdr_32bit", I);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
        } else if (ptr->getType()->isIntegerTy(64)) {
            CallI = CallInst::Create(func_corruptIntAdr_64bit, args, "call_corruptIntAdr_64bit", I);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
        } else if (ptr->getType()->isFloatTy()) {
            CallI = CallInst::Create(func_corruptFloatAdr_32bit, args, "call_corruptFloatAdr_32bit", I);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
        } else if (ptr->getType()->isDoubleTy()) {
            CallI = CallInst::Create(func_corruptFloatAdr_64bit, args, "call_corruptFloatAdr_64bit", I);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
        } else {
            args.pop_back();

            BasicBlock::iterator BINext = BI;
            BINext++;
            Instruction* INext = &*BINext;

            assert(INext);

            /* Convert ptr to int64 */
            p2iI = new PtrToIntInst(ptr, i64Ty, "convert_ptr2i64", I);
            assert(p2iI);

            /* Corrupt */
            args.push_back(p2iI);
            CallI = CallInst::Create(func_corruptPtr2Int_64bit, args, "call_corruptPtr2Int_64bit", I);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
            corruptVal = &(*CallI);

            /* convert int64 to ptr */
            i2pI = new IntToPtrInst(corruptVal, ptr->getType(), "convert_i642ptr", I);
            assert(p2iI);
        }
    }
    if (CallI) {

        if (comment == "Value") {

            if (i2pI == NULL)
                corruptVal = &(*CallI);
            else
                corruptVal = &(*i2pI);
            I->replaceAllUsesWith(corruptVal);

            /* Because of the preceeding method invocation, we messed up last argument in the call instruction.
                We need to manually set this value to the result of Insturction I */
            BasicBlock::iterator BINext = BI;
            BINext++;
            INext = &*BINext;
            INext->setOperand(/*4*/INext->getNumOperands() - 1, I);
        } else {
            /* if (comment == "Address") */
            if (i2pI == NULL)
                corruptVal = &(*CallI);
            else
                corruptVal = &(*i2pI);
            I->setOperand(0, corruptVal);

        }
    }
    return true;
}

bool FlipIt::DynamicFaults::inject_Alloc_Ptr(Instruction* I, std::vector<Value*> args, CallInst* CallI, BasicBlock::iterator BI,  BasicBlock* BB) {

    Instruction* INext = NULL;
    PtrToIntInst* p2iI = NULL;
    IntToPtrInst* i2pI = NULL;
    Value* corruptVal = NULL;
    Type* i64Ty = Type::getInt64Ty(I->getContext());

    /* we are corrupting the pointer returned from the allocation */
    args.push_back(I);
    if (BI == BB->end()) {
        if (I->getType()->isIntegerTy(8)) {
            CallI = CallInst::Create(func_corruptIntAdr_8bit, args, "call_corruptIntAdr_8bit", BB);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
        } else if (I->getType()->isIntegerTy(16)) {
            CallI = CallInst::Create(func_corruptIntAdr_16bit, args, "call_corruptIntAdr_16bit", BB);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
        } else if (I->getType()->isIntegerTy(32)) {
            CallI = CallInst::Create(func_corruptIntAdr_32bit, args, "call_corruptIntAdr_32bit", BB);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
        } else if (I->getType()->isIntegerTy(64)) {
            CallI = CallInst::Create(func_corruptIntAdr_64bit, args, "call_corruptIntAdr_64bit", BB);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
        } else if (I->getType()->isFloatTy()) {
            CallI = CallInst::Create(func_corruptFloatAdr_32bit, args, "call_corruptFloatAdr_32bit", BB);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
        } else if (I->getType()->isDoubleTy()) {
            CallI = CallInst::Create(func_corruptFloatAdr_64bit, args, "call_corruptFloatAdr_64bit", BB);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
        } else {
            args.pop_back();
            /* Convert ptr to int64 */
            p2iI = new PtrToIntInst(I, i64Ty, "convert_ptr2i64", BB);
            assert(p2iI);

            /* Corrupt */
            args.push_back(p2iI);
            CallI = CallInst::Create(func_corruptPtr2Int_64bit, args, "call_corruptPtr2Int_64bit", BB);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
            corruptVal = &(*CallI);

            /* convert int64 to ptr */
            i2pI = new IntToPtrInst(corruptVal, I->getType(), "convert_i642ptr", BB);
            assert(i2pI);
        }
    } else {
        BasicBlock::iterator BINext = BI;
        BINext++;
        INext = &*BINext;
        assert(INext);

        if (I->getType()->isIntegerTy(8)) {
            CallI = CallInst::Create(func_corruptIntAdr_8bit, args, "call_corruptIntAdr_8bit", INext);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
        } else if (I->getType()->isIntegerTy(16)) {
            CallI = CallInst::Create(func_corruptIntAdr_16bit, args, "call_corruptIntAdr_16bit", INext);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
        } else if (I->getType()->isIntegerTy(32)) {
            CallI = CallInst::Create(func_corruptIntAdr_32bit, args, "call_corruptIntAdr_32bit", INext);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
        } else if (I->getType()->isIntegerTy(64)) {
            CallI = CallInst::Create(func_corruptIntAdr_64bit, args, "call_corruptIntAdr_64bit", INext);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
        } else if (I->getType()->isFloatTy()) {
            CallI = CallInst::Create(func_corruptFloatAdr_32bit, args, "call_corruptFloatAdr_32bit", INext);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
        } else if (I->getType()->isDoubleTy()) {
            CallI = CallInst::Create(func_corruptFloatAdr_64bit, args, "call_corruptFloatAdr_64bit", INext);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
        } else {
            args.pop_back();
            /* Convert ptr to int64 */
            p2iI = new PtrToIntInst(I, i64Ty, "convert_ptr2i64", INext);
            assert(p2iI);

            /* Corrupt */
            args.push_back(p2iI);
            CallI = CallInst::Create(func_corruptPtr2Int_64bit, args, "call_corruptPtr2Int_64bit", INext);
            assert(CallI);
            CallI->setCallingConv(CallingConv::C);
            corruptVal = &(*CallI);

            /* convert int64 to ptr */
            i2pI = new IntToPtrInst(corruptVal, I->getType(), "convert_i642ptr", INext);
            assert(i2pI);
        }
    }

    /* make sure everyone is using corrupt value */
    if (CallI) {

        if (i2pI == NULL)
            corruptVal = &(*CallI);
        else
            corruptVal = &(*i2pI);
        I->replaceAllUsesWith(corruptVal);

        /* Because of the preceeding method invocation, we messed up last argument in the call instruction.
            We need to manually set this value to the result of Insturction I */
        BasicBlock::iterator BINext = BI;
        BINext++;
        INext = &*BINext;
        INext->setOperand(/*4*/INext->getNumOperands() - 1, I);
        comment = "Result";
    }

    return true;
}

int FlipIt::DynamicFaults::selectArgument(CallInst* callInst) {
    int arg = -1;
    int possArgLen = callInst->getNumArgOperands();
    std::vector<int> argPos;
    if ( callInst->getCalledFunction() == NULL)
        return arg;

    bool argFound = false;
    string funcName = callInst->getCalledFunction()->getName().str();
    if (funcProbs.find(funcName) != funcProbs.end())
        if (funcProbs[funcName] == 0)
            return arg;

    // populate with possible args.
    if (funcName.find("llvm.lifetime") != string::npos) {
        argPos.push_back(1);
    } else if (funcName.find("llvm.dbg") != string::npos) {
        return arg;
    } else if (funcName.find("toggleInjector") != string::npos) {
        return arg;
    } else {
        /* populate with valid arguemnts detection of constant integers should
        fix the LLVM intrinsic problem */
        for (int i = 0; i < possArgLen; i++) {
            ConstantInt* CI = dyn_cast<llvm::ConstantInt>(callInst->getArgOperand(i));
            if (!( CI != NULL && funcName.find("llvm.") != string::npos) )
                argPos.push_back(i);
        }
    }


    /* select possible arg based on "arith", "ctrl", and "ptr" */
    while (argFound == false && argPos.size() > 0)  {
        int a = rand() % argPos.size();
        if (ctrl_err && callInst->getArgOperand(a)->getType()->isIntegerTy()) {
            Value* v = (Value*) callInst->getArgOperand(a);
            if ( v->getName().str().find("indvars") == 0
                || v->getName().str().substr(0, 3) == "inc") {
                arg = a;
                injectionType = "Control-Loop";
                argFound = true;
            }
        } else if (arith_err && (callInst->getArgOperand(a)->getType()->isIntegerTy()
                || callInst->getArgOperand(a)->getType()->isFloatTy()
                || callInst->getArgOperand(a)->getType()->isDoubleTy() )
                && !callInst->getArgOperand(a)->getType()->isIntegerTy(1)) {
            arg = a;
            injectionType = "Arithmetic";
            argFound = true;
        } else if (ptr_err) {
            arg = a;
            injectionType = "Pointer";
            argFound = true;
        }
        argPos.erase(argPos.begin() + a);
    }

    return arg;
}

bool FlipIt::DynamicFaults::inject_Call(Instruction* I, std::vector<Value*> args, CallInst* CallI,
                                BasicBlock::iterator BI,  BasicBlock* BB) {
    PtrToIntInst* p2iI = NULL;
    IntToPtrInst* i2pI = NULL;
    Value* corruptVal = NULL;
    Type* i64Ty = Type::getInt64Ty(I->getContext());
    if (dyn_cast<CallInst>(I)->getNumArgOperands() == 0)
        return false;

    int opNum = selectArgument(dyn_cast<CallInst>(I));
    if (opNum == -1)
        return false;

    args.push_back(I->getOperand(opNum));

    /* corrupting scalar values */
    if (I->getOperand(opNum)->getType()->isIntegerTy(8)) {
        CallI = CallInst::Create(func_corruptIntData_8bit, args, "call_corruptIntData_8bit", I);
        assert(CallI);
        CallI->setCallingConv(CallingConv::C);
    } else if (I->getOperand(opNum)->getType()->isIntegerTy(16)) {
        CallI = CallInst::Create(func_corruptIntData_16bit, args, "call_corruptIntData_16bit", I);
        assert(CallI);
        CallI->setCallingConv(CallingConv::C);
    } else if (I->getOperand(opNum)->getType()->isIntegerTy(32)) {
        CallI = CallInst::Create(func_corruptIntData_32bit, args, "call_corruptIntData_32bit", I);
        assert(CallI);
        CallI->setCallingConv(CallingConv::C);
    } else if (I->getOperand(opNum)->getType()->isIntegerTy(64)) {
        CallI = CallInst::Create(func_corruptIntData_64bit, args, "call_corruptIntData_64bit", I);
        assert(CallI);
        CallI->setCallingConv(CallingConv::C);
    } else if (I->getOperand(opNum)->getType()->isFloatTy()) {
        CallI = CallInst::Create(func_corruptFloatData_32bit, args,
                                 "call_corruptFloatData_32bit", I);
        assert(CallI);
        CallI->setCallingConv(CallingConv::C);
    } else if (I->getOperand(opNum)->getType()->isDoubleTy()) {
        CallI = CallInst::Create(func_corruptFloatData_64bit, args,
                                 "call_corruptFloatData_64bit", I);
        assert(CallI);
        CallI->setCallingConv(CallingConv::C);
    } else {
        args.pop_back();

        /* Convert ptr to int64 */
        p2iI = new PtrToIntInst(I->getOperand(opNum), i64Ty, "convert_ptr2i64", I);
        assert(p2iI);

        /* Corrupt */
        args.push_back(p2iI);
        CallI = CallInst::Create(func_corruptPtr2Int_64bit, args, "call_corruptPtr2Int_64bit", I);
        assert(CallI);
        CallI->setCallingConv(CallingConv::C);
        corruptVal = &(*CallI);

        /* convert int64 to ptr */
        i2pI = new IntToPtrInst(corruptVal, I->getOperand(opNum)->getType(), "convert_i642ptr", I);
        assert(p2iI);
    }

    /* make sure everyone is using corrupt value */
    if (CallI != NULL) {
        if (i2pI == NULL)
            corruptVal = &(*CallI);
        else
            corruptVal = &(*i2pI);
        BI->setOperand(opNum, corruptVal);

        strStream << "Arg # " << opNum;
        comment = strStream.str();
        strStream.str("");
    }
    return true;
}

bool FlipIt::DynamicFaults::inject_GetElementPtr_Ptr(Instruction* I, std::vector<Value*> args,
                                             CallInst* CallI, BasicBlock::iterator BI,
                                             BasicBlock* BB) {
    Instruction* INext = NULL;
    PtrToIntInst* p2iI = NULL;
    IntToPtrInst* i2pI = NULL;
    Value* corruptVal = NULL;
    BI = *I;
    Type* i64Ty = Type::getInt64Ty(I->getContext());

    if (BI == BB->end()) {
        /* Convert ptr to int64 */
        p2iI = new PtrToIntInst(I, i64Ty, "convert_ptr2i64", BB);
        assert(p2iI);

        /* Corrupt */
        args.push_back(p2iI);
        CallI = CallInst::Create(func_corruptPtr2Int_64bit, args, "call_corruptPtr2Int_64bit", BB);
        assert(CallI);
        CallI->setCallingConv(CallingConv::C);
        corruptVal = &(*CallI);

        /* convert int64 to ptr */
        i2pI = new IntToPtrInst(corruptVal, I->getType(), "convert_i642ptr", BB);
        assert(i2pI);
    } else {
        BasicBlock::iterator BINext = BI;
        BINext++;
        INext = &*BINext;
        assert(INext);

        /* Convert ptr to int64 */
        p2iI = new PtrToIntInst(I, i64Ty, "convert_ptr2i64", INext);
        assert(p2iI);

        /* Corrupt */
        args.push_back(p2iI);
        assert(func_corruptPtr2Int_64bit);
        CallI = CallInst::Create(func_corruptPtr2Int_64bit, args,
                                 "call_corruptPtr2Int_64bit", INext);
        assert(CallI);
        CallI->setCallingConv(CallingConv::C);
        corruptVal = &(*CallI);

        /* convert int64 to ptr */
        i2pI = new IntToPtrInst(corruptVal, I->getType(), "convert_i642ptr", INext);
        assert(i2pI);
    }

    /* make sure everyone is using corrupt value */
    if (CallI) {
        corruptVal = &(*i2pI);
        I->replaceAllUsesWith(corruptVal);

        /* Because of the preceeding method invocation, we messed up last argument in
         * the call instruction.  We need to manually set this value */
        BasicBlock::iterator BINext = BI;
        BINext++;
        INext = &*BINext;
        INext->setOperand(/*4*/INext->getNumOperands() -1, I);
        comment = "Result";
    }
    return true;
}


void FlipIt::DynamicFaults::cacheFunctions(Module::FunctionListType &functionList) {
    StringRef lstr;
    for (Module::iterator it = functionList.begin(); it != functionList.end(); ++it) {
        lstr = it->getName();
        string cstr = lstr.str();
        if (cstr.find("corruptIntData_8bit") != std::string::npos) {
            func_corruptIntData_8bit =&*it;
        } else if (cstr.find("corruptIntData_16bit") != std::string::npos) {
            func_corruptIntData_16bit =&*it;
        } else if (cstr.find("corruptIntData_32bit") != std::string::npos) {
            func_corruptIntData_32bit =&*it;
        } else if (cstr.find("corruptIntData_64bit") != std::string::npos) {
            func_corruptIntData_64bit =&*it;
        } else if (cstr.find("corruptPtr2Int_64bit") != std::string::npos) {
            func_corruptPtr2Int_64bit =&*it;
        } else if (cstr.find("corruptFloatData_32bit") != std::string::npos) {
            func_corruptFloatData_32bit =&*it;
        } else if (cstr.find("corruptFloatData_64bit") != std::string::npos) {
            func_corruptFloatData_64bit =&*it;
        } else if (cstr.find("corruptIntAdr_8bit") != std::string::npos) {
            func_corruptIntAdr_8bit =&*it;
        } else if (cstr.find("corruptIntAdr_16bit") != std::string::npos) {
            func_corruptIntAdr_16bit =&*it;
        } else if (cstr.find("corruptIntAdr_32bit") != std::string::npos) {
            func_corruptIntAdr_32bit =&*it;
        } else if (cstr.find("corruptIntAdr_64bit") != std::string::npos) {
            func_corruptIntAdr_64bit =&*it;
        } else if (cstr.find("corruptFloatAdr_32bit") != std::string::npos) {
            func_corruptFloatAdr_32bit =&*it;
        } else if (cstr.find("corruptFloatAdr_64bit") != std::string::npos) {
            func_corruptFloatAdr_64bit =&*it;
        }
    }/*end for*/

    assert(func_corruptIntData_8bit != NULL  && func_corruptIntData_16bit != NULL
        && func_corruptIntData_32bit != NULL && func_corruptIntData_64bit != NULL
        && func_corruptPtr2Int_64bit != NULL  && func_corruptFloatData_32bit != NULL
        && func_corruptFloatData_64bit != NULL && func_corruptIntAdr_8bit != NULL
        && func_corruptIntAdr_16bit != NULL  && func_corruptIntAdr_32bit != NULL
        && func_corruptIntAdr_64bit != NULL && func_corruptFloatAdr_32bit != NULL
        && func_corruptFloatAdr_64bit != NULL);
}

void FlipIt::DynamicFaults::enumerateSites(std::vector<Instruction*>& ilist, Function *F,
                                   unsigned& displayIdx, std::vector<Instruction*>* selectInsts) {
    for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; I++) {
        Value *in = &(*I);
        if (in == NULL)
            continue;
        errs()  << *I;
        if ( (isa<StoreInst>(in) || isa<LoadInst>(in) || isa<BinaryOperator>(in) || isa<CmpInst>(in)
            || isa<CallInst>(in) || isa<AllocaInst>(in) || isa<GetElementPtrInst>(in)) ) {

            if (selectInsts != NULL) {
                if(std::find(selectInsts->begin(), selectInsts->end(), &*I) == selectInsts->end()) {
                    errs() << "\n";
                    continue;
                }
            }
            ilist.push_back(&*I);
            errs()  << "; Fault Index: " << displayIdx++;
        }
        errs() <<'\n';
    }
}

bool FlipIt::DynamicFaults::injectFault(Instruction* I) {
    bool ret = false;
    comment = "";
    injectionType = "";
    skipAmount = 0;
    if (ctrl_err && injectControl(I)) {
        ret = true;
        //injectionType = "Control";
    } else if (arith_err && injectArithmetic(I)) {
        ret = true;
        injectionType = "Arithmetic";
    } else if (ptr_err && injectPointer(I)) {
        ret = true;
        injectionType = "Pointer";
    } else if ( (ctrl_err || arith_err || ptr_err) && injectCall(I) ) {
        ret = true;
    }
    /*
    else {
        errs() << "Warning: Didn't injection into \"" << *inst << "\"\n";
    }
    */
    if (ret) {
        // Site #,   injection type, comment, inst
        errs() << '#' << faultIdx++ << '\t' << injectionType << '\t' << comment  << "\t";
        if (MDNode *N = I->getMetadata("dbg")) {
            DILocation Loc(N);
            unsigned Line = Loc.getLineNumber();
            StringRef File = Loc.getFilename();
            // StringRef Dir = Loc.getDirectory();
            errs() << File << ":" << Line << "\n";
        } else {
            errs() << *I << '\n';
        }
    }
    return ret;
}
/****************************************************************************************/

