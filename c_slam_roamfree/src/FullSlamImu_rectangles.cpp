/*
 * FullSlamImu.cpp
 *
 *  Created on: Sep 8, 2014
 *      Author: davide
 */

#include "FullSlamImu_rectangles.h"

#include <tf_conversions/tf_eigen.h>
#include <visualization_msgs/Marker.h>

#include <iostream>

using namespace ROAMestimation;
using namespace std;

namespace roamfree_c_slam {

FullSlamImu_rectangles::FullSlamImu_rectangles(std::string imuTopic) :
		filter(NULL), imuHandler(NULL), tracksHandler(NULL) {
	//setup roamfree
	initRoamfree();

	//setup the camera
	initCamera();

	//setup the imu handler
	initIMU();

	//subscribe to sensor topics
	imu_sub = n.subscribe(imuTopic, 60000, &FullSlamImu_rectangles::imuCb, this);
	tracks_sub = n.subscribe("/tracks", 60000, &FullSlamImu_rectangles::tracksCb, this);
	markers_pub = n.advertise<visualization_msgs::Marker>(
			"/visualization/features", 1);
}

FullSlamImu_rectangles::~FullSlamImu_rectangles() {
	if (filter != NULL)

		delete filter;

	if (imuHandler)
		delete imuHandler;

	if (tracksHandler)
		delete tracksHandler;

}

void FullSlamImu_rectangles::run() {
	ros::NodeHandle n("~");

	ros::Rate rate(5);

	while (ros::ok()) {
		//rate.sleep();

		ros::spinOnce();

		if (filter->getWindowLenght() > 1.0
				&& tracksHandler->getNActiveFeatures() >= 1) {

			//filter->getNthOldestPose(0)->setFixed(true);

			double curTs = filter->getNewestPose()->getTimestamp();
			PoseVertexWrapper_Ptr cur;

			//ROS_INFO("Run estimation");
			bool ret = filter->estimate(0);

			filter->forgetOldNodes(2.5);
		}

		if (filter->getOldestPose()) {
			publishFeatureMarkers();
			publishCameraPose();
		}
	};

}

void FullSlamImu_rectangles::imuCb(const sensor_msgs::Imu& msg) {
	//ROS_INFO("imu callback");
	double t = msg.header.stamp.toSec();

	// fill temporaries with measurements
	double za[] = { msg.linear_acceleration.x, msg.linear_acceleration.y,
			msg.linear_acceleration.z };
	double zw[] = { msg.angular_velocity.x, msg.angular_velocity.y,
			msg.angular_velocity.z };

	imuHandler->addMeasurement(za, zw, t);

}

void FullSlamImu_rectangles::tracksCb(const c_slam_msgs::TrackedObject& msg) {
	//ROS_INFO("Tracks callback");

	double t = msg.imageStamp.toSec();

	// compute the center of mass
	Eigen::VectorXd z(8);
	z << msg.polygon.points[0].x, msg.polygon.points[0].y, msg.polygon.points[1].x, msg.polygon.points[1].y, msg.polygon.points[2].x, msg.polygon.points[2].y, msg.polygon.points[3].x, msg.polygon.points[3].y;

	static const Eigen::MatrixXd cov = Eigen::MatrixXd::Identity(8, 8);

	tracksHandler->addFeatureObservation(msg.id, t, z, cov);

}

void FullSlamImu_rectangles::initRoamfree() {
	filter = FactorGraphFilterFactory::getNewFactorGraphFilter();
	filter->setLowLevelLogging(true); // default log folder
	system("mkdir -p /tmp/roamfree/");
	system("rm -f /tmp/roamfree/*.log");
	filter->setDeadReckoning(true);
	filter->setSolverMethod(GaussNewton);
}

void FullSlamImu_rectangles::initCamera() {

	tracksHandler = new RectangleHandler(2.0);
	tracksHandler->setTimestampOffsetTreshold(1.0 / 5.0 / 2.0);

	//the camera intrinsic calibration matrix
	Eigen::VectorXd CM(9);
	CM << 565.59102697808, 0.0, 337.839450567586, 0.0, 563.936510489792, 199.522081717361, 0.0, 0.0, 1.0;

	Eigen::VectorXd T_OC(7);
	T_OC << 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0;

	tracksHandler->init(filter, "Track", T_OC, CM);
}

void FullSlamImu_rectangles::initIMU()
{
	//setup the imu handler
	imuHandler = new ImuHandler(filter, true, true);
	Eigen::VectorXd T_OS_IMU(7), x0(7);
	T_OS_IMU << 0.0, 0.0, 0.0, 0.5, 0.5, -0.5, 0.5;
	x0 << 0.0, -1.0, 0.0, 0.5, -0.5, 0.5, -0.5;
	imuHandler->setSensorframe(T_OS_IMU, x0);
}

void FullSlamImu_rectangles::publishFeatureMarkers() {
	std::vector<long int> ids;

	tracksHandler->getFeaturesIds(ids);

	for (int k = 0; k < ids.size(); ++k) {

		visualization_msgs::Marker msg;

		msg.header.stamp = ros::Time::now();
		msg.header.frame_id = "/world";
		msg.type = visualization_msgs::Marker::CUBE;
		//msg.lifetime = ros::Duration(0.2);
		msg.frame_locked = false;
		msg.ns = "roamfree_visualodometry";
		msg.id = k;
		msg.action = visualization_msgs::Marker::ADD;

		msg.color.r = 1.0;
		msg.color.g = 0.0;
		msg.color.b = 0.0;
		msg.color.a = 1.0;

		Eigen::VectorXd fw(7);
		Eigen::VectorXd dim(2);

		bool ret;

		ret = tracksHandler->getFeaturePoseInWorldFrame(ids[k], fw);
		assert(ret);
		ret = tracksHandler->getFeatureDimensions(ids[k], dim);
		assert(ret);

		msg.pose.position.x = fw(0);
		msg.pose.position.y = fw(1);
		msg.pose.position.z = fw(2);

		msg.pose.orientation.w = fw(3);
		msg.pose.orientation.x = fw(4);
		msg.pose.orientation.y = fw(5);
		msg.pose.orientation.z = fw(6);

		msg.scale.x = dim(0);
		msg.scale.y = dim(1);
		msg.scale.z = 0.05;


		markers_pub.publish(msg);

	}
}

void FullSlamImu_rectangles::publishCameraPose() {
	ROAMestimation::PoseVertexWrapper_Ptr cameraPose_ptr =
			filter->getNewestPose();
	const Eigen::VectorXd &camera = cameraPose_ptr->getEstimate();

	tf::Transform T_WR_tf(
			tf::Quaternion(camera(4), camera(5), camera(6), camera(3)),
			tf::Vector3(camera(0), camera(1), camera(2)));

	pose_tf_br.sendTransform(
			tf::StampedTransform(T_WR_tf, ros::Time(cameraPose_ptr->getTimestamp()),
					"world", "camera_link"));

}

} /* namespace roamfree_c_slam */

int main(int argc, char *argv[]) {
	ros::init(argc, argv, "roamfree_full_slam_imu");

	roamfree_c_slam::FullSlamImu_rectangles n(argv[1]);

	ROS_INFO("Localization node started");
	n.run();
	ROS_INFO("Localization node shut down");

	return 0;
}
