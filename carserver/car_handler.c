#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/ip.h>
#include <netinet/ether.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <libconfig.h>

#include "car_handler.h"
#include "nanoboard.h"
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) > (b)) ? (a) : (b))
struct car Car;
double DIST_BREAK_BEGIN;
double DIST_MIN;
int handler_counter;

int init_car(struct Nano_Board * boards,config_t * config, int* conn){
	int ubatP, ubatC, distF, distR;
	memset(&Car,0,sizeof(Car));
	
	if(!(config_lookup_int(config,"BatterySensorPower",&ubatP)&&
		config_lookup_int(config,"BatterySensorCompute",&ubatC)&&
		config_lookup_int(config,"DistanceSensorFront",&distF)&&
		config_lookup_int(config,"DistanceSensorRear",&distR)&&
		config_lookup_bool(config,"EnableDistanceHandlerFront",&(Car.enDistHandlerFront))&&
		config_lookup_bool(config,"EnableDistanceHandlerRear",&(Car.enDistHandlerRear))&&
		config_lookup_bool(config,"EnableBatteryHandler",&(Car.enBattHandler))))
	{ 
		fprintf(stderr,"Problem with reading assignment of Sensors\n");
		return -1;
	}
	if (!(config_lookup_float(config,"Dist_min",&DIST_MIN)&&
		config_lookup_float(config,"Dist_break_begin",&DIST_BREAK_BEGIN))){
		fprintf(stderr,"can't read Dist_min and Dist_break_begin");
	}
	{ printf("read settings from Configfile: Board with ubatP: %d, ubatC: %d, distF: %d, distR: %d\n"
		  "enable: DistHandlerFront: %s, Rear: %s, BattHandler: %s\n"
		  "dist min: %f, dist max: %f",ubatP, ubatC, distF,distR,(Car.enDistHandlerFront)?"yes":"no",(Car.enDistHandlerRear)?"yes":"no",(Car.enBattHandler)?"yes":"no",DIST_MIN,DIST_BREAK_BEGIN);
	}
	
	
	Car.board = boards;
	Car.ubat_power = &(boards[ubatP].outdat.values.ubat);
	Car.ubat_compute = &(boards[ubatC].outdat.values.ubat);
	Car.distance_front = &(boards[distF].outdat.values.distance);
	Car.distance_rear = &(boards[distR].outdat.values.distance);
	Car.v_left = 0;
	Car.v_right = 0;
	Car.conn_boards = conn;

	handler_counter = 0;
	return 0;
}

#define FWD 1
#define BWD 2
struct handler_state{
	int state;
	float vr_max;
	float vl_max;
	
};
struct handler_state bh_state;
struct handler_state dhf_state, dhr_state;

	
	
#define IDLE 0
#define BATT_1_EMPTY 1
#define BATT_2_EMPTY 2
void battery_handler(struct handler_state* s){
	s-> state = IDLE;
	if (*(Car.ubat_power) < 1000)
		s->state = BATT_1_EMPTY;
	if (*(Car.ubat_compute) < 1000)
		s->state = BATT_2_EMPTY;
	if( s->state != IDLE){
		s->vr_max =0;
		s->vl_max =0;
	}else{
		s->vr_max = 1;
		s->vl_max = 1;
	}
}


#define DIST_BREAKING 1
#define DIST_STOP 2
void dist_handler(struct handler_state* s,float* distance){
	//printf("Distance: %f, Dist_break_begin: %f\n",*distance,DIST_BREAK_BEGIN);

	if(*(distance) == -1 || (*distance > DIST_BREAK_BEGIN))
	{
		s->state = IDLE;
		s->vr_max = 1;
		s->vl_max = 1;
	}
	else if(*distance < DIST_MIN){
		s->state = DIST_STOP;
		s->vr_max = 0;
		s->vl_max = 0;
	}
	else
	{
		//printf("Distance for breaking: %f\n",*distance);
		s->state = DIST_BREAKING;
		float v = (*(distance) - DIST_MIN)* 1.0/(DIST_BREAK_BEGIN-DIST_MIN);
		s->vr_max = v;
		s->vl_max = v;
	}

}

void acc_handler(void){
	int i;
	int accsum;
	// sum up x and y
	for(i = 0; i <2 ; i++){
		Car.acc[i] = Car.board[0].outdat.values.Acc.acc[i]+Car.board[3].outdat.values.Acc.acc[i]-Car.board[1].outdat.values.Acc.acc[i]-Car.board[2].outdat.values.Acc.acc[i];
		Car.acc[i] = Car.acc[i]/4;
	}
    // sum up z
	accsum = 0;
	for(i=0;i < 4; i++){
			accsum += Car.board[i].outdat.values.Acc.acc[2];
	}
	Car.acc[2]=accsum/4;
	//printf("Acc: x:%d, y:%d, z:%d\n",Car.acc[0],Car.acc[1],Car.acc[2]);
}

		
		
