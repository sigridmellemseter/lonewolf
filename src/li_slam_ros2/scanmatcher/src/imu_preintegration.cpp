// BSD 3-Clause License
//
// Copyright (c) 2020, Tixiao Shan
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "scanmatcher/utility.h"

#include <std_msgs/msg/string.hpp>

#include <gtsam/geometry/Rot3.h>
#include <gtsam/geometry/Pose3.h>
#include <gtsam/slam/PriorFactor.h>
#include <gtsam/slam/BetweenFactor.h>
#include <gtsam/navigation/GPSFactor.h>
#include <gtsam/navigation/ImuFactor.h>
#include <gtsam/navigation/CombinedImuFactor.h>
#include <gtsam/nonlinear/NonlinearFactorGraph.h>
#include <gtsam/nonlinear/LevenbergMarquardtOptimizer.h>
#include <gtsam/nonlinear/Marginals.h>
#include <gtsam/nonlinear/Values.h>
#include <gtsam/inference/Symbol.h>

#include <gtsam/nonlinear/ISAM2.h>
#include <gtsam_unstable/nonlinear/IncrementalFixedLagSmoother.h>

using gtsam::symbol_shorthand::X; // Pose3 (x,y,z,r,p,y)
using gtsam::symbol_shorthand::V; // Vel   (xdot,ydot,zdot)
using gtsam::symbol_shorthand::B; // Bias  (ax,ay,az,gx,gy,gz)

using namespace std;


// class IMUPreintegration : public ParamServer
class IMUPreintegration : public ParamServer
{
public:
  rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr subImu;
  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr subOdometry;
  rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr pubImuOdometry;
  rclcpp::Publisher<nav_msgs::msg::Path>::SharedPtr pubImuPath;

  // map -> odom
  tf2::Transform map_to_odom;
  //tf2_ros::TransformBroadcaster tfMap2Odom;
  // odom -> base_link
  //tf2_ros::TransformBroadcaster tfOdom2BaseLink;

  bool systemInitialized = false;

  gtsam::noiseModel::Diagonal::shared_ptr priorPoseNoise;
  gtsam::noiseModel::Diagonal::shared_ptr priorVelNoise;
  gtsam::noiseModel::Diagonal::shared_ptr priorBiasNoise;
  gtsam::noiseModel::Diagonal::shared_ptr correctionNoise;
  gtsam::Vector noiseModelBetweenBias;


  gtsam::PreintegratedImuMeasurements * imuIntegratorOpt_;
  gtsam::PreintegratedImuMeasurements * imuIntegratorImu_;

  std::deque<sensor_msgs::msg::Imu> imuQueOpt;
  std::deque<sensor_msgs::msg::Imu> imuQueImu;

  gtsam::Pose3 prevPose_;
  gtsam::Vector3 prevVel_;
  gtsam::NavState prevState_;
  gtsam::imuBias::ConstantBias prevBias_;

  gtsam::NavState prevStateOdom;
  gtsam::imuBias::ConstantBias prevBiasOdom;

  bool doneFirstOpt = false;
  double lastImuT_imu = -1;
  double lastImuT_opt = -1;

  gtsam::ISAM2 optimizer;
  gtsam::NonlinearFactorGraph graphFactors;
  gtsam::Values graphValues;

  const double delta_t = 0;

  int key = 1;
  int imuPreintegrationResetId = 0;

  gtsam::Pose3 imu2Lidar =
    gtsam::Pose3(
    gtsam::Rot3(1, 0, 0, 0),
    gtsam::Point3(-extTrans.x(), -extTrans.y(), -extTrans.z()));
  gtsam::Pose3 lidar2Imu =
    gtsam::Pose3(gtsam::Rot3(1, 0, 0, 0), gtsam::Point3(extTrans.x(), extTrans.y(), extTrans.z()));

  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_;

