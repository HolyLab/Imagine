#include "Actuator_Controller.hpp"

Actuator_Controller::Actuator_Controller() : lowPosLimit(10000.0), upPosLimit(40000.0), maxVelocity(1100.0), maxAcceleration(1000.0), micro(1000.0)
{
	HANDLE_ERROR(APTInit()); // Initialize and set up the connection to the actuator controller
	long lHWType = HWTYPE_SCC001;
	HANDLE_ERROR(GetNumHWUnitsEx(lHWType, &this->plNumUnits)); // Get the number of HWUnits
	long lIndex = CHAN1_INDEX;
	HANDLE_ERROR(GetHWSerialNumEx(lHWType, lIndex, &this->plSerialNum[CHAN1_INDEX])); // Get the serial number of the X axis HWUnit
	lIndex = CHAN2_INDEX;
	HANDLE_ERROR(GetHWSerialNumEx(lHWType, lIndex, &this->plSerialNum[CHAN2_INDEX])); // Get the serial number of the Y axis HWUnit

	for (int i = 0; i < 2; ++i) { // Move the two axis back to home
		long lSerialNum = this->plSerialNum[i];
		HANDLE_ERROR(InitHWDevice(lSerialNum)); // Each time initialize the specific HWUnit before using

		float fMinVel = 0.0f,
			fAccn = 2.0f, // unit: mm/s^2
			fMaxVel = 2.0f; // unit: mm/s
		HANDLE_ERROR(MOT_SetVelParams(lSerialNum, fMinVel, fAccn, fMaxVel));

		long plDirection = 2, 
			plLimSwitch = 1;
		float pfHomeVel = 2.0f,
			pfZeroOffset = 0.1f;
		HANDLE_ERROR(MOT_SetHomeParams(lSerialNum, plDirection, plLimSwitch, pfHomeVel, pfZeroOffset));

		BOOL bWait = true;
		HANDLE_ERROR(MOT_MoveHome(lSerialNum, bWait)); // Move to Home & wait till complete

		MOT_SetBLashDist(lSerialNum, 0.0f); // Set the back slash length to 0.0f
	}
}

Actuator_Controller::~Actuator_Controller() 
{
	for(int i=0; i<2; ++i) {
		long lSerialNum = this->plSerialNum[i];
		HANDLE_ERROR(InitHWDevice(lSerialNum)); // Each time initialize the specific HWUnit before using
		HANDLE_ERROR(MOT_StopProfiled(lSerialNum)); // Halt the axis motion		
	}
	HANDLE_ERROR(APTCleanUp()); // Disconnect from the actuator controller
	clearCmd(); // Clean up the movements vector
}

bool Actuator_Controller::moveToX(const double to) // Alert: Must wait this movement done before other actions
{	
	long lSerialNum = this->plSerialNum[CHAN1_INDEX];
	HANDLE_ERROR(InitHWDevice(lSerialNum)); // Initialize the X axis HWUnit first

	float fMinVel = 0.0f,
		  fMaxVel = 2.0f, // unit: mm/s
		  fAccn = 2.0f; // unit: mm/s^2
	HANDLE_ERROR(MOT_SetVelParams(lSerialNum, fMinVel, fAccn, fMaxVel)); // Set up default params

	float fAbsPos = static_cast<float>(to);
	const float lowLimit = 0.0f; // Hard-coded lower- & upper- position limits
	const float upLimit = 70000.0f;
	if(fAbsPos >= lowLimit && fAbsPos <= upLimit) {
		fAbsPos /= static_cast<float>(micro);
		bool bWait = true;
		haltAxisMotion(); // Safe protection, halt any motion
		HANDLE_ERROR(MOT_MoveAbsoluteEx(lSerialNum, fAbsPos, bWait)); // Move to the absolute pos & wait for complete
		return true;
	} 
	else {
		printf("The desired X axis position is out-of-range. \n");
		return false;
	}
}

