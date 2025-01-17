/* Copyright (c) 2019 Perlatecnica no-profit organization
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "../../../roobokart/modes/mode_cross/CrossMode.h"

// Constructor
CrossMode::CrossMode(Serial* ser,Devices* devices,int yourmode, Planning *planning)
{
	mymode = yourmode;
	currDevices = devices;
	currPlanning = planning;
	dirPID = new PID(100, -100, 40, 10, 5);
}


int CrossMode::runMode(void)
{
#ifdef DEBUG_CROSS_MODE
	printf("Cross Mode: It works!\r\n");
#endif
	int speed = currPlanning->getSpeed() ;
	double dt = 0;
	double spd = currDevices->getSPDirection();
	double delSpeed;
	currentmode = mymode;
	nextmode = currPlanning->SetCurrentMode(currentmode);
	currDevices->tof->display(currentmode);

	//currDevices->mems->calibrateLSM6DSL(50); 20190104 Non credo serva ricalibrare
	pidtimer.start();

	double det = 0;

	// It stores the current yaw as setpoint
	//currDevices->mems->compute();
	//setPointYaw = currDevices->mems->attitude.yaw;
	setPointYaw = currPlanning->getSetPointYaw();

	totalYaw = 0;
	crosstimer.start();

	while(currentmode == mymode){

		rfrontIR = currDevices->rfrontIR->read();
		lfrontIR = currDevices->lfrontIR->read();
		cfrontIR = currDevices->cfrontIR->read();

		currentDirection = currPlanning->GetDirection();
		currentDirection = DIR_RIGHT;// TEMP DEBUG
		float currentdirection = lfrontIR - rfrontIR;


		switch(currentDirection){

		case DIR_FRONT:
			if(crosstimer.read() < 2){
				// It gather the current yaw
				currDevices->mems->compute();
				currentYaw = currDevices->mems->attitude.yaw;
				// PID evaluation of direction. The yaw setpoint is followed
				det = (double)pidtimer.read();
				direction = (int8_t)(dirPID->evaluate(det,setPointYaw,currentYaw));
				pidtimer.reset();
				currDevices->currMotors.turn(-direction, 50, MOTOR_LEFT , MOTOR_RIGHT);
			}

			// The crossroad has the following layout -|
			// The white lane is on the right, the mode can immediately switch to nav mode
			if(currPlanning->getCrossCode() == 2){

					currentmode = nextmode;
					crosstimer.stop();
					crosstimer.reset();
			}

			// The crossroad has the following layout +
			if(currPlanning->getCrossCode() == 0){
					if(crosstimer.read_ms() >= 2000){
						currDevices->tof->display("ECCE"); //DEBUG
						currDevices->currMotors.turn(10, 50, MOTOR_LEFT , MOTOR_RIGHT);
						if(RIGHT_IR_WHITE){
							currentmode = nextmode;
							crosstimer.stop();
							crosstimer.reset();
						}
					}

			}
			break;

		case DIR_LEFT:
			if(crosstimer.read_ms() < 1500){
				direction = (int8_t)(dirPID->evaluate(det,spd,currentdirection)); //0.3
//				currPlanning->accelerate();
				delSpeed = direction;
				if (delSpeed<0) delSpeed *= -1;
				delSpeed = delSpeed/100*speed/1.9;
				//currDevices->currMotors.turn(direction, speed, MOTOR_LEFT , MOTOR_RIGHT);
			}
//			if(crosstimer.read_ms() > 2000 && crosstimer.read_ms() < 2500){
//				double angle = (crosstimer.read_ms() - 2000) * (3/1000);
//				dt = (double)pidtimer.read();
//				direction = (int8_t)(dirPID->evaluate(dt,1,currDevices->mems->attitude.yaw)); //0.3
//				pidtimer.reset();
//				//currDevices->currMotors.turn(direction, speed, MOTOR_LEFT , MOTOR_RIGHT);
//			}
			else if(crosstimer.read_ms() > 1500 && crosstimer.read_ms() < 2400){
//				direction = (int8_t)(dirPID->evaluate(det,spd,currentdirection)); //0.3
				//				currPlanning->accelerate();
								delSpeed = direction;
								if (delSpeed<0) delSpeed *= -1;
								delSpeed = delSpeed/100*speed/1.9;
								currDevices->currMotors.turn(-45, speed, MOTOR_LEFT , MOTOR_RIGHT);
			}
			else if(crosstimer.read_ms() > 2900){
							currentmode = nextmode;
							crosstimer.stop();
							crosstimer.reset();
						}
			break;

		case DIR_RIGHT:
			if(crosstimer.read_ms() > 100 && crosstimer.read_ms() < 300){
				currDevices->currMotors.turn(80, 80, MOTOR_LEFT , MOTOR_RIGHT);
			}

			else if(crosstimer.read_ms() > 300){
				currentmode = nextmode;
							crosstimer.stop();
							crosstimer.reset();
			}
			break;
		}
		wait_ms(100);
	}
	return currentmode;
}



