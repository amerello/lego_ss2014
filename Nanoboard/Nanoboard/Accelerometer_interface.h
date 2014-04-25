/*
 * Accelerometer_interface.h
 *
 *  Created on: Jan 22, 2014
 *      Author: AndreasHuber
 */

#ifndef ACCELEROMETER_INTERFACE_H_
#define ACCELEROMETER_INTERFACE_H_

#include "terasic_lib/terasic_includes.h"
#include "terasic_lib/accelerometer_adxl345_spi.h"
#include "terasic_lib/I2C.h"
#include "terasic_lib/adc_spi_read.h"

// for each axis the actual values are saved.
// in addition to this we save the sum of each axis since last transmittion to main processor
// to get a better value. We are not sure which value is more accurate.
struct acc {
	short acc[3];
	short sum[3];
};
short ACC_cali[3];

//to reduce the bias we sum up the sum of 100 values and subtract the mean of values.
int calibrate_acc(){
	bool bSuccess;
	alt_16 szXYZ[3];
	int sumXYZ[3];

	/*Initialize the Accelerometer via SPI interface.*/
	bSuccess = ADXL345_SPI_Init(GSENSOR_SPI_BASE);
    int i;
    for(i = 0; i< 3; i++){
    	sumXYZ[i]=0;
    }
#ifdef VERBOSE
    printf("Calibrating Acceleration sensor...");
#endif
    /*If initialization is successful, continue reading.*/
	for(i=0;i<100&& bSuccess;)
	{
		/*Check if SPI interface has read data or not. If data is available read it.*/
		if (ADXL345_SPI_IsDataReady(GSENSOR_SPI_BASE))
		{
			bSuccess = ADXL345_SPI_XYZ_Read(GSENSOR_SPI_BASE, szXYZ);
			if (bSuccess)
			{
#ifdef VERBOSE
				printf("Calibration #%d,sumXYZ[0]=%d\n",i,sumXYZ[0]);
#endif
				int j;
				for(j = 0; j< 3; j++)
					sumXYZ[j]+=szXYZ[j];
			    i++;
			}
		}
	}
#ifdef VERBOSE
	if (!bSuccess){
	printf("\r\nFailed to access accelerometer\r\n");
	return -1;
	}
	else{
		printf("sum: x:%d, y:%d, z: %d\n",sumXYZ[0],sumXYZ[1],sumXYZ[2]);
		for(i = 0; i< 3; i++){
			ACC_cali[i]=sumXYZ[i]/25;
		}
		// special handling for z-axis: subtract 1000 mg
		ACC_cali[2]-= 1000;
		printf("offset: x:%d, y:%d, z:%d\n",ACC_cali[0],ACC_cali[1],ACC_cali[2]);
		printf("...done\n");
		return 0;
	}
#else
	if(!bSuccess) return -1; else return 0;
#endif
}

struct acc *ACC_acc;
const int ACC_mg_per_digit = 4;

bool init_accel(struct acc* Acc){
 	ACC_acc = Acc;
 	int i;
 	for(i=0;i<3; i++)
 		ACC_acc->sum[i]=0;

    return calibrate_acc();
}
void handle_accel(void)
{

	alt_16 szXYZ[3];
	/*Check if SPI interface has read data or not. If data is available read it.*/
	if (ADXL345_SPI_IsDataReady(GSENSOR_SPI_BASE))
	{

		if(ADXL345_SPI_XYZ_Read(GSENSOR_SPI_BASE, szXYZ))
		{
			int i;
			for(i=0;i<3;i++){
				ACC_acc->acc[i]=szXYZ[i]*ACC_mg_per_digit-ACC_cali[i];
				ACC_acc->sum[i]=ACC_acc->acc[i]+ ACC_acc->sum[i];

			}
		}
	}
}

#endif /* ACCELEROMETER_INTERFACE_H_ */
