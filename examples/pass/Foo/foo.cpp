#include "foo.h"
 
bool Food::Foo::runOnModule(Module &M) {
    errs() << "Inside Foo::runOnModule\n\n";
	/* 
		Command line arguments to FlipIt pass are the arguments of the constructor:
		string funcList (-funcList)
		string configPath (-config)
		double siteProb (-prob)
		int byte_val (-byte)
		int singleInj (-singleInj)
		bool arith_err (-arith)
		bool ctrl_err (-ctrl)
		bool ptr_err (-ptr)
        string srcFile (-srcFile)
        string statFiel (-stateFile)
	*/
    std::string srcFile = "none.c";
    std::string stateFile = "foo";
	DynamicFaults* flipit = new DynamicFaults("", "FlipIt.config", 0.95, 0, 1, 1, 1, 1, srcFile, stateFile, &M);
    
    for (auto F = M.getFunctionList().begin(),
        E = M.getFunctionList().end(); F != E; F++) {
      	
      	// select certain instution from this fucntion to corrupt
      	if (F->getName().str() == "work") {
	        errs() << "\n Calling FlipIt's corruptInstruction on every other"
                    "instruction in function " << F->getName() << "\n";
      		int i = 0;
      		for (auto BB = F->begin(), BBe = F->end(); BB !=BBe; BB++) {
                for (auto I = BB->begin(), Ie = BB->end(); I != Ie; I++ ) {
                    if ( (isa<StoreInst>(I) || isa<LoadInst>(I)
                        || isa<BinaryOperator>(I) || isa<CmpInst>(I)
                        || isa<CallInst>(I) || isa<AllocaInst>(I)
                        || isa<GetElementPtrInst>(I)) ) {

                        // corrupt every other instruction inside the "work" function
                        if (i % 2 == 0)
                            flipit->corruptInstruction(I);
                        i++;
                    }
                }
	        }
      	}
	}

    delete flipit;
	return false;
}

void Food::Foo::display()
{
	errs() << "Food::Foo:display\n";
	if (potato)
		errs() << "You like potatos\n";
	if (carrot)
		errs() << "You like carrots\n";
}

