#include "Piezo_Controller.hpp"

#include "timer_g.hpp"
#include <iostream>

extern Timer_g gTimer;

Piezo_Controller::Piezo_Controller() : lowPosLimit(500.0), upPosLimit(18500.0), maxVelocity(1500.0),maxAcceleration(10000.0), maxDeceleration(10000.0), micro(1000.0)
{
	//
	// Inquire and set up the connection to the E-861 controller USB port
	//

	const int USBInfoSize = 100;
	char *USBInfo = (char *) malloc(sizeof(char)*USBInfoSize);
	const char *USBFilter = "E-861";
	int NumOfUSB = PI_EnumerateUSB(USBInfo, USBInfoSize, USBFilter); // Inquire all the available "E-861" controller USB ports
	if(NumOfUSB != 1) { // In our case, only one "E-861" USB port exists in the system
		printf("\n ERROR: The inquire of the available E-861 USB port fails. \n");
		return;
	}

	this->USBID = PI_ConnectUSB(USBInfo); // Connect to this single "E-861" USB port
	if(this->USBID != 0) {
		printf("ERROR: The connection to USB Port fails. \n");
		return;
	}

	if(!PI_IsConnected(this->USBID)) { // Double check that the USB port is connected
		printf("ERROR: The USB Port %d is not connected. \n", this->USBID);
		return;
	}

	//
	// Inquire the stage axis information and set the SVO mode ON
	//

	this->szAxis[0] = 'x'; // Initialize the axis information
	this->szAxis[1] = 'x';
	this->szAxis[2] = 'x';
	this->szAxis[3] = 'x';	

	if(!PI_qSAI(this->USBID, this->szAxis, 4)) { // Inquire the stage axis information
		printf("ERROR: The inquire of stage axis information fails. \n");
		return;
	}

	this->szAxis[1] = '\0'; // Only use the first axis as only 1 axis exists in the piezo stage
	BOOL bFlag = true;
	if(!PI_SVO(this->USBID, &this->szAxis[0], &bFlag)) { // Set the SVO mode ON
		printf("ERROR: The SVO mode can not be set ON. \n");
		return;
	}

	if(!PI_qSVO(this->USBID, &this->szAxis[0], &bFlag)) { // Inquire the SVO mode of the axis
		printf("ERROR: The inquire of the SVO mode fails. \n");
		return;
	}
	else {
		if(!bFlag) {
			printf("ERROR: The SVO mode can not be set ON. \n");
			return;
		}
	}

	//
	// Set up the default piezo stage velocity and acceleration
	//

	const double Velocity = maxVel(); // unit micrometre
	if(!setVelocity(Velocity)) { // Set up the new piezo stage velocity
		printf("ERROR: The setting of new velocity fails. \n");
		return;
	}
	const double Acceleration = maxAcc(); // unit micrometre
	if(!setAcceleration(Acceleration)) { // Set up the new piezo stage acceleration
		printf("ERROR: The setting of new acceleration fails. \n");
		return;
	}
	const double Deceleration = maxDec(); // unit micrometre
	if(!setDeceleration(Deceleration)) { // Set up the new piezo stage acceleration
		printf("ERROR: The setting of new deceleration fails. \n");
		return;
	}

	//
	// Inquire whether the piezo stage has a reference sensor and set the Reference mode ON
	//

	BOOL refSwitch = false;
	if(PI_qTRS(this->USBID, &this->szAxis[0], &refSwitch)) { // Inquire whether the axis has reference sensor
		if(!refSwitch) {
			printf("ERROR: The piezo stage has no reference switch. \n");
			return;
		}
	}
	else {
		printf("ERROR: The piezo stage reference switch information can not be inquired. \n");
		return;
	}

	if(!PI_FRF(this->USBID, &this->szAxis[0])) { // Reference the first axis of the piezo stage
		printf("ERROR: The piezo stage axis can not be referenced. \n");
		return;
	}

	BOOL ControllerReady = false;
	while(!ControllerReady) {
		if(!PI_IsControllerReady(this->USBID, &ControllerReady)) { // Check whether the reference is done
			printf("ERROR: The inquire of controller status fails. \n");
			return;
		}
	}

	BOOL frfResult = false;
	if(PI_qFRF(this->USBID, &this->szAxis[0], &frfResult)) { // Inquire whether the axis has already been referenced
		if(!frfResult) {
			printf("ERROR: the piezo stage can not be referenced. \n");
			return;
		}
	}
	else {
		printf(" The inquire of Piezo stage reference information fails \n");
		return;
	}

	//
	// Set the trigger output port to output +0.0V TTL signal by default
	//

	const long OutputChannel[1] = {0};
	const BOOL ChannelLow[1] = {0};
	const int ArraySize = 1;
	if(!PI_DIO(this->USBID, OutputChannel, ChannelLow, ArraySize)) { // Set +0.0V TTL to all the Ouput ports
		printf("ERROR: The setting of +0.0V ouput trigger to all output ports fails \n");
		return;
	}

	//
	// Initialize ParaLnth & ParaAccr
	//

	this->ParaLnth = 0.5;
	this->ParaAccr = 10000.0;

	//
	// Set the default position recorder parameters
	//

	// Set Table 1 to record the actual position
	int recordTableId[] = {1}; // piezo internal record table ID
	char recordSouceId[] = "1"; // piezo record source axis ID
	int recordOptions[] = {2}; // piezo record options
	if (!PI_DRC(this->USBID, recordTableId, recordSouceId, recordOptions)) {
		printf("ERROR: The settting of the position record options for table 1 fails. \n");
		return;
	}

	// Set Table 2 not to record any data
	recordTableId[0] = 2;
	recordOptions[0] = 0;
	if (!PI_DRC(this->USBID, recordTableId, recordSouceId, recordOptions)) {
		printf("ERROR: The settting of the position record options for table 2 fails. \n");
		return;
	}

	// Set the position data trigger to be type "1"
	recordTableId[0] = 0;
	int triggerSouceId[] = {1};
	if (!PI_DRT(this->USBID, recordTableId, triggerSouceId, "0", 1)) {
		printf("ERROR: The setting of the position record triggering option fails. \n");
		return;
	}

	// Set the piezo position recorder rate
	int recordRate = 10;
	if (!PI_RTR(this->USBID, recordRate)) {
		printf("ERROR: The setting of the position recorder rate fails. \n");
		return;
	}

	// Set the default scan type to be uni-directional
	this->setScanType(false);
	this->setPCount();
}

