//
// Created by Mingkai Chen on 2017-04-10.
//

#ifndef DEFECTOR_TOKENVECTORPASS_HPP
#define DEFECTOR_TOKENVECTORPASS_HPP


#include "llvm/Analysis/LoopInfo.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

#include <unordered_set>
#include <unordered_map>

#include "edlib.h"


using BUG_REPO = std::unordered_map<std::string, std::unordered_set<uint64_t> >;


namespace tokenprofiling
{


struct TOKEN_REPO
{
	std::vector<std::string> tokens_;
	std::vector<size_t> usage_;
	std::unordered_map<std::string, uint64_t> token_map_;

	void frequent_map (std::unordered_map<uint64_t, uint64_t>& infreq, size_t min_uses)
	{
		uint64_t newi = 0;
		for (uint64_t i = 0, n = usage_.size(); i < n; i++)
		{
			if (usage_[i] >= min_uses)
			{
				infreq[i] = newi++;
			}
		}
	}
};


struct TOK_INFO
{
	std::string modname_;
	bool buggy_;
	std::vector<uint64_t> vecs_;

	void prune (const std::unordered_map<uint64_t, uint64_t>& frequent_toks)
	{
		std::vector<uint64_t> trash;
		for (size_t i = 0, n = vecs_.size(); i < n; i++)
		{
			if (frequent_toks.end() == frequent_toks.find(vecs_[i]))
			{
				trash.push_back(i);
			}
			else
			{

				vecs_[i] = frequent_toks.at(vecs_[i]);
			}
		}
		for (auto rit = trash.rbegin(), ret = trash.rend(); rit != ret; rit++)
		{
			vecs_.erase(vecs_.begin() + *rit);
		}
	}

	void print (std::ofstream& o)
	{
		if (1 < vecs_.size() && o.good())
		// prune tokens of size 1
		{
			// out put ttv
			o << modname_ << "," << buggy_;
			for (uint64_t tk : vecs_)
			{
				o << "," << tk;
			}
			o << std::endl;
		}
	}
};


using VEC_INFO = std::vector<TOK_INFO>;


struct TokenVectorPass : public llvm::ModulePass
{
	static char ID;

	BUG_REPO bugs_;

	uint64_t last_token = 0;

	TOKEN_REPO* tok_repo_;
	VEC_INFO token_vectors_;

	// function name to function id (ordered topographically)
	std::unordered_map<std::string, uint64_t> func_ids_;
	// temporary token vector mapping function id to vector
	std::unordered_map<uint64_t, std::vector<uint64_t> > tok_vector_;

	// for cleaning method calls
	std::list<std::string> neighbors_;
	std::unordered_set<std::string> methods_;

	TokenVectorPass(TOKEN_REPO* repo) : llvm::ModulePass(ID), tok_repo_(repo) {}

	TokenVectorPass(BUG_REPO& bugs, TOKEN_REPO* repo) :
		llvm::ModulePass(ID), bugs_(bugs), tok_repo_(repo) {}

	void getAnalysisUsage(llvm::AnalysisUsage& au) const override
	{
		au.addRequired<llvm::DominatorTreeWrapperPass>();
		au.addRequired<llvm::LoopInfoWrapperPass>();
	}

	void methodClean (std::string& method_label)
	{
		if (method_label.empty()) return;
		size_t labelsize = method_label.size();
		short noise_potential = 0;
		// find minimal edit distance
		for (std::string neigh_lbl : neighbors_)
		{
			int dist = edlibAlign(neigh_lbl.data(), neigh_lbl.size(),
				method_label.data(), labelsize,
				edlibDefaultAlignConfig()).editDistance;
			if (dist > 40)
			{
				noise_potential++;
			}
		}
		// remove if noisy
		if (noise_potential > 5)
		{
			method_label = "";
			return;
		}
		// update neighbors
		neighbors_.push_back(method_label);
		if (neighbors_.size() > 10) // CLNI k = 10
		{
			neighbors_.pop_front();
		}
		// cull out similar tokens with absurdly long labels
		for (std::string meths : methods_)
		{
			int dist = edlibAlign(meths.data(), meths.size(),
				method_label.data(), labelsize,
				edlibDefaultAlignConfig()).editDistance;
			if (dist < 3 && dist != 0 && labelsize > 20)
			{
				method_label = meths;
				break;
			}
		}
		methods_.emplace(method_label);
	}

	bool runOnModule(llvm::Module& m) override;

	void storeToken(uint64_t func_id, std::string label);

	uint64_t setFuncId(const std::string& func_name);

	void tokenSetup (llvm::Module& module, std::unordered_set<uint64_t>& err_funcs,
		std::unordered_map<std::string, std::unordered_set<uint64_t> >& immediate_dependents,
		std::unordered_set<std::string>& structs);
};


} // namespace tokenprofiling

#endif //DEFECTOR_TOKENVECTORPASS_HPP
