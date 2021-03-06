#pragma once

#include <ros/ros.h>
#include <yaml-cpp/yaml.h>
#include <geometry_msgs/PoseWithCovarianceStamped.h>

#include <isam/sclam_monocular.h>

#include "manycal/WriteCalibration.h"
#include "manycal/Registrations.h"
#include "graphopt/GraphOptimizer.h"

#include "lookup/LookupInterface.h"
#include "argus_msgs/ImageFiducialDetections.h"
#include "extrinsics_array/ExtrinsicsInterface.h"

#include "camplex/FiducialInfoManager.h"

#include "argus_utils/geometry/PoseSE3.h"
#include "argus_utils/geometry/VelocityIntegrator.hpp"
#include "argus_utils/synchronization/SynchronizationTypes.h"

#include <deque>
#include <unordered_map>

namespace argus
{
// TODO Track the groundedness of the various extrinsics trees to inform users
// when they don't have enough priors

/*! \brief Calibrates arrays of cameras and fiducials. */
class ArrayCalibrator
{
public:

	ArrayCalibrator( ros::NodeHandle& nh, ros::NodeHandle& ph );
	~ArrayCalibrator();

	void ProcessUntil( const ros::Time& until );
	void Print();
	void Save();

private:

	ros::NodeHandle _nodeHandle;
	ros::NodeHandle _privHandle;
	ros::ServiceServer _writeServer;

	ros::Timer _spinTimer;
	ros::Duration _spinLag;

	Mutex _buffMutex;
	std::vector<ros::Subscriber> _detSubs;

	struct DetectionData
	{
		std::string sourceName;
		ros::Time timestamp;
		FiducialDetection detection;
	};

	typedef std::deque<DetectionData> DetectionsBuffer;
	DetectionsBuffer _detBuffer;
	ros::Time _buffTime;
	ros::Duration _maxLag;

	std::string _savePath;

	/*! \brief Stores subscriptions to odometry and detection inputs. */
	typedef std::unordered_map<std::string, CameraRegistration::Ptr> CameraRegistry;
	typedef std::unordered_map<std::string, FiducialRegistration::Ptr> FiducialRegistry;

	// NOTE Must be deque so that memory isn't moved and references don't break
	std::deque<TargetRegistration> _targetRegistry;
	CameraRegistry _cameraRegistry;
	FiducialRegistry _fiducialRegistry;

	Mutex _optMutex;	
	GraphOptimizer _graph;
	std::vector<isam::FiducialFactor::Ptr> _observations;

	double _detectionImgVariance;

	bool WriteHandler( manycal::WriteCalibration::Request& req,
	                   manycal::WriteCalibration::Response& res );

	void TimerCallback( const ros::TimerEvent& event );

	// Attempt to process a detection, returns success
	bool ProcessDetection( const std::string& sourceName,
	                       const ros::Time& time,
	                       const FiducialDetection& det );

	void DetectionCallback( const argus_msgs::ImageFiducialDetections::ConstPtr& msg );
};
} // end namespace manycal
