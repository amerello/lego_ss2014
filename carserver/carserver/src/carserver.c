#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#include <arpa/inet.h> //htons()
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

#include <libconfig.h> // Library for configuration file parsing

#include "nanoboard.h"
#include "car_handler.h"

#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) > (b)) ? (a) : (b))

#define TRUE 1
#define FALSE 0


const char* nano_init;//[] = "Nano Board Version 1.0\n";
/* {"FRONT LEFT SIDE\n"
			"OUTPUT:\n"
			"bool connBroken\n"
			"bool enMotorController\n"
			"ushort ubat\n"
			"short accX\n"
			"short accY\n"
 	 	 	"short accZ\n"
			"short accXsum\n"
			"short accYsum\n"
			"short accZsum\n"
		        "uint duty\n"
			"uint network_timeout\n"
			"int X\n"
			"int W\n"
			"int P\n"
			"int I\n"
			"int D\n"
			"int FF\n"
			"float distance\n"
			"INPUT:\n"
			"alt_u8 LED\n"
			"bool enConnBrokenCtr\n"
			"bool setEnMotorController\n"
			"int W\n\n"};
*/
struct {
	unsigned short port[4];
	struct in_addr addr[4];
	}options;
int run;
/* Signal handler that interrupts the event loop. */
void
cleanup(int sig)
{
	fprintf(stderr,"\nReceived signal %d. Shutting down...\n",sig);
	run = 0;
}

/* Opens a domain socket, calls bind() and listen(), stores information in the
 * sockaddr structure and returns the socket descriptor or -1 on failure. */
int
open_dom(struct sockaddr_un *sa, const char *filename)
{
	int sd,slen;

	memset(sa,0,sizeof(*sa));
	sa->sun_family = AF_UNIX;
	strncpy(sa->sun_path,filename,sizeof(sa->sun_path));
	unlink(sa->sun_path);
	slen = sizeof(sa->sun_family) + strlen(sa->sun_path);

	if (0 > (sd=socket(AF_UNIX,SOCK_SEQPACKET,0))) {
		perror("socket() failed");
		return -1;
	}

	if (0 > bind(sd,(struct sockaddr *)sa,slen)) {
		perror("bind() failed");
		return -1;
	}

	if (0 > listen(sd,0)) {
		perror("listen() failed");
		return -1;
	}

	return sd;
}
int MIN_MSGS;


