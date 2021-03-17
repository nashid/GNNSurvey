# defector
defect prediction using DBN generated semantic features by https://ece.uwaterloo.ca/~lintan/publications/deeplearn-icse16.pdf

Defect prediction extracts token vectors from a project's source code.

For each training file with lines of code labelled buggy or clean, color each token vector by the corresponding state.

Feed the token vectors through Deep Belief Network to obtain the feature vectors.

# Dataset

We choose open source projects and artificially insert defects.

For simplicity, we will directly insert defects into our token vectors.

# Token profiler

Token profiler takes a buggy instruction in csv and a series of files and outputs two csv files:
1. `token_vector.csv`: file with each line mapping token integer value to its label
2. `token_encoding.csv`: file with each line containing the token label corresponding to the integer value represented by the line number

## Usage

> <path/to/defector>/bin/tokenprofiler -b "bug.csv" <sample>.ll ...

bug.csv is optional but must specify defects in the following format:

<\Filename>, <\Line #>

## Token Vector Processing

### Pruning

We remove all token vectors of size 1, since these vectors are most likely just function declaration and are independent of other internal functions

We remove all tokens used less than 3 times across all projects

We remove all noise identified by CLNI using edit distance supplied in https://github.com/Martinsos/edlib

### Inject Faults (Experiment)

We will not specify a bug file `bug.csv`. Instead, we inject faults into token vectors

Randomly select non-pruned vectors and inject specific defective tokens 
(defects related to program structure), or not inject tokens (defects related to syntax and operation), or simply mark specific existing as defective.

### Defect Types

Specified from (http://www.cs.kent.edu/~jmaletic/cs63901/forms/DefectTypes.pdf)

Not captured by Tokens
- Documentation

- Build/Package

- Assignment

Not easily captured by Tokens

- Data

- Interface

Captured by Tokens

- Syntax/Static

- Checking (if statements)

- Function

- System

- Environment

Mutation Operations:

- Interface and functions defective by mutation

- Artificially label checks as defective

- Remove if and else to simulate lack of checks

- Add/remove system/environment/internal calls

- Swap calls to simulate incorrect data

# Tests

Tests chosen are ideally high quality (high coverage and build passing). 
This criteria is to ensure minimum initial defects which makes it more likely that injected defects are the only defects.

## Jansson

Decoding and manipulating JSON Data - https://github.com/akheron/jansson

How to analyze: 

- include <stdlib.h> to dump.c for non-linux OS

- build to generate include files from their config

## pugixml

C++ XML Parser - https://github.com/zeux/pugixml
