#pragma once

#include "paraset/ContinuousPolicy.h"
#include "paraset/PolicyModules.h"
#include "broadcast/BroadcastMultiReceiver.h"

#include "argus_utils/random/MultivariateGaussian.hpp"

namespace argus
{

// TODO Subscribe to parameter updates
class ContinuousPolicyManager
{
public:

	ContinuousPolicyManager();
	
	void Initialize( ros::NodeHandle& ph );

private:

	ContinuousPolicy _policyInterface;
	ros::Publisher _actionPub;

	VarReLUGaussian::Ptr _network;
	percepto::TerminalSource<VectorType> _networkInput;
	percepto::Parameters::Ptr _networkParameters;

	BroadcastMultiReceiver _inputStreams;
	double _sampleDevs;

	MultivariateGaussian<> _dist;
	ros::Timer _timer;

	void UpdateCallback( const ros::TimerEvent& event );
};

}