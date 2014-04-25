/*
 * Ultrasound_interface.h
 *
 *  Created on: Nov 18, 2013
 *      Author: OliverChen
 */

#ifndef ULTRASOUND_INTERFACE_H_
#define ULTRASOUND_INTERFACE_H_

#include "io.h"

//write_operation
#define trigger_sensor(base_addr)	 IOWR(base_addr+(0<<2), 0, 1) //TODO
#define trigger_clear(base_addr)	 IOWR(base_addr+(0<<2), 0, 0) //TODO
//read operation
#define get_sensor_control(base_addr)	 IORD(base_addr+(0<<2), 0)
#define get_sensor_data(base_addr)	 IORD(base_addr+(1<<2), 0)
#define get_sensor_state(base_addr)	 IORD(base_addr+(2<<2), 0)

/*
This function is used to measure the distance between the Ultrasound device and the front obstacle. But if the distance is over 3 meter, it will return -1.
 */

float measuredis(const unsigned int base_address)
	{
	 unsigned int state;
	 unsigned int distance0;
	 float distance;
	 state=get_sensor_state(base_address);
		while(state==1)
		{
			state=get_sensor_state(base_address);
		}
		trigger_clear(base_address);
		trigger_sensor(base_address);
		trigger_clear(base_address);
		state=get_sensor_state(base_address);
		while(state!=2)
		{
				state=get_sensor_state(base_address);
		}
		distance0=get_sensor_data(base_address);
		if(distance0==-1)
			distance = -1;
		else
			distance=distance0*0.0000034;
		return distance;
	}

#endif /* ULTRASOUND_INTERFACE_H_ */