Piezo_Controller::~Piezo_Controller() 
{
	if(!checkControllerReady()) {
		printf("ERROR: The controller is NOT ready. \n");
		return;
	}
	if(!haltAxisMotion()) {
		printf("ERROR: The axis is STILL moving. \n");
		return;
	}
	PI_CloseConnection(this->USBID); // Close the connection to "E-861" USB port
	if(PI_IsConnected(this->USBID)) { // Check whether the USB port is disconnected
		printf("ERROR: The USB Port %d is STILL connected. \n", this->USBID);
		return;
	}
}

double Piezo_Controller::minPos()
{
	return this->lowPosLimit; 
}
double Piezo_Controller::maxPos()
{
	return this->upPosLimit;
}
double Piezo_Controller::maxVel()
{
	return this->maxVelocity;
}
double Piezo_Controller::maxAcc() 
{
	return this->maxAcceleration;
}
double Piezo_Controller::maxDec()
{
	return this->maxDeceleration;
}

bool Piezo_Controller::moveTo(const double to)
{
	if(!checkControllerReady()) {
		printf("ERROR: The controller is NOT ready. \n");
		return false;
	}
	if(!haltAxisMotion()) {
		printf("ERROR: The axis is STILL moving. \n");
		return false;
	}

	const double Velocity = maxVel(); // Unit micrometre
	if(!setVelocity(Velocity)) {
		printf("ERROR: The setting of new velocity fails. \n");
		return false;
	}
	const double Acceleration = maxAcc(); // Unit micrometre
	if(!setAcceleration(Acceleration)) {
		printf("ERROR: The setting of new acceleration fails. \n");
		return false;
	}
	const double Deceleration = maxDec(); // Unit micrometre
	if(!setDeceleration(Deceleration)) {
		printf("ERROR: The setting of new deceleration fails. \n");
		return false;
	}

	if(!moving(to)) {
		printf("ERROR: The moving to new position %f fails. \n", to);
		return false; // Move and check destination
	}
	else return true;
}

