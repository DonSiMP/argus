#include "fiducial_array/PoseEstimation.h"
#include "argus_utils/GeometryUtils.h"

#include <opencv2/calib3d/calib3d.hpp>

using namespace argus_msgs;

namespace fiducial_array
{

// TODO if info is null, assume normalized and undistorted detections
argus_utils::PoseSE3 EstimateArrayPose( const std::vector< cv::Point2f >& imagePoints,
                                        const image_geometry::PinholeCameraModel* cameraModel,
                                        const std::vector< cv::Point3f >& fiducialPoints,
                                        const argus_utils::PoseSE3& guess )
{
	cv::Matx33f cameraMat;
	cv::Mat distortionCoeffs;
	if( cameraModel != nullptr )
	{
		cameraMat = cameraModel->intrinsicMatrix();
		distortionCoeffs = cameraModel->distortionCoeffs();
	}
	else
	{
		cameraMat = cv::Matx33f::eye();
	}
	
	// TODO Figure out why this is no good!
	// Initialize guess
	cv::Mat rvec;
	cv::Mat tvec( 3, 1, CV_64FC1 ); // Must allocate tvec
	cv::Matx33d R;
	Eigen::Matrix4d H = guess.ToTransform().matrix();
	for( unsigned int i = 0; i < 3; i++ )
	{
		for( unsigned int j = 0; j < 3; j++ )
		{
			R(i,j) = H(i,j);
		}
		tvec.at<double>(i) = H(i,3);
	}
	cv::Rodrigues( R, rvec );
	
	cv::solvePnP( fiducialPoints, imagePoints, cameraMat, distortionCoeffs, rvec, tvec, false );
	
	cv::Rodrigues( rvec, R );
	H << R(0,0), R(0,1), R(0,2), tvec.at<double>(0),
	     R(1,0), R(1,1), R(1,2), tvec.at<double>(1),
	     R(2,0), R(2,1), R(2,2), tvec.at<double>(2),
	          0,      0,      0,      1;
	
	static argus_utils::PoseSE3 postrotation( 0, 0, 0, 0.5, -0.5, 0.5, 0.5 );
	static argus_utils::PoseSE3 prerotation( 0, 0, 0, -0.5, 0.5, -0.5, 0.5 );
	
	return prerotation * argus_utils::PoseSE3( H );// * postrotation;
}
	
}