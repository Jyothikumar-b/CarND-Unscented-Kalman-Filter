#include <iostream>
#include "tools.h"

using Eigen::VectorXd;
using Eigen::MatrixXd;
using std::vector;

Tools::Tools() {}

Tools::~Tools() {}

VectorXd Tools::CalculateRMSE(const vector<VectorXd> &estimations,
                              const vector<VectorXd> &ground_truth) {
  /**
  TODO:
    * Calculate the RMSE here.
  */
  VectorXd rmse= VectorXd(4);
  rmse.fill(0.0);

  if(estimations.size()==0){
	    cout<<"Estimation Size = 0";
	    return rmse;
	}
	//  * the estimation vector size should equal ground truth vector size
	if(estimations.size()!=ground_truth.size()){
	    cout<<"size mismatch";
	    return rmse;
	}
	// ... your code here

	//accumulate squared residuals
	VectorXd residual(4);
	for(int i=0; i < estimations.size(); ++i){
        residual<<(estimations[i]-ground_truth[i]).array().pow(2);
        rmse=rmse+residual;
	}

	//calculate the mean
    int	n=estimations.size();
	rmse<<rmse*(1.0/n);

	//calculate the squared root
	rmse<<rmse.array().pow(0.5);

	//return the result
	cout<<"RMSE Calculated"<<endl;
	return rmse;
}
