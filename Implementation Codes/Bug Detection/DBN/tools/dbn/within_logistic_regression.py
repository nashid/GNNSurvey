import pandas as pd
import numpy as np
from sklearn.linear_model import LogisticRegression
from sklearn.model_selection import KFold, train_test_split

def main():
	tr = pd.read_csv('filepath.csv').as_matrix()
	np.random.shuffle(tr)
	labels = tr[:].astype('int32')
	x = tr[:].astype('float32') #fill in tr indexes!
	X_train, X_test, labels_train, labels_test = train_test_split(x,labels, test_size=0.33, 
									random_state=42)
	MIN_K = 1
	MAX_K = 10
	K_RANGE = 1
	N_SPLITS = 10

	kfold = KFold(n_splits = N_SPLITS, shuffle = True)
	ks = np.arange(MIN_K, MAX_K, K_RANGE)

	for k in ks:
		score = 0 #should also initialize true/false positive/negative counts or smth
		
		for train, test in kfold.split(X_train, labels_train):
			lr = LogisticRegression()
			lr.fit(X_train[train], labels_train[train])
			score = score + np.sum(np.equal(labels_train[test], lr.predict(X_train[test])))
		
		print "Logistic Regression Cross-Validation Score:", (score/float(test.shape[0]*N_SPLITS))
		lr.fit(X_test,labels_test)
		score = lr.score(X_test,labels_test)
		print "Logistic Regression Test Score:", score
		

if __name__ == '__main__':
	main()

