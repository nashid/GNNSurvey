# defector

Motivated by DBN approach for defect prediction proposed by Wang, Liu, and Tan (https://ece.uwaterloo.ca/%7Elintan/publications/deeplearn-icse16.pdf), defector targets C files using llvm to obtain tokens.

Token vectors are still listed in topographic ordering. However, unlike Java, C files can consist of many times more lines of code which makes defect targeting difficult should we choose per file labelling.

Instead, good and bad code regions are labelled per dependency branch. That is, a token vector is constructed for each set of methods/class.

## Problems

C++ is a multi-paradigm language: It can be a procedural, functional, or object-oriented language. 
This means dependency and responsibility must be defined differently for each paradigm.

In object oriented languages, classes hold the responsibility to verify input and are ultimately responsible for all internal defects. 
So if an instruction in a class is defective, then the whole class can be safely labelled as defective.

Similarly, in function languages, functions are each responsible for their inputs and by extension their defects.

Our goal is to restrict each vector token to only represent regions of high interdependency given its intended paradigm.

One method of defect labeling is to identify a defective block or instruction and statically propagate backwards in its data dependency graph until we reach a pointer where dependency is unknown.
We don't care about control dependence, because the defect is present regardless whether or not it's executed. This idea is similar to taint analysis, except we're restricting how far from the defect we look. 
That is, instead of analyzing input dependency of the entire system, we only care about dependence within some API.

nodes are extracted as tokens: 

1. nodes of method invocations and class instance creations

2. declaration nodes, i.e., method declarations, type declarations,
and enum declarations,

3. control-flow nodes such as while statements, catch clauses, if statements, throw statements, etc.

## Areas of improvement

Implement symbolic execution to sufficiently label operational tokens