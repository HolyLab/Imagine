#include "Actuator_Controller.hpp"

Actuator_Controller::Actuator_Controller() : lowPosLimit(10000.0), upPosLimit(40000.0), maxVelocity(1100.0),maxAcceleration(1000.0), micro(1000.0)
{
	HANDLE_ERROR(APTInit()); // Initialize and set up the connection to the actuator controller

	long lHWType = HWTYPE_SCC001;
	HANDLE_ERROR(GetNumHWUnitsEx(lHWType, &this->plNumUnits)); // Get the number of HWUnits 
	// printf("The number of HWUnits is %d \n", this->plNumUnits);

	long lIndex = CHAN1_INDEX;
	HANDLE_ERROR(GetHWSerialNumEx(lHWType, lIndex, &this->plSerialNum[0])); // Get the serial number of the X axis HWUnit 
	// printf("The serical number of the X axis HWUnit is %d \n", this->plSerialNum[0]); 
	lIndex = CHAN2_INDEX;
	HANDLE_ERROR(GetHWSerialNumEx(lHWType, lIndex, &this->plSerialNum[1])); // Get the serial number of the Y axis HWUnit
	// printf("The serical number of the Y axis HWUnit is %d \n", this->plSerialNum[1]);

	for (int i = 0; i < 2; ++i) {
		long lSerialNum = this->plSerialNum[i];
		HANDLE_ERROR(InitHWDevice(lSerialNum)); // Each time initialize the specific HWUnit before using

		float fMinVel = 0.0, 
			fAccn = 2.0, // unit: mm/s^2
			fMaxVel = 2.0; // unit: mm/s
		HANDLE_ERROR(MOT_SetVelParams(lSerialNum, fMinVel, fAccn, fMaxVel)); 
		// HANDLE_ERROR(MOT_GetVelParams(lSerialNum, &fMinVel, &fAccn, &fMaxVel)); // Set up & Get default params
		// printf("The Vel parameters are %d %f %f %f. \n", i, fMinVel, fAccn, fMaxVel);

		long plDirection = 2, 
			plLimSwitch = 1;
		float pfHomeVel = 2.0,
			pfZeroOffset = 0.1;
		HANDLE_ERROR(MOT_SetHomeParams(lSerialNum, plDirection, plLimSwitch, pfHomeVel, pfZeroOffset));
		// HANDLE_ERROR(MOT_GetHomeParams(lSerialNum, &plDirection, &plLimSwitch, &pfHomeVel, &pfZeroOffset)); // Set up & Get default home params
		// printf("The home Vel parameters are %d %d %d %f %f. \n", i, plDirection, plLimSwitch, pfHomeVel, pfZeroOffset);

		BOOL bWait = true;
		HANDLE_ERROR(MOT_MoveHome(lSerialNum, bWait)); // Move to Home & wait till complete

		// float pfPosition = -1.0;
		// HANDLE_ERROR(MOT_GetPosition(lSerialNum, &pfPosition)); // Get the current position
		// printf("The current position is %f \n", pfPosition);

		// set the back slash length to 0.0
		MOT_SetBLashDist(lSerialNum, 0.0);
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
	clearCmd(); // Clean up the movements
}

bool Actuator_Controller::moveToX(const double to) // Alert: wait the movement done before other actions
{	
	long lSerialNum = this->plSerialNum[CHAN1_INDEX];
	HANDLE_ERROR(InitHWDevice(lSerialNum)); // Initialize the X axis HWUnit before using

	float fMinVel = 0.0, 
		fAccn = 2.0, // unit: mm/s^2
		fMaxVel = 2.0; // unit: mm/s
	HANDLE_ERROR(MOT_SetVelParams(lSerialNum, fMinVel, fAccn, fMaxVel)); // Set up default params

	float fAbsPos = static_cast<float>(to);
	const float lowLimit = 0.0;
	const float upLimit = 70000.0;
	if ((fAbsPos >= lowLimit) && (fAbsPos <= upLimit)) {
		fAbsPos /= static_cast<float>(micro);
		bool bWait = true;
		HANDLE_ERROR(MOT_MoveAbsoluteEx(lSerialNum, fAbsPos, bWait)); // Move to the absolute pos & wait for complete
		return true;
	} else {
		printf("The desired X axis position is out-of-range. \n");
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
double Actuator_Controller::minPos2() // large range
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
	if(getDim() == 0) { 
		moveToX(to);
	} else if(getDim() == 1) {
		long lSerialNum = this->plSerialNum[CHAN2_INDEX];
		HANDLE_ERROR(InitHWDevice(lSerialNum)); // Initialize the X axis HWUnit before using

		float fMinVel = 0.0, 
			fAccn = 2.0, // unit: mm/s^2
			fMaxVel = 2.0; // unit: mm/s
		HANDLE_ERROR(MOT_SetVelParams(lSerialNum, fMinVel, fAccn, fMaxVel)); // Set up default params

		float fAbsPos = static_cast<float>(to);
		if ((fAbsPos >= minPos2()) && (fAbsPos <= maxPos2())) {
			fAbsPos /= static_cast<float>(micro);
			bool bWait = true;
			HANDLE_ERROR(MOT_MoveAbsoluteEx(lSerialNum, fAbsPos, bWait)); // Move to the absolute pos & wait for complete
			return true;
		} else {
			printf("The desired Y axis position is out-of-range. \n");
			return false;
		}
	}
}

bool Actuator_Controller::prepareCmd()
{
	if(!haltAxisMotion()) {
		printf("ERROR: The channel axis is STILL moving. \n");
		return false;
	} else {
		return true;
	}
}
bool Actuator_Controller::runCmd()
{
	setReturnFlag(false);
	this->workerThread = boost::thread(&Actuator_Controller::runMovements, this); // create & initialize the workThread
	// printf(" The workerThread ID is %d \n", this->workerThread.get_id());
	do {
		timewait(5);
	} while (!getReturnFlag());
	// printf("workerThread returned \n");
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
	int i = 0; // only check the first movement

	double from = (*this->movements[i]).from;
	double to = (*this->movements[i]).to;
	double duration = (*this->movements[i]).duration;
	int trigger = (*this->movements[i]).trigger;

	double Velocity = abs(to - from) / (duration / this->micro / this->micro); // unit micrometre / second
	if (Velocity > maxVel()) {
		printf("Error in the velocity requirement of movement %d \n", i);
		return false;
	}

	double Acceleration = maxAcc(); // unit: micrometer / second^2

	double Length = 4.0 * Velocity * Velocity / Acceleration; // Extra travel length for acceleration
	double actFrom, actTo; // The actual from & to of each movement

	if(from <= to) {		
		actFrom = from - Length;
		actTo = to + Length;
	} else if(from > to) {
		actFrom = from + Length;
		actTo = to - Length;
	}
	if ((from >= minPos2()) && (from <= maxPos2()) && (to >= minPos2()) && (to <= maxPos2())) {
		return true;
	}
	else {
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
	if(vel < maxVel()) {
		newVelocity = static_cast<float>(vel) / static_cast<float>(this->micro);
	} else {
		newVelocity = static_cast<float>(maxVel()) / static_cast<float>(this->micro);
	}

	float newAcceleration;
	if(!(acc > 0.0)) return false;
	if(acc < maxAcc()) {
		newAcceleration = static_cast<float>(acc) / static_cast<float>(this->micro);
	} else {
		newAcceleration = static_cast<float>(maxAcc()) / static_cast<float>(this->micro);
	}

	long lSerialNum = this->plSerialNum[CHAN2_INDEX];
	HANDLE_ERROR(InitHWDevice(lSerialNum));
	float fMinVel = 0.0;
	float fAccn = newAcceleration;
	float fMaxVel = newVelocity;
	HANDLE_ERROR(MOT_SetVelParams(lSerialNum, fMinVel, fAccn, fMaxVel));
	return true;
}

void Actuator_Controller::runMovements()
{
	for(int i=0; i<getMovementsSize(); i++)
	{
		double from = (*this->movements[i]).from;
		double to = (*this->movements[i]).to;

		printf(" Inside of runMovement: %d %f %f %d %d %d \n", i, from, to, _isnan(from), _isnan(to), (*this->movements[i]).trigger);
		if( !_isnan(from) || _isnan(to) )
		{
			if(!Triggering(i))
			{
				printf("ERROR: The pure triggering() did nothing at movement %d \n", i);
				//return;
			}
		}
		else
		{
			if(!prepare(i)) 
			{
				printf("ERROR: The prepare() fails at movement %d \n", i);
				return;
			}
			if(!run(i)) 
			{
				printf("ERROR: The run() fails at movement %d \n", i);
				return;
			}
			if(!wait(i))
			{
				printf("ERROR: The wait() fails at movement %d \n", i);
				return;
			}
		}		
	}
	return;
}

bool Actuator_Controller::prepare(const int i)
{
	if(i == 0) {
		double from = (*this->movements[i]).from;
		double to = (*this->movements[i]).to;
		double duration = (*this->movements[i]).duration;
		int trigger = (*this->movements[i]).trigger;

		double Velocity = abs(to - from) / (duration / this->micro / this->micro); // unit micrometre / second
		double Acceleration = maxAcc(); // unit: micrometer / second^2

		double Length = 4.0 * Velocity * Velocity / Acceleration; // Extra travel length for acceleration
		double actFrom, actTo; // The actual from & to of each movement

		if(from <= to) {		
			actFrom = from - Length;
			actTo = to + Length;
		} else if(from > to) {
			actFrom = from + Length;
			actTo = to - Length;
		}
		oneActMovement.actFrom = actFrom;
		oneActMovement.actTo = actTo;
		this->magicActFrom = actFrom;

		// printf(" Movement: %d %f %f %f %f \n", i, from, to, actFrom, actTo);

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
	double actTo;
	if(i == 0) actTo = oneActMovement.actTo;
	if(i == 1) actTo = this->magicActFrom;

	long lSerialNum = this->plSerialNum[CHAN2_INDEX];

	float fAbsPos = static_cast<float>(actTo);
	if ((fAbsPos >= minPos2()) && (fAbsPos <= maxPos2())) {
		fAbsPos /= static_cast<float>(micro);
		bool bWait = false;
		HANDLE_ERROR(MOT_MoveAbsoluteEx(lSerialNum, fAbsPos, bWait)); // Move to the absolute pos: to
		return true;
	} else {
		printf("The desired Y axis position is out-of-range. \n");
		return false;
	}
}
bool Actuator_Controller::wait(const int i)
{	
	double from = (*this->movements[i]).from;
	double to = (*this->movements[i]).to;
	double actFrom = oneActMovement.actFrom;
	double actTo = oneActMovement.actTo;
	int trigger = (*this->movements[i]).trigger;

	long lSerialNum = this->plSerialNum[CHAN2_INDEX];
	float CurrentPos;
	// int trigStatus = 0; // 0 for not trigged yet, 1 for already trigged
	do {
		HANDLE_ERROR(MOT_GetPosition(lSerialNum, &CurrentPos)); // Get the current position
		CurrentPos *= static_cast<float>(micro);
		if ((CurrentPos < to && CurrentPos > from) || (CurrentPos > to && CurrentPos < from)) {
			if (i == 0) setReturnFlag(true);
		}

		if (abs((CurrentPos - actTo) / actTo) < 1.0E-3) { // Detect the end of the motion
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

	if(boost::this_thread::interruption_requested()) {
		boost::this_thread::interruption_point(); // abort the current waiting process if requested
	}

	return true;
}

bool Actuator_Controller::setReturnFlag(const bool i) {
	this->returnFlag = i;
	return true;
}

bool Actuator_Controller::getReturnFlag() {
	return this->returnFlag;
}

bool Actuator_Controller::Triggering(const int i)
{
	double duration = (*this->movements[i]).duration;
	clock_t endwait;
	endwait = clock () + long(duration / this->micro / this->micro * double(CLOCKS_PER_SEC));
	while(clock() < endwait) {}	
	return false; // Triggering can not be actually executed
}