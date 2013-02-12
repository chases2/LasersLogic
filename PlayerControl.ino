/*
PlayerControl.ino
Amelia Peterson
1/11/13

This code manages the I/O for the player's arduino (IR and RF transmitters and receivers, triggers, item selection),
damage, status conditions, and items.

Notes:
	Ensure that VirtualWire and IRremote do not use the same timers. The IRremote timer can be edited in IRremoteInt.h
(make sure you change the pins after you change the timer). I have VirtualWire using Timer 1 and IRremote using Timer 2.

*/
#include <VirtualWire.h>
#include <IRremote.h>

int RF_input = 9;  		//Must be a PWM pin - RF Receiver Digital Output
int RF_output = 8;		//RF Transmitter Digital input
int RF_trigger = 7;		//Normally GND, Power to trigger
int item = A0;			//Analog input for item selection (currently selects one of three items)
int IR_input = 3;  		//Must be a PWM pin - IR Receiver Data Output
int IR_output = 2;		//Anode of IR LED
int IR_trigger = 1;		//Normally GND, Power to trigger
IRrecv irrecv(RF_input); 	     //Create an IRrecv object
decode_results decodedSignal ; 	     //Stores results from IR detector
char Stats[2] = {100, 5};  	//Health, Attack
long items[3] = {0xA60A, 0xA00A, 0xA81E};//Magic Missile, Mass Heal, 30 MASSIVE
bool Team = 0;     		//0 or 1
IRsend irsend;

void setup(){
  //RF Setup//
  vw_setup(2000);	        // Bits per sec
  vw_set_ptt_inverted(true);    // Required for DR3100
  vw_set_tx_pin(RF_input); 
  vw_set_ptt_inverted(true);    // Required for DR3100
  vw_setup(2000);	        // Bits per sec
  vw_set_rx_pin(RF_output);
  vw_rx_start();                // Start the receiver PLL running
  pinMode(IR_trigger, INPUT);
  //IR Setup//
    pinMode(IR_input, OUTPUT);  //set pin 3 to IR input
  irrecv.enableIRIn();          //Begin the receiving process. This will enable the timer interrupt which consumes a small amount of CPU every 50 Âµs.
  pinMode(RF_trigger, OUTPUT);  
}

void IR_fire(){
  long shot = 0xA000;
  shot = shot+(Team<<11);
  shot = shot+Stats[1];
  irsend.sendSony(shot,16);
}

void RF_fire(){
  int index = analogRead(item);
  index = index%3;
  vw_send((uint8_t *)items[index], 4);
  vw_wait_tx(); // Wait until the whole message is gone
}

void Get_Damage(){
  if(irrecv.decode(&decodedSignal)==true){
    if(decodedSignal.rawlen==16){
      char data[3];
      parse(decodedSignal.value, data);
      if(data[0]!=0xA){
        //handle damage
      }
    }    
  }
  if (vw_have_message()){
    uint8_t buf[VW_MAX_MESSAGE_LEN];
    uint8_t buflen = VW_MAX_MESSAGE_LEN;
     if (vw_get_message(buf, &buflen)){ //If the data is not corrupted
      if(buflen!=16){
        char data[3];
        parse(*buf,data);
        //handle damage
       }
    }
  }
}

void parse(long unsigned int signal, char* data){
  //parse into header, carrier, damage
  data[0] = signal>>12;    //Header
  data[1] = signal>>8;     //Team, Carrier
  data[2] = signal;        //Level
}
void loop(){
  //Currently, the RF and IR Trigger pins are continuously
  //polled, but later on they will be implemented with 
  //interrupts.
  if(RF_trigger==HIGH){
    RF_fire();
  }
  if(IR_trigger==HIGH){
    IR_fire();
  }
  Get_Damage();
  

}