bool Actuator_Controller::moveToY(const double to)
{
	long lSerialNum = this->plSerialNum[CHAN2_INDEX];
	HANDLE_ERROR(InitHWDevice(lSerialNum)); // Initialize the Y axis HWUnit first

	float fMinVel = 0.0f, 
		  fMaxVel = 2.0f, // unit: mm/s
		  fAccn = 2.0f; // unit: mm/s^2
	HANDLE_ERROR(MOT_SetVelParams(lSerialNum, fMinVel, fAccn, fMaxVel)); // Set up default params

	float fAbsPos = static_cast<float>(to);
	if (fAbsPos >= minPos2() && fAbsPos <= maxPos2()) {
		fAbsPos /= static_cast<float>(micro);
		bool bWait = true;
		haltAxisMotion();
		HANDLE_ERROR(MOT_MoveAbsoluteEx(lSerialNum, fAbsPos, bWait)); // Move to the absolute pos & wait for complete
		return true;
	} 
	else {
		printf("The desired Y axis position is out-of-range. \n");
		return false;
	}
}

double Actuator_Controller::minPos() // small range
{ 
	return this->lowPosLimit;
}
double Actuator_Controller::maxPos()
{
	if(getDim() == 0) return 70000.0;
	if(getDim() == 1) return this->upPosLimit;
}

double Actuator_Controller::minPos2() // big range
{
	double local_lowPosLimit = 0.0;
	return local_lowPosLimit;
}
double Actuator_Controller::maxPos2()
{
	if(getDim() == 0) return 70000.0;
	double local_upPosLimit = 50000.0;
	if(getDim() == 1) return local_upPosLimit;
}

double Actuator_Controller::maxVel()
{
	return this->maxVelocity;
}

double Actuator_Controller::maxAcc() 
{
	return this->maxAcceleration;
}

bool Actuator_Controller::curPos(double* pos) // current position in um
{
	long lSerialNum;
	if(getDim() == 0) lSerialNum = this->plSerialNum[CHAN1_INDEX];
	if(getDim() == 1) lSerialNum = this->plSerialNum[CHAN2_INDEX];
	HANDLE_ERROR(InitHWDevice(lSerialNum));
	float CurrentPos;
	HANDLE_ERROR(MOT_GetPosition(lSerialNum, &CurrentPos)); // Get the current position
	*pos = static_cast<double>(CurrentPos) * this->micro;
	return true;
}

bool Actuator_Controller::moveTo(const double to)
{
	if(getDim() == 0)
		return moveToX(to);
	else if(getDim() == 1)
		return moveToY(to);	
}

bool Actuator_Controller::prepareCmd()
{
	if(!haltAxisMotion()) {
		printf("ERROR: The channel axis is STILL moving. \n");
		return false;
	} 
	else
		return true;
}

bool Actuator_Controller::runCmd()
{
	setReturnFlag(false);
	this->workerThread = boost::thread(&Actuator_Controller::runMovements, this); // create & initialize the workThread

	do {
		timewait(5);
	} while (!getReturnFlag());

	return true;
}

bool Actuator_Controller::waitCmd()
{
	this->workerThread.join();	// the main thread joins the worker thread and sleeps
	return true;
}

bool Actuator_Controller::abortCmd()
{
	this->workerThread.interrupt(); // send request to interrupt the workerThread
	this->workerThread.join(); // the main thread joins the workerThread and waits it to terminate
	return true;
}