bool Piezo_Controller::testCmd()
{
	int i = 0; // Only test the 1st movement

	double from = (*this->movements[i]).from;
	double to = (*this->movements[i]).to;
	double duration = (*this->movements[i]).duration;

	double Velocity = abs(to - from) / (duration / this->micro / this->micro); // Unit micrometre / second
	this->magicAcc = this->ParaAccr; // Acceleration rate during A->B
	double Acceleration = this->magicAcc; // Unit micrometre / second^2

	double Length = this->ParaLnth * Velocity * Velocity / Acceleration; // Extra travel length for acceleration
	double actFrom, actTo; // The actual from & to of each movement
	if(from <= to) {		
		actFrom = from - Length;
		actTo = to + Length;
	} 
	else if(from > to) {
		actFrom = from + Length;
		actTo = to - Length;
	}
	this->oneActMovement.actFrom = actFrom;
	this->oneActMovement.actTo = actTo;

	// Test the variables
	if((from < this->lowPosLimit) || (from > this->upPosLimit)) {
		this->lastErrorMsg = "The start position is out of bound.";
		return false;
	}
	if((to < this->lowPosLimit) || (to > this->upPosLimit)) {
		this->lastErrorMsg = "The end position is out of bound.";
		return false;
	}
	if(Velocity > this->maxVel()) {
		this->lastErrorMsg = "The velocity will be out of limit.";
		return false;
	}
	if((actFrom < this->lowPosLimit) || (actFrom > this->upPosLimit)) {
		this->lastErrorMsg = "The actFrom is out of bound.";
		return false;
	}
	if((actTo < this->lowPosLimit) || (actTo > this->upPosLimit)) {
		this->lastErrorMsg = "The actTo is out of bound.";
		return false;
	}

	return true;
}

bool Piezo_Controller::prepareCmd()
{
	/*
	if(!checkControllerReady()) {
	this->lastErrorMsg = "The controller is NOT ready.";
	return false;
	}
	if(!haltAxisMotion()) {
	this->lastErrorMsg = "The axis is STILL moving.";
	return false;
	}
	*/

	// Move the piezo stage to the "actFrom"
	// Set the velocity & acceleration for the bi-dir scanning, execute only ONCE

	if (this->pCount == 1) {
		double actFrom = this->oneActMovement.actFrom;
		if(!moveTo(actFrom)) return false;

		if(getScanType()) {
			setScanType(false);
			if(!prepare(0)) {
				this->lastErrorMsg = "The prepare() in prepareCmd fails.";
				return false;
			}
			setScanType(true);
		}

		this->pCount = 0;
	}


	// Set the piezo position record rate
	/*
	int duration = static_cast<int>((*this->movements[0]).duration); // Time in micro second
	int recordRate = duration / 50 / 1024;
	if (!PI_RTR(this->USBID, recordRate)) {
	printf("ERROR: The setting of the position recorder rate fails. \n");
	}
	*/

	return true;
}

bool Piezo_Controller::runCmd()
{
	this->workerThread = boost::thread(&Piezo_Controller::runMovements, this); // Create & initialize the workThread	
	return true;
}

bool Piezo_Controller::waitCmd()
{
	this->workerThread.join();	// the main thread joins the worker thread and sleeps
	return true;
}

bool Piezo_Controller::abortCmd()
{
	this->workerThread.interrupt(); // Send request to interrupt the workerThread
	this->workerThread.join(); // The main thread joins the workerThread and waits till terminate

	if(!checkControllerReady()) {
		this->lastErrorMsg = "The controller is NOT ready.";
		return false;
	}
	if(!haltAxisMotion()) {
		this->lastErrorMsg = "The axis is STILL moving.";
		return false;
	}

	return true;
}

