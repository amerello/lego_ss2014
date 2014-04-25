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
#define trigger_sensor(base_addr)	 IOWR(base_addr+(0<<2), 0, 1)
#define trigger_clear(base_addr)	 IOWR(base_addr+(0<<2), 0, 0)
//read operation
#define get_sensor_control(base_addr)	 IORD(base_addr+(0<<2), 0)
#define get_sensor_data(base_addr)	 IORD(base_addr+(1<<2), 0)
#define get_sensor_state(base_addr)	 IORD(base_addr+(2<<2), 0)

/*
This function is used to measure the distance between the Ultrasound device and the front obstacle.
But if the distance is over 3 meter, it will return -1.
This function is blocking.
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
		printf("state: %d\n",state);
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
		return distance/2;
	}

/* start measuring not blocking */
unsigned int US_base_address;
float* US_distance;
unsigned int US_state;

// initialize measuredis
void init_measuredis(const unsigned int base_address,float* distance){
	US_base_address=base_address;
	US_distance = distance;
	US_state = 0;
}
// nonblocking function, reads values into US_distance
void handle_measuredis(void)
{
	unsigned int state=get_sensor_state(US_base_address);

	switch(US_state){
	case 0:
		if (state != 1){
		  US_state = 1;
		  trigger_clear(US_base_address);
		  trigger_sensor(US_base_address);
		  trigger_clear(US_base_address);
		}
		break;
	case 1:
		if(state == 2){
			US_state=0;
			int distance0=get_sensor_data(US_base_address);
			if(distance0==-1)
				*US_distance = -1;
			else
				*US_distance=distance0*0.0000034/2;
		}
		break;
	}




}

#endif /* ULTRASOUND_INTERFACE_H_ */
