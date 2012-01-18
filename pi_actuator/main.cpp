
#include "Actuator_Controller.hpp"

int main()
{
	Actuator_Controller *test = new Actuator_Controller;

	test->setDim(0);
	test->moveTo(40.0E3); // Move to 40.00mm on the X axis
	test->moveTo(20.0E3);
	double pos;
	test->curPos(&pos);
	printf("current pos: %f \n", pos);

	test->setDim(1);
	test->moveTo(30.0E3); // Move to 30.00mm on the Y axis
	test->moveTo(10.0E3);
	test->curPos(&pos);
	printf("current pos: %f \n", pos);

	test->clearCmd();
	// Variables:     from, to, duration, trigger
	test->addMovement(20.0E3, 30.0E3, 5.0E6, 1); // Add new movements, from 20mm to 30mm using 5 second, 1 is dummy trigger input
	test->addMovement(35.0E3, 25.0E3, 4.0E6, 1);
	test->addMovement(20.0E3, 30.0E3, 5.0E6, 0);

	
	for (int i = 0; i < 3; ++i) {
		printf("Inside of test %d \n", i);
		if (!test->prepareCmd()) {
			printf("Error in prepareCmd() \n");
		} else {
			printf("prepareCmd() \n");
		}

		if (!test->runCmd()) {
			printf("Error in runCmd() \n");
		} else {
			printf("runCmd() \n");
		}

		if (i == 1) {
			if (!test->abortCmd()) { // Demo how to use abortCmd()
				printf("Error in abortCmd() \n");
			} else {
				printf("Success in abordCmd() \n");
			}
		}

		if (!test->waitCmd()) {
			printf("Error in waitCmd() \n");
		} else  {
			printf("Success in waitCmd() \n");
		}
	}

	test->~Actuator_Controller();

	return true;
}