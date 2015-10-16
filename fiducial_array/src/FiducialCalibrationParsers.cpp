#include "argus_utils/YamlUtils.h"
#include "argus_utils/GeometryUtils.h"
#include "fiducial_array/FiducialCalibrationParsers.h"
#include "extrinsics_array/ExtrinsicsArrayCalibrationParsers.h"

#include <fstream>
#include <boost/foreach.hpp>

using namespace argus_utils;
using namespace extrinsics_array;

namespace fiducial_array
{

bool ParseFiducialCalibration( const YAML::Node& yaml, FiducialInfo& info )
{
	// Only the point are required
	if( !yaml["intrinsics"]["points_x"] ||
		!yaml["intrinsics"]["points_y"] ||
		!yaml["intrinsics"]["points_z"] )
	{
		std::cerr << "Could not read fiducial info from YAML." << std::endl;
		return false;
	}
	
	// Parse the points
	std::vector<double> pointsX = yaml["intrinsics"]["points_x"].as< std::vector<double> >();
	std::vector<double> pointsY = yaml["intrinsics"]["points_y"].as< std::vector<double> >();
	std::vector<double> pointsZ = yaml["intrinsics"]["points_z"].as< std::vector<double> >();
	if( (pointsX.size() != pointsY.size()) && (pointsY.size() != pointsZ.size()) )
	{
		std::cerr << "Point fields must have same number of elements." << std::endl;
		return false;
	}

	info.points.clear();
	info.points.reserve( pointsX.size() );
	geometry_msgs::Point point;
	for( unsigned int i = 0; i < pointsX.size(); i++ )
	{
		point.x = pointsX[i];
		point.y = pointsY[i];
		point.z = pointsZ[i];
		info.points.push_back( point );
	}
	return true;
}

bool ReadFiducialCalibration( const std::string& path, FiducialInfo& info )
{
	YAML::Node yaml;
	try 
	{
		 yaml = YAML::LoadFile( path );
	}
	catch( YAML::BadFile e )
	{
		return false;
	}
	
	return ParseFiducialCalibration( yaml, info );
}

bool PopulateFiducialCalibration( const FiducialInfo& info, YAML::Node& yaml )
{
	std::vector<double> pointsX, pointsY, pointsZ;
	for( unsigned int i = 0; i < info.points.size(); i++ )
	{
		pointsX.push_back( info.points[i].x );
		pointsY.push_back( info.points[i].y );
		pointsZ.push_back( info.points[i].z );
	}
	yaml[ "intrinsics" ][ "points_x" ] = pointsX;
	yaml[ "intrinsics" ][ "points_y" ] = pointsY;
	yaml[ "intrinsics" ][ "points_z" ] = pointsZ;
	return true;
}

bool WriteFiducialCalibration( const std::string& path, const FiducialInfo& info )
{
	std::ofstream output( path );
	if( !output.is_open() )
	{
		return false;
	}
	
	YAML::Node yaml;
	if( !PopulateFiducialCalibration( info, yaml ) ) { return false; }
	output << yaml;
	return true;
}

bool ParseFiducialArrayCalibration( const YAML::Node& yaml, FiducialArrayInfo& info )
{
	if( !ParseExtrinsicsArrayCalibration( yaml, info.extrinsics ) ) { return false; }
	
	info.fiducials.clear();
	info.fiducials.reserve( info.extrinsics.memberNames.size() );
	
	BOOST_FOREACH( const std::string& memberName, info.extrinsics.memberNames )
	{
		FiducialInfo fidInfo;
		if( !ParseFiducialCalibration( yaml[memberName], fidInfo ) ) { return false; }
		info.fiducials.push_back( fidInfo );
	}
	return true;
}

bool ReadFiducialArrayCalibration( const std::string& path,
							  FiducialArrayInfo& info )
{
	YAML::Node yaml;
	try 
	{
		 yaml = YAML::LoadFile( path );
	}
	catch( YAML::BadFile e )
	{
		return false;
	}
	
	return ParseFiducialArrayCalibration( yaml, info );
}

bool PopulateFiducialArrayCalibration( const FiducialArrayInfo& info, YAML::Node& yaml )
{
	if( !PopulateExtrinsicsArrayCalibration( info.extrinsics, yaml ) ) { return false; }
	
	for( unsigned int i = 0; i < info.extrinsics.memberNames.size(); i++ )
	{
		std::string memberName = info.extrinsics.memberNames[i];
		YAML::Node member;
		if( !PopulateFiducialCalibration( info.fiducials[i], member ) ) { return false; }
		yaml[ memberName ] = member;
	}
	return true;
}

bool WriteFiducialArrayCalibration( const std::string& path, const FiducialArrayInfo& info )
{
	std::ofstream output( path );
	if( !output.is_open() )
	{
		return false;
	}
	
	YAML::Node yaml;
	if( !PopulateFiducialArrayCalibration( info, yaml ) ) { return false; }
	output << yaml;
	return true;
}

} // end namespace fiducial_array
