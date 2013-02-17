/*
PlayerControl.ino
Amelia Peterson
2/16/13

This code manages the I/O for the player's arduino (IR and RF transmitters and receivers, triggers, item selection),
damage, status conditions, and items.

Notes:
	Ensure that VirtualWire and IRremote do not use the same timers. The IRremote timer can be edited in IRremoteInt.h
(make sure you change the pins after you change the timer). I have VirtualWire using Timer 1 and IRremote using Timer 2.

*/
#include <VirtualWire.h>
#include <IRremote.h>
typedef void (*funcptr)(byte carrier, byte value);

struct Status{
	byte Carrier;
	unsigned long duration;
	unsigned long time_in;
}

//RF Device Variables//
const int RF_input = 9;  		//Must be a PWM pin - RF Receiver Digital Output
const int RF_output = 8;		//RF Transmitter Digital input
const int RF_trigger = 7;		//Normally GND, Power to trigger
const int item = A0;			//Analog input for item selection (currently selects one of three items)
IRrecv irrecv(RF_input); 	     	//Create an IRrecv object
//IR Device Variables//
const int IR_input = 3;  		//Must be a PWM pin - IR Receiver Data Output
const int IR_output = 2;		//Anode of IR LED
const int IR_trigger = 1;		//Normally GND, Power to trigger
decode_results decodedSignal; 	     	//Stores results from IR detector
IRsend irsend;
//Control Variables//
char Stats[2] = {100, 5};  		//Health, Attack
int items[3] = {0xA60A, 0xA00A, 0xA81E};//Magic Missile, Mass Heal, 30 MASSIVE
Status current[3];

funcptr carriers[8] = {Normal, Timed, Buf, Clear, Massive, Special, Element, LD};//Function pointers for each carrier code 0x0-0x7
bool statcon = 0;                       //True if Player has any status conditions
bool Team = 0;     			//0 or 1

void setup(){
  //RF Setup//
  vw_setup(2000);	        // Bits per sec
  vw_set_ptt_inverted(true);    // Required for DR3100
  vw_set_tx_pin(RF_input); 
  vw_set_ptt_inverted(true);    // Required for DR3100
  vw_setup(2000);	        // Bits per sec
  vw_set_rx_pin(RF_output);
  vw_rx_start();                // Start the receiver PLL running
  pinMode(RF_trigger, INPUT);
  //IR Setup//
  pinMode(IR_input, OUTPUT);    //set pin 3 to IR input
  irrecv.enableIRIn();          //Begin the receiving process. This will enable the timer interrupt which consumes a small amount of CPU every 50 Âµs.
  pinMode(IR_trigger, OUTPUT);
  //Initilize Variables
  Status temp = {0, 0, 0};  
}

void IR_fire(){
  int shot = 0xA000;          //Has Header A, carrier code 0x0  
  shot = shot+(Team<<11);      //inclux`de Team # in shot code
  shot = shot+Stats[1];        //add Attack stat
  irsend.sendSony(shot,16);    //Send attack, sendSony(data,#bits)
}

void RF_fire(){
  int index = analogRead(item);      //Grab item number from analog input pin
  index = index%3;                   //Calculate item index based on raw analog value          
  vw_send((uint8_t *)items[index], 2);//Send AoE, vw_send(data,#bytes)
  vw_wait_tx();                      // Wait until the whole message is gone
}

void Get_Damage(){
  if(irrecv.decode(&decodedSignal)==true){      //If IR signal has been received...
    if(decodedSignal.rawlen==16){               //If IR signal is not 16 bits...
      byte data[3];                             //data = {Header, Carrier, Value byte}
      parse(decodedSignal.value, data);         //Parse code into Header, Team/Carrier, Value byte
      if(data[0]!=0xA && Team!=(data[1]>>7)){   //If Header is not equal to 0xA and code was not sent from Team member...
        carriers[data[1]](data[1],data[2]);     //Call function corresponding to carrier
      }
    }    
  }
  if (vw_have_message()){                       //If RF signal has been received...
    uint8_t buf[VW_MAX_MESSAGE_LEN];            //Initialize buffer for message
    uint8_t buflen = VW_MAX_MESSAGE_LEN;        //Initialize variable to store code length
     if (vw_get_message(buf, &buflen)){         //If the data is not corrupted
      if(buflen!=16){                           //If the message length is not equal to 16
        byte data[3];                           //data = {Header, Team/Carrier, Value byte}
        parse(*buf,data);                       //Parse code into Header, Carrier, Value byte
	if(data[0]!=0xA && Team!=(data[1]>>7)){ //If Header is not equal to 0xA and code was not sent from Team member...
          carriers[data[1]](data[1],data[2]);   //Call function corresponding to carrier
	}
      }
    }
  }
}

void parse(long unsigned int signal, byte* data){
  //parse into header, carrier, damage
  data[0] = (signal>>12);     	    //Header
  data[1] = (signal>>8)&(0x0F);     //Team, Carrier
  data[2] = signal;        	    //Level/Value
}
void Normal(byte carrier, byte value){
  signed byte sign = 2*(value>>7);	//0 or 2 -> sign of value will be (-1+sign) = -1 or 1
  signed byte val = value<<1;           //Grab value of code (0-127)
  Stats[0] = Stats[0] + (-1+sign)*val;  //Health = Health + (-1+sign)(Value) - Add or Subtract Health
}
void Timed(byte carrier, byte value){

}
void Buf(byte carrier, byte value){
  byte sign = 2*(value>>7);      //0 or 2 -> sign of value will be (-1+sign) = -1 or 1
  byte stat = value>>6;          //Grab Stat to be added to or subtracted from
  byte val = value<<2;           //Grab value of code (0-63)
  Stats[stat] = Stats[stat] + (-1+sign)*(val);//Stat = Stat + (-1+sign)(Value) - Add or Subtract from Stat
}
void Clear(byte carrier, byte value){

}
void Massive(byte carrier, byte value){

}
void Special(byte carrier, byte value){

}
void Element(byte carrier, byte value){
  
}
void LD(byte carrier, byte value){

}
void StatusConditions(){

}
void loop(){
  //Currently, the RF and IR Trigger pins are continuously
  //polled, but later on they will be implemented with 
  //interrupts.
  if(RF_trigger==HIGH){      //If RF trigger pulled...
    RF_fire();
  }
  if(IR_trigger==HIGH){      //If IR trigger pulled...
    IR_fire();
  }
  
  Get_Damage();              //Detect if IR or RF signal was received
  
  if(statcon){              //Accumulate damage from Status Conditions
    StatusConditions();
  }
}