#define V_MAX 200
void handle_car(void){
	battery_handler(&bh_state);
	dist_handler(&dhf_state,Car.distance_front);
	dist_handler(&dhr_state,Car.distance_rear);
	acc_handler();
	
	float vl =Car.v_left;
	float vr =Car.v_right;
	if(Car.enBattHandler){
		vl = vl*bh_state.vl_max;
		vr = vr*bh_state.vr_max;
	}

	if(Car.enDistHandlerFront){
		if(Car.v_left > 0)
			vl = MIN(dhf_state.vl_max,vl);
		if(Car.v_right > 0)
			vr = MIN(dhf_state.vr_max,vr);
	}
	if(Car.enDistHandlerRear){
		if(Car.v_left < 0)
			vl = MAX(-dhr_state.vl_max,vl);
		if(Car.v_right < 0)
			vr = MAX(-dhr_state.vr_max,vr);
	}

	//printf("Batt Handler state: %d, Dist Handler: %d, %d\n",bh_state.state,dhf_state.state,dhr_state.state);
	//printf("vr: %f, vl: %f",vr,vl);


	//if voltage is to low, led's are blinking.
	//you should have recharged the batteries 10 minutes ago :)
	if(bh_state.state != IDLE){
		if (handler_counter % 1000 == 0){
		
		if ( (handler_counter/1000) %2 == 0){
			Car.board[0].indat.values.led = 86; //turning on alternatively
			Car.board[1].indat.values.led = 86;
			Car.board[2].indat.values.led = 86;
			Car.board[3].indat.values.led = 86;
		}
		else {
			Car.board[0].indat.values.led = 0;
			Car.board[1].indat.values.led = 0;
			Car.board[2].indat.values.led = 0;
			Car.board[3].indat.values.led = 0;
			}
		}
		}

	else{
		//printf("Status is idle, turning on one led\n");
		Car.board[0].indat.values.led = 1;
		Car.board[1].indat.values.led = 2;
		Car.board[2].indat.values.led = 4;
		Car.board[3].indat.values.led = 8;
	}
	
	if (handler_counter >=30000)
		handler_counter=0;
	else
		handler_counter++;
	
	Car.board[0].indat.values.W = (int)(vr*V_MAX);
	Car.board[1].indat.values.W = (int)(vr*V_MAX);
	Car.board[2].indat.values.W = (int)(vl*-V_MAX);
	Car.board[3].indat.values.W = (int)(vl*-V_MAX);
	
}
	
int parse_command(char* cmd){
	/*
	distance_front : %f (if -1.0 no obstacle)
	distance_rear : %f (if -1.0 no obstacle)
	ubat_power : %d (real voltage)  >12 = 100%; <10 = 0%
	ubat_compute : %d (real voltage)  >12 = 100%; <10 = 0%
	v_left : %f (0-1) show in percentage
	v_right : %f (0-1) show in percentage

	nano board status:
	board_status: [0,0,0,0] (0 is down, 1 is up)
	*/
	float l,r;
	printf("lets parse the command...\n");
	printf("the command string is: %s\n",cmd);
	int check = sscanf(cmd,"[%f,%f]",&l,&r);
	printf("sscanf returns %d values: L: %f, R: %f\n",check,l,r);	
	if(check == 2 && l <= 1 && r <= 1 && l >= -1 && r >= -1){
		Car.v_left = l;
		Car.v_right = r;
		printf("I: %6d \t D: %5d \t P: %6i FF: %5d \t",Car.board[0].outdat.values.I, Car.board[0].outdat.values.D, Car.board[0].outdat.values.P,Car.board[0].outdat.values.FF);
		printf("Duty = %5u   W = %d Ubat: %d ", Car.board[0].outdat.values.duty, Car.board[0].outdat.values.W, Car.board[0].outdat.values.ubat);
		printf("MotorContr:%d\n",Car.board[0].outdat.values.enMotorController);
		printf("Batt Handler state: %d, Dist Handler: %d, %d\n",bh_state.state,dhf_state.state,dhr_state.state);
		printf("Acc: x:%d, y:%d, z:%d\n",Car.acc[0],Car.acc[1],Car.acc[2]);
		return sprintf(cmd,"{\"distance_front\":-1.0,\"distance_rear\":%f,"
				           "\"ubat_power\":%d,\"ubat_compute\":%d,"
						   "\"v_left\":%f,\"v_right\":%f,\"board_status\":[%d,%d,%d,%d],\"acc\":[%d,%d,%d]}",
						   *(Car.distance_rear),
						   *(Car.ubat_power),*(Car.ubat_compute),Car.v_left,
						   Car.v_right,*Car.conn_boards & 1,(*Car.conn_boards & 2)>>1,
						   (*Car.conn_boards & 4)>>2,(*Car.conn_boards & 8)>>3,
						   Car.acc[0],Car.acc[1],Car.acc[2]);

	}else if(strcmp("GetValues",cmd)){
		sprintf(cmd,"{AccX:%d,AccY:%d,AccZ:%d",Car.acc[0],Car.acc[1],Car.acc[2]);
		printf("%s\n",cmd);
	}
	
	strcpy(cmd,"Default return value");
	return 21;
}