int
main(int argc, char **argv)
{
	
	
	struct sockaddr_un sa_dom_srv;
	struct sockaddr_un sa_dom;
	struct Nano_Board nano[4];
        ssize_t len;
	socklen_t sa_dom_len;
	fd_set rfds,rfd;
	int ret,maxfd,sd_in,sd_dom_srv,sd_dom,i,conn_boards=0;
	int conn_ctr[4];
	char buffer[ETH_FRAME_LEN];
	config_t conf;

	memset(&sa_dom,0,sizeof(sa_dom));
	memset(&sa_dom_len,0,sizeof(sa_dom_len));
	
	memset(nano,0,4*sizeof(struct Nano_Board));
	
	(void)signal(SIGINT,cleanup);
	(void)signal(SIGTERM,cleanup);
	
	config_init(&conf);
	
	if(CONFIG_FALSE == config_read_file(&conf,"carserver.cfg")){	
		fprintf(stderr,"Problem with reading Configuration in line %d:\n%s\n",
			config_error_line(&conf),
			config_error_text(&conf));
			config_destroy(&conf);
			return -1;
	}
	
	if (0 > config_lookup_int(&conf,"MIN_MSGS",&MIN_MSGS))
		{
			fprintf(stderr,"could not read MIN_MSGS\n");
			run = 0;
		}

	FD_ZERO(&rfds); //Initializes the descriptor to the null set.
	maxfd =-1;
	const char * addr;
	int port,led,W;
	int eCBC;
	config_setting_t *setting;
	setting = config_lookup(&conf, "Boards");
	if( 4 != config_setting_length(setting)){
		fprintf(stderr,"wrong number of boards configured");
		return -1;
	}
	run = 1;
	for(i = 0; i< 4&& run; i++){
		sd_in = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		if(sd_in < 0){
			perror("opening Socket failed");
			run=0;
		}
		config_setting_t *board = config_setting_get_elem(setting, i);
		if(!(config_setting_lookup_string(board, "Address", &addr)&&
			config_setting_lookup_int(board, "Port", &port)&&
			config_setting_lookup_int(board, "led", &led)&&
			config_setting_lookup_int(board, "W", &W)&&
			config_setting_lookup_bool(board, "enConnBrokenCtr",&eCBC)))
		{
			fprintf(stderr,"Could not read ip-Address and Port of board %d\n",i);
			run=0;
		}
		nano[i].sa.sin_family = AF_INET;
		
		printf("ip address and port of board number %d: %s, %d \n",i, addr,port);
		inet_aton(addr,&(nano[i].sa.sin_addr)); //Converts IP Address from char* to struct in_addr
		
		nano[i].sa.sin_port = htons(port); //converts unsigned short int from host byte order to network byte order
		nano[i].indat.values.led = led;
		nano[i].indat.values.W = W;
		nano[i].indat.values.setEnMotorController = TRUE;
		nano[i].indat.values.enConnBrokenCtr =eCBC;
	
		if(0 > connect(sd_in, (struct sockaddr *)&(nano[i].sa),sizeof(struct sockaddr))){
			fprintf(stderr,"connect number %d failed\n",i);
			run=0;
		}
		nano[i].sd = sd_in;
			
		buffer[0]= 'I';
		ret = send(sd_in,buffer,1,MSG_DONTWAIT);
		if(0 >= ret){
			close(sd_in);
			printf("nano-board number %d did not receive."
				" closed connection.\n",i);
			run =0;
		}
		printf("sent I, this is %d byte\n",ret);
		nano[i].init = TRUE;
		FD_SET(sd_in,&rfds); //Adds the file descriptor sd_in to the set rfds
		maxfd = MAX(maxfd,sd_in);
		conn_boards |= 1<<i;
		conn_ctr[i] =0;
		// don't reconnect in the first 3 seconds:
		nano[i].last_conn_attempt = time(NULL)+3;
	}
	if (0 > config_lookup_string(&conf,"InitString",&nano_init))
	{
		fprintf(stderr,"could not read InitString\n");
		run = 0;
	}
	const char * str;
	if (config_lookup_string(&conf,"ClientSocket",&str)){
	  if (0 > (sd_dom_srv = open_dom(&sa_dom_srv,str))){
		fprintf(stderr,"open_dom() failed\n");
		return -1;
	  }
	} else {
		printf("Could not read ClientSocket from config file\n");
	}
	//init conn_timer 1sec in advance to make sure that there are enogh messages in the first second.
	time_t conn_timer = time(NULL)+1;
	struct timeval timeout;
	FD_SET(sd_dom_srv, &rfds);
	maxfd = MAX(maxfd,sd_dom_srv);
	timeout.tv_sec=1;
	timeout.tv_usec=0;
	sd_dom = -1;
	// battery power, battery compute , dist front, dist rear
	if(0 > init_car(nano,&conf,&conn_boards)){
		return -1;
	}
	
	/* Event loop: runs while run == 1 */
	while (run) {
		
		handle_car();
		// make sure that all nanoboards stay connected
		if(conn_timer< time(NULL)){
			for(i = 0; i< 4; i++){
				if(conn_ctr[i] < MIN_MSGS){
					conn_boards &= ~(1<<i);
					FD_CLR(nano[i].sd,&rfds);
					close(nano[i].sd);
					fprintf(stderr,"Board %d did not reach obligatory number of messages: only %d, not %d\n",i,conn_ctr[i],MIN_MSGS);
				}
				conn_ctr[i]=0;
				
			}
			conn_timer = time(NULL);
		}
		
			
		// if one board is disconnected, try to reconnect it
		for(i = 0; i<4;i++){
			if((conn_boards & 1<<i)==0 && nano[i].last_conn_attempt+1<time(NULL)){
			    nano[i].last_conn_attempt= time(NULL);
				nano[i].sd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
				if(nano[i].sd < 0){
					perror("opening Socket failed");
					continue;	
				}
				printf("reconnect board number %d\n",i);
				if(0 > (len = connect(nano[i].sd, (struct sockaddr *)&(nano[i].sa),sizeof(struct sockaddr)))){
						fprintf(stderr,"reconnect number %d failed error: %s\n",i,strerror(len));
						continue;
				}
				buffer[0]= 'I';
				ret = send(nano[i].sd,buffer,1,MSG_DONTWAIT);
				if(0 >= ret){
					printf("Was not able to send\n");
					close(nano[i].sd);
					continue;
				}
				nano[i].init = TRUE;
				FD_SET(nano[i].sd,&rfds);
				
				maxfd = MAX(maxfd,nano[i].sd);
				
			}
		}
		
		rfd = rfds;
		//printf("Waiting for incoming connections\n");
		ret = select(maxfd+1,&rfd,NULL,NULL,&timeout);
		if (0 > ret) {
			if (errno == EINTR)
				continue;
			perror("select() failed");
			run = 0;
		}
		
		/* Check the listening socket for incoming connections. */
		if (FD_ISSET(sd_dom_srv,&rfd)) {
			/* New client, accept() and add him. */
			sd_dom=accept(sd_dom_srv,(struct sockaddr*)&sa_dom,
					&sa_dom_len);
			if (0 > sd_dom) {
				perror("accept() failed");
				run = 0;
			}

			FD_SET(sd_dom,&rfds);
			maxfd = MAX(maxfd,sd_dom);

			fprintf(stdout,">> client connected to %s\n",
					sa_dom_srv.sun_path);
		}

		/* Read something from the client? */
		if (sd_dom != -1 && FD_ISSET(sd_dom,&rfd)) {
			printf("Read something from the Client.");
			len = recv(sd_dom,buffer,ETH_FRAME_LEN,0);
			if (0 >= len) {
				FD_CLR(sd_dom,&rfds);
				close(sd_dom);
				sd_dom = -1;

				fprintf(stdout,"client disconnected from %s\n",
						sa_dom_srv.sun_path);

				continue;
			}

			fprintf(stdout,">> received command from %s:\n",
					sa_dom_srv.sun_path);
			// read command and
			//int resp = read_command(buffer,len)
			//if (0 > resp)
			//	continue;
			len = parse_command(buffer); 
			/*sprintf(buffer,"X:%d, W:%d, AccX:%d, AccY:%d\n",nano[0].outdat.values.X,
				nano[0].outdat.values.W, nano[0].outdat.values.Acc.sum[0],
				nano[0].outdat.values.Acc.sum[1]);
			*/
			printf("response to command is:\n%s\n(len:%d)\n",buffer,len);
			ret = send(sd_dom,buffer,len,MSG_DONTWAIT);
			if (0 >= ret) {
				FD_CLR(sd_dom,&rfds);
				close(sd_dom);
				sd_dom = -1;

				fprintf(stdout,"client disconnected from %s\n",
						sa_dom_srv.sun_path);

				continue;
			}
		}
		/* Read from a NanoBoard. */
		for(i =0; i < 4 ; i++){
			if (FD_ISSET(nano[i].sd,&rfd)){
				conn_ctr[i]++;
				//printf("Received something from nanoboard #%d.\n",i);
				len = recv(nano[i].sd,buffer,ETH_FRAME_LEN,0);
				if(len < 0){
				printf("recv did not work. sd: %d ;error: %s\n",nano[i].sd,strerror(len));
				conn_boards &= ~(1<<i);
				FD_CLR(nano[i].sd,&rfds);
				close(nano[i].sd);
				continue;
				} else
				if(sizeof(outdata)+1!=len||buffer[0]!='R'){
					if(nano[i].init == TRUE){
						printf("Init frame received. Try to compare:\n");
						nano[i].init=FALSE;
                        buffer[len]='\0';
						if(strcmp(nano_init,buffer)!=0){
							printf("compared strings, but there is a"
								"fault:\nimx6:%sNano:%s\n",nano_init,buffer);
							printf("buffer[0] = %c, size of message: %d\n",buffer[0],len);
							if( buffer[0] == 'I' || buffer[0]== 'R'){
								printf("Ignore this character.\n");
								nano[i].init = TRUE;
								continue;
							}else
								run =0;
						}else{
							printf("string is the same, connected!\n");
							conn_boards |= (1<<i);
						}
					} else {
						FD_CLR(nano[i].sd,&rfds);
						close(nano[i].sd);
						fprintf(stderr,"nano-board number %d sent wrong data."
						 "\n length is %d, should be %d. first char in buffer"
 						 " is %c. \n closed connection.\n",
						 i,len,sizeof(outdata)+1,buffer[0]);
						conn_boards &= ~(1<<i);
						FD_CLR(nano[i].sd,&rfds);
				        close(nano[i].sd);
						if(conn_boards==0){
							printf("no more boards, stop running.\n");
							run =0;
						}
 						continue;
					}
				}else
				{
					memcpy(&(nano[i].outdat.serial),buffer+1,len-1);
				}
				// copy indat to buffer:
				memcpy(buffer+1,&(nano[i].indat.serial),sizeof(indata));
				buffer[0]='S';
				//printf("W: %d\n",nano[i].indat.values.W);
				ret = send(nano[i].sd,buffer,
					sizeof(indata)+1,MSG_DONTWAIT);
				if(0 >= ret){
					FD_CLR(sd_dom,&rfds);
					close(nano[i].sd);
					printf("nano-board number %d did not receive."
						" closed connection.\n",i);
					run = 0;
				}
			}
		}
	}
	for(i =0; i< 4; i++)
		if(conn_boards & 1<<i)
			close(nano[i].sd);	
		
	if (sd_dom > -1 && FD_ISSET(sd_dom,&rfds))
		close(sd_dom);
	close(sd_dom_srv);
	unlink(sa_dom_srv.sun_path);
	config_destroy(&conf);
	fprintf(stderr,"Shutdown complete.\n\n");

	return 0;
		
}
		


			
		
        
	
	

	
	




