#include "ukf.h"
#include "Eigen/Dense"
#include <iostream>

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::vector;

/**
 * Initializes Unscented Kalman filter
 * This is scaffolding, do not modify
 */
UKF::UKF() {
  is_initialized_ = false;
  // if this is false, laser measurements will be ignored (except during init)
  use_laser_ = true;

  // if this is false, radar measurements will be ignored (except during init)
  use_radar_ = true;

  // initial state vector
  x_ = VectorXd(5);

  // initial covariance matrix
  P_ = MatrixXd(5, 5);

  // Process noise standard deviation longitudinal acceleration in m/s^2
  std_a_ = 1.5;

  // Process noise standard deviation yaw acceleration in rad/s^2
  std_yawdd_ = 0.5;

  //DO NOT MODIFY measurement noise values below these are provided by the sensor manufacturer.
  // Laser measurement noise standard deviation position1 in m
  std_laspx_ = 0.15;

  // Laser measurement noise standard deviation position2 in m
  std_laspy_ = 0.15;

  // Radar measurement noise standard deviation radius in m
  std_radr_ = 0.3;

  // Radar measurement noise standard deviation angle in rad
  std_radphi_ = 0.03;

  // Radar measurement noise standard deviation radius change in m/s
  std_radrd_ = 0.3;
  //DO NOT MODIFY measurement noise values above these are provided by the sensor manufacturer.

  // Weights of sigma points
  weights_=VectorXd(2*n_aug_+1);

  // State dimension
  n_x_=5;

  // Augmented state dimension
  n_aug_=7;

  // Augmented state dimension
  n_z_=3;

  // Sigma point spreading parameter
  lambda_=3-n_aug_;

  //Predicted Sigma points
  Xsig_pred= MatrixXd(n_x_, 2 * n_aug_ + 1);

  H_=MatrixXd(2, 5);
  R_lider=MatrixXd(2, 2);

  R_lider << std_laspx_*std_laspx_, 0,
                0, std_laspy_*std_laspy_;


  H_ << 1, 0, 0, 0, 0,
        0, 1, 0, 0, 0;



}

UKF::~UKF() {}

/**
 * @param {MeasurementPackage} meas_package The latest measurement data of
 * either radar or laser.
 */
void UKF::ProcessMeasurement(MeasurementPackage meas_package) {
    cout<<"Process Started->"<<endl;;
  if(!is_initialized_){
      //Initialize state vector
      x_<<1,1,0,0,1;
      //Initialize process covariance matrix
      P_<<1,0,0,0,0,
            0,1,0,0,0,
            0,0,1,0,0,
            0,0,0,1,0,
            0,0,0,0,1;
      //Initialize time stamp
      time_us_=meas_package.timestamp_;

      //Initialize first measurement
      if(meas_package.sensor_type_==MeasurementPackage::LASER){
          x_(0)=meas_package.raw_measurements_(0);
          x_(1)=meas_package.raw_measurements_(1);
      }else if(meas_package.sensor_type_==MeasurementPackage::RADAR){
          x_(0)=meas_package.raw_measurements_(0)*cos(meas_package.raw_measurements_(1));
          x_(1)=meas_package.raw_measurements_(0)*sin(meas_package.raw_measurements_(1));
      }
      is_initialized_=true;
      return;
  }else{
      double diff_t=(meas_package.timestamp_-time_us_)/1000000.0;
      time_us_=meas_package.timestamp_;
      //Prediction
      Prediction(diff_t);
      //Update
      if(meas_package.sensor_type_==MeasurementPackage::LASER){
          UpdateLidar(meas_package);
      }else if(meas_package.sensor_type_==MeasurementPackage::RADAR){
          //Prediction(diff_t);
          UpdateRadar(meas_package);
      }
  }
  cout<<"Process Stopped"<<endl;
}


/**
 * Predicts sigma points, the state, and the state covariance matrix.
 * @param {double} delta_t the change in time (in seconds) between the last
 * measurement and this one.
 */
