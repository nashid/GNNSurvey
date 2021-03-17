Defector DBN

This directory contains the scripts to train a DBN from the token vectors produced in tokenprofiler to generate semantic features and then see how they perform in classifying whether or not a given method/class is buggy or clean. Currently, only the basic DBN and Logistic Regression classfier used in the source paper are present.
##Dependencies:
	- Keras (change back-end in config file if you want to use Theano(GPU) or TensorFlow(CPU))
	- Keras_extensions : clone the GitHub repository and follow the installation instructions
	- scikit-learn


