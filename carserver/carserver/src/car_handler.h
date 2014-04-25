
struct car{
	struct Nano_Board* board;
	int* conn_boards;
	unsigned short* ubat_power;
	unsigned short* ubat_compute;
	float* distance_front;
	float* distance_rear;
	float v_left;
	float v_right;
	int enDistHandlerFront;
	int enDistHandlerRear;
	int enBattHandler;
	int acc[3];
	};
int parse_command(char* cmd);

int init_car(struct Nano_Board * boards,config_t * config,int *conn);
void handle_car(void);

