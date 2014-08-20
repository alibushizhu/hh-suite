/*
 * HHblits.h
 *
 *  Created on: Apr 1, 2014
 *      Author: meiermark
 */

#ifndef HHBLITS_H_
#define HHBLITS_H_

#include <iostream>
#include <fstream>
#include <cstdio>
#include <algorithm>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <vector>
#include <math.h>
#include <limits.h>
#include <float.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <cassert>
#include <stdexcept>
#include <map>
#include <omp.h>

#include <emmintrin.h>

using std::cout;
using std::cerr;
using std::endl;
using std::ios;
using std::ifstream;
using std::ofstream;
using std::string;
using std::stringstream;
using std::vector;
using std::pair;

#include <sys/time.h>

extern "C" {
#include <ffindex.h>
}

#include "cs.h"
#include "context_library.h"
#include "library_pseudocounts-inl.h"
#include "crf_pseudocounts-inl.h"
#include "abstract_state_matrix.h"

#include "hhdecl.h"
#include "list.h"
#include "hash.h"
#include "util.h"
#include "hhutil.h"

#include "hhhmm.h"
#include "hhhit.h"
#include "hhalignment.h"
#include "hhhalfalignment.h"
#include "hhfullalignment.h"
#include "hhhitlist.h"

#include "hhmatrices.h"
#include "hhfunc.h"

#include "hhdatabase.h"

#include "hhprefilter.h"

class HHblits;

#include "log.h"
#include "hhhmmsimd.h"
#include "hhviterbimatrix.h"
#include "hhviterbirunner.h"

#include "hhposteriormatrix.h"
#include "hhposteriordecoderinputdata.h"
#include "hhposteriordecoderrunner.h"

const char HHBLITS_REFERENCE[] =
		"Remmert M., Biegert A., Hauser A., and Soding J.\nHHblits: Lightning-fast iterative protein sequence searching by HMM-HMM alignment.\nNat. Methods 9:173-175 (2011)\n";

class HHblits {
public:
	HHblits(Parameters& parameters, std::vector<HHblitsDatabase*>& databases);
	virtual ~HHblits();

	void Reset();

	static void ProcessAllArguments(int argc, char** argv, Parameters& par);

    //writer for non-mpi version
    void writeHHRFile(char* hhrFile);
    void writeAlisFile(char* basename);
    void writeScoresFile(char* scoresFile);
    void writePairwiseAlisFile(char* pairwieseAlisFile, char outformat);
    void writeAlitabFile(char* alitabFile);
    void writeOptimizedHHRFile(char* reduceHHRFile);
    void writePsiFile(char* psiFile);
    void writeHMMFile(char* HMMFile);
    void writeA3MFile(char* A3MFile);
    void writeMatricesFile(char* matricesOutputFileName);

    //output writer for mpi version
    std::map<int, Alignment*>& getAlis();
    static void writeHHRFile(HHblits& hhblits, std::stringstream& out);
    static void writeScoresFile(HHblits& hhblits, std::stringstream& out);
    static void writePairwiseAlisFile(HHblits& hhblits, std::stringstream& out);
    static void writeAlitabFile(HHblits& hhblits, std::stringstream& out);
    static void writeOptimizedHHRFile(HHblits& hhblits, std::stringstream& out);
    static void writePsiFile(HHblits& hhblits, std::stringstream& out);
    static void writeHMMFile(HHblits& hhblits, std::stringstream& out);
    static void writeA3MFile(HHblits& hhblits, std::stringstream& out);
    static void writeMatricesFile(HHblits& hhblits, stringstream& out);

    static void prepareDatabases(Parameters& par, std::vector<HHblitsDatabase*>& databases);

	void run(FILE* query_fh, char* query_path);

protected:
	// substitution matrix flavours
	float __attribute__((aligned(16))) P[20][20];
	float __attribute__((aligned(16))) R[20][20];
	float __attribute__((aligned(16))) Sim[20][20];
	float __attribute__((aligned(16))) S[20][20];
	float __attribute__((aligned(16))) pb[21];
	float __attribute__((aligned(16))) qav[21];

	// secondary structure matrices
	float S73[NDSSP][NSSPRED][MAXCF];
	float S33[NSSPRED][MAXCF][NSSPRED][MAXCF];

	Parameters par;

	cs::ContextLibrary<cs::AA>* context_lib = NULL;
	cs::Crf<cs::AA>* crf = NULL;
	cs::Pseudocounts<cs::AA>* pc_hhm_context_engine = NULL;
	cs::Admix* pc_hhm_context_mode = NULL;
	cs::Pseudocounts<cs::AA>* pc_prefilter_context_engine = NULL;
	cs::Admix* pc_prefilter_context_mode = NULL;

	// HHblits variables
    // verbose mode

	// Set to 1 if input in HMMER format (has already pseudocounts)
	char config_file[NAMELEN];

	//database filenames
	std::vector<HHblitsDatabase*> dbs;

	// Create query HMM with maximum of par.maxres match states
	HMM* q = NULL;
	// Create query HMM with maximum of par.maxres match states (needed for prefiltering)
	HMM* q_tmp = NULL;

	// output A3M generated by merging A3M alignments for significant hits to the query alignment
	Alignment Qali;
	// output A3M alignment with no sequence filtered out (only active with -all option)
	Alignment Qali_allseqs;

	ViterbiMatrix* viterbiMatrices[MAXBINS];
	PosteriorMatrix* posteriorMatrices[MAXBINS];

	HitList hitlist; // list of hits with one Hit object for each pairwise comparison done
	HitList optimized_hitlist;
	std::map<int, Alignment*> alis;

	void perform_realign(HMMSimd& q_vec, const char input_format, std::vector<HHEntry*>& hits_to_realign, const int premerge, Hash<char>* premerged_hits);
	void mergeHitsToQuery(Hash<Hit>* previous_hits, Hash<char>* premerged_hits, int& seqs_found, int& cluster_found);
	void add_hits_to_hitlist(std::vector<Hit>& hits, HitList& hitlist);
	void optimizeQSC(std::vector<HHEntry*>& selected_entries,
			const int N_searched, HMMSimd& q_vec, char query_input_format,
			HitList& output_list);
	void get_entries_of_selected_hits(HitList& input, std::vector<HHEntry*>& selected_entries);
	void get_entries_of_all_hits(HitList& input, std::vector<HHEntry*>& selected_entries);


private:
	static void help(Parameters& par, char all = 0);
	static void ProcessArguments(int argc, char** argv, Parameters& par);

	void RescoreWithViterbiKeepAlignment(HMMSimd& q_vec, Hash<Hit>* previous_hits);

//	//redundancy filter
//    void wiggleQSC(HitList& hitlist, int n_redundancy, Alignment& Qali,
//            char inputformat, float* qsc, size_t nqsc,
//            HitList& reducedFinalHitList);
//    void wiggleQSC(int n_redundancy, float* qsc, size_t nqsc, HitList& reducedFinalHitList);
//	void reduceRedundancyOfHitList(int n_redundancy, int query_length,
//			HitList& hitlist, HitList& reducedHitList);
//	void recalculateAlignmentsForDifferentQSC(HitList& hitlist, Alignment& Qali,
//			char inputformat, float* qsc, size_t nqsc,
//			HitList& recalculated_hitlist);
//	void uniqueHitlist(HitList& hitlist);
};

#endif /* HHBLITS_H_ */
