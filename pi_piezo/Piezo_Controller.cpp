#include "Piezo_Controller.hpp"

Piezo_Controller::Piezo_Controller() : lowPosLimit(500.0), upPosLimit(18500.0), maxVelocity(1500.0),maxAcceleration(1000.0), maxDeceleration(1000.0), micro(1000.0)
{
	//
	// Inquire and set up the connection to the E-861 controller USB port
	//

	const int USBInfoSize = 100;
	char *USBInfo = (char *) malloc(sizeof(char)*USBInfoSize);
	const char *USBFilter = "E-861";
	int NumOfUSB = PI_EnumerateUSB(USBInfo, USBInfoSize, USBFilter); // Inquire all the available "E-861" controller USB ports
	if (NumOfUSB != 1) // In our case, only one "E-861" USB port exists in the system
	{
		printf(" The inquire of the available E-861 USB port fails \n");
		exit (EXIT_FAILURE);
	}
	else
	{
		printf(" The number of available USB Port is %d \n", NumOfUSB);
		printf(" The USB Port Information is %s \n", USBInfo);
	}
		
	this->USBID = PI_ConnectUSB(USBInfo); // Connect to this "E-861" USB port
	if (this->USBID != 0)
	{
		printf(" The connection to USB Port fails \n"); // deletable
		exit (EXIT_FAILURE);
	}
	else
	{
		// deletable
		printf(" The connected USB Port ID is %d \n", this->USBID);
	}

	if (!PI_IsConnected(this->USBID)) // Double check that the USB port is connected
	{
		// deletable
		printf(" The USB Port %d is not connected \n", this->USBID);
		exit (EXIT_FAILURE);
	}


	//
	// Inquire the stage axis information and set the SVO mode ON
	//

	this->szAxis[0] = 'x'; // Initialize the axis information
	this->szAxis[1] = 'x';
	this->szAxis[2] = 'x';
	this->szAxis[3] = 'x';	
	
	if (!PI_qSAI(this->USBID,this->szAxis,4)) // Inquire the stage axis information
	{
		// deletable
		printf(" The inquire of stage axis information fails \n");
		exit (EXIT_FAILURE);
	}
	else
	{
		// deletable
		printf(" The available stage axis information is %s \n", this->szAxis);
	}
	
	this->szAxis[1] = '\0'; // Only use the first axis as only 1 axis exists in the piezo stage
	BOOL bFlag = true;
	if (!PI_SVO(this->USBID, & this->szAxis[0], &bFlag)) // Set the SVO mode ON
	{
		// deletable
		printf(" The SVO mode can not be set ON \n");
		exit (EXIT_FAILURE);
	}
	else
	{
		// deletable
		printf(" The SVO mode is ON (Closed-Loop) \n");
	}
	
	if (!PI_qSVO(this->USBID, & this->szAxis[0], &bFlag)) // Inquire the SVO mode of the axis
	{
		// deletable
		printf(" The inquire of the SVO mode fails \n");
		exit (EXIT_FAILURE);
	}
	else
	{
		if (!bFlag)
		{
			// deletable
			printf(" The SVO mode can not be set ON \n");
			exit (EXIT_FAILURE);
		}
	}


	//
	// Inquire whether the piezo stage has a reference sensor and set the Reference mode ON
	//

	BOOL refSwitch;
	if (PI_qTRS(this->USBID, & this->szAxis[0], &refSwitch)) // Inquire whether the axis has reference sensor
	{
		if(!refSwitch)
		{
			// deletable
			printf(" The piezo stage has no reference switch \n");
			exit (EXIT_FAILURE);
		}
	}
	else
	{
		// deletable
		printf(" The piezo stage reference switch information can not be inquired \n");
		exit (EXIT_FAILURE);
	}
	

	if (!PI_FRF(this->USBID, & this->szAxis[0])) // Reference the first axis of the piezo stage
	{
		// deletable
		printf(" The piezo stage axis can not be referenced \n");
		exit (EXIT_FAILURE);
	}
	else
	{
		// deletable
		printf(" The piezo stage is referencing... \n");
	}

	BOOL ControllerReady = false;
	while (!ControllerReady)
	{
		PI_IsControllerReady(this->USBID, &ControllerReady); // Check whether the reference is done
	}

	if (ControllerReady)
	{
		// deletable
		printf(" The controller referencing is done \n");
	}

	BOOL frfResult;
	if (PI_qFRF(this->USBID, & this->szAxis[0], &frfResult)) // Inquire whether the axis has already been referenced
	{
		// deletable
		printf(" The piezo stage axis %c is referenced? %d \n", this->szAxis[0], frfResult); // deletable
		if (!frfResult)
		{
			// deletable
			printf(" the piezo stage can not be referenced \n");
			exit (EXIT_FAILURE);
		}
	}
	else
	{
		// deletable
		printf(" The inquire of Piezo stage reference information fails \n");
		exit (EXIT_FAILURE);
	}
		
	//
	// Set up the default piezo stage velocity and acceleration
	//

	const double Velocity = 500.0; // unit micrometre
	setVelocity(Velocity); // Set up the new piezo stage velocity
	const double Acceleration = 1000.0; // unit micrometre
	setAcceleration(Acceleration); // Set up the new piezo stage acceleration
	const double Deceleration = 1000.0; // unit micrometre
	setDeceleration(Deceleration); // Set up the new piezo stage acceleration

	//
	// Set all the output ports to output +0.0V TTL signal 
	//

	const long OutputChannel[1] = {0};
	const BOOL ChannelLow[1] = {0};
	const int ArraySize = 1;
	if (!PI_DIO(this->USBID, OutputChannel, ChannelLow, ArraySize)) // Set +0.0V TTL to all the Ouput ports
	{
		// deletalbe
		printf(" The setting of +0.0V ouput trigger to all output ports fails \n");
		exit (EXIT_FAILURE);
	}
	
}
Piezo_Controller::~Piezo_Controller() 
{
	if (!checkControllerReady()) exit(EXIT_FAILURE);
	if (!haltAxisMotion()) exit(EXIT_FAILURE);
		
	PI_CloseConnection(this->USBID); // Close the connection to "E-861" USB port
	if (PI_IsConnected(this->USBID)) // Check whether the USB port is disconnected
	{
		// deletable
		printf(" The USB Port %d is still connected \n", this->USBID);
		exit(EXIT_FAILURE);
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
	if (!checkControllerReady()) return false;
	if (!haltAxisMotion()) return false;
	
	const double Velocity = 500.0; // Unit micrometre
	setVelocity(Velocity);
	const double Acceleration = 1000.0; // Unit micrometre
	setAcceleration(Acceleration);
	const double Deceleration = 1000.0; // Unit micrometre
	setDeceleration(Deceleration);
	
	if(!moving(to)) return false; // Move and check destination
	return true;
}
    
bool Piezo_Controller::prepareCmd()
{
	if(prepare(0))
	{
		return true;
	}
	else return false;
}
bool Piezo_Controller::runCmd()
{
//	if(!runMovements()) return false;
	this->workerThread = boost::thread(&Piezo_Controller::runMovements, this); // create & initialize the workThread
	printf(" The workerThread ID is %d \n", this->workerThread.get_id());
	return true;
}
bool Piezo_Controller::waitCmd()
{
	this->workerThread.join();	// the main thread joins the worker thread and sleeps
	return true;
}
bool Piezo_Controller::abortCmd()
{
	this->workerThread.interrupt(); // send request to interrupt the workerThread
	this->workerThread.join(); // the main thread joins the workerThread and waits it to terminate

	if(!checkControllerReady()) return false; 
	if(!haltAxisMotion()) 
	{
		return false;
	}
	else return true;
}

int Piezo_Controller::getMovementsSize()
{
	return this->movements.size();
}
bool Piezo_Controller::checkControllerReady()
{
	BOOL ControllerReady = false;
	while(!ControllerReady)
	{
		if(!PI_IsControllerReady(this->USBID, &ControllerReady)) return false; // Check whether the controller is ready
	}
	printf(" Controller ready \n");
	return true;
}
bool Piezo_Controller::haltAxisMotion()
{
	BOOL MovingStatus;
	if(PI_IsMoving(this->USBID, &this->szAxis[0], &MovingStatus)) // Check whether the axis is moving
	{
		if(MovingStatus)
		{
			if(!PI_HLT(this->USBID, &this->szAxis[0])) // Halt the motion of axis smoothly
			{
				return false; 
			}
			else
			{
				printf(" halt axis motion \n");
				return true;
			}
		}
		else
		{
			printf(" halt axis motion \n");
			return true;
		}
	}
	else return false;
}

bool Piezo_Controller::setVelocity(const double vel)
{
	double newVelocity;
	if(vel < 0.0) return false;
	if(vel < maxVel())
	{
		newVelocity = vel/this->micro;
	}
	else
	{
		newVelocity = maxVel()/this->micro;
	}

	if(!PI_VEL(this->USBID, & this->szAxis[0], &newVelocity)) // Set up the new piezo stage velocity
	{
		printf(" The setting of new piezo velocity fails \n");
		return false;
	}

	double curVelocity;
	if(PI_qVEL(this->USBID, & this->szAxis[0], &curVelocity)) // Inquire the current piezo stage velocity
	{
		// deletable
		printf(" The current piezo stage velocity is %f \n", curVelocity);
		if ( abs(curVelocity-newVelocity)/newVelocity > 0.01 )
		{
			printf(" The setting of the new piezo stage velocity fails \n");
			return false;
		}
		else return true;
	}
	else
	{
		printf(" The inquire of current piezo stage velocity fails \n");
		return false;
	}
}
bool Piezo_Controller::setAcceleration(const double acc)
{
	double newAcceleration;
	if(acc < 0.0) return false;
	if(acc < maxAcc())
	{
		newAcceleration = acc/this->micro;
	}
	else
	{
		newAcceleration = maxAcc()/this->micro;
	}

	if(!PI_ACC(this->USBID, & this->szAxis[0], &newAcceleration)) // Set up the new piezo stage acceleration
	{
		printf(" The setting of new piezo stage acceleration fails \n");
		return false;
	}

	double curAcceleration;
	if(PI_qACC(this->USBID, & this->szAxis[0], &curAcceleration)) // Inquire the current piezo stage acceleration
	{
		// deletable
		printf(" The current piezo stage acceleration is %f \n", curAcceleration);
		if(abs(curAcceleration-newAcceleration)/newAcceleration > 0.01)
		{
			printf(" The setting of the new piezo stage acceleration fails \n");
			return false;
		}
		else return true;
	}
	else
	{
		printf(" The inquire of current piezo stage acceleration fails \n");
		return false;
	}
}
bool Piezo_Controller::setDeceleration(const double dec)
{
	double newDeceleration;
	if(dec < 0.0) return false;
	if(dec < maxDec())
	{
		newDeceleration = dec/this->micro;
	}
	else
	{
		newDeceleration = maxDec()/this->micro;
	}

	if(!PI_DEC(this->USBID, & this->szAxis[0], &newDeceleration)) // Set up the new piezo stage deceleration
	{
		printf(" The setting of new piezo stage deceleration fails \n");
		return false;
	}

	double curDeceleration;
	if(PI_qDEC(this->USBID, & this->szAxis[0], &curDeceleration)) // Inquire the current piezo stage deceleration
	{
		// deletable
		printf(" The current piezo stage deceleration is %f \n", curDeceleration);
		if(abs(curDeceleration-newDeceleration)/newDeceleration > 0.01)
		{
			printf(" The setting of the new piezo stage deceleration fails \n");
			return false;
		}
		else return true;
	}
	else
	{
		printf(" The inquire of current piezo stage deceleration fails \n");
		return false;
	}
}

void Piezo_Controller::runMovements()
{
	for(int i=0; i<getMovementsSize(); i++)
	{
		double from = (*this->movements[i]).from;
		double to = (*this->movements[i]).to;

		printf(" Inside of runMovement: %d %f %f %d %d \n", i, from, to, _isnan(from), _isnan(to));
		if( _isnan(from) || _isnan(to) )
		{
			if(!Triggering(i)) exit(EXIT_FAILURE);
		}
		else
		{
			if(!prepare(i)) exit(EXIT_FAILURE);
			if(!run(i)) exit(EXIT_FAILURE);
			if(!wait(i)) exit(EXIT_FAILURE);
		}
		boost::this_thread::interruption_point(); // abort the current movement if requested
	}
}

bool Piezo_Controller::prepare(const int i)
{
	double from = (*this->movements[i]).from;
	double to = (*this->movements[i]).to;
	double duration = (*this->movements[i]).duration;
	int trigger = (*this->movements[i]).trigger;
	
	double Velocity = (abs(to - from) / duration) * this->micro; // unit micrometre / second
	double Acceleration = maxAcc(); // unit micrometre / second^2
	double Deceleration = maxDec();
	
	double Length = 2.0 * Velocity * Velocity / Acceleration; // Extra travel length for acceleration
	double actFrom, actTo; // The actual from & to of each movement
	if(from <= to)
	{		
		actFrom = from - Length;
		actTo = to + Length;
	}
	else if(from > to)
	{
		actFrom = from + Length;
		actTo = to - Length;
	}
	oneActMovement.actFrom = actFrom;
	oneActMovement.actTo = actTo;

	printf(" Movement: %d %f %f %f %f \n", i, from, to, actFrom, actTo);

	if(!moveTo(actFrom)) return false; // Move to "actFrom"
	
	if(!setVelocity(Velocity)) return false;
	if(!setAcceleration(Acceleration)) return false;
	if(!setDeceleration(Deceleration)) return false;
	return true;
}
bool Piezo_Controller::run(const int i)
{
	double actTo = oneActMovement.actTo;
	if(Qmoving(actTo))  // Move to "actTo"
	{
		printf(" The piezo is moving to %f \n", actTo);
		return true;
	}
	else
	{
		printf(" The moving to %f fails \n", actTo);
		return false;	
	}
}
bool Piezo_Controller::wait(const int i)
{	
	double from = (*this->movements[i]).from;
	double to = (*this->movements[i]).to;
	int trigger = (*this->movements[i]).trigger;
		
	BOOL MovingStatus = true;
	double CurrentPos;
	int volStatus = 0; // 0 for LOW, 1 for HIGH
	while(MovingStatus)
	{
		if(!PI_IsMoving(this->USBID, &this->szAxis[0], &MovingStatus)) return false;
		
		if(PI_qPOS(this->USBID, &this->szAxis[0], &CurrentPos)) // Inquire the current piezo stage position
		{
			CurrentPos = CurrentPos * this->micro;
			if( (CurrentPos < to && CurrentPos > from) || (CurrentPos > to && CurrentPos < from) )
			{
				if(trigger == 1)
				{
					if(volStatus < 1) // trigger to HIGH if within the range
					{
						if(!setTrigger(1)) return false;
						volStatus = 1;
					}
				}

				if(trigger == 0)
				{
					if(volStatus > 0) // trigger to HIGH if within the range
					{
						if(!setTrigger(0)) return false;
						volStatus = 0;
					}
				}				
			}
			else
			{
				if(volStatus > 0) // trigger to LOW if outside of the range
				{
					if(!setTrigger(0)) return false;
					volStatus = 0;
				}
			}
		}
		else return false;
		boost::this_thread::interruption_point(); // abort the current waiting process if requested
	}

	if(!MovingStatus)
	{
		printf(" The moving to the new position %f is done \n", CurrentPos);
	}
	return true;
}

bool Piezo_Controller::Qmoving(const double to)
{
	if(to >= minPos() && to <= maxPos())
	{
		double NewPos = to/this->micro;
		if(PI_MOV(this->USBID, &this->szAxis[0], &NewPos)) // Moving the piezo stage to the new position
		{
			printf(" The axis is moving to the new position %f ... \n", NewPos);
			return true;
		}
		else
		{
			printf(" The moving to the new position %f fails \n", NewPos);
			return false;
		}
	}
	else return false;
}
bool Piezo_Controller::moving(const double to)
{
	if(to >= minPos() && to <= maxPos())
	{
		double NewPos = to/this->micro;
		if(PI_MOV(this->USBID, &this->szAxis[0], &NewPos)) // Moving the piezo stage to the new position
		{
			printf(" The axis is moving to the new position %f ... \n", NewPos);
		}
		else
		{
			printf(" The moving to the new position %f fails \n", NewPos);
			return false;
		}

		BOOL MovingStatus = true;
		double CurrentPos;
		while(MovingStatus)
		{
			if(!PI_IsMoving(this->USBID, &this->szAxis[0], &MovingStatus)) return false;
		}
		if(!MovingStatus)
		{
			printf(" The moving to the new position %f is done \n", NewPos);
		}	
		if(PI_qPOS(this->USBID, &this->szAxis[0], &CurrentPos)) // Inquire the current piezo stage position
		{
			printf(" The current piezo stage position is %f \n", CurrentPos);
			if(abs(CurrentPos-NewPos)/NewPos > 0.01) return false;
			return true;
		}
		else 
		{
			printf(" The inquire of the current position fails \n");
			return false;
		}
	}
	else
	{
		printf(" The destination is out of bound \n");
		return false;
	}
}
bool Piezo_Controller::Triggering(const int i)
{
	double duration = (*this->movements[i]).duration;
	int trigger = (*this->movements[i]).trigger;
	printf(" Inside of Triggering: %d %f %d \n", i, duration, trigger);
	if(trigger == 1)
	{
		if(!setTrigger(1)) return false;
	}
	else if(trigger == 0)
	{
		if(!setTrigger(0)) return false;
	}
	else
	{
		printf(" The trigger input is invalid \n");
		return false;
	}

	clock_t endwait = clock() + long(duration * double(CLOCKS_PER_SEC) / 1000.0);
	while (clock() < endwait)
	{
		boost::this_thread::interruption_point(); // abort the current triggering process if requested
	}
	if(!setTrigger(0)) return false;
	return true;
}
bool Piezo_Controller::setTrigger(const int trigger)
{
	if(trigger == 0 || trigger == 1) // Trigger the TTL output signal to #7 output pin
	{
//		if(!checkControllerReady()) return false;
		const long OutputChannel[1] = {3};
		BOOL ChannelVol[1];
		if(trigger == 0) 
		{
			ChannelVol[0] = false;
		}
		else if(trigger == 1)
		{
			ChannelVol[0] = true;
		}
		const int ArraySize = 1;
		if (!PI_DIO(this->USBID, OutputChannel, ChannelVol, ArraySize)) return false;
		printf(" the trigger output is %d \n", ChannelVol[0]);
	}
	return true;
}
