/*
 * Ultrasound_interface.h
 *
 *  Created on: Nov 18, 2013
 *      Author: OliverChen
 */

#ifndef ULTRASOUND_INTERFACE_H_
#define ULTRASOUND_INTERFACE_H_

//write_operation
#define trigger_sensor(base_addr)	 IOWR(base_addr+(0<<2), 0, 1) //TODO


//read operation
#define get_sensor_control(base_addr)	 IORD(base_addr+(0<<2), 0)
#define get_sensor_data(base_addr)	 IORD(base_addr+(1<<2), 0)
#define get_sensor_state(base_addr)	 IORD(base_addr+(2<<2), 0)



#endif /* ULTRASOUND_INTERFACE_H_ */