bool Piezo_Controller::curPos(double* pos) // Current position in um (1.0E-6 meter)
{
	if(PI_qPOS(this->USBID, &this->szAxis[0], pos)) { // Inquire the current piezo stage position
		*pos = *pos * this->micro;
		return true;		
	}
	else {
		this->lastErrorMsg="error in curPos";
		return false;
	}
}

bool Piezo_Controller::dumpFeedbackData(const string& filename)
{
	string thisFilename = filename;
	FILE *thisfile = NULL;
	thisfile = fopen(thisFilename.c_str(), "w");
	if (thisfile == NULL) {
		printf("ERROR: Fails to open file for saving the recorded positions. \n");
		return false;
	}

	for (int i = 0; i < this->recordPositionVector.size(); ++i) {
		double *thisActualPos = (*this->recordPositionVector[i]).actualPositions;
		for (int j = 0; j < 1024; ++j) {
			fprintf(thisfile, "%5d %5d %12.8f \n", i, j, thisActualPos[j]);
		}
	}

	for (int i = 0; i < this->recordPositionVector.size(); ++i) {
		delete this->recordPositionVector[i];
	}
	this->recordPositionVector.clear();

	fclose(thisfile);
	return true;
}


int Piezo_Controller::getMovementsSize()
{
	return this->movements.size();
}

bool Piezo_Controller::checkControllerReady()
{
	BOOL ControllerReady = false;
	while(!ControllerReady) {
		if(!PI_IsControllerReady(this->USBID, &ControllerReady)) return false; // Check whether the controller is ready
	}
	return true;
}

bool Piezo_Controller::haltAxisMotion()
{
	BOOL MovingStatus;
	if(PI_IsMoving(this->USBID, &this->szAxis[0], &MovingStatus)) { // Check whether the axis is moving
		if(MovingStatus) {
			if(!PI_HLT(this->USBID, &this->szAxis[0])) { // Halt the motion of axis smoothly
				this->lastErrorMsg="Halt the motion of axis fails.";
				return false; 
			}
			else
				return true;
		}
		else
			return true;
	}
	else {
		this->lastErrorMsg="Halt the motion of axis fails.";
		return false;
	}
}

bool Piezo_Controller::setVelocity(const double vel)
{
	double newVelocity;
	if(!(vel > 0.0)) {
		this->lastErrorMsg="Velocity is less than 0.0.";
		return false;
	}
	if(vel < maxVel())
		newVelocity = vel/this->micro;
	else 
		newVelocity = maxVel()/this->micro;

	if(!PI_VEL(this->USBID, &this->szAxis[0], &newVelocity)) { // Set up the new piezo stage velocity
		this->lastErrorMsg = "The setting of new piezo velocity fails.";
		return false;
	}

	return true;
}

bool Piezo_Controller::setAcceleration(const double acc)
{
	double newAcceleration;
	if(!(acc > 0.0)) return false;
	if(acc < maxAcc())
		newAcceleration = acc/this->micro;
	else
		newAcceleration = maxAcc()/this->micro;

	if(!PI_ACC(this->USBID, &this->szAxis[0], &newAcceleration)) { // Set up the new piezo stage acceleration
		this->lastErrorMsg = "The setting of new piezo stage acceleration fails.";
		return false;
	}

	return true;
}

bool Piezo_Controller::setDeceleration(const double dec)
{
	double newDeceleration;
	if(!(dec > 0.0)) return false;
	if(dec < maxDec()) 
		newDeceleration = dec/this->micro;
	else
		newDeceleration = maxDec()/this->micro;

	if(!PI_DEC(this->USBID, &this->szAxis[0], &newDeceleration)) { // Set up the new piezo stage deceleration
		this->lastErrorMsg = "The setting of new piezo stage deceleration fails.";
		return false;
	}

	return true;
}