bool Actuator_Controller::testCmd()
{
	// Only need to check the first movement parameters

	double from = (*this->movements[0]).from;
	double to = (*this->movements[0]).to;
	double duration = (*this->movements[0]).duration;

	if (from > this->maxPos2() || from < this->minPos2()) {
		printf("Error: the START position is out-of-bound. \n");
		return false;
	}
	if (to > this->maxPos2() || to < this->minPos2()) {
		printf("Error: the STOP position is out-of-bound. \n");
		return false;
	}
	if (abs(to - from) < 1.0) {
		printf("Warning: the distance between the START & STOP positons are less than 1 um. "
			"The stage is in no-movement scanning setting. \n");
		return true;
	}

	double Velocity = abs(to - from) / (duration / this->micro / this->micro); // unit: micrometre / second
	if (Velocity > maxVel()) {
		printf("Error: The calculated stage movement velocity is greater than 1.1 mm/s, the maximal allowed velocity. " 
			"The stage velocity is equal to (the travel length / the travel time). "
			"Please change the camera & position settings accordingly. \n");
		return false;
	}
	else if (Velocity < 100.0) { // the lower limit of Bakewell Stage velocity, 0.1 mm/s
		printf("Error: The calculated stage movement velocity is less than 0.1 mm/s, the minimum allowed velocity. "
			"The stage velocity is equal to (the travel length / the travel time). "
			"Please change the camear & position settings accordingly. \n");
		return false;
	}

	double Acceleration = maxAcc(); // unit: micrometer / second^2
	double Length = 4.0 * Velocity * Velocity / Acceleration; // Extra travel length for acceleration & deceleration
	double actFrom, actTo; // The actual from & to of each movement

	if (from <= to) {		
		actFrom = from - Length;
		actTo = to + Length;
	} 
	else if (from > to) {
		actFrom = from + Length;
		actTo = to - Length;
	}

	if (actFrom >= minPos2() && actFrom <= maxPos2() && actTo >= minPos2() && actTo <= maxPos2())
		return true;
	else if (actFrom < minPos2() || actFrom > maxPos2()) {
		printf("Error: The stage movement requires adding an extra acceleration length to the START position "
			"so as to make sure that the stage is moving at a constant speed in the required range. "
			"This extra acceleration length is equal to (4.0 * velocity^2 / acceleration). "
			"IN THIS CASE -- the extra acceleration length can not be satisfied. "
			"Please change camera & position settings accordingly. \n");
		return false;
	}
	else if (actTo < minPos2() || actTo > maxPos2()) {
		printf("Error: The stage movement requires adding an extra acceleration length to the STOP position "
			"so as to make sure that the stage is moving at a constant speed in the required range. "
			"The extra acceleration length is equal to (4.0 * velocity^2 / acceleration). "
			"IN THIS CASE -- the extra acceleration length can not be satisfied. "
			"Please change camera & position settings accordingly. \n");
		return false;
	}
}

int Actuator_Controller::getMovementsSize()
{
	return this->movements.size();
}

bool Actuator_Controller::haltAxisMotion()
{
	long lSerialNum;
	if(getDim() == 0) lSerialNum = this->plSerialNum[CHAN1_INDEX];
	if(getDim() == 1) lSerialNum = this->plSerialNum[CHAN2_INDEX];
	HANDLE_ERROR(InitHWDevice(lSerialNum));
	HANDLE_ERROR(MOT_StopProfiled(lSerialNum));
	return true;
}

bool Actuator_Controller::setVelParams(const double vel, const double acc)
{
	float newVelocity;
	if(!(vel > 0.0)) return false;
	if(vel < maxVel())
		newVelocity = static_cast<float>(vel) / static_cast<float>(this->micro);
	else
		newVelocity = static_cast<float>(maxVel()) / static_cast<float>(this->micro);
	
	float newAcceleration;
	if(!(acc > 0.0)) return false;
	if(acc < maxAcc())
		newAcceleration = static_cast<float>(acc) / static_cast<float>(this->micro);
	else
		newAcceleration = static_cast<float>(maxAcc()) / static_cast<float>(this->micro);

	long lSerialNum = this->plSerialNum[CHAN2_INDEX];
	HANDLE_ERROR(InitHWDevice(lSerialNum));
	float fMinVel = 0.0f,
		  fAccn = newAcceleration,
		  fMaxVel = newVelocity;
	HANDLE_ERROR(MOT_SetVelParams(lSerialNum, fMinVel, fAccn, fMaxVel));
	return true;
}

void Actuator_Controller::runMovements()
{
	for(int i=0; i<getMovementsSize(); i++) {
		double from = (*this->movements[i]).from;
		double to = (*this->movements[i]).to;

		printf(" Inside of runMovement: %d %f %f %d %d %d \n", i, from, to, _isnan(from), _isnan(to), (*this->movements[i]).trigger);
		if(_isnan(from) && _isnan(to)) {
			// No actual triggering function existed for Thorlabs bsc102
			// Wait for the required time
			double duration = (*this->movements[i]).duration;
			if(duration > 0.0) timewait(duration);
		}
		else {
			if(!prepare(i)) {
				printf("ERROR: The prepare() fails at movement %d \n", i);
				return;
			}
			if(!run(i)) {
				printf("ERROR: The run() fails at movement %d \n", i);
				return;
			}
			if(!wait(i)) {
				printf("ERROR: The wait() fails at movement %d \n", i);
				return;
			}
		}		
	}
}

