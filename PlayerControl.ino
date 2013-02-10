/*
PlayerControl.ino
1/10/13
*/
#include <VirtualWire.h>
#include <IRremote.h>

int RF_input = 9;  //Must be a PWM pin
int RF_output = 8;
int RF_trigger = 7;
int item = A0;
int IR_input = 3;  //Must be a PWM pin
int IR_output = 2;
int IR_trigger = 1;
IRrecv irrecv(RF_input); //create an IRrecv object
decode_results decodedSignal ; //stores results from IR detector
char Stats[2] = {100, 5};  //Health, Attack
long items[3] = {0xA045, 0xA110, 0xA230};  //Magic Missile, Mass Heal, 30 MASSIVE
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
  //IR Setup//
    pinMode(IR_input, OUTPUT);  //set pin 3 to IR input
  irrecv.enableIRIn();          //Begin the receiving process. This will enable the timer interrupt which consumes a small amount of CPU every 50 µs.  
}

void IR_fire(){
  long shot = 0xA000;
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
      parse(&decodedSignal.value, data);
      if(data[0]!='10'){
        //handle damage
      }
    }    
  }
  if (vw_have_message()){
   
  }
}

void parse(long unsigned int* signal, char* data){
  //parse into header, carrier, damage
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