void Piezo_Controller::runMovements()
{
	for(int i=0; i<getMovementsSize(); i++)	{
		double from = (*this->movements[i]).from;
		double to = (*this->movements[i]).to;

		printf(" Inside of runMovement: %d %f %f %d %d %d \n", i, from, to, _isnan(from), _isnan(to), (*this->movements[i]).trigger);
		std::cout<<"%%%%%%%%  1 : "<<gTimer.read()<<std::endl;

		if(_isnan(from) && _isnan(to)) {
			if(!Triggering(i)) {
				printf("ERROR: The pure triggering() fails at movement %d \n", i);
				return;
			}
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
		if (boost::this_thread::interruption_requested()) break; // Check whether the interruption is requested
	}

	boost::this_thread::interruption_point(); // Abort the current movement if requested
}

bool Piezo_Controller::prepare(const int i)
{
	if(i == 0) { // For both types of scanning
		std::cout<<"%%%%%%%%  2 : "<<gTimer.read()<<std::endl;

		double from = (*this->movements[i]).from;
		double to = (*this->movements[i]).to;
		double duration = (*this->movements[i]).duration;
		int trigger = (*this->movements[i]).trigger;

		double Velocity = abs(to - from) / (duration / this->micro / this->micro); // Unit micrometre / second			
		this->magicAcc = this->ParaAccr; // Acceleration rate during A->B
		double Acceleration = this->magicAcc; // Unit micrometre / second^2
		double Deceleration = Acceleration;

		double Length = this->ParaLnth * Velocity * Velocity / Acceleration; // Extra travel length for acceleration
		double actFrom, actTo; // The actual from & to of each movement
		if(from <= to) {		
			actFrom = from - Length;
			actTo = to + Length;
		}
		else if(from > to) {
			actFrom = from + Length;
			actTo = to - Length;
		}
		this->oneActMovement.actFrom = actFrom;
		this->oneActMovement.actTo = actTo;

		if(!this->getScanType()) { // Execute only for the uni-directional scanning
			if(!setVelocity(Velocity)) return false;		
			if(!setAcceleration(Acceleration)) return false;
			if(!setDeceleration(Deceleration)) return false;
		}

		std::cout<<"%%%%%%%%  3 : "<<gTimer.read()<<std::endl;
	}
	else if(i == 1) { // For the uni-directional scanning
		double Velocity = this->maxVel();
		this->magicAcc = this->maxAcc(); // Acceleration rate during B->A
		double Acceleration = this->magicAcc; // Unit micrometre / second^2
		double Deceleration = Acceleration;

		if(!setVelocity(Velocity)) return false;
		if(!setAcceleration(Acceleration)) return false;
		if(!setDeceleration(Deceleration)) return false;
	}

	return true;
}

bool Piezo_Controller::run(const int i)
{
	std::cout<<"%%%%%%%%  4 : "<<gTimer.read()<<std::endl;

	double actTo;
	if(i == 0)
		actTo = this->oneActMovement.actTo;
	else if(i == 1)
		actTo = this->oneActMovement.actFrom;

	if(Qmoving(actTo)) { // Move to "actTo" and retun immediately
		std::cout<<"%%%%%%%%  5 : "<<gTimer.read()<<std::endl;
		return true;
	}
	else {
		printf("ERROR: The moving to %f fails. \n", actTo);
		return false;	
	}
}

bool Piezo_Controller::wait(const int i)
{	
	std::cout<<"%%%%%%%%  6 : "<<gTimer.read()<<std::endl;

	double from = (*this->movements[i]).from;
	double to = (*this->movements[i]).to;
	int trigger = (*this->movements[i]).trigger;

	BOOL MovingStatus = true;
	double CurrentPos;
	int trigStatus = 0; // 0 for not trigged yet, 1 for already trigged
	while(MovingStatus) {
		if(!PI_IsMoving(this->USBID, &this->szAxis[0], &MovingStatus)) return false;

		if(trigStatus == 0) {
			if(PI_qPOS(this->USBID, &this->szAxis[0], &CurrentPos)) { // Inquire the current piezo stage position
				CurrentPos = CurrentPos * this->micro;
				if( (CurrentPos < to && CurrentPos > from) || (CurrentPos > to && CurrentPos < from) ) {
					std::cout<<"%%%%%%%%  b point : "<<gTimer.read()<<std::endl;

					if((trigger == 1) || (trigger == 0))
						if(!setTrigger(trigger)) return false;				
					trigStatus = 1;
				}
			}
		}
		if(boost::this_thread::interruption_requested()) {
			if(!haltAxisMotion()) {
				printf("ERROR: The axis is STILL moving. \n");
				return false;
			}
			break;
		}
	}
	// boost::this_thread::interruption_point(); // abort the current waiting process if requested

	/*
	if(!MovingStatus && i == 0) {
		// load the recorded position data and save into memory only when i == 0
		int recordTableId[] = {1};
		int numRecordTable = 1;
		int startIdx = 1;
		int numDatas = 1024;
		double recordData[1024];
		if (!PI_qDRR_SYNC(this->USBID, 1, startIdx, numDatas, recordData)) {
			printf("ERROR: The read of recorded position data fails. \n");
			return false;
		}
		// save the recorded position data
		recordPositionVector.push_back(new RecordedPositions(recordData));
	}
	*/

	std::cout<<"%%%%%%%%  7 : "<<gTimer.read()<<std::endl;
	
	return true;
}

bool Piezo_Controller::Qmoving(const double to)
{
	if(to >= minPos() && to <= maxPos()) {
		double NewPos = to/this->micro;
		if(PI_MOV(this->USBID, &this->szAxis[0], &NewPos)) { // Moving the piezo stage to the new position
			return true;
		}
		else {
			printf("ERROR: The moving to the new position %f fails. \n", NewPos);
			return false;
		}
	}
	else return false;
}

bool Piezo_Controller::moving(const double to)
{
	if(to >= minPos() && to <= maxPos()) {
		double NewPos = to/this->micro;
		if(!PI_MOV(this->USBID, &this->szAxis[0], &NewPos)) { // Moving the piezo stage to the new position
			printf("ERROR: The moving to the new position %f fails \n", NewPos);
			return false;
		}

		BOOL MovingStatus = true;
		double CurrentPos;
		while(MovingStatus) {
			if(!PI_IsMoving(this->USBID, &this->szAxis[0], &MovingStatus)) return false;
		}

		return true;
	}
	else {
		printf(" The destination is out of bound \n");
		return false;
	}
}

bool Piezo_Controller::Triggering(const int i)
{
	std::cout<<"%%%%%%%%  8 : "<<gTimer.read()<<std::endl;
	// Triggering
	int trigger = (*this->movements[i]).trigger;
	if((trigger == 0) || (trigger == 1)) {
		if(!setTrigger(trigger)) {
			this->lastErrorMsg="The triggering failed.";
			return false;
		}
	}

	// Wait for the duration of time
	double duration = (*this->movements[i]).duration;
	if(duration > 0.0) {
		clock_t endwait = clock () + long(duration / this->micro / this->micro * double(CLOCKS_PER_SEC));
		while(clock() < endwait) {}
	}

	std::cout<<"%%%%%%%%  9 : "<<gTimer.read()<<std::endl;
	return true;
}

bool Piezo_Controller::setTrigger(const int trigger)
{
	if(trigger == 0 || trigger == 1) { // Trigger the #7 output TTL output pin
		const long OutputChannel[1] = {3};
		BOOL ChannelVol[1] = {(trigger != 0)};
		const int ArraySize = 1;
		if(!PI_DIO(this->USBID, OutputChannel, ChannelVol, ArraySize)) {
			this->lastErrorMsg="Set trigger fails.";
			return false;
		}
		else {
			return true;
		}
	}
	else {
		printf("\n The trigger input is WRONG. Only 0 & 1 are allowed. \n");
		return false;
	}
}

void Piezo_Controller::setScanType(const bool b) {
	this->isBiDirectionalImaging = b;
}

bool Piezo_Controller::getScanType() {
	return this->isBiDirectionalImaging;
}

void Piezo_Controller::setPCount() {
	this->pCount = 1;
}

