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
//#include <algorithm>
//#include <vector>
//#include <string>

#ifdef COMPILE_PASS
FlipIt::DynamicFaults::DynamicFaults() : ModulePass(FlipIt::DynamicFaults::ID) {
#else
FlipIt::DynamicFaults::DynamicFaults(Module* M) {
    funcList = "";
    configPath = "FlipIt.config";
    siteProb = 1e-8;
    byte_val = -1;
    singleInj = 1;
    arith_err = true;
    ctrl_err = true;
    ptr_err = true;
    srcFile = "UNKNOWN"; 
    stateFile = "FlipItState"; 
    
    //Module::FunctionListType &functionList = M->getFunctionList();
    init();
    //cacheFunctions();
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
                            bool _ctrl_err, bool _ptr_err, std::string _srcFile,
                            std::string _stateFile, Module* Mod): ModulePass(ID)
#else

FlipIt::DynamicFaults::DynamicFaults(string _funcList, string _configPath, double _siteProb, 
                            int _byte_val, int _singleInj, bool _arith_err, 
                            bool _ctrl_err, bool _ptr_err, std::string _srcFile,
                            std::string _stateFile, Module* Mod)
#endif
{
    M = Mod;
    funcList = _funcList;
    configPath =  _configPath;
    siteProb = _siteProb;
    byte_val = _byte_val;
    singleInj = _singleInj;
    arith_err = _arith_err;
    ctrl_err = _ctrl_err;
    ptr_err = _ptr_err;
    srcFile = _srcFile;
    stateFile = _stateFile;

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
    
#ifndef COMPILE_PASS
   // Module::FunctionListType &functionList = M->getFunctionList();
    init();
    //cacheFunctions();
#endif
}


bool FlipIt::DynamicFaults::runOnModule(Module &Mod) {

    M = &Mod;
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


    //Module::FunctionListType &functionList = M.getFunctionList();
    //vector<std::string> flist = splitAtSpace(funcList);
    init();


    /*Corrupt all instruction in vaible functions or in selectedList */
    for (auto F = M->begin(), FE = M->end(); F != FE; ++F) {
        
        /* extract the pure function name, i.e. demangle if using c++*/
        std::string cstr = demangle(F->getName().str());
        if (F->begin() == F->end() || !viableFunction(cstr, flist))
            continue;

        logfile->logFunctionHeader(faultIdx, cstr);
        inst_iterator I, E, Inext;
        I = inst_begin(F);
        E = inst_end(F);
        for ( ; I != E;) {
            Inext = I;
            Inext++;
            Value *in = &(*I);
            if (in == NULL)
                continue;
            if ( (isa<StoreInst>(in) || isa<LoadInst>(in)
                || isa<BinaryOperator>(in) || isa<CmpInst>(in)
                || isa<CallInst>(in) || isa<AllocaInst>(in) 
                || isa<GetElementPtrInst>(in)) ) 
            {   
                injectFault(&(*I));
            }

            /* find next inst that is original to the function */
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

bool FlipIt::DynamicFaults::viableFunction(std::string func, std::vector<std::string>& flist) {
   
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
    
    if (funcList.length() == 0
        || std::find(flist.begin(), flist.end(), func) != flist.end()) {
        return true;
    }
    return false;
}
void  FlipIt::DynamicFaults::init() {
    faultIdx = 0;
    readConfig(configPath);
    splitAtSpace();
    /*Cache function references of the function defined in Corrupt.c to all inserting of
     *call instructions to them */
    unsigned long sum = cacheFunctions();

    // TODO: get stateFile from config
#ifndef COMPILE_PASS
    sum = 0;
#endif
    faultIdx = updateStateFile(stateFile.c_str(), sum);
    logfile = new LogFile(srcFile, faultIdx); 
    
    //set up args to be used in corrupt calls
    args.reserve(5);
    for (int i = 0; i < 5; i++)
        args.push_back(NULL);
    args[1] = ConstantInt::get(IntegerType::getInt32Ty(getGlobalContext()), singleInj);
    
    // create constant ints for each byte position
    for (int i=-1; i<8; i++) {
        byteVal[i] = ConstantInt::get(IntegerType::getInt32Ty(
            getGlobalContext()), i);
    }
    i64Ty = Type::getInt64Ty(getGlobalContext());
}

void FlipIt::DynamicFaults::readConfig(string path) {
    std::ifstream infile;
    string line;
    infile.open(path.c_str());
    //errs() << "reading config file " << path << "\n";
    // read inst probs
    getline(infile, line); // INSTRUCTIONS:
    getline(infile, line);
    while (line != "FUNCTIONS:" && infile) {
        unsigned long found = line.find("=");

        if (found != string::npos) {
            auto constFP = ConstantFP::get(Type::getDoubleTy(
                getGlobalContext()), atof(line.substr(found+1).c_str()));
            instProbs.insert(std::pair<std::string, Value*>(
                line.substr(0, found), constFP));
            //instProbs[line.substr(0, found)] = atof(line.substr(found+1).c_str());
        }
        getline(infile, line);
    }
    // default injection probability
    instProbs["default"] = ConstantFP::get(Type::getDoubleTy(
            getGlobalContext()), siteProb); 
    

    // read func probs
    while (infile) {
        unsigned long found = line.find("=");

        if (found != string::npos && line[0] != '#') {
            auto constFP = ConstantFP::get(Type::getDoubleTy(
                getGlobalContext()), atof(line.substr(found+1).c_str()));
            funcProbs.insert(std::pair<std::string, Value*>(
                line.substr(0, found), constFP));
            //funcProbs[line.substr(0, found)] = atof(line.substr(found+1).c_str());
        }
        getline(infile, line);
    }
    funcProbs["zero"] = ConstantFP::get(Type::getDoubleTy(
            getGlobalContext()), 0); 
    infile.close();
}


bool  FlipIt::DynamicFaults::finalize() {
    logfile->close();
    //delete logfile;
    return oldFaultIdx != faultIdx;
}

void FlipIt::DynamicFaults::splitAtSpace() {
    //std::vector<std::string> strLst;
    std::istringstream isstr(funcList);
    copy(std::istream_iterator<std::string>(isstr), std::istream_iterator<std::string>(),
	   std::back_inserter<std::vector<std::string> >(flist));
}
/*
vector<string> FlipIt::DynamicFaults::splitAtSpace(string spltStr) {
    std::vector<std::string> strLst;
    std::istringstream isstr(spltStr);
    copy(std::istream_iterator<std::string>(isstr), std::istream_iterator<std::string>(),
	   std::back_inserter<std::vector<std::string> >(flist));
}
*/
Value* FlipIt::DynamicFaults::getInstProb(Instruction* I) {
    /*First check if it is a call to a function listed in the config file*/
    if (CallInst *callInst = dyn_cast<CallInst>(I)) {
        if (callInst->getCalledFunction() == NULL) /* function pointers will be null */
            return funcProbs["zero"];

        string funcName = callInst->getCalledFunction()->getName().str();
        if (funcProbs.find(funcName) != funcProbs.end())
            return funcProbs[funcName];
    }

    /* Get the probability from the instruction's type from the config
    file or the default probabilty given as a command line argument */
    string type = I->getOpcodeName();
    return instProbs.find(type) != instProbs.end() ? instProbs[type] : instProbs["default"];
}

unsigned long FlipIt::DynamicFaults::updateStateFile(const char* stateFile, unsigned long sum)
{
    unsigned long startNum = 0;

    // grab lock
    string homePath(getenv("FLIPIT_PATH"));
    string lockPath = homePath + "/.lock";
    int fd = open(lockPath.c_str(), O_RDWR | O_CREAT, 0666);
    while (flock(fd, LOCK_EX | LOCK_NB)) {}

    // read and update the state file    
    string stateFilePath = homePath + "/." + stateFile;
    std::fstream file(stateFilePath);

    // read file only if it was corectly open
    if (!file.is_open()) {
        errs() << "Error opening state file: " << stateFilePath 
                << "\nAssuming fault index of 0 and creating the file\n";
        file.close();
        file.open(stateFilePath, std::fstream::out);
        if (!file.is_open()) {
            file.close();
            return 0;
        }
    }
    else {
        file >> startNum;
    }
    
    // clear the file cause the eof bit is reached
    file.clear();
    file.seekg(0, ios::beg);
    //errs() << "startNum = " << startNum << " sum = " << sum << "\n";
    file << (startNum + sum);
    file.close();

    // release file lock
    flock(fd, LOCK_UN);
    close(fd);
    return startNum;
}

bool FlipIt::DynamicFaults::injectControl_NEW(Instruction* I) {

    /* Build argument list before calling Corrupt function */
    args[0] = ConstantInt::get(IntegerType::getInt32Ty(getGlobalContext()), faultIdx);
    args[2] = getInstProb(I);
    args[3] = byteVal[byte_val];


    /* Choose a fault site in CmpInst and insert Corrupt function call */
    if (isa<CmpInst>(I))
    {
        /* select a random arg to corrupt because corrupting the result will yeild a
        50% chance of branching incorrectly */
        unsigned int opPos = rand() % 2;
        
        /* LLVM doesn't like attempting to corrupt NULL */
        if (I->getOperand(opPos) == NULL)
    	    opPos = (opPos+1) % 2;
        injectionType = CONTROL_BRANCH;
        return injectInOperand(I, opPos);
    }

    /* check to see if instruction modifies a looping variable such as i++
        if so we need to inject into it and mark the injection type 'control' */
    if (isa<StoreInst>(I)) {
        if (I->getName().str().find("indvars") == 0
            || I->getName().str().substr(0, 3) == "inc")
        {
            injectionType = CONTROL_LOOP;
            return injectInOperand(I, 0); // value to be store
        }
    }
    if (I->getName().str().find("indvars") == 0
        || I->getName().str().substr(0, 3) == "inc")
    {
        injectionType = CONTROL_LOOP;
        return injectResult(I);
    }

    /* TODO: Add support of PHI Instruction */
    return false;
}

bool FlipIt::DynamicFaults::injectArithmetic_NEW(Instruction* I)
{
    /* Build argument list before calling Corrupt function */
    args[0] = ConstantInt::get(IntegerType::getInt32Ty(getGlobalContext()), faultIdx);
    args[2] = getInstProb(I);
    args[3] = byteVal[byte_val];

    /* We handle these in a special way */
    if (isa<CallInst>(I))
        return false;

    /* store instruction required differnt injection logic than binary operators */
    if (isa<StoreInst>(I)) {
        if (I->getType()->isIntegerTy()
            || I->getType()->isFloatingPointTy())
        {
            bool ret = injectInOperand(I, 0);
            comment = VALUE;
            return ret;
        }
    }
    
    if (!isa<CmpInst>(I)) {
        if (I->getType()->isIntegerTy()
            || I->getType()->isFloatingPointTy())
        {
            bool ret = injectResult(I);
            if (isa<LoadInst>(I))
                comment = VALUE;
            return ret;
        }
    }
    return false;
}


bool FlipIt::DynamicFaults::injectPointer_NEW(Instruction* I)
{
    /*Build argument list before calling Corrupt function*/
    args[0] = ConstantInt::get(IntegerType::getInt32Ty(getGlobalContext()), faultIdx);
    args[2] = getInstProb(I);
    args[3] = byteVal[byte_val];

    //if (isa<AllocaInst>(I) || isa<GetElementPtrInst>) {
    if (I->getType()->isPointerTy()) {
        if (isa<StoreInst>(I)) {
            bool ret = injectInOperand(I, 0); //Value
            comment = VALUE;
            return ret;
        }
        bool ret = injectResult(I);
        if (isa<LoadInst>(I)) {
            comment = VALUE;
        }
        return ret;
    }
    return false;
}

bool FlipIt::DynamicFaults::injectCall_NEW(Instruction* I)
{
    /*Build argument list before calling Corrupt function*/
    args[0] = ConstantInt::get(IntegerType::getInt32Ty(getGlobalContext()), faultIdx);
    args[2] = getInstProb(I);
    args[3] = byteVal[byte_val];
    
    if (!isa<CallInst>(I))
        return false; 
    if (dyn_cast<CallInst>(I)->getNumArgOperands() == 0)
    {
        bool ret = injectResult(I);
        comment = RESULT;
        return ret;
    }
    int opNum = selectArgument(dyn_cast<CallInst>(I));
    if (opNum == -1)
        return false;

    return injectInOperand(I, opNum);
}

bool FlipIt::DynamicFaults::injectResult(Instruction* I) {
    args[4] = I;
    BasicBlock::iterator INext(I);
    INext++;
    Value* corruptVal = NULL;
    CallInst* call = NULL;
    auto type = I->getType();
    /*Integer Data*/
    if (type->isIntegerTy(8)) {
        call = CallInst::Create(func_corruptIntData_8bit, args,
                                 "call_corruptIntData_8bit", INext);
        call->setCallingConv(CallingConv::C);
    } else if (type->isIntegerTy(16)) {
        call = CallInst::Create(func_corruptIntData_16bit, args,
                                  "call_corruptIntData_16bit", INext);
        call->setCallingConv(CallingConv::C);
    } else if (type->isIntegerTy(32)) {
        call = CallInst::Create(func_corruptIntData_32bit, args,
                               "call_corruptIntData_32bit", INext);
        call->setCallingConv(CallingConv::C);
    } else if (type->isIntegerTy(64)) {
        call = CallInst::Create(func_corruptIntData_64bit, args,
                               "call_corruptIntData_64bit", INext);
        call->setCallingConv(CallingConv::C);
    } else if (type->isFloatTy()) {
    /*Float Data*/
        call = CallInst::Create(func_corruptFloatData_32bit, args,
                                 "call_corruptFloatData_32bit", INext);
        call->setCallingConv(CallingConv::C);
    } else if (type->isDoubleTy()) {
        call = CallInst::Create(func_corruptFloatData_64bit, args,
                                 "call_corruptFloatData_64bit", INext);
        call->setCallingConv(CallingConv::C);
    } 
    else if (type->isPointerTy()) { 
        type = type->getSequentialElementType();
        if (type->isIntegerTy(8)) {
            call = CallInst::Create(func_corruptIntAdr_8bit, args,
                                "call_corruptIntAdr_8bit", INext);
            call->setCallingConv(CallingConv::C);
        } else if (type->isIntegerTy(16)) {
            call = CallInst::Create(func_corruptIntAdr_16bit, args,
                                "call_corruptIntAdr_16bit", INext);
            call->setCallingConv(CallingConv::C);
        } else if (type->isIntegerTy(32)) {
            call = CallInst::Create(func_corruptIntAdr_32bit, args,
                                "call_corruptIntAdr_32bit", INext);
            call->setCallingConv(CallingConv::C);
        } else if (type->isIntegerTy(64)) {
            call = CallInst::Create(func_corruptIntAdr_64bit, args,
                                "call_corruptIntAdr_64bit", INext);
            call->setCallingConv(CallingConv::C);
        } else if (type->isFloatTy()) {
            call = CallInst::Create(func_corruptFloatAdr_32bit, args,
                                "call_corruptFloatAdr_32bit", INext);
            call->setCallingConv(CallingConv::C);
        } else if (type->isDoubleTy()) {
            call = CallInst::Create(func_corruptFloatAdr_64bit, args,
                                "call_corruptFloatAdr_64bit", INext);
            call->setCallingConv(CallingConv::C);
        } else {
            /* Convert ptr to int64 */
            auto p2iI = new PtrToIntInst(I, i64Ty, "convert_ptr2i64", INext);

            /* Corrupt */
            args[4] = p2iI;
            call = CallInst::Create(func_corruptPtr2Int_64bit, args,
                                    "call_corruptPtr2Int_64bit", INext);
            call->setCallingConv(CallingConv::C);

            /* convert int64 to ptr */
            corruptVal = new IntToPtrInst(call, I->getType(), "convert_i642ptr", INext);
        }
    }
    if (corruptVal == NULL) {
        corruptVal = call;
    }
    if (corruptVal) {
        I->replaceAllUsesWith(corruptVal);

        /* Because of the preceeding method invocation, we messed up last argument in the call instruction.
            We need to manually set this value to the result of Insturction I */
        INext = I;
        INext++;
        if (isa<CallInst>(INext))
            INext->setOperand(4, I); // hard coded. If like others, it says we have an extra argument
        else
            INext->setOperand(0, I);
        comment = RESULT;
        return true;
    }
    return false;
}

bool FlipIt::DynamicFaults::injectInOperand(Instruction* I, int operand)
{
    /* We assume that operand is vaild */
    args[4] = I->getOperand(operand); // value stored
    Value* corruptVal = NULL;
    CallInst* call = NULL;
    auto type = I->getOperand(operand)->getType();
    /*Integer Data*/
    if (type->isIntegerTy(8)) {
        call = CallInst::Create(func_corruptIntData_8bit, args,
                                "call_corruptIntData_8bit", I);
        call->setCallingConv(CallingConv::C);
    } else if (type->isIntegerTy(16)) {
        call = CallInst::Create(func_corruptIntData_16bit, args,
                                "call_corruptIntData_16bit", I);
        call->setCallingConv(CallingConv::C);
    } else if (type->isIntegerTy(32)) {
        call = CallInst::Create(func_corruptIntData_32bit, args,
                                "call_corruptIntData_32bit", I);
        call->setCallingConv(CallingConv::C);
    } else if (type->isIntegerTy(64)) {
        call = CallInst::Create(func_corruptIntData_64bit, args,
                                "call_corruptIntData_64bit", I);
        call->setCallingConv(CallingConv::C);
    } else if (type->isFloatTy()) {
    /*Float Data*/
        call = CallInst::Create(func_corruptFloatData_32bit, args,
                                 "call_corruptFloatData_32bit", I);
        call->setCallingConv(CallingConv::C);
    } else if (type->isDoubleTy()) {
        call = CallInst::Create(func_corruptFloatData_64bit, args,
                               "call_corruptFloatData_64bit", I);
        call->setCallingConv(CallingConv::C);
    } 
    else if (type->isPointerTy()) { 
        type = type->getSequentialElementType();
        if (type->isIntegerTy(8)) {
            call = CallInst::Create(func_corruptIntAdr_8bit, args,
                                "call_corruptIntAdr_8bit", I);
            call->setCallingConv(CallingConv::C);
        } else if (type->isIntegerTy(16)) {
            call = CallInst::Create(func_corruptIntAdr_16bit, args,
                                "call_corruptIntAdr_16bit", I);
            call->setCallingConv(CallingConv::C);
        } else if (type->isIntegerTy(32)) {
            call = CallInst::Create(func_corruptIntAdr_32bit, args,
                                "call_corruptIntAdr_32bit", I);
            call->setCallingConv(CallingConv::C);
        } else if (type->isIntegerTy(64)) {
            call = CallInst::Create(func_corruptIntAdr_64bit, args,
                                "call_corruptIntAdr_64bit", I);
            call->setCallingConv(CallingConv::C);
        } else if (type->isFloatTy()) {
            call = CallInst::Create(func_corruptFloatAdr_32bit, args,
                                "call_corruptFloatAdr_32bit", I);
            call->setCallingConv(CallingConv::C);
        } else if (type->isDoubleTy()) {
            call = CallInst::Create(func_corruptFloatAdr_64bit, args,
                                "call_corruptFloatAdr_64bit", I);
            call->setCallingConv(CallingConv::C);
        } else {
            type = args[4]->getType();
            auto p2iI = new PtrToIntInst(args[4], i64Ty, "convert_ptr2i64", I);

            /* Corrupt */
            args[4] = p2iI;
            call = CallInst::Create(func_corruptPtr2Int_64bit, args,
                                    "call_corruptPtr2Int_64bit", I);
            call->setCallingConv(CallingConv::C);

            /* convert int64 to ptr */
            corruptVal = new IntToPtrInst(call, type, "convert_i642ptr", I);
        }
    }
    if (corruptVal == NULL) {
        corruptVal = call;
    }
    if (corruptVal) {
        I->setOperand(operand, corruptVal);
        comment = operand + 1;
        return true;
    }

    return false;
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
    } else if (funcName.find("__STORE_") != string::npos) {
        argPos.push_back(1); //(fptr, fvalue, gptr, gvalue)
    } else if (funcName.find("__LOAD_") != string::npos) {
        return arg; // applies mask inside function
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
                injectionType = CONTROL_LOOP;
                //injectionType = "Control-Loop";
                argFound = true;
            }
        } else if (arith_err && (callInst->getArgOperand(a)->getType()->isIntegerTy()
                || callInst->getArgOperand(a)->getType()->isFloatTy()
                || callInst->getArgOperand(a)->getType()->isDoubleTy() )
                && !callInst->getArgOperand(a)->getType()->isIntegerTy(1)) {
            arg = a;
            if (callInst->getArgOperand(a)->getType()->isIntegerTy()) {
                injectionType = ARITHMETIC_FIX;
            }   else if (callInst->getArgOperand(a)->getType()->isFloatTy()
                || callInst->getArgOperand(a)->getType()->isDoubleTy()) {
                injectionType = ARITHMETIC_FP;
            }
            //injectionType = "Arithmetic";
            argFound = true;
        } else if (ptr_err) {
            arg = a;
            injectionType = POINTER;
            //injectionType = "Pointer";
            argFound = true;
        }
        argPos.erase(argPos.begin() + a);
    }

    return arg;
}


unsigned long FlipIt::DynamicFaults::cacheFunctions() { //Module::FunctionListType &functionList) {
    unsigned long sum = 0; // # insts in module
    for (auto F = M->getFunctionList().begin(), E = M->getFunctionList().end(); F != E; F++) {
        string cstr = F->getName().str();
        if (cstr.find("corruptIntData_8bit") != std::string::npos) {
            func_corruptIntData_8bit =&*F;
        } else if (cstr.find("corruptIntData_16bit") != std::string::npos) {
            func_corruptIntData_16bit =&*F;
        } else if (cstr.find("corruptIntData_32bit") != std::string::npos) {
            func_corruptIntData_32bit =&*F;
        } else if (cstr.find("corruptIntData_64bit") != std::string::npos) {
            func_corruptIntData_64bit =&*F;
        } else if (cstr.find("corruptPtr2Int_64bit") != std::string::npos) {
            func_corruptPtr2Int_64bit =&*F;
        } else if (cstr.find("corruptFloatData_32bit") != std::string::npos) {
            func_corruptFloatData_32bit =&*F;
        } else if (cstr.find("corruptFloatData_64bit") != std::string::npos) {
            func_corruptFloatData_64bit =&*F;
        } else if (cstr.find("corruptIntAdr_8bit") != std::string::npos) {
            func_corruptIntAdr_8bit =&*F;
        } else if (cstr.find("corruptIntAdr_16bit") != std::string::npos) {
            func_corruptIntAdr_16bit =&*F;
        } else if (cstr.find("corruptIntAdr_32bit") != std::string::npos) {
            func_corruptIntAdr_32bit =&*F;
        } else if (cstr.find("corruptIntAdr_64bit") != std::string::npos) {
            func_corruptIntAdr_64bit =&*F;
        } else if (cstr.find("corruptFloatAdr_32bit") != std::string::npos) {
            func_corruptFloatAdr_32bit =&*F;
        } else if (cstr.find("corruptFloatAdr_64bit") != std::string::npos) {
            func_corruptFloatAdr_64bit =&*F;
        }
        /* TODO: check for function viability */
        if (F->begin() != F->end() && viableFunction(cstr, flist))
            for (auto BB = F->begin(), BBe = F->end(); BB != BBe; BB++) 
                sum += BB->size();
    }/*end for*/

    assert(func_corruptIntData_8bit != NULL  && func_corruptIntData_16bit != NULL
        && func_corruptIntData_32bit != NULL && func_corruptIntData_64bit != NULL
        && func_corruptPtr2Int_64bit != NULL  && func_corruptFloatData_32bit != NULL
        && func_corruptFloatData_64bit != NULL && func_corruptIntAdr_8bit != NULL
        && func_corruptIntAdr_16bit != NULL  && func_corruptIntAdr_32bit != NULL
        && func_corruptIntAdr_64bit != NULL && func_corruptFloatAdr_32bit != NULL
        && func_corruptFloatAdr_64bit != NULL);
    
    return sum;
}

bool FlipIt::DynamicFaults::corruptInstruction(Instruction* I) {
//errs() << "faultIdx = " << faultIdx << "\n";
#ifndef COMPILE_PASS
        std::vector<std::string> dummyVector;
        if(!viableFunction(I->getParent()->getParent()->getName().str(),
            dummyVector)) {
            //errs() << "Returning false for function: " <<
            //    I->getParent()->getParent()->getName().str() <<"\n";
            return false;
        }    
#endif
        return injectFault(I);
}

bool FlipIt::DynamicFaults::injectFault(Instruction* I) {
    bool inj = false;
    comment = 0; injectionType = 0;
    if (ctrl_err && injectControl_NEW(I)) {
    //if (ctrl_err && injectControl(I)) {
        inj = true;
        //injectionType = "Control";
    } else if (arith_err && injectArithmetic_NEW(I)) {
    //} else if (arith_err && injectArithmetic(I)) {
        inj = true;
        Value* tmp = I;
        if (isa<StoreInst>(I)) {
            tmp = I->getOperand(0); // value
        }
        if (tmp->getType()->isIntegerTy()) {
            injectionType = ARITHMETIC_FIX;
        }   else if (tmp->getType()->isFloatTy()
            || tmp->getType()->isDoubleTy()) {
            injectionType = ARITHMETIC_FP;
        }
    } else if (ptr_err && injectPointer_NEW(I)) {
        inj = true;
        injectionType = POINTER;
    } else if ( (ctrl_err || arith_err || ptr_err) && injectCall_NEW(I) ) {
        inj = true;
    }
    /*
    else {
        errs() << "Warning: Didn't injection into \"" << *inst << "\"\n";
    }
    */
    if (inj) {
        // Site #,   injection type, comment, inst
#ifndef COMPILE_PASS
        logfile->logFunctionHeader(faultIdx, I->getParent()->getParent()->getName().str());
        faultIdx = updateStateFile(stateFile.c_str(), 1);

#endif
        logfile->logInst(faultIdx++, injectionType, comment, I);
    }
    return inj;
}
/****************************************************************************************/

