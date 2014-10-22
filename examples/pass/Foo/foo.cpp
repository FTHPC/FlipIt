#include "foo.h"
 
bool Food::Foo::runOnModule(Module &M) {
    errs() << "Inside Foo::runOnModule looking at module containg the following functions:\n";
    
    std::vector<Instruction*>* insts = new std::vector<Instruction*>();
    Module::FunctionListType &functionList = M.getFunctionList();
	for (Module::iterator it = functionList.begin(); it != functionList.end(); ++it) {
      	errs().write_escaped(it->getName()) << "\n";
      	
      	// select certain instution from this fucntion to corrupt
      	if (it->getName().str() == "work") {
      		int i = 0;
      		for (inst_iterator I = inst_begin(it), E = inst_end(it); I != E; I++) {
	            Value *in = &(*I);
	            if (in == NULL)
	                continue;
	            
	            if ( (isa<StoreInst>(in) || isa<LoadInst>(in) || isa<BinaryOperator>(in) || isa<CmpInst>(in)
	                || isa<CallInst>(in) || isa<AllocaInst>(in) || isa<GetElementPtrInst>(in)) ) {

	            	// corrupt every other instruction inside the "work" function
	                if (i % 2 == 0)
	                    (*insts).push_back(&*I);
	            }
	            i++;
	        }
      	}
	}

	errs() << "\n Calling FlipIt's runOnModule to corrupt functions\n";
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
	*/
	DynamicFaults* flipit = new DynamicFaults("", "FlipIt.config", 0.95, 0, 1, 1, 1, 1);
    flipit->runOnModuleCustom(M, insts);
      
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

