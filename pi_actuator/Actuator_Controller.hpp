#ifndef ACTUATOR_CONTROLLER_HPP
#define ACTUATOR_CONTROLLER_HPP

#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <float.h>
#include <boost/thread.hpp>

#include "positioner.hpp"
#include "APTAPI.h"
#include "HANDLE_ERROR.h"


class Actuator_Controller : public Positioner
{
	// Common members
private:
	long plNumUnits;
	long plSerialNum[2];
	bool returnFlag;
public:
	Actuator_Controller(); // Initialize the connection to the controller, set up the actuator stage
	~Actuator_Controller(); // Default destructor

	// Members related to the X axis (CHAN1 axis)
public:
	bool moveToX(const double to); // Move to any absolute position on the X axis
	bool moveToY(const double to); // Move to position in range within on the Y axis
	
	// Members related to the Y axis (CHAN2 axis)
private:	
	const double lowPosLimit; // Lower limit of the moving range. Unit: 1.0E-6 meter
	const double upPosLimit; // Up limit of the moving range for Y-axis
	const double maxVelocity; // Maximum velocity
	const double maxAcceleration; // Maximum acceleration
	const double micro; // Const parameter for unit conversion

	double magicAcc;
	
	struct actMovement { // Actual "from & to" of each current movement 
		double actFrom, actTo;
	} oneActMovement;

	boost::thread workerThread; // workerThread for runCmd()
		
public:
	double minPos(); // Return the lower limit of the range
	double maxPos(); // Return the upper limit of the range, for addMovements
	double minPos2(); // for moveTo()
	double maxPos2();
	double maxVel(); // Return the maximum possible velocity
	double maxAcc(); // Return the maximum possible acceleration
	
	bool curPos(double* pos); // current position in um
	bool moveTo(const double to); // move as quick as possible to "to"

	bool prepareCmd(); // after prepare, runCmd() will move it in real.
    bool runCmd();
	bool waitCmd(); // wait all movements to complete
	bool abortCmd(); // abort the current set of movements

	bool testCmd();
	
//protected:
	int getMovementsSize(); // get the size of the movements
	bool haltAxisMotion(); // halt the motion of Y axis (channel 2 axis)
	bool setVelParams(const double vel, const double acc); // set up the velocity & acc of the Y axis movement
	
	void runMovements();

	bool prepare(const int i); // prepare, run, wait within each runMovements()
	bool run(const int i);
	bool wait(const int i);

	void setReturnFlag(const bool i);
	bool getReturnFlag();

};

#endif // ACTUATOR_CONTROLLER_HPP