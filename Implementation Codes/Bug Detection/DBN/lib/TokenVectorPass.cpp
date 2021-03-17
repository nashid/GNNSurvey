//
// Created by Mingkai Chen on 2017-04-10.
//


#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/GraphWriter.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/IR/CallSite.h"
#include "llvm/Pass.h"

#include <experimental/optional>
#include <fstream>

#include "TokenVectorPass.hpp"


using namespace std::experimental;
using namespace llvm;
using namespace tokenprofiling;


namespace tokenprofiling
{
	char TokenVectorPass::ID = 0;
}


static optional<unsigned> getLineNumber(Module& m, Instruction& inst)
{
	const DebugLoc& loc = inst.getDebugLoc();
	optional<unsigned> line;
	if (loc) {
		line = loc.getLine(); // line numbers start from 1
	}
	return line;
}


void TokenVectorPass::storeToken(uint64_t func_id, std::string label)
{
	if (nullptr == tok_repo_) return;
	if (label.empty()) return;
	uint64_t tk;
	auto it = tok_repo_->token_map_.find(label);
	if (it == tok_repo_->token_map_.end())
	{
		tk = tok_repo_->token_map_[label] = last_token++;
		tok_repo_->tokens_.push_back(label);
		tok_repo_->usage_.push_back(1);
	}
	else
	{
		tk = it->second;
		tok_repo_->usage_[tk]++;
	}

	tok_vector_[func_id].push_back(tk);
}


uint64_t TokenVectorPass::setFuncId(const std::string& func_name)
{
	uint64_t fid;
	auto it = func_ids_.find(func_name);
	if (it == func_ids_.end())
	{
		fid = func_ids_[func_name] = func_ids_.size();
	}
	else
	{
		fid = it->second;
	}
	return fid;
}


void TokenVectorPass::tokenSetup (llvm::Module& module, std::unordered_set<uint64_t>& err_funcs,
	std::unordered_map<std::string, std::unordered_set<uint64_t> >& immediate_dependents,
	std::unordered_set<std::string>& structs)
{
	std::string modname = module.getSourceFileName();
	std::unordered_set<uint64_t>& err_lines = bugs_[modname];
	for (auto& f : module)
	{
		optional<uint64_t> struct_id;
		Function::ArgumentListType& args = f.getArgumentList();
		if (args.size() > 0)
		{
			Argument& a = args.front();
			std::string astr;
			llvm::raw_string_ostream rso(astr);
			a.print(rso);
			size_t ptridx = astr.find_first_of('*');
			if (ptridx != std::string::npos)
			{
				astr = astr.substr(1, ptridx-1);
				if (structs.find(astr) != structs.end())
				{
					auto it = func_ids_.find(astr);
					if (it == func_ids_.end())
					{
						struct_id = func_ids_[astr] = func_ids_.size();
						storeToken(struct_id.value(), "declare("+astr+")");
					}
					else
					{
						struct_id = it->second;
					}
				}
			}
		}
		if (f.isDeclaration())
		{
			// mark declaration as a token
			std::string declname = f.getName().str();
			if (0 != declname.find("llvm.") &&
				0 != declname.find("__"))
			{
				methodClean(declname);
				storeToken(setFuncId(declname), "declare("+declname+")");
			}
			continue;
		}
		LoopInfo& LI = getAnalysis<LoopInfoWrapperPass>(f).getLoopInfo();
		std::string funcName = f.getName().str();
		uint64_t fid = setFuncId(funcName);
		std::unordered_set<BasicBlock*> potential_else;
		if ((bool) struct_id)
		{
			immediate_dependents[funcName].emplace(struct_id.value());
		}
		for (auto& bb : f)
		{
			if (potential_else.end() != potential_else.find(&bb))
			{
				potential_else.erase(&bb);
				storeToken(fid, "else");
			}
			for (auto& i : bb)
			{
				// check for method calls, control flow statements, allocations
				CallSite cs(&i);
				if (auto* instr = cs.getInstruction())
				{
					if (auto directCall = dyn_cast<llvm::Function>(cs.getCalledValue()->stripPointerCasts()))
					{
						std::string callname = directCall->getName().str();
						if (0 == callname.find("llvm.") ||
							0 == callname.find("__")) continue;
						methodClean(callname);
						storeToken(fid, callname);
						// this function has a dependency on callee function, record dependence
						immediate_dependents[callname].emplace(fid);
					}
					else
					{
						// this function has a dependency on callee function, but can't record dependence
						std::string instname = instr->getName().str();
						methodClean(instname);
						storeToken(fid, instname);
					}
				}
				else if (BranchInst* bi = llvm::dyn_cast<BranchInst>(&i))
				{
					if (bi->isConditional()) // loop or if
					{
						bool loops = false;
						Loop* ourloop = LI.getLoopFor(&bb);
						for (auto succs : successors(&bb))
						{
							Loop* theirloop = LI.getLoopFor(succs);
							if (theirloop != ourloop)
							{
								loops = true;
								break;
							}
						}
						if (loops)
						{
							storeToken(fid, "while");
						}
						else
						{
							storeToken(fid, "if");
							if (bi->getNumSuccessors() > 1)
							{
								potential_else.emplace(bi->getSuccessor(1));
							}
						}
					}
					else
					{
						for (size_t i = 0; i < bi->getNumSuccessors(); i++)
						{
							auto it = potential_else.find(bi->getSuccessor(i));
							if (potential_else.end() != it)
							{
								potential_else.erase(it);
							}
						}
					}
				}
				else if (InvokeInst* ii = llvm::dyn_cast<InvokeInst>(&i))
				{
					std::string invname = ii->getName().str();
					if (invname.find("throw") != std::string::npos)
					{
						storeToken(fid, "throw");
					}
				}
				else if (llvm::dyn_cast<LandingPadInst>(&i))
				{
					storeToken(fid, "catch");
				}
				else if (llvm::dyn_cast<SwitchInst>(&i))
				{
					storeToken(fid, "switch");
				}
				else if (llvm::dyn_cast<UnreachableInst>(&i))
				{
					storeToken(fid, "unreachable");
				}
				else if (llvm::dyn_cast<AllocaInst>(&i) || llvm::dyn_cast<LoadInst>(&i))
				{
					std::string typestr;
					switch(i.getType()->getTypeID())
					{
						case llvm::Type::TypeID::VoidTyID:
							break;
						case llvm::Type::TypeID::HalfTyID:
							typestr = "16bit";
							break;
						case llvm::Type::TypeID::FloatTyID:
							typestr = "32bit";
							break;
						case llvm::Type::TypeID::DoubleTyID:
							typestr = "64bit";
							break;
						case llvm::Type::TypeID::X86_FP80TyID:
							typestr = "80bit";
							break;
						case llvm::Type::TypeID::FP128TyID:
						case llvm::Type::TypeID::PPC_FP128TyID:
							typestr = "128bit";
							break;
						case llvm::Type::TypeID::IntegerTyID:
							typestr = "integer";
							break;
						case llvm::Type::TypeID::FunctionTyID:
							typestr = "function";
							break;
						case llvm::Type::TypeID::StructTyID:
							typestr = "struct";
							break;
						case llvm::Type::TypeID::ArrayTyID:
							typestr = "array";
							break;
						case llvm::Type::TypeID::PointerTyID:
							typestr = "pointer";
							break;
						case llvm::Type::TypeID::VectorTyID:
							typestr = "vector";
							break;
						case llvm::Type::TypeID::LabelTyID:
						case llvm::Type::TypeID::MetadataTyID:
						case llvm::Type::TypeID::X86_MMXTyID:
						case llvm::Type::TypeID::TokenTyID:
							break;
					}
					if (!typestr.empty())
					{
						storeToken(fid, typestr);
					}
				}

				optional<unsigned> lineno = getLineNumber(module, i);
				if ((bool) lineno)
				{
					// check for bug
					auto it = err_lines.find(lineno.value());
					if (err_lines.end() != it)
					{
						err_funcs.emplace(fid);
					}
				}
			}
		}
	}
}


