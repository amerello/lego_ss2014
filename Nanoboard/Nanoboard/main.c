#include <stdio.h>
#include <system.h>
#include <io.h>
#include <alt_types.h>
#include "altera_up_avalon_rs232.h"
#include "terasic_lib/adc_spi_read.h"

//#define VERBOSE

#include "Ultrasound_interface.h"
#include "Accelerometer_interface.h"
void delay (volatile unsigned int del)
{
	while (del != 0)
	{
		del --;
	}
}


#define TRUE 1
#define FALSE 0

//Motor Controlling
#define PWM_EN (0x80000000 | A_2_CHANNEL_PWM_0_BASE)
#define PWM_PERIOD PWM_EN+1
#define PWM_DUTY1 PWM_EN+2
#define PWM_DUTY2 PWM_EN+3
#define PWM_PHASE1 PWM_EN+4
#define PWM_PHASE2 PWM_EN+5
// command pwms for motor with this method.
// phase: clock cycles before pwm is set
// duty: clock cycles after pwm is set
//        _________
//       |         |
//       |         |
// ______|         |________
// <----> <------->
// phase    duty
// phase1 and duty1 set the rotation speed in left direction,
// phase2 and duty2 in right direction.
// period= fpga clock cycles used for one pwm period
// enable: 1=left, 2=right.
void motor_setting(unsigned long phase1, unsigned long duty1,
		unsigned long phase2, unsigned long duty2, unsigned long period,
		unsigned long enable) {
	volatile unsigned int * pwm_en = (volatile unsigned int *) PWM_EN;
	volatile unsigned int * pwm_period = (volatile unsigned int *) PWM_PERIOD;
	volatile unsigned int * pwm_phase1 = (volatile unsigned int *) PWM_PHASE1;
	volatile unsigned int * pwm_phase2 = (volatile unsigned int *) PWM_PHASE2;
	volatile unsigned int * pwm_duty1 = (volatile unsigned int *) PWM_DUTY1;
	volatile unsigned int * pwm_duty2 = (volatile unsigned int *) PWM_DUTY2;

	*pwm_en = enable;
	*pwm_period = period;
	*pwm_phase1 = phase1;
	*pwm_phase2 = phase2;
	*pwm_duty1 = duty1;
	*pwm_duty2 = duty2;

}

// we use a special uart core from the Altera University Program for serial communication.
// the advantage of this is that it has a 128 bit buffer and we don't have to work with interrupts.
// we can simply check if there are packets in the buffer and read until it is empty.

// read first date out of the buffer
void read_first(alt_up_rs232_dev* dev,alt_u8* buffer){
	alt_u8 p = 0;
	alt_up_rs232_read_data(dev,buffer,&p);
}
// write a string into the buffer, wait if buffer is to small
alt_u8 write_string(alt_up_rs232_dev* dev,alt_u8* string){
	int i = 0;
	alt_u8 c;
	for(i=0,c=0; string[i] != 0 && c ==0;i++){
		while(alt_up_rs232_get_available_space_in_write_FIFO(dev)==0);
		c=alt_up_rs232_write_data(dev,string[i]);
	}
	return c;
}
// data, the board gets from main processor
typedef union indata{
	struct {
	alt_u8  led;
	alt_u8  enConnBrokenCtr;
	alt_u8  setEnMotorController;
	alt_u8  voidData;
	int     W;
	}values;
	alt_u8 serial[8];
} indata;
// data, the board sends out to the main processor
typedef union outdata{
	struct {
		alt_u8 connBroken;
		alt_u8 enMotorController;
		unsigned short  ubat;
		struct acc Acc;
		uint  duty;
		uint  network_timeout;
		int X;  //Value to be controlled
		int W;  //Output of the Controller
		int P;  //Proportional part of the Controller
		int I;  //Integral part of the Controller
		int D;  //Differential part of the Controller
		int FF; // Feed forward part
		float distance; // ultrasound distance
	}values;
	alt_u8 serial[48];
}outdata;