  IMUPreintegration(const rclcpp::NodeOptions & options)
  : ParamServer("imu_preintegration", options)
    // tfMap2Odom(this),
    // tfOdom2BaseLink(this)
  {

    auto imu_callback =
      [this](const sensor_msgs::msg::Imu::ConstPtr msg) -> void
      {
        imuHandler(msg);
      };
    auto odom_callback =
      [this](const nav_msgs::msg::Odometry::ConstPtr msg) -> void
      {
        odometryHandler(msg);
      };
    subImu = create_subscription<sensor_msgs::msg::Imu>(
      imuTopic, rclcpp::SensorDataQoS(), imu_callback);
    subOdometry = create_subscription<nav_msgs::msg::Odometry>(
      "odometry", 5, odom_callback);

    pubImuOdometry = create_publisher<nav_msgs::msg::Odometry>(odomTopic, 2000);
    pubImuPath = create_publisher<nav_msgs::msg::Path>("imu_path", 1);


    // map_to_odom    = tf2::Transform(tf2::createQuaternionFromRPY(0, 0, 0), tf2::Vector3(0, 0, 0));
    Eigen::Quaterniond quat_eig =
      Eigen::AngleAxisd(0, Eigen::Vector3d::UnitX()) *
      Eigen::AngleAxisd(0, Eigen::Vector3d::UnitY()) *
      Eigen::AngleAxisd(0, Eigen::Vector3d::UnitZ());
    geometry_msgs::msg::Quaternion quat_msg = tf2::toMsg(quat_eig);
    tf2::Quaternion quat_tf;
    tf2::fromMsg(quat_msg, quat_tf);
    map_to_odom = tf2::Transform(quat_tf, tf2::Vector3(0, 0, 0));

    boost::shared_ptr<gtsam::PreintegrationParams> p = gtsam::PreintegrationParams::MakeSharedU(
      imuGravity);
    p->accelerometerCovariance = gtsam::Matrix33::Identity(3, 3) * pow(imuAccNoise, 2);     // acc white noise in continuous
    p->gyroscopeCovariance = gtsam::Matrix33::Identity(3, 3) * pow(imuGyrNoise, 2);         // gyro white noise in continuous
    p->integrationCovariance = gtsam::Matrix33::Identity(3, 3) * pow(1e-4, 2);       // error committed in integrating position from velocities
    gtsam::imuBias::ConstantBias prior_imu_bias((gtsam::Vector(6) << 0, 0, 0, 0, 0, 0).finished());      // assume zero initial bias

    priorPoseNoise =
      gtsam::noiseModel::Diagonal::Sigmas(
      (gtsam::Vector(
        6) << 1e-2, 1e-2, 1e-2, 1e-2, 1e-2, 1e-2).finished());                                                                      // rad,rad,rad,m, m, m
    priorVelNoise = gtsam::noiseModel::Isotropic::Sigma(3, 1e2);       // m/s
    priorBiasNoise = gtsam::noiseModel::Isotropic::Sigma(6, 1e-3);      // 1e-2 ~ 1e-3 seems to be good
    correctionNoise = gtsam::noiseModel::Isotropic::Sigma(6, 1e-2);     // meter
    noiseModelBetweenBias =
      (gtsam::Vector(6) << imuAccBiasN, imuAccBiasN, imuAccBiasN, imuGyrBiasN, imuGyrBiasN,
      imuGyrBiasN).finished();

    imuIntegratorImu_ = new gtsam::PreintegratedImuMeasurements(p, prior_imu_bias);     // setting up the IMU integration for IMU message thread
    imuIntegratorOpt_ = new gtsam::PreintegratedImuMeasurements(p, prior_imu_bias);     // setting up the IMU integration for optimization
  }

  void resetOptimization()
  {
    gtsam::ISAM2Params optParameters;
    optParameters.relinearizeThreshold = 0.1;
    optParameters.relinearizeSkip = 1;
    optimizer = gtsam::ISAM2(optParameters);

    gtsam::NonlinearFactorGraph newGraphFactors;
    graphFactors = newGraphFactors;

    gtsam::Values NewGraphValues;
    graphValues = NewGraphValues;
  }

