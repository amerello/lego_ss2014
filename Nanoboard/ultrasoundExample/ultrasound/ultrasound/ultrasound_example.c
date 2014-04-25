/*
This is an example program to show how the ultrasound device can be used to measure the device.
 */


#include <stdio.h>
#include "system.h"
#include "Ultrasound_interface.h"

int main()
{
 unsigned int i;
 float distance;
 while(1)
 {
	 for (i=1;i<=5000000;i++); //delay some time
	 distance = measuredis(ULTRASOUND_INTERFACE0_BASE);
	 printf("Distance=%f\n",distance);
 }
  return 0;
}