void UKF::Prediction(double delta_t) {

  cout<<"Prediction--";
  //Calculate Augmented Sigma points
  MatrixXd Xsig_aug= MatrixXd(n_aug_, 2 * n_x_ + 1);
  AugmentedSigmaPoints(&Xsig_aug);

  //Predict the next state from augmented sigma points
  SigmaPointPrediction(&Xsig_pred,Xsig_aug,delta_t);

  //Calculate x,p for new state
  PredictMeanAndCovariance(&x_,&P_,Xsig_pred);
}

/**
 * Updates the state and the state covariance matrix using a laser measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateLidar(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Use lidar data to update the belief about the object's
  position. Modify the state vector, x_, and covariance, P_.

  You'll also need to calculate the lidar NIS.
  */
  cout<<"Lidar calculation--";
  VectorXd z =VectorXd(2);
  z<<meas_package.raw_measurements_(0),meas_package.raw_measurements_(1);

  VectorXd z_pred = H_ * x_;
	VectorXd y = z - z_pred;
	MatrixXd Ht = H_.transpose();
	MatrixXd S = H_ * P_ * Ht + R_lider;
	MatrixXd Si = S.inverse();
	MatrixXd PHt = P_ * Ht;
	MatrixXd K = PHt * Si;


	//new estimate
	x_ = x_ + (K * y);
	long x_size = x_.size();
	MatrixXd I = MatrixXd::Identity(x_size, x_size);
	P_ = (I - K * H_) * P_;
	double nis;
	nis = y.transpose()*Si*y;
	cout<<"End"<<endl;
	cout<<"LIDAR NIS :"<<nis<<endl;
}

