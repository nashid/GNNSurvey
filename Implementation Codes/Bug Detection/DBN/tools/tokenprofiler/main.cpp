//
// Created by Mingkai Chen on 2017-04-03.
//


#include "llvm/ADT/Triple.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/AsmParser/Parser.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/CodeGen/CommandFlags.h"
#include "llvm/CodeGen/LinkAllAsmWriterComponents.h"
#include "llvm/CodeGen/LinkAllCodegenComponents.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Linker/Linker.h"
#include "llvm/MC/SubtargetFeature.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileUtilities.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetSubtargetInfo.h"
#include "llvm/Transforms/Scalar.h"

#include <memory>
#include <string>
#include <fstream>
#include <unordered_map>
#include <unordered_set>

#include "TokenVectorPass.hpp"

using namespace llvm;
using std::string;
using std::unique_ptr;
using std::vector;
using llvm::sys::ExecuteAndWait;
using llvm::sys::findProgramByName;
using llvm::legacy::PassManager;

static cl::OptionCategory tokenProfilerCategory{"token profiler options"};

static cl::opt<string> bugFile{"b",
							   cl::desc{"Filename of the bugs specifying module, function and line of code"},
							   cl::value_desc{"filename"},
							   cl::init(""),
							   cl::cat{tokenProfilerCategory}};

static cl::list<string> inPath{cl::Positional,
							   cl::desc{"<Modules to analyze>"},
							   cl::value_desc{"bitcode filename"},
							   cl::OneOrMore,
							   cl::cat{tokenProfilerCategory}};


static tokenprofiling::VEC_INFO token_vectors;


static void analyzeModule(Module& module, BUG_REPO& bugs,
	tokenprofiling::TOKEN_REPO& tok_repo)
{
	tokenprofiling::TokenVectorPass* tpass = new tokenprofiling::TokenVectorPass(bugs, &tok_repo);

	// Build up all of the passes that we want to run on the module.
	legacy::PassManager pm;
	pm.add(new DominatorTreeWrapperPass());
	pm.add(new LoopInfoWrapperPass());
	pm.add(tpass);
	pm.add(createVerifierPass());
	pm.run(module);

	token_vectors.insert(token_vectors.end(),
	tpass->token_vectors_.begin(), tpass->token_vectors_.end());
}


static void parseForBugs(BUG_REPO& bugs)
{
	std::ifstream bFile(bugFile);
	std::string bugline;
	while (bFile.good())
	{
		std::getline(bFile, bugline);
		// split
		size_t delimidx = bugline.find_first_of(',');
		std::string modname = bugline.substr(0, delimidx);
		uint64_t line = std::atoi(bugline.substr(delimidx+1).data());
		bugs[modname].emplace(line);
		bugline.clear();
	}
	bFile.close();
}


int main(int argc, char** argv)
{
	// This boilerplate provides convenient stack traces and clean LLVM exit
	// handling. It also initializes the built in support for convenient
	// command line option handling.
	sys::PrintStackTraceOnErrorSignal(argv[0]);
	llvm::PrettyStackTraceProgram X(argc, argv);
	llvm_shutdown_obj shutdown;
	cl::HideUnrelatedOptions(tokenProfilerCategory);

	InitializeAllTargets();
	InitializeAllTargetMCs();
	InitializeAllAsmPrinters();
	InitializeAllAsmParsers();
	cl::AddExtraVersionPrinter(TargetRegistry::printRegisteredTargetsForVersion);
	cl::ParseCommandLineOptions(argc, argv);

	// Construct an IR file from the filename passed on the command line.
	SMDiagnostic err;
	LLVMContext context;
	std::vector<string> objectFiles;
	BUG_REPO bugs;
	if (!bugFile.empty())
	{
		parseForBugs(bugs);
	}

	tokenprofiling::TOKEN_REPO tok_repo;
	for (auto& inVal : inPath)
	{
		unique_ptr<Module> module = parseIRFile(inVal, err, context);
		if (!module.get()) {
			errs() << "Error reading bitcode file: " << inVal << "\n";
			err.print(argv[0], errs());
			return -1;
		}
		analyzeModule(*module, bugs, tok_repo);
	}

	std::unordered_map<uint64_t, uint64_t> freq;
	tok_repo.frequent_map(freq, 3);
	std::ofstream tv_out("token_vector.csv", std::ofstream::out);

	for (tokenprofiling::TOK_INFO& info : token_vectors)
	{
		info.prune(freq);

		if (false == info.vecs_.empty())
		{
			info.print(tv_out);
		}
	}

	std::ofstream encoding_out("token_encoding.csv", std::ofstream::out);
	if (encoding_out.good())
	{
		std::vector<signed> indices(freq.size(), -1);
		for (auto it : freq)
		{
			indices[it.second] = it.first;
		}
		for (signed i : indices)
		{
			std::string tkstr = tok_repo.tokens_[i];
			encoding_out << tkstr << std::endl;
		}
	}
	encoding_out.close();

	return 0;
}
