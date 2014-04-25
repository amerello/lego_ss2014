/*---------------------------------------------------------------
Ultrasound Interface
features: Measure the distance
This code is designed by Gang Chen and is verified by Biao Hu.
Author: Gang CHEN @ TUM 
Copyritht: the copyright of this IP is belong to Gang CHEN.
Correspongding: cheng@in.tum.de hub@in.tum.de
-----------------------------------------------------------------*/
module Ultrasound_interface(clk,reset_n,chipselect,address,write,writedata,read,byteenable,readdata,trigger_out,feedback_in);
parameter idle=4'b0001, trigger=4'b0010, wait_feedback=4'b0100, measure_feedback=4'b1000;
input clk;
input reset_n;
input chipselect;
input [1:0]address; 
input write;  
input [31:0] writedata;  
input read;  
input [3:0] byteenable;  
output reg [31:0] readdata; 
input feedback_in; 
output  reg trigger_out;

reg [31:0] control_reg; 
reg [31:0] distance_count; 
reg [31:0] state_reg; 

reg  control_reg_sel;   
reg  distance_count_sel;   
reg  state_reg_sel; 

reg feedback_in1;
reg feedback_in2;

wire start_bit;
assign start_bit=control_reg[0];

reg trig0, trig1;
reg [31:0] tri_count; //20us   50*20=1000
reg [3:0] state;
//function apart
always@(posedge clk or negedge reset_n)
	begin 
		if(!reset_n)
			begin
				trig0<=0;
				trig1<=0;
			end
		else
			begin
				trig0<=start_bit;
				trig1<=trig0;
			end
	end

always@(posedge clk or negedge reset_n)
	begin 
		if(!reset_n)
			begin
				feedback_in1<=0;
				feedback_in2<=0;
			end
		else
			begin
				feedback_in1<=feedback_in;
				feedback_in2<=feedback_in1;
			end
	end	


always@(posedge clk or negedge reset_n)
	begin 
		if(!reset_n)
			begin
				state<=idle;
				tri_count<=0;
				trigger_out<=0;
				state_reg<=2;
			end
		else
			begin
				case(state)
					idle:
						begin
							tri_count<=0;
							trigger_out<=0;
							state_reg<=2;
							if(trig0==1&&trig1==0)
								begin
									state<=trigger;
								end
							else
								begin
									state<=idle;
								end
						end
					trigger:
						begin
							state_reg<=1; //bit 0: busy state; bit 1: data is ready
							if(tri_count[10])  //1024 count is greater than 10us, 50*20=1000
								begin
									trigger_out<=0;
									tri_count<=0;
									state<=wait_feedback;
								end
							else
								begin
									trigger_out<=1;
									tri_count<=tri_count+1;
									state<=trigger;
								end
						end
					wait_feedback:
						begin
							state_reg<=1; //bit 0: busy state; bit 1: data is ready
							trigger_out<=0;
							if(feedback_in1==1&&feedback_in2==0)
								begin
									tri_count<=tri_count+1;
									state<=measure_feedback;
								end
							else
								begin
									state<=wait_feedback;
									tri_count<=0;
								end
						end
					measure_feedback:
						begin
							trigger_out<=0;
							if(feedback_in1==1)
								begin
									if(tri_count[19]&&tri_count[18]&&tri_count[17])
										begin
											distance_count<=-1;
											tri_count<=0;
											state<=idle;
											state_reg<=2;
										end
									else
										begin
											tri_count<=tri_count+1;
											state<=measure_feedback;
											state_reg<=1;//data is ready
										end
									
								end
							else
								begin
									distance_count<=tri_count;
									tri_count<=0;
									state<=idle;
									state_reg<=2;//bit 0: busy state; bit 1: data is ready
								end
						end
					default:
					   begin
							state<=idle;
							tri_count<=0;
							trigger_out<=0;
							state_reg<=2;
						end
				endcase
			end
	end

//bus interface
always @ (address)  
begin  
	control_reg_sel<=0;   
	distance_count_sel<=0;   
	state_reg_sel<=0; 
    case(address)  
        2'b00:control_reg_sel<=1;  
        2'b01:distance_count_sel<=1;
        2'b10:state_reg_sel<=1;  
		2'b11:state_reg_sel<=1;	//default value
     endcase  
end 

//wirte control register 
always @ (posedge clk or negedge reset_n)  
begin  
	if(!reset_n)   
		control_reg<=0;  
	else 
		begin
			if(write & chipselect & control_reg_sel)  
				begin  
				if(byteenable[0])  control_reg[7:0]<=writedata[7:0]; 
				if(byteenable[1])  control_reg[15:8]<=writedata[15:8];
				if(byteenable[2])  control_reg[23:16]<=writedata[23:16];
				if(byteenable[3])  control_reg[31:24]<=writedata[31:24];
				end  
		end  
end


//read register 
always @ (address or read or control_reg or distance_count or state_reg or chipselect)  
begin  
    if(read & chipselect)  
         case(address)  
             2'b00:
				begin
				readdata<=control_reg;
				end				
            2'b01:
				begin
				readdata<=distance_count;
				end				
             2'b10:
				begin
				readdata<=state_reg;
				end				
            2'b11:
				begin
				readdata<=state_reg; //default value
				end
         endcase   
end
endmodule 
