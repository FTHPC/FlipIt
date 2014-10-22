/***********************************************************************************************/
/* This file is licensed under the University of Illinois/NCSA Open Source License.            */
/* See LICENSE.TXT for details.                                                                */
/***********************************************************************************************/

/***********************************************************************************************/
/*                                                                                             */
/* Name: foo.h                                                                                 */
/*                                                                                             */
/* Description: Header file for  an example compiler pass that uses FlipIt to corrupt certain  */ 
/*              instructinos.                                                                  */
/*                                                                                             */
/***********************************************************************************************/

#include <vector>
	using std::vector;
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include <llvm/Support/CommandLine.h>
#include "llvm/IR/Instruction.h"
#include <llvm/Support/InstIterator.h>
using namespace llvm;

#include "../FlipIt/faults.h"
using namespace FlipIt;


static cl::opt<bool> potato("potato", cl::desc("Do you like potatos?"), cl::value_desc("0/1"), cl::init(1), cl::ValueRequired);
static cl::opt<bool> carrot("carrot", cl::desc("Do you like carrots?"), cl::value_desc("0/1"), cl::init(1), cl::ValueRequired);

namespace Food{


	class Foo : public ModulePass {

		public:
			static char ID;
			Foo(): ModulePass(ID) { }

  			bool runOnModule(Module &M);
  			void display();
  }; 
}  

char Food::Foo::ID = 0;
static RegisterPass<Food::Foo> F1("Foo", "Pass demonstrating how to use FlipIt from inside of a pass");