  void resetParams()
  {
    lastImuT_imu = -1;
    doneFirstOpt = false;
    systemInitialized = false;
  }

  void odometryHandler(const nav_msgs::msg::Odometry::ConstPtr & odomMsg)
  {
    double currentCorrectionTime = ROS_TIME(odomMsg);

    // make sure we have imu data to integrate
    if (imuQueOpt.empty()) {
      return;
    }

    float p_x = odomMsg->pose.pose.position.x;
    float p_y = odomMsg->pose.pose.position.y;
    float p_z = odomMsg->pose.pose.position.z;
    float r_x = odomMsg->pose.pose.orientation.x;
    float r_y = odomMsg->pose.pose.orientation.y;
    float r_z = odomMsg->pose.pose.orientation.z;
    float r_w = odomMsg->pose.pose.orientation.w;
    int currentResetId = round(odomMsg->pose.covariance[0]);
    gtsam::Pose3 lidarPose = gtsam::Pose3(
      gtsam::Rot3::Quaternion(r_w, r_x, r_y, r_z), gtsam::Point3(
        p_x, p_y, p_z));

    // correction pose jumped, reset imu pre-integration
    if (currentResetId != imuPreintegrationResetId) {
      resetParams();
      imuPreintegrationResetId = currentResetId;
    }


    // 0. initialize system
    if (systemInitialized == false) {
      resetOptimization();

      // pop old IMU message
      while (!imuQueOpt.empty()) {
        if (ROS_TIME(&imuQueOpt.front()) < currentCorrectionTime - delta_t) {
          lastImuT_opt = ROS_TIME(&imuQueOpt.front());
          imuQueOpt.pop_front();
        } else {
          break;
        }
      }
      // initial pose
      prevPose_ = lidarPose.compose(lidar2Imu);
      gtsam::PriorFactor<gtsam::Pose3> priorPose(X(0), prevPose_, priorPoseNoise);
      graphFactors.add(priorPose);
      // initial velocity
      prevVel_ = gtsam::Vector3(0, 0, 0);
      gtsam::PriorFactor<gtsam::Vector3> priorVel(V(0), prevVel_, priorVelNoise);
      graphFactors.add(priorVel);
      // initial bias
      prevBias_ = gtsam::imuBias::ConstantBias();
      gtsam::PriorFactor<gtsam::imuBias::ConstantBias> priorBias(B(0), prevBias_, priorBiasNoise);
      graphFactors.add(priorBias);
      // add values
      graphValues.insert(X(0), prevPose_);
      graphValues.insert(V(0), prevVel_);
      graphValues.insert(B(0), prevBias_);
      // optimize once
      optimizer.update(graphFactors, graphValues);
      graphFactors.resize(0);
      graphValues.clear();

      imuIntegratorImu_->resetIntegrationAndSetBias(prevBias_);
      imuIntegratorOpt_->resetIntegrationAndSetBias(prevBias_);

      key = 1;
      systemInitialized = true;
      return;
    }


    // reset graph for speed
    if (key == 100) {
      // get updated noise before reset
      gtsam::noiseModel::Gaussian::shared_ptr updatedPoseNoise =
        gtsam::noiseModel::Gaussian::Covariance(optimizer.marginalCovariance(X(key - 1)));
      gtsam::noiseModel::Gaussian::shared_ptr updatedVelNoise =
        gtsam::noiseModel::Gaussian::Covariance(optimizer.marginalCovariance(V(key - 1)));
      gtsam::noiseModel::Gaussian::shared_ptr updatedBiasNoise =
        gtsam::noiseModel::Gaussian::Covariance(optimizer.marginalCovariance(B(key - 1)));
      // reset graph
      resetOptimization();
      // add pose
      gtsam::PriorFactor<gtsam::Pose3> priorPose(X(0), prevPose_, updatedPoseNoise);
      graphFactors.add(priorPose);
      // add velocity
      gtsam::PriorFactor<gtsam::Vector3> priorVel(V(0), prevVel_, updatedVelNoise);
      graphFactors.add(priorVel);
      // add bias
      gtsam::PriorFactor<gtsam::imuBias::ConstantBias> priorBias(B(0), prevBias_, updatedBiasNoise);
      graphFactors.add(priorBias);
      // add values
      graphValues.insert(X(0), prevPose_);
      graphValues.insert(V(0), prevVel_);
      graphValues.insert(B(0), prevBias_);
      // optimize once
      optimizer.update(graphFactors, graphValues);
      graphFactors.resize(0);
      graphValues.clear();

      key = 1;
    }


    // 1. integrate imu data and optimize
    while (!imuQueOpt.empty()) {
      // pop and integrate imu data that is between two optimizations
      sensor_msgs::msg::Imu * thisImu = &imuQueOpt.front();
      double imuTime = ROS_TIME(thisImu);
      if (imuTime < currentCorrectionTime - delta_t) {
        double dt = (lastImuT_opt < 0) ? (1.0 / 500.0) : (imuTime - lastImuT_opt);
        imuIntegratorOpt_->integrateMeasurement(
          gtsam::Vector3(
            thisImu->linear_acceleration.x, thisImu->linear_acceleration.y,
            thisImu->linear_acceleration.z),
          gtsam::Vector3(
            thisImu->angular_velocity.x, thisImu->angular_velocity.y,
            thisImu->angular_velocity.z), dt);

        lastImuT_opt = imuTime;
        imuQueOpt.pop_front();
      } else {
        break;
      }
    }
    // add imu factor to graph
    const gtsam::PreintegratedImuMeasurements & preint_imu =
      dynamic_cast<const gtsam::PreintegratedImuMeasurements &>(*imuIntegratorOpt_);
    gtsam::ImuFactor imu_factor(X(key - 1), V(key - 1), X(key), V(key), B(key - 1), preint_imu);
    graphFactors.add(imu_factor);
    // add imu bias between factor
    graphFactors.add(
      gtsam::BetweenFactor<gtsam::imuBias::ConstantBias>(
        B(key - 1), B(key), gtsam::imuBias::ConstantBias(),
        gtsam::noiseModel::Diagonal::Sigmas(
          sqrt(imuIntegratorOpt_->deltaTij()) *
          noiseModelBetweenBias)));
    // add pose factor
    gtsam::Pose3 curPose = lidarPose.compose(lidar2Imu);
    gtsam::PriorFactor<gtsam::Pose3> pose_factor(X(key), curPose, correctionNoise);
    graphFactors.add(pose_factor);
    // insert predicted values
    gtsam::NavState propState_ = imuIntegratorOpt_->predict(prevState_, prevBias_);
    graphValues.insert(X(key), propState_.pose());
    graphValues.insert(V(key), propState_.v());
    graphValues.insert(B(key), prevBias_);
    // optimize
    optimizer.update(graphFactors, graphValues);
    optimizer.update();
    graphFactors.resize(0);
    graphValues.clear();
    // Overwrite the beginning of the preintegration for the next step.
    gtsam::Values result = optimizer.calculateEstimate();
    prevPose_ = result.at<gtsam::Pose3>(X(key));
    prevVel_ = result.at<gtsam::Vector3>(V(key));
    prevState_ = gtsam::NavState(prevPose_, prevVel_);
    prevBias_ = result.at<gtsam::imuBias::ConstantBias>(B(key));
    // Reset the optimization preintegration object.
    imuIntegratorOpt_->resetIntegrationAndSetBias(prevBias_);
    // check optimization
    if (failureDetection(prevVel_, prevBias_)) {
      resetParams();
      return;
    }


    // 2. after optiization, re-propagate imu odometry preintegration
    prevStateOdom = prevState_;
    prevBiasOdom = prevBias_;
    // first pop imu message older than current correction data
    double lastImuQT = -1;
    while (!imuQueImu.empty() && ROS_TIME(&imuQueImu.front()) < currentCorrectionTime - delta_t) {
      lastImuQT = ROS_TIME(&imuQueImu.front());
      imuQueImu.pop_front();
    }
    // repropogate
    if (!imuQueImu.empty()) {
      // reset bias use the newly optimized bias
      imuIntegratorImu_->resetIntegrationAndSetBias(prevBiasOdom);
      // integrate imu message from the beginning of this optimization
      for (int i = 0; i < (int)imuQueImu.size(); ++i) {
        sensor_msgs::msg::Imu * thisImu = &imuQueImu[i];
        double imuTime = ROS_TIME(thisImu);
        double dt = (lastImuQT < 0) ? (1.0 / 500.0) : (imuTime - lastImuQT);

        imuIntegratorImu_->integrateMeasurement(
          gtsam::Vector3(
            thisImu->linear_acceleration.x, thisImu->linear_acceleration.y,
            thisImu->linear_acceleration.z),
          gtsam::Vector3(
            thisImu->angular_velocity.x, thisImu->angular_velocity.y,
            thisImu->angular_velocity.z), dt);
        lastImuQT = imuTime;
      }
    }

    ++key;
    doneFirstOpt = true;
  }

