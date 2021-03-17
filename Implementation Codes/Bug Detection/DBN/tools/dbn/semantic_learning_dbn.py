from __future__ import division

import time

import numpy as np
np.random.seed(1234)
srng_seed = np.random.randint(2**30)

from keras.models import Sequential
from keras.optimizers import SGD
from keras_extensions.initializers import glorot_uniform_sigm

from keras_extensions.logging import log_to_file
from keras_extensions.rbm import GBRBM, RBM
from keras_extensions.dbn import DBN
from keras_extensions.layers import SampleBernoulli

#configuration
input_dim = 0
#hidden_dim = 0
internal_dim = 100
batch_size = 32 #choose 32 as it is a common choice and should be fine here
nb_epoch = 200
lr = 0.0001
momentum_schedule = [(0, 0.5), (5,0.9)]

@log_to_file('example.log')
def main():
	#grab input data set and set up dataset here
	X_train = []
	X_test = []
	print('Creating training model')
	#start with a GBRBM and then followed by 5 more RBMs for 5*2 = 10 hidden layers
	dbn = DBN([
		GBRBM(input_dim, internal_dim, init=glorot_uniform_sigm),
		RBM(internal_dim, internal_dim, init=glorot_uniform_sigm),
		RBM(internal_dim, internal_dim, init=glorot_uniform_sigm),
		RBM(internal_dim, internal_dim, init=glorot_uniform_sigm),
		RBM(internal_dim, internal_dim, init=glorot_uniform_sigm),
		RBM(internal_dim, internal_dim, init=glorot_uniform_sigm)
	])	

	def get_layer_loss(rbm,layer_no):
		return rbm.contrastive_divergence_loss(nb_gibbs_steps=1)

	def get_layer_optimizer(layer_no):
		return SGD((layer_no+1)*lr, 0., decay=0.0, nesterov=False)
	
	dbn.compile(layer_optimizer=get_layer_optimizer, layer_loss=get_layer_loss)

	#Train
	#train off token vectors from early version of software
	print('Training')
	begin_time = time.time()

	dbn.fit(X_train,batch_size,nb_epoch,verbose=1,shuffle=False)
	
	end_time = time.time()
	print('Training took %f minutes' % ((end_time - begin_time)/60.0))

	#save model parameters from training
	print('Saving model')
	dbn.save_weights('dbn_weights.hdf5', overwrite=True)
	
	#load model  from save
	print('Loading model')
	dbn.load_weights('dbn_weights.hdf5')

	#generate hidden features from input data
	print('Creating inference model')
	F = dbn.get_forward_inference_layers()
	B = dbn.get_backwards_inference_layers()
	inference_model = Sequential()
	for f in F:
		inference_model.add(f)
		inference_model.add(SampleBernoulli(mode='random'))
	for b in B[:-1]:
		inference_model.add(b)
		inference_model.add(SampleBernoulli(mode='random'))
	#last layer is a gaussian layer
	inference_model.add(B[-1])
	
	print('Compiling Theano graph')
	opt = SGD()
	inference_model.compile(opt, loss='mean_squared_error')

	print('Doing inference')
	h = inference_model.predict(X_test)

if __name__ == '__main__':
	main()
	
