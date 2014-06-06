/**
 * CMA-ES, Covariance Matrix Adaptation Evolution Strategy
 * Copyright (c) 2014 Inria
 * Author: Emmanuel Benazera <emmanuel.benazera@lri.fr>
 *
 * This file is part of libcmaes.
 *
 * libcmaes is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libcmaes is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libcmaes.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cmaes.h"
#include <fstream>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <cstdlib>
#include <iostream>

using namespace libcmaes;

void tokenize(const std::string &str,
	      std::vector<std::string> &tokens,
	      const std::string &delim)
{
  
  // Skip delimiters at beginning.
  std::string::size_type lastPos = str.find_first_not_of(delim, 0);
  // Find first "non-delimiter".
  std::string::size_type pos = str.find_first_of(delim, lastPos);
  while (std::string::npos != pos || std::string::npos != lastPos)
    {
      // Found a token, add it to the vector.
      tokens.push_back(str.substr(lastPos, pos - lastPos));
      // Skip delimiters.  Note the "not_of"
      lastPos = str.find_first_not_of(delim, pos);
      // Find next "non-delimiter"
      pos = str.find_first_of(delim, lastPos);
    }
}

// load dataset into memory
void load_mnist_dataset(const std::string &filename,
			const int &n,
			dMat &features, dMat &labels)
{
  // matrices for features and labels, examples in col 
  features.resize(784,n);
  labels.resize(1,n);
  std::ifstream fin(filename);
  std::string line;
  std::getline(fin,line); // bypass first line
  int ne = 0;
  while(std::getline(fin,line)
	&& ne < n)
    {
      //std::cout << "line: " << line << std::endl;
      std::vector<std::string> strvalues;
      tokenize(line,strvalues,",");
      std::vector<double> values;
      for (size_t i=0;i<strvalues.size();i++)
	values.push_back(strtod(strvalues.at(i).c_str(),NULL));
      labels(0,ne) = values.at(0);
      for (size_t i=1;i<values.size();i++)
	features(i-1,ne) = values.at(i);
      ++ne;
    }
  fin.close();
  if (n > ne)
    {
      features.resize(784,ne);
      labels.resize(1,ne);
    }
}

// build the network
//- input layer
//- hidden layer
//- top layer + loss function.
class nn
{
public:
  nn()
  {};
  
  nn(const std::vector<int> &lsizes)
    :_lsizes(lsizes)
  {
    for (size_t i=0;i<_lsizes.size()-1;i++)
      {
	_lweights.push_back(dMat::Random(_lsizes.at(i),_lsizes.at(i+1)));
	_lb.push_back(dVec::Random(_lsizes.at(i)));
	_allparams_dim += _lweights.at(i).size() + _lsizes.at(i);
      }
  }
  
  ~nn() {};

  static dMat sigmoid(const dMat &M)
  {
    dMat expM(M.rows(),M.cols());
    for (int i=0;i<M.rows();i++)
      {
	for (int j=0;j<M.cols();j++)
	  {
	    expM(i,j) = exp(-M(i,j));
	  }
      }
    dMat denom = expM + dMat::Constant(M.rows(),M.cols(),1);
    return denom.cwiseInverse();
  }

  static dMat softmax(const dMat &M)
  {
    dMat expM(M.rows(),M.cols());
    for (int i=0;i<M.rows();i++)
      {
	for (int j=0;j<M.cols();j++)
	  {
	    expM(i,j) = exp(M(i,j));
	  }
      }
    dMat sums = expM.colwise().sum();
    // div by row vector.
    for (int i=0;i<expM.cols();i++)
      {
	for (int j=0;j<expM.rows();j++)
	  {
	    expM(j,i) /= sums(0,i);
	  }
      }
    return expM;
  }
  
  // forward pass over all features at once.
  void forward_pass(const dMat &features,
		    const dMat &labels)
  {
    to_matrices();
    dMat lfeatures;
    for (size_t i=0;i<_lweights.size();i++)
      {
	dMat activation;
	if (i == 0)
	  activation = (_lweights.at(i) * features).colwise() + _lb.at(i);
	else activation = (_lweights.at(i) * lfeatures).colwise() + _lb.at(i);
	if (i != _lweights.size()-1)
	  sigmoid(lfeatures);
	else softmax(lfeatures);
      }
    
    // loss.
    if (labels.size() > 0) // training mode.
      {
	dMat delta = lfeatures - labels;
	_loss = delta.sum();
      }
  };

  void to_array()
  {
    _allparams.clear();
    _allparams.reserve(_allparams_dim);
    auto vit = _allparams.begin();
    for (size_t i=0;i<_lweights.size();i++)
      {
	std::copy(_lweights.at(i).data(),_lweights.at(i).data()+_lweights.at(i).size(),vit);
	vit += _lweights.at(i).size();
	std::copy(_lb.at(i).data(),_lb.at(i).data()+_lsizes.at(i),vit);
	vit += _lsizes.at(i);
      }
  }

  void to_matrices()
  {
    auto vit = _allparams.begin();
    for (size_t i=0;i<_lweights.size();i++)
      {
	std::copy(vit,vit+_lweights.at(i).size(),_lweights.at(i).data());
        vit += _lweights.at(i).size();
	std::copy(vit,vit+_lsizes.at(i),_lb.at(i).data());
	vit += _lsizes.at(i);
      }
  }
  
  std::vector<int> _lsizes; /**< layer sizes. */
  std::vector<dMat> _lweights; /**< weight matrice, per layer. */
  std::vector<dVec> _lb;
  unsigned int _allparams_dim = 0;
  std::vector<double> _allparams; /**< all parameters, flat representation. */
  double _loss = std::numeric_limits<double>::max(); /**< current loss. */
};

// global nn variables etc...
dMat gfeatures(784,100);
dMat glabels(1,100);
nn gmnistnn;

// objective function
FitFunc nn_of = [](const double *x, const int N)
{
  std::copy(x,x+N,gmnistnn._allparams.begin()); // beware.
  gmnistnn.forward_pass(gfeatures,glabels);

  //debug
  std::cout << "net loss: " << gmnistnn._loss << std::endl;
  //debug
  
  return gmnistnn._loss;
};

DEFINE_string(fdata,"train.csv","name of the file that contains the training data for MNIST");
DEFINE_int32(n,100,"max number of examples to train from");

//TODO: train with batches.
int main(int argc, char *argv[])
{
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  FLAGS_logtostderr=1;
  google::SetLogDestination(google::INFO,"");
  load_mnist_dataset(FLAGS_fdata,FLAGS_n,gfeatures,glabels);

  //debug
  /*std::cout << "gfeatures: " << gfeatures << std::endl;
    std::cout << "glabels: " << glabels << std::endl;*/
  //debug
  
  //double minloss = 1e-3; //TODO.
  int maxsolveiter = 1e4;
  //int lambda = 1e3;
  std::vector<int> lsizes = {784, 10, 10};
  gmnistnn = nn(lsizes);

  CMAParameters<> cmaparams(gmnistnn._allparams_dim);//,lambda);
  cmaparams._max_iter = maxsolveiter;
  CMASolutions cmasols = cmaes<>(nn_of,cmaparams);
  std::cout << "status: " << cmasols._run_status << std::endl;
}