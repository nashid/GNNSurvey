import pandas as pd
import numpy as np
import sys
from sklearn import tree
from sklearn.model_selection import KFold, train_test_split
import sklearn.metrics
import DBN
def main(project):
	DBN.test_DBN(project=proj,flag='within')
	DBN.predict(project=proj,flag='within')	
	tr = pd.read_csv(str(project)+'_semantic_features.csv').as_matrix()
	np.random.shuffle(tr)
	labels = tr[:,-1].astype('int32')
	x = tr[:,:-2].astype('float32') #fill in tr indexes!
	X_train, X_test, labels_train, labels_test = train_test_split(x,labels, test_size=0.33, 
									random_state=42)
	MIN_K = 1
	MAX_K = 10
	K_RANGE = 1
	N_SPLITS = 10

	kfold = KFold(n_splits = N_SPLITS, shuffle = True)
	ks = np.arange(MIN_K, MAX_K, K_RANGE)

	for k in ks:
		score = 0
		for train, test in kfold.split(X_train, labels_train):
			lr = tree.DecisionTreeClassifier()
			lr.fit(X_train[train], labels_train[train])
			score = score + np.sum(np.equal(labels_train[test], lr.predict(X_train[test])))
		
		print "Decision Tree Cross-Validation Score:", (score/float(test.shape[0]*N_SPLITS))
	lr.fit(X_test,labels_test)
	score = lr.score(X_test,labels_test)
		
	y_pred = lr.predict(X_test)
	#print(y_pred)
	print "Decision Tree Test Score:", score
	
	print "Decision Tree Precision Score: ", sklearn.metrics.precision_score(labels_test, y_pred)
	print "Decision Tree Recall Score:", sklearn.metrics.recall_score(labels_test, y_pred)
	print "Decision Tree F1 Score: ", sklearn.metrics.f1_score(labels_test, y_pred)
	
if __name__ == '__main__':
	if len(sys.argv) > 1:
		proj = sys.argv[1]
		main(proj)
	else:
		print('Please use this script with a project name! ie jansson or pugixml')
