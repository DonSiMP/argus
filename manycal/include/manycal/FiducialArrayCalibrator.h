#pragma once

#include <ros/ros.h>
#include <isam/sclam_monocular.h>

#include "lookup/LookupInterface.h"

#include "camplex/FiducialCommon.h"
#include "camplex/FiducialInfoManager.h"

#include "extrinsics_array/ExtrinsicsInterface.h"
#include "graphopt/GraphOptimizer.h"
#include "graphopt/sclam_fiducial.h"

#include "manycal/ManycalCommon.h"

#include "argus_utils/synchronization/ThreadsafeQueue.hpp"

namespace argus
{

// TODO Add service calls to write results

/*! \brief Estimates the extrinsics of an array of fiducials with a single image stream.
 *
 *  Fiducials are initialized as they are observed by the camera, with the first
 *  fiducial placed at the origin. Consecutive observations are initialized by using
 *  the last estimated pose of the camera. 
 *
 *  ROS Parameters:
 *  =======================
 *  ~reference_frame: string
 *      Name of the fiducial array reference frame. Will be used when outputting the
 *      calibration file.
 *  ~batch_period: unsigned int
 *      The number of observation factors in between batch optimizations.
 *  ~lookup_namespace: string (default: "/lookup")
 *      The lookup parameter base namespace.
 *
 *  ROS Topics:
 *  ===========
 *  detections: argus_msgs::ImageFiducialDetections
 *      Stream of detections coming from a single camera. Should be rectified and normalized.
 */
class FiducialArrayCalibrator
{
public:
	
	typedef std::vector<FiducialDetection> FiducialDetections;

	EIGEN_MAKE_ALIGNED_OPERATOR_NEW;

	FiducialArrayCalibrator();
	void Initialize( ros::NodeHandle& ph, LookupInterface& lookup );

	void BufferDetections( const FiducialDetections& detections );
	void Spin();

	void WriteResults();
	
private:
	
	LookupInterface _lookupInterface;
	FiducialInfoManager _fiducialManager;
	ExtrinsicsInterface _extrinsicsManager;

	std::string _referenceFrame;

	unsigned int _minDetectionsPerImage;
	unsigned int _detCounter;
	unsigned int _batchPeriod;
	
	PoseSE3::CovarianceMatrix _priorCovariance;

	typedef ThreadsafeQueue<FiducialDetections> DetectionsBuffer;
	DetectionsBuffer _detectionsBuffer;

	GraphOptimizer _optimizer;
	isam::MonocularIntrinsics_Node::Ptr _cameraIntrinsics; // Unoptimized cam intrinsics factor
	isam::PoseSE3_Node::Ptr _fiducialReference; // Unoptimized pose of fiducial reference frame
	std::vector <isam::PoseSE3_Node::Ptr> _cameraPoses;
	std::vector <isam::FiducialFactor::Ptr> _observations;
	
	struct FiducialRegistration
	{
		isam::PoseSE3_Node::Ptr extrinsics;
		isam::FiducialIntrinsics_Node::Ptr intrinsics;
		isam::PoseSE3_Prior::Ptr extrinsicsPrior;
	};
	typedef std::unordered_map <std::string, FiducialRegistration> FiducialRegistry;
	FiducialRegistry _fiducialRegistry;
		
	// Attempt to initialize the camera pose from detections. Returns a nullptr
	// if fails. Does not add the node to the slam instance on success.
	isam::PoseSE3_Node::Ptr InitializeCameraPose( const std::vector<FiducialDetection>& dets );

	// Attempt to initialize a fiducial from prior info. Returns success.
	bool InitializeFiducialFromPrior( const std::string& name );

	/*! \brief Creates the fiducial and reads its intrinsics via lookup. */
	void RegisterFiducial( const std::string& name, const argus::PoseSE3& pose,
	                       bool addPrior );
	
};
	
}
