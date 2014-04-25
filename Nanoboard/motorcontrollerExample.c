#include <stdio.h>
#include <system.h>
#include <io.h>
#include <alt_types.h>
#include "altera_avalon_uart.h"
//#define MOTOR_ENCODER
#define UART_2_ETH

void delay (volatile unsigned int del)
{
	while (del != 0)
	{
		del --;
	}
}

#ifdef MOTOR_ENCODER

#define PWM_EN (0x80000000 | A_2_CHANNEL_PWM_0_BASE)
#define PWM_PERIOD PWM_EN+1
#define PWM_DUTY1 PWM_EN+2
#define PWM_DUTY2 PWM_EN+3
#define PWM_PHASE1 PWM_EN+4
#define PWM_PHASE2 PWM_EN+5

void motor_setting(unsigned long phase1, unsigned long duty1,unsigned long phase2, unsigned long duty2,
		unsigned long period,unsigned long enable)
{
	volatile unsigned int * pwm_en= (volatile unsigned int *)PWM_EN;
	volatile unsigned int * pwm_period=(volatile unsigned int *)PWM_PERIOD;
	volatile unsigned int * pwm_phase1=(volatile unsigned int *)PWM_PHASE1;
	volatile unsigned int * pwm_phase2=(volatile unsigned int *)PWM_PHASE2;
	volatile unsigned int * pwm_duty1=(volatile unsigned int *)PWM_DUTY1;
	volatile unsigned int * pwm_duty2=(volatile unsigned int *)PWM_DUTY2;

	* pwm_en=enable;
	* pwm_period=period;
	* pwm_phase1=phase1;
	* pwm_phase2=phase2;
	* pwm_duty1=duty1;
	* pwm_duty2=duty2;

}

void delay (volatile unsigned int del)
{
	while (del != 0)
	{
		del --;
	}
}

int main (void)
{
	volatile unsigned int * pEnc = (volatile unsigned int *) (0x80000000 | MOTOR_ENCODER_0_BASE);
	volatile unsigned int * pDir = (volatile unsigned int *) (0x80000000 | MOTOR_ENCODER_0_BASE + 4);
	volatile unsigned int count;
	volatile unsigned int direction;


	unsigned long phase1=0x0,phase2=0x000,duty1=2000,duty2=4000,period=10000,enable=0x2;

	count = 0; direction = 0;
	printf ("Start\n");
	while (1)
	{
		*pEnc = 1;
		delay (1000000);
		count = *pEnc;
		direction = *pDir;
		printf ("Speed = %u\n", count);
		printf ("Direction = %u\n", direction);
		printf ("Duty = %u\n", duty2);

		if (count < 12000)
		{
			if (duty2 < 9900)
			duty2 += 100;
		}
		else
		{
			if (duty2 > 100)
			duty2 -= 100;
		}
		motor_setting(phase1,duty1,phase2,duty2,period,enable);

	}

}
#endif

#ifdef UART_2_ETH

int main(void)
{
	FILE *fp0;
	FILE *fp1;
	char u0 = 0;
	char u1 = 0;
	int i = 200;

	fp0 = fopen("/dev/uart_0", "r+");
	fp1 = fopen("/dev/uart_1", "r+");

	printf("Start\n");

    while(1)
    {
//   	u0 = 'A';
//    	u1 = '1';
//    	putc(u0, fp0);
//    	putc(u1, fp1);
//    	printf("Transmitted\n");
//    	delay(1000000);
    	u0 = getc(fp0);
    	printf("u0 = %c\n", u0);
//    	    	u1 = getc(fp1);

//    	printf("u0 = %c\n", u0);
//    	printf("u1 = %c\n", u1);
    }

	u0 = 0;
    while (i > 0)
    {
    	i--;
    	putc(u0, fp0);
    	u0++;
    }
    printf("End\n");
    fclose(fp0);
    fclose(fp1);

	return(0);
}

#endif