bool Actuator_Controller::prepare(const int i)
{
	if(i == 0) {
		double from = (*this->movements[i]).from;
		double to = (*this->movements[i]).to;
		double duration = (*this->movements[i]).duration;
		int trigger = (*this->movements[i]).trigger;

		// special case: to = from, no-movement scanning
		if(abs(to - from) < 1.0) {
			if(!moveTo(from)) return false; // Move to "from"
			return true;
		}

		// general case
		double Velocity = abs(to - from) / (duration / this->micro / this->micro); // unit micrometre / second
		double Acceleration = maxAcc(); // unit: micrometer / second^2

		double Length = 4.0 * Velocity * Velocity / Acceleration; // Extra travel length for acceleration
		double actFrom, actTo; // The actual from & to of each movement

		if(from <= to) {		
			actFrom = from - Length;
			actTo = to + Length;
		} 
		else if(from > to) {
			actFrom = from + Length;
			actTo = to - Length;
		}
		oneActMovement.actFrom = actFrom;
		oneActMovement.actTo = actTo;

		if(!moveTo(actFrom)) return false; // Move to "actFrom"
		if(!setVelParams(Velocity, Acceleration)) return false;
	}
	else if(i == 1) {
		double Velocity = this->maxVel();
		double Acceleration = maxAcc(); // unit: micrometer / second^2
		if(!setVelParams(Velocity, Acceleration)) return false;
	}

	return true;
}

bool Actuator_Controller::run(const int i)
{
	// Special case: to = from
	double from = (*this->movements[i]).from;
	double to = (*this->movements[i]).to;
	if(abs(to - from) < 1.0) return true;

	// General case
	double actTo;
	if(i == 0) actTo = this->oneActMovement.actTo;
	if(i == 1) actTo = this->oneActMovement.actFrom;

	long lSerialNum = this->plSerialNum[CHAN2_INDEX];

	float fAbsPos = static_cast<float>(actTo);
	if (fAbsPos >= minPos2() && fAbsPos <= maxPos2()) {
		fAbsPos /= static_cast<float>(micro);
		bool bWait = false;
		haltAxisMotion();
		HANDLE_ERROR(MOT_MoveAbsoluteEx(lSerialNum, fAbsPos, bWait)); // Move to the absolute pos: to
		return true;
	} else {
		printf("The desired Y axis position is out-of-range. \n");
		return false;
	}
}

bool Actuator_Controller::wait(const int i)
{	
	double from, to, actFrom, actTo, duration;

	// Special case: to = from
	from = (*this->movements[0]).from;
	to = (*this->movements[0]).to;
	duration = (*this->movements[0]).duration;
	if (abs(to - from) < 1.0) {		
		duration /= this->micro;
		timewait(duration);
		// Execution interuption is not implemented here.
	}
	else { 
		// General case
		if(i == 0) {
			from = (*this->movements[i]).from;
			to = (*this->movements[i]).to;
			actFrom = oneActMovement.actFrom;
			actTo = oneActMovement.actTo;
		}
		else if(i == 1) {
			actFrom = oneActMovement.actTo;
			actTo = oneActMovement.actFrom;
		}

		int trigger = (*this->movements[i]).trigger;

		long lSerialNum = this->plSerialNum[CHAN2_INDEX];
		float CurrentPos;

		do {
			timewait(300);
			HANDLE_ERROR(MOT_GetPosition(lSerialNum, &CurrentPos)); // Get the current position
			CurrentPos *= static_cast<float>(micro);

			if (i == 0) {
				if ((CurrentPos < to && CurrentPos > from) || (CurrentPos > to && CurrentPos < from))
					setReturnFlag(true);
			}

			if (abs((CurrentPos - actTo) / (actFrom - actTo)) < 1.0E-3) { // Detect the end of the motion
				timewait(300);
				if (!haltAxisMotion()) printf("Error in haltAxisMotion(). \n");
				timewait(300);
				break;
			}

			if(boost::this_thread::interruption_requested()) {
				if (!haltAxisMotion()) printf("Error in haltAxisMotion(). \n"); // Halt the motion before terminating the thread
				timewait(300);
				break;
			}
		} while (true);
	}

	if(boost::this_thread::interruption_requested())
		boost::this_thread::interruption_point(); // Abort the current waiting process if requested

	return true;
}

void Actuator_Controller::setReturnFlag(const bool i) {
	this->returnFlag = i;
}

bool Actuator_Controller::getReturnFlag() {
	return this->returnFlag;
}
