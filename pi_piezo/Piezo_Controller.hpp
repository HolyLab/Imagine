#ifndef PIEZO_CONTROLLER_HPP
#define PIEZO_CONTROLLER_HPP

#include "positioner.hpp"
#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <float.h>
#include <boost/thread.hpp>
#include "PI_GCS2_DLL.h"
	// Usage of Lib file to link dll statically in MS VC++
	//   - make sure the linker finds the PI_GCS2_DLL.lib
	//   - make sure the PI_GCS2_DLL.dll is accessible at program launch

class Piezo_Controller : public Positioner
{
private:
	const double lowPosLimit; // Lower limit of the piezo stage moving range
	const double upPosLimit; // Up limit of the piezo stage moving range

	const double maxVelocity; // Maximum velocity
	const double maxAcceleration; // Maximum acceleration. Unit: ALL micrometre
	const double maxDeceleration; // Maximum deceleration

	const double micro; // Const parameter for unit conversion
	
	int USBID; // ID of the E-861 controller USB port
	char szAxis[4]; // Axis information of the piezo stage

	double magicActFrom;
	double magicAcc;

	struct RecordedPositions { // Struct for storing the actualPositions recorded by PI Piezo
		double actualPositions[1024];
		RecordedPositions(const double inputPos[]) {
			memcpy(this->actualPositions, inputPos, 1024*sizeof(double));
		}
		~RecordedPositions();
	};
	vector<RecordedPositions*> recdPos;

	struct actMovement // Actual "from & to" of each current movement 
	{
		double actFrom,
			actTo;
	} oneActMovement;

	boost::thread workerThread; // workerThread for runCmd()

	double ParaLnth; // Parameter for Acceleration Length
	double ParaAccr; // Parameter for Acceleration Rate
		
public:
	Piezo_Controller(); // Initialize the connection to the controller, set up the piezo stage
	~Piezo_Controller(); // Default destructor

	double minPos(); // Return the min value of the position
	double maxPos(); // Return the max value of the position. NOTE: the unit is micrometer

	double maxVel(); // Return the maximum possible velocity of the piezo stage
	double maxAcc(); // Return the maximum possible acceleration of the piezo stage
	double maxDec(); // Return the maximum possible deceleration
	
	bool moveTo(const double to); // move as quick as possible to "to"
	bool testCmd(); // test the validaty of the first movement

	bool prepareCmd(); // after prepare, runCmd() will move it in real.
    bool runCmd();
	bool waitCmd(); // wait all movements to complete
	bool abortCmd(); // abort the movements
	
	bool curPos(double* pos); // current position in um

protected:
	int getMovementsSize(); // get the size of the movements

	bool checkControllerReady(); // check the piezo controller is ready for command input
	bool haltAxisMotion(); // halt the motion of axis

	bool setVelocity(const double vel); // set up the velocity of the piezo stage
	bool setAcceleration(const double acc); // set up the acceleration of the piezo stage
	bool setDeceleration(const double dec); // set up the acceleration of the piezo stage

	void runMovements();

	bool prepare(const int i);
	bool run(const int i);
	bool wait(const int i);

	bool Qmoving(const double to); // Return immediately after sending the move command
	bool moving(const double to); // Move with Non-trigger & destination-checking
	bool Triggering(const int i); // Pure triggering
	bool setTrigger(const int trigger); // Change the trigger signal
};

#endif // PIEZO_CONTROLLER_HPP