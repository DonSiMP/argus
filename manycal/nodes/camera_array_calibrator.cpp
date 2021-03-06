#include <ros/ros.h>

#include "manycal/CameraArrayCalibrator.h"
#include "argus_utils/utils/ParamUtils.h"

#include "extrinsics_array/ExtrinsicsCalibrationParsers.h"

#include "vizard/PoseVisualizer.h"
#include "camplex/FiducialVisualizer.h"

#include <boost/foreach.hpp>
#include <set>

using namespace argus;

class CameraObjectCalibrationNode
{
public:

	CameraObjectCalibrationNode( ros::NodeHandle& nh, ros::NodeHandle& ph )
		: _calibrator( nh )
	{
		ros::NodeHandle ch( ph.resolveName( "calibration" ) );
		_calibrator.ReadParams( ch ) ;

		double rate;
		GetParamRequired( ph, "update_rate", rate );
		_updateTimer = nh.createTimer( ros::Duration( 1.0 / rate ), &CameraObjectCalibrationNode::TimerCallback, this );

		_enableVis = ph.hasParam( "visualization" );
		if( _enableVis )
		{
			ros::NodeHandle ch( ph.resolveName( "visualization/camera" ) );
			ros::NodeHandle fh( ph.resolveName( "visualization/fiducial" ) );
			_camVis.ReadParams( ch );
			_fidVis.ReadParams( fh );

			std::string refFrame;
			GetParamRequired( ph, "visualization/reference_frame", refFrame );
			_camVis.SetFrameID( refFrame );
			_fidVis.SetFrameID( refFrame );
			_visPub = nh.advertise<MarkerMsg>( "markers", 10 );
		}

		unsigned int detBuffLen;
		GetParam<unsigned int>( ph, "detections_buffer_len", detBuffLen, 10 );
		_detSub = nh.subscribe( "detections",
		                        detBuffLen,
		                        &CameraObjectCalibrationNode::DetectionCallback,
		                        this );
	}

	void WriteResults( const std::string& path )
	{
		std::vector<CameraObjectCalibration> cams = _calibrator.GetCameras();
		std::string refFrame = _calibrator.GetReferenceFrame();
		std::vector<RelativePose> poses;
		BOOST_FOREACH( const CameraObjectCalibration& cam, cams )
		{
			RelativePose p;
			p.childID = cam.name;
			p.parentID = refFrame;
			p.pose = cam.extrinsics;
			poses.push_back( p );
		}

		ROS_INFO_STREAM( "Saving extrinsics to " << path );
		if( !WriteExtrinsicsCalibration( path, poses ) )
		{
			ROS_ERROR_STREAM( "Could not save extrinsics to " << path );
		}
	}

private:

	CameraArrayCalibrator _calibrator;

	ros::Subscriber _detSub;
	ros::Publisher _visPub;

	bool _enableVis;
	ros::Timer _updateTimer;
	PoseVisualizer _camVis;
	FiducialVisualizer _fidVis;

	std::set<std::string> _registeredCameras;
	std::set<std::string> _registeredFiducials;

	void DetectionCallback( const argus_msgs::ImageFiducialDetections::ConstPtr& msg )
	{
		ImageFiducialDetections dets( *msg );
		if( _registeredCameras.count( dets.sourceName ) == 0 )
		{
			_calibrator.RegisterCamera( dets.sourceName );
			_registeredCameras.insert( dets.sourceName );
		}
		BOOST_FOREACH( const FiducialDetection& fd, dets.detections )
		{
			if( _registeredFiducials.count( fd.name ) == 0 )
			{
				_calibrator.RegisterFiducial( fd.name );
				_registeredFiducials.insert( fd.name );
			}
		}

		_calibrator.BufferDetection( dets );
	}


	void TimerCallback( const ros::TimerEvent& event )
	{
		if( _enableVis )
		{
			std::vector<FiducialObjectCalibration> fids = _calibrator.GetFiducials();
			std::vector<CameraObjectCalibration> cams = _calibrator.GetCameras();

			std::vector<PoseSE3> fidPoses;
			std::vector<Fiducial> fidInts;
			std::vector<std::string> fidNames;
			BOOST_FOREACH( const FiducialObjectCalibration &fid, fids )
			{
				fidPoses.push_back( fid.extrinsics );
				fidInts.push_back( fid.intrinsics );
				fidNames.push_back( fid.name );
			}

			std::vector<PoseSE3> camPoses;
			std::vector<std::string> camNames;
			BOOST_FOREACH( const CameraObjectCalibration &cam, cams )
			{
				camPoses.push_back( cam.extrinsics );
				camNames.push_back( cam.name );
			}

			std::vector<MarkerMsg> fidMarkers = _fidVis.ToMarkers( fidPoses, fidInts, fidNames );
			std::vector<MarkerMsg> camMarkers = _camVis.ToMarkers( camPoses, camNames );
			BOOST_FOREACH( const MarkerMsg &msg, fidMarkers )
			{
				_visPub.publish( msg );
			}
			BOOST_FOREACH( const MarkerMsg &msg, camMarkers )
			{
				_visPub.publish( msg );
			}
		}

		_calibrator.Spin();
	}
};

int main( int argc, char**argv )
{
	ros::init( argc, argv, "camera_array_calibrator" );

	ros::NodeHandle nh;
	ros::NodeHandle ph( "~" );

	std::string outputPath;
	GetParam<std::string>( ph, "output_path", outputPath, "out.yaml" );

	CameraObjectCalibrationNode node( nh, ph );
	ros::spin();

	node.WriteResults( outputPath );


	return 0;
}