  bool failureDetection(const gtsam::Vector3 & velCur, const gtsam::imuBias::ConstantBias & biasCur)
  {
    Eigen::Vector3f vel(velCur.x(), velCur.y(), velCur.z());
    if (vel.norm() > 10) {
      RCLCPP_WARN(get_logger(), "Large velocity, reset IMU-preintegration!");
      return true;
    }

    Eigen::Vector3f ba(biasCur.accelerometer().x(),
      biasCur.accelerometer().y(), biasCur.accelerometer().z());
    Eigen::Vector3f bg(biasCur.gyroscope().x(), biasCur.gyroscope().y(), biasCur.gyroscope().z());
    if (ba.norm() > 0.1 || bg.norm() > 0.1) {
      RCLCPP_WARN(get_logger(), "Large bias, reset IMU-preintegration!");
      return true;
    }

    return false;
  }

  void imuHandler(const sensor_msgs::msg::Imu::ConstPtr & imu_raw)
  {
    sensor_msgs::msg::Imu thisImu = imuConverter(*imu_raw);
    // publish static tf
    // tfMap2Odom.sendTransform(tf2::StampedTransform(map_to_odom, thisImu.header.stamp, "map", "odom"));
    geometry_msgs::msg::Pose pose;
    tf2::toMsg(map_to_odom, pose);
    geometry_msgs::msg::TransformStamped trans_stamped;
    trans_stamped.header.stamp = thisImu.header.stamp;
    trans_stamped.header.frame_id = "map";
    trans_stamped.child_frame_id = "odom";
    trans_stamped.transform.translation.x = pose.position.x;
    trans_stamped.transform.translation.y = pose.position.y;
    trans_stamped.transform.translation.z = pose.position.z;
    trans_stamped.transform.rotation = pose.orientation;
    //tfMap2Odom.sendTransform(trans_stamped);

    imuQueOpt.push_back(thisImu);
    imuQueImu.push_back(thisImu);

    if (doneFirstOpt == false) {
      return;
    }

    double imuTime = ROS_TIME(&thisImu);
    double dt = (lastImuT_imu < 0) ? (1.0 / 500.0) : (imuTime - lastImuT_imu);
    lastImuT_imu = imuTime;

    // integrate this single imu message
    imuIntegratorImu_->integrateMeasurement(
      gtsam::Vector3(
        thisImu.linear_acceleration.x, thisImu.linear_acceleration.y,
        thisImu.linear_acceleration.z),
      gtsam::Vector3(
        thisImu.angular_velocity.x, thisImu.angular_velocity.y,
        thisImu.angular_velocity.z), dt);

    // predict odometry
    gtsam::NavState currentState = imuIntegratorImu_->predict(prevStateOdom, prevBiasOdom);

    // publish odometry
    nav_msgs::msg::Odometry odometry;
    odometry.header.stamp = thisImu.header.stamp;
    odometry.header.frame_id = "odom";
    odometry.child_frame_id = "odom_imu";

    // transform imu pose to ldiar
    gtsam::Pose3 imuPose = gtsam::Pose3(currentState.quaternion(), currentState.position());
    gtsam::Pose3 lidarPose = imuPose.compose(imu2Lidar);

    odometry.pose.pose.position.x = lidarPose.translation().x();
    odometry.pose.pose.position.y = lidarPose.translation().y();
    odometry.pose.pose.position.z = lidarPose.translation().z();
    odometry.pose.pose.orientation.x = lidarPose.rotation().toQuaternion().x();
    odometry.pose.pose.orientation.y = lidarPose.rotation().toQuaternion().y();
    odometry.pose.pose.orientation.z = lidarPose.rotation().toQuaternion().z();
    odometry.pose.pose.orientation.w = lidarPose.rotation().toQuaternion().w();

    odometry.twist.twist.linear.x = currentState.velocity().x();
    odometry.twist.twist.linear.y = currentState.velocity().y();
    odometry.twist.twist.linear.z = currentState.velocity().z();
    odometry.twist.twist.angular.x = thisImu.angular_velocity.x + prevBiasOdom.gyroscope().x();
    odometry.twist.twist.angular.y = thisImu.angular_velocity.y + prevBiasOdom.gyroscope().y();
    odometry.twist.twist.angular.z = thisImu.angular_velocity.z + prevBiasOdom.gyroscope().z();
    odometry.pose.covariance[0] = double(imuPreintegrationResetId);
    pubImuOdometry->publish(odometry);

    // publish imu path
    static nav_msgs::msg::Path imuPath;
    static double last_path_time = -1;
    if (imuTime - last_path_time > 0.1) {
      last_path_time = imuTime;
      geometry_msgs::msg::PoseStamped pose_stamped;
      pose_stamped.header.stamp = thisImu.header.stamp;
      pose_stamped.header.frame_id = "odom";
      pose_stamped.pose = odometry.pose.pose;
      imuPath.poses.push_back(pose_stamped);
      double t1 = imuPath.poses.front().header.stamp.sec +
        imuPath.poses.front().header.stamp.nanosec * 1e-9;
      double t2 = imuPath.poses.back().header.stamp.sec +
        imuPath.poses.back().header.stamp.nanosec * 1e-9;
      while (!imuPath.poses.empty() &&
        // abs(imuPath.poses.front().header.stamp.toSec() - imuPath.poses.back().header.stamp.toSec()) > 3.0)
        //abs(ROS_TIME(imuPath.poses.front()) - ROS_TIME(imuPath.poses.back())) > 3.0)
        abs(t1 - t2) > 3.0)
      {
        imuPath.poses.erase(imuPath.poses.begin());
      }
      // if (pubImuPath.getNumSubscribers() != 0)
      {
        imuPath.header.stamp = thisImu.header.stamp;
        // imuPath.header.frame_id = "odom";
        imuPath.header.frame_id = "map";
        pubImuPath->publish(imuPath);
      }
    }

    // publish transformation
    tf2::Transform tCur;
    geometry_msgs::msg::TransformStamped odom_2_baselink;
    odom_2_baselink.header.stamp = thisImu.header.stamp;
    odom_2_baselink.header.frame_id = "odom";
    odom_2_baselink.child_frame_id = "base_link";
    odom_2_baselink.transform.translation.x = odometry.pose.pose.position.x;
    odom_2_baselink.transform.translation.y = odometry.pose.pose.position.y;
    odom_2_baselink.transform.translation.z = odometry.pose.pose.position.z;
    odom_2_baselink.transform.rotation = odometry.pose.pose.orientation;
    //tfOdom2BaseLink.sendTransform(odom_2_baselink);

  }


};


int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::NodeOptions options;
  options.use_intra_process_comms(true);
  rclcpp::executors::SingleThreadedExecutor exec;

  auto imu_pre = std::make_shared<IMUPreintegration>(options);
  exec.add_node(imu_pre);

  cout << "\033[1;32m----> IMU Preintegration Started.\033[0m" << endl;

  exec.spin();
  rclcpp::shutdown();

  return 0;
}