/**
 * Updates the state and the state covariance matrix using a radar measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateRadar(MeasurementPackage meas_package) {
  //Converted the predicted value into measurement space
  cout<<"RADAR Life --";
  MatrixXd Zsig,S;
  VectorXd z_pred;
  PredictRadarMeasurement(&z_pred, &S, &Zsig);

  //Update the state vector
  UpdateState(&x_, &P_, meas_package.raw_measurements_ , Zsig, z_pred, S);
}

void UKF::AugmentedSigmaPoints(MatrixXd* Xsig_out) {

  //create augmented mean vector
  cout<<"--AugmentedSigmaPoints--";
  VectorXd x_aug = VectorXd(7);

  //create augmented state covariance
  MatrixXd P_aug = MatrixXd(7, 7);

  //create sigma point matrix
  MatrixXd Xsig_aug = MatrixXd(n_aug_, 2 * n_aug_ + 1);

  //create augmented mean state
  x_aug.head(5) = x_;
  x_aug(5) = 0;
  x_aug(6) = 0;

  //create augmented covariance matrix
  P_aug.fill(0.0);
  P_aug.topLeftCorner(5,5) = P_;
  P_aug(5,5) = std_a_*std_a_;
  P_aug(6,6) = std_yawdd_*std_yawdd_;

  //create square root matrix
  MatrixXd L = P_aug.llt().matrixL();

  //create augmented sigma points
  Xsig_aug.col(0)  = x_aug;
  for (int i = 0; i< n_aug_; i++)
  {
    Xsig_aug.col(i+1)       = x_aug + sqrt(lambda_+n_aug_) * L.col(i);
    Xsig_aug.col(i+1+n_aug_) = x_aug - sqrt(lambda_+n_aug_) * L.col(i);
  }

  //write result
  *Xsig_out = Xsig_aug;
}


void UKF::SigmaPointPrediction(MatrixXd* Xsig_out, MatrixXd Xsig_aug, double t) {

  //create matrix with predicted sigma points as columns
  cout<<"--SigmaPointPrediction--";
  MatrixXd Xsig_pred = MatrixXd(n_x_, 2 * n_aug_ + 1);

  double delta_t = t;

  //predict sigma points
  for (int i = 0; i< 2*n_aug_+1; i++)
  {
    //extract values for better readability
    double p_x = Xsig_aug(0,i);
    double p_y = Xsig_aug(1,i);
    double v = Xsig_aug(2,i);
    double yaw = Xsig_aug(3,i);
    double yawd = Xsig_aug(4,i);
    double nu_a = Xsig_aug(5,i);
    double nu_yawdd = Xsig_aug(6,i);

    //predicted state values
    double px_p, py_p;

    //avoid division by zero
    if (fabs(yawd) > 0.001) {
        px_p = p_x + v/yawd * ( sin (yaw + yawd*delta_t) - sin(yaw));
        py_p = p_y + v/yawd * ( cos(yaw) - cos(yaw+yawd*delta_t) );
    }
    else {
        px_p = p_x + v*delta_t*cos(yaw);
        py_p = p_y + v*delta_t*sin(yaw);
    }

    double v_p = v;
    double yaw_p = yaw + yawd*delta_t;
    double yawd_p = yawd;

    //add noise
    px_p = px_p + 0.5*nu_a*delta_t*delta_t * cos(yaw);
    py_p = py_p + 0.5*nu_a*delta_t*delta_t * sin(yaw);
    v_p = v_p + nu_a*delta_t;

    yaw_p = yaw_p + 0.5*nu_yawdd*delta_t*delta_t;
    yawd_p = yawd_p + nu_yawdd*delta_t;

    //write predicted sigma point into right column
    Xsig_pred(0,i) = px_p;
    Xsig_pred(1,i) = py_p;
    Xsig_pred(2,i) = v_p;
    Xsig_pred(3,i) = yaw_p;
    Xsig_pred(4,i) = yawd_p;
  }

  //write result
  *Xsig_out = Xsig_pred;
}

void UKF::PredictMeanAndCovariance(VectorXd* x_out, MatrixXd* P_out, MatrixXd Xsig_pred) {

  cout<<"--PredictMeanAndCovariance--";
  //create vector for weights
  VectorXd weights = VectorXd(2*n_aug_+1);

  //create vector for predicted state
  VectorXd x = VectorXd(n_x_);

  //create covariance matrix for prediction
  MatrixXd P = MatrixXd(n_x_, n_x_);

  // set weights
  double weight_0 = lambda_/(lambda_+n_aug_);
  weights(0) = weight_0;
  for (int i=1; i<2*n_aug_+1; i++) {  //2n+1 weights
    double weight = 0.5/(n_aug_+lambda_);
    weights(i) = weight;
  }

  //predicted state mean
  x.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //iterate over sigma points
    x = x+ weights(i) * Xsig_pred.col(i);
  }

  //predicted state covariance matrix
  P.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //iterate over sigma points

    // state difference
    VectorXd x_diff = Xsig_pred.col(i) - x;
    //angle normalization
    x_diff(3)=atan2(sin(x_diff(3)),cos(x_diff(3)));
    //while (x_diff(3)> M_PI) x_diff(3)-=2.*M_PI;
    //while (x_diff(3)<-M_PI) x_diff(3)+=2.*M_PI;

    P = P + weights(i) * x_diff * x_diff.transpose() ;
  }

  cout<<"-END--"<<endl;
  //write result
  *x_out = x;
  *P_out = P;
}


void UKF::PredictRadarMeasurement(VectorXd* z_out, MatrixXd* S_out, MatrixXd* Zsig_out) {

  cout<<"--PredictRadarMeasurement-";
  //set vector for weights
  VectorXd weights = VectorXd(2*n_aug_+1);
   double weight_0 = lambda_/(lambda_+n_aug_);
  weights(0) = weight_0;
  for (int i=1; i<2*n_aug_+1; i++) {
    double weight = 0.5/(n_aug_+lambda_);
    weights(i) = weight;
  }

  //create matrix for sigma points in measurement space
  MatrixXd Zsig = MatrixXd(n_z_, 2 * n_aug_ + 1);

  //transform sigma points into measurement space
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 sigma points

    // extract values for better readability
    double p_x = Xsig_pred(0,i);
    double p_y = Xsig_pred(1,i);
    double v  = Xsig_pred(2,i);
    double yaw = Xsig_pred(3,i);

    double v1 = cos(yaw)*v;
    double v2 = sin(yaw)*v;

    // measurement model
    Zsig(0,i) = sqrt(p_x*p_x + p_y*p_y);                        //r
    Zsig(1,i) = atan2(p_y,p_x);                                 //phi
    Zsig(2,i) = (p_x*v1 + p_y*v2 ) / sqrt(p_x*p_x + p_y*p_y);   //r_dot
  }

  //mean predicted measurement
  VectorXd z_pred = VectorXd(n_z_);
  z_pred.fill(0.0);
  for (int i=0; i < 2*n_aug_+1; i++) {
      z_pred = z_pred + weights(i) * Zsig.col(i);
  }

  //innovation covariance matrix S
  MatrixXd S = MatrixXd(n_z_,n_z_);
  S.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 sigma points
    //residual
    VectorXd z_diff = Zsig.col(i) - z_pred;

    //angle normalization
    //cout<<i<<" - "<<z_diff(1)<<"|||"<<weights(i)<<endl;
    z_diff(1)=atan2(sin(z_diff(1)),cos(z_diff(1)));
    //while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
    //while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;

    S = S + weights(i) * (z_diff * z_diff.transpose());
    //cout<< weights(i)*(z_diff * z_diff.transpose())<<endl;
  }


  //add measurement noise covariance matrix
  MatrixXd R = MatrixXd(n_z_,n_z_);
  R <<    std_radr_*std_radr_, 0, 0,
          0, std_radphi_*std_radphi_, 0,
          0, 0,std_radrd_*std_radrd_;
  S = S + R;

  //write result
  *z_out = z_pred;
  *S_out = S;
  *Zsig_out=Zsig;
  cout<<"-Done--";
}

void UKF::UpdateState(VectorXd* x_out, MatrixXd* P_out, VectorXd z, MatrixXd Zsig, VectorXd z_pred, MatrixXd S) {
  //set vector for weights
  cout<<"--UpdateState-";
  VectorXd weights = VectorXd(2*n_aug_+1);
    double weight_0 = lambda_/(lambda_+n_aug_);
  weights(0) = weight_0;
  for (int i=1; i<2*n_aug_+1; i++) {  //2n+1 weights
    double weight = 0.5/(n_aug_+lambda_);
    weights(i) = weight;
  }

  //create example vector for predicted state mean
  VectorXd x = *x_out;

  //create example matrix for predicted state covariance
  MatrixXd P = *P_out;

  //create matrix for cross correlation Tc
  MatrixXd Tc = MatrixXd(n_x_, n_z_);

  //calculate cross correlation matrix
  Tc.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points

    //residual
    VectorXd z_diff = Zsig.col(i) - z_pred;
    //angle normalization
    z_diff(1)=atan2(sin(z_diff(1)),cos(z_diff(1)));
    //while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
    //while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;

    // state difference
    VectorXd x_diff = Xsig_pred.col(i) - x;
    //angle normalization
    x_diff(3)=atan2(sin(x_diff(3)),cos(x_diff(3)));
    //while (x_diff(3)> M_PI) x_diff(3)-=2.*M_PI;
    //while (x_diff(3)<-M_PI) x_diff(3)+=2.*M_PI;

    Tc = Tc + weights(i) * x_diff * z_diff.transpose();
  }

  //Kalman gain K;
  MatrixXd K = Tc * S.inverse();

  //residual
  VectorXd z_diff = z - z_pred;

  //angle normalization
  z_diff(1)=atan2(sin(z_diff(1)),cos(z_diff(1)));
  //while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
  //while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;

  //update state mean and covariance matrix
  x = x + K * z_diff;
  P = P - K*S*K.transpose();

  //write result
  *x_out = x;
  *P_out = P;
  cout<<"-Done !!"<<endl;

  double nis;
  nis = z_diff.transpose()*S.inverse()*z_diff;
  cout<<"RADAR NIS :"<<nis<<endl;

}
