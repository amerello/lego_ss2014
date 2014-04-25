
#ifndef NANO 
#define NANO


typedef unsigned char alt_u8;

struct acc{
	short acc[3];
	short sum[3];
};

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

typedef union outdata{
	struct {
		alt_u8 connBroken;
		alt_u8 enMotorController;
		unsigned short  ubat;
		struct acc Acc;
		uint  duty;
		uint  network_timeout;
		int X;
		int W;
		int P;
		int I;
		int D;
		int FF;
		float distance;
	}values;
	alt_u8 serial[48];
}outdata;

struct Nano_Board {
	struct sockaddr_in sa;
	short init;
	int sd;
	indata indat;
	outdata outdat; 
	time_t last_conn_attempt;
	};
#endif