bool TokenVectorPass::runOnModule(llvm::Module& module)
{
	// associate structs with labels
	std::unordered_set<std::string> structs;
	for (auto* s : module.getIdentifiedStructTypes())
	{
		std::string sname = s->getName().str();
		structs.emplace(sname);
	}

	std::unordered_set<uint64_t> err_funcs;
	std::unordered_map<std::string, std::unordered_set<uint64_t> > immediate_dependents;
	tokenSetup(module, err_funcs, immediate_dependents, structs);

	// todo: merge dependent functions' token vectors
	std::vector<std::vector<uint64_t> > dependencies;
	{
		std::vector<std::unordered_set<uint64_t> > clusters;
		std::vector<signed> clusteridx(func_ids_.size(), -1);
		for (auto call_rel : immediate_dependents)
		{
			std::string calleestr = call_rel.first;
			auto it = func_ids_.find(calleestr);
			// external calls are ignored
			if (func_ids_.end() == it) continue;

			uint64_t callee = it->second;
			signed eeidx = clusteridx[callee];
			for (uint64_t caller : call_rel.second)
			{
				signed eridx = clusteridx[caller];
				if (eeidx < 0 && eridx < 0)
				{
					clusteridx[callee] = clusteridx[caller] = clusters.size();
					clusters.push_back({caller, caller});
				}
				else if (eeidx < 0)
				{
					clusteridx[callee] = eridx;
					clusters[eridx].emplace(callee);
				}
				else if (eridx < 0)
				{
					clusteridx[caller] = eeidx;
					clusters[eeidx].emplace(caller);
				}
				else if (eeidx != eridx)
				{
					if (clusters[eeidx].size() > clusters[eridx].size())
					{
						for (uint64_t ers : clusters[eridx])
						{
							clusteridx[ers] = eeidx;
						}
						clusters[eeidx].insert(clusters[eridx].begin(), clusters[eridx].end());
					}
					else
					{
						for (uint64_t ees : clusters[eeidx])
						{
							clusteridx[ees] = eridx;
						}
						clusters[eridx].insert(clusters[eeidx].begin(), clusters[eeidx].end());
					}
				}
			}
		}
		std::unordered_set<signed> cidx_set;
		for (signed cidx : clusteridx)
		{
			cidx_set.emplace(cidx);
		}
		for (signed cidx : cidx_set)
		{
			if (cidx < 0) continue;
			std::vector<uint64_t> depset(clusters[cidx].begin(), clusters[cidx].end());
			std::sort(depset.begin(), depset.end());
			dependencies.push_back(depset);
		}
	}

	std::unordered_set<uint64_t> painted;
	for (std::vector<uint64_t>& clust : dependencies)
	{
		std::vector<uint64_t> vecs;
		bool buggy = false;
		for (uint64_t func : clust)
		{
			painted.emplace(func);
			if (err_funcs.end() != err_funcs.find(func)) buggy = true;
			std::vector<uint64_t>& tvec = tok_vector_[func];
			vecs.insert(vecs.end(), tvec.begin(), tvec.end());
		}
		token_vectors_.push_back({module.getName().str(), buggy, vecs});
	}
	for (auto tvit : tok_vector_)
	{
		uint64_t func = tvit.first;
		if (painted.end() == painted.find(func))
		{
			token_vectors_.push_back({module.getName().str(),
			err_funcs.end() != err_funcs.find(func), tvit.second});
		}
	}

	return true;
}