int main()
{

	//init Motor
	 // init Encoder and direction sensor
	volatile unsigned int * pEnc = (volatile unsigned int *) (0x80000000
			| MOTOR_ENCODER_0_BASE);
	volatile unsigned int * pDir = (volatile unsigned int *) (0x80000000
			| MOTOR_ENCODER_0_BASE + 4);
	volatile unsigned int direction =0;
     // init pwm parameters
	unsigned long phase1 = 0x000, phase2 = 0x000,
			period = 100000, enable = 0x2;


	int i = 0, j= 0;
	alt_u8 c = 0;
	indata indat;
	outdata outdat;
	alt_u8 LED = 0;
	alt_u8 versionName[] = "Nano Board Version 1.0\n";

	//PID Parameters
	const int I_Parameter =50;
	const int P_Parameter =30;
	const int FF_Parameter =0;
	const int D_Parameter = 1;

    outdat.values.ubat = 0;
	outdat.values.I = 0;
	outdat.values.W = 0;
	int prevW = 0;
	outdat.values.P  = 0;
	outdat.values.D  = 0;
	outdat.values.FF = 0;
	outdat.values.X  = 0;
	int prevX=0;
	outdat.values.enMotorController=FALSE;
	outdat.values.distance=-1;

	// Counters
	unsigned int connection_broken_counter 	= 0;
	unsigned int uart_timeout 				= 0;
	unsigned int display_Counter			= 1;
	unsigned int dist_Counter				= 2;

	// bools
	alt_u8 adcDone = TRUE;
	alt_u8 connBroken = TRUE;
	alt_u8 setEnMotorwasoff = FALSE;

	// init sensors
	init_measuredis(ULTRASOUND_INTERFACE_0_BASE,&(outdat.values.distance));
	init_accel(&(outdat.values.Acc));

	// init serial
	alt_up_rs232_dev *dev = alt_up_rs232_open_dev("/dev/uart_0");

	if(dev == 0)
		goto error;

    // switch on all leds
	LED = 255;
	IOWR(LED_BASE,0,LED);

	while(1)
	{

		display_Counter = (display_Counter +1 ) % 100;
        dist_Counter = (dist_Counter+1)%10;
		// check length of incoming message
		i = alt_up_rs232_get_used_space_in_read_FIFO(dev);

		//INIT COMMUNICATION
		if(connBroken){
			outdat.values.W=0;
			indat.values.setEnMotorController=FALSE;

			// is there one char in the buffer?
			if(i >= 1){
				read_first(dev,&c);
				// after connection was broken first character is 'I'
				if(c == 'I'){
					connBroken = FALSE;
#ifdef VERBOSE
					printf("Received Init character\n");
#endif
					// send init string
					write_string(dev,versionName);

					uart_timeout = 0;
				}
			}
			// blink the Leds on no connection
			if(display_Counter==0){
				LED = ~LED;
			    IOWR(LED_BASE,0,LED);
			}

		}
		else // connected to main processor
		{
			// count rounds since last received packet
			uart_timeout++;
			outdat.values.network_timeout = uart_timeout;

			if(uart_timeout >=50)
			{
				connBroken = TRUE;
#ifdef VERBOSE
				printf("Uart Connection Broken!!!\n");
#endif
				uart_timeout = 0;
			}
            // is the packet in the buffer a whole indat packet?
			if( i >= sizeof(indat)+1)
			{
				// RECEIVE

				// drop all data until start bit received
				read_first(dev,&c);
				if(c == 'S')
				{
					uart_timeout = 0;
					// receive indata
					for(j=0;j<sizeof(indat);j++){
						read_first(dev,indat.serial+j);
					}

					// Forward W from indat to outdat
					outdat.values.W = indat.values.W;
					// Set LEDs as received

					IOWR(LED_BASE,0,indat.values.led);

					// SEND
					// init Character for response is 'R'
					alt_up_rs232_write_data(dev,'R');
					//Send outdat packet
					for(j=0;j<sizeof(outdat);j++){
						alt_up_rs232_write_data(dev,outdat.serial[j]);
					}
					// clear acc sums
					for(j=0;j<3; j++)
					  outdat.values.Acc.sum[j] = 0;
				}
			}

		}
		// Display Values
		if (display_Counter == 0) {
		  outdat.values.ubat = ADC_Read(0)*1250/2630;
#ifdef VERBOSE
		  if(!outdat.values.enMotorController){

			printf("Motor Controller not enabled");
			printf(" conn broken: %s", outdat.values.connBroken?"yes":"no");
			printf(" acc: X=%dmg Y=%dmg Z=%dmg uBat:%d accSumX=%d, dist:%f\n",outdat.values.Acc.acc[0],outdat.values.Acc.acc[1],outdat.values.Acc.acc[2],outdat.values.ubat,outdat.values.Acc.sum[0],outdat.values.distance);
		  }
		  else {
			printf("X = %4d  \t", outdat.values.X);
			printf("Direction = %u\t", direction);
			printf("I: %6d \t D: %5d \t P: %6i FF: %5d \t",outdat.values.I, outdat.values.D, outdat.values.P,outdat.values.FF);
			printf("Duty = %5u   W = %d Ubat: %d ", outdat.values.duty, outdat.values.W, outdat.values.ubat);
			printf("Distance=%f  Con_ctr: %u en: %c\n",outdat.values.distance,connection_broken_counter,outdat.values.connBroken?'y':'n');
		  }
#endif

		}


		if(outdat.values.enMotorController)
		{
			int x_temp;

			// read Encoder
			*pEnc = 1;
			delay(5000);
			prevX = outdat.values.X;
			x_temp = *pEnc * 2; // * 2 for similar size of W and X
			//U[X]=rotations per 100us
			direction = *pDir;
			// add direction
			if (direction == 2)
				x_temp *= -1;
			outdat.values.X = x_temp;

			int diff = outdat.values.W -outdat.values.X;
			// P Controller
			outdat.values.P = (diff) * P_Parameter;

			// I Controller
			outdat.values.I += (diff)*I_Parameter;

			// proportional to Command Variable (Feed Forward)
			outdat.values.FF = outdat.values.W * FF_Parameter;

			// D Controller
			outdat.values.D = (diff - (prevW - prevX)) * D_Parameter;
			prevW = outdat.values.W;
            // Sum of all Controllers
			int temp_duty= outdat.values.P + outdat.values.I
					 + outdat.values.D + outdat.values.FF;
			// get direction
			if(temp_duty < 0)
				enable = 2;
			else
				enable = 1;
			temp_duty = abs(temp_duty);


			if (temp_duty <= 10000)
				temp_duty = 0;
			if (temp_duty >= 99000)
				temp_duty = 99000;
			// 100 times maximal duty and no rotation => connection broken!!
			if(indat.values.enConnBrokenCtr)
			{
				if(temp_duty >= 99000 && outdat.values.X == 0)
					connection_broken_counter++;
				else
					connection_broken_counter =0;

				if(connection_broken_counter > 100)
				{
					outdat.values.connBroken = TRUE;
					outdat.values.enMotorController = FALSE;
					connection_broken_counter = 0;
				}
			}
			outdat.values.duty = temp_duty;

			motor_setting(phase1, outdat.values.duty, phase2, outdat.values.duty, period, enable);
		}
		else
		{
			// setEnMotorController has to be reseted and then set again:
			if(!indat.values.setEnMotorController)
				setEnMotorwasoff = TRUE;
			motor_setting(phase1,0,phase2,0,period,enable);
			outdat.values.I = 0;
			outdat.values.duty=0;
			int x_temp;
			// read Encoder
			*pEnc = 1;
			delay(5000);
			x_temp = *pEnc * 2; // * 2 for similar size of W and X
			//U[X]=rotations per 100us
			direction = *pDir;
			// add direction
			if (direction == 2)
				x_temp *= -1;
			outdat.values.X = x_temp;
			if(indat.values.setEnMotorController && setEnMotorwasoff){
				outdat.values.enMotorController = TRUE;
				setEnMotorwasoff = FALSE;
			}
		}

		// handle Ultrasound and acceleration sensor
		switch(dist_Counter){
		case 0:
			handle_measuredis();
			break;
		case 5:
			handle_accel();
			break;
		}


	}
	return 0;
	error:
#ifdef VERBOSE
	printf("ERROR occured!!!!!");
#endif
	return -1;
}


