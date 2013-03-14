/*
PlayerControl_Caster.ino
Amelia Peterson
3/13/13

This code manages the I/O for the player's arduino (IR and RF transmitters and receivers, triggers, item selection),
damage, status conditions, and items.

Notes:
Ensure that VirtualWire and IRremote do not use the same timers. The IRremote timer can be edited in IRremoteInt.h
(make sure you change the pins after you change the timer). I have VirtualWire using Timer 1 and IRremote using Timer 2.

*/
#include <VirtualWire.h>
#include <IRremote.h>
typedef void (*funcptr)(byte carrier, byte value);
//Accelerometer Definitions
#define ADC_AMPLITUDE 1024//amplitude of the 10bit-ADC of Arduino is 1024LSB
#define ADC_REF  5	//ADC reference is 5v
#define ZERO_X	1.22 //accleration of X-AXIS is 0g, the voltage of X-AXIS is 1.22v
#define ZERO_Y	1.22 //
#define ZERO_Z	1.25 //
#define SENSITIVITY 0.25//sensitivity of X/Y/Z axis is 0.25v/g

struct Status{				//Status hold timed status conditions
	byte Carrier;
        byte Value;
	unsigned long duration;
	unsigned long time_in;
};

//RF Device Variables//
const int RF_input = 9; //Must be a PWM pin - RF Receiver Digital Output
const int RF_output = 8;	//RF Transmitter Digital input
const int RF_trigger = 7;	//Normally GND, Power to trigger
const int X = A0;       //X input from ADXL335
const int Y = A1;       //Y input from ADXL335
const int Z = A2;       //Z input from ADXL335
const int item = A3;	//Analog input for item selection (currently selects one of three items)
const int spell = A4;   //Analog input for spell selection
IRrecv irrecv(RF_input); //Create an IRrecv object
//IR Device Variables//
const int IR_input = 2; //Must be a PWM pin - IR Receiver Data Output
decode_results decodedSignal; //Stores results from IR detector
IRsend irsend;
//Lights and Sound
const int RED = 4;	//Red LED for Damage
const int GRN = 5;	//Green LED for Heal
const int BLU = 6;	//Blue LED for Status condition
const int Buzzer = 11;	//Buzzer for sounds
//Control Variables//
char Stats[3] = {100, 5, 0}; //Health, Attack, Spell Damage
int items[3] = {0xA60A, 0xA00A, 0xA81E};//Magic Missile, Mass Heal, 30 MASSIVE
int spells[3] = {0xA60A, 0xA00A, 0xA81E);
Status current[3];

funcptr carriers[8] = {Normal, Timed, Buf, Clear, Massive, Special, Element, LD};//Function pointers for each carrier code 0x0-0x7
bool statcon = 0; //True if Player has any status conditions
bool Team = 0; //0 or 1

void setup(){
  //LEDs and Buzzer  
  pinMode(RED, OUTPUT);
  pinMode(GRN, OUTPUT);
  pinMode(BLU, OUTPUT);
  pinMode(Buzzer, OUTPUT);
  //Power Up sound and lights
  digitalWrite(RED, HIGH);
  delay(200);
  digitalWrite(GRN, HIGH);
  delay(200);
  digitalWrite(BLU, HIGH);
  delay(200);
  analogWrite(Buzzer, 50);
  delay(50);
  analogWrite(Buzzer, 0);
  delay(50);
  analogWrite(Buzzer, 50);
  delay(50);
  analogWrite(Buzzer,0);
  digitalWrite(RED, LOW);
  digitalWrite(GRN, LOW);
  digitalWrite(BLU, LOW);
  //RF Setup//
  vw_setup(2000);	// Bits per sec
  vw_set_ptt_inverted(true); // Required for DR3100
  vw_set_tx_pin(RF_input);
  vw_set_ptt_inverted(true); // Required for DR3100
  vw_setup(2000);	// Bits per sec
  vw_set_rx_pin(RF_output);
  vw_rx_start(); // Start the receiver PLL running
  pinMode(RF_trigger, INPUT);
  //IR Setup//
  irrecv.enableIRIn(); //Begin the receiving process. This will enable the timer interrupt which consumes a small amount of CPU every 50 Âµs.
  //Initilize Variables
  Status temp = {0, 0, 0};
  //Initilize Variables
  Status temp = {0, 0, 0};
  for(int i=0; i<3; i++){
	current[i] = temp;
  }
  //  println("Setup complete");
}

void getXYZ(int16_t *x,int16_t *y,int16_t *z){
  *x = analogRead(X_AXIS_PIN);
  *y= analogRead(Y_AXIS_PIN);
  *z = analogRead(Z_AXIS_PIN);
}

void getAcceleration(float *ax,float *ay,float *az){
  int x,y,z;
  float xvoltage,yvoltage,zvoltage;
  getXYZ(&x,&y,&z);
  xvoltage = (float)x*ADC_REF/ADC_AMPLITUDE;
  yvoltage = (float)y*ADC_REF/ADC_AMPLITUDE;
  zvoltage = (float)z*ADC_REF/ADC_AMPLITUDE;
}

signed int CalculateCast(){
  float acc[3];
  stats[2] = 0; 
  signed int spell_select;
  float spell_read, multiplier;
  spell_select = AnalogRead(spell);
  for(int i=0; i<10; i++){
    if(!RF_Trigger){
      break;
    }
    getAcceleration(&acc[0],&acc[1],&acc[2]);
    spell_read = AnalogRead(spell);
    multiplier = abs(spell_select-(float)spell_read)/spell_select
    spell_read = spell_read%3;
    stats[2] = (char)(damage+acc[spell_read]*multiplier);
    delay(100);
  }
  return spell_select;
}

void RF_fire(){
  int cast = spells(CalculateCast());
  cast = cast + stats[2];  
  vw_send((uint8_t *)cast, 2);//Send AoE, vw_send(data,#bytes)
  vw_wait_tx(); // Wait until the whole message is gone
}

void Get_Damage(){
  if(irrecv.decode(&decodedSignal)==true){ //If IR signal has been received...
    if(decodedSignal.rawlen==16){ //If IR signal is not 16 bits...
      byte data[3]; //data = {Header, Carrier, Value byte}
      parse(decodedSignal.value, data); //Parse code into Header, Team/Carrier, Value byte
      if(data[0]!=0xA && Team!=(data[1]>>7)){ //If Header is not equal to 0xA and code was not sent from Team member...
        carriers[data[1]](data[1],data[2]); //Call function corresponding to carrier
      }
    }
  }
  if (vw_have_message()){ //If RF signal has been received...
    uint8_t buf[VW_MAX_MESSAGE_LEN]; //Initialize buffer for message
    uint8_t buflen = VW_MAX_MESSAGE_LEN; //Initialize variable to store code length
     if (vw_get_message(buf, &buflen)){ //If the data is not corrupted
      if(buflen!=16){ //If the message length is not equal to 16
        byte data[3]; //data = {Header, Team/Carrier, Value byte}
        parse(*buf,data); //Parse code into Header, Carrier, Value byte
if(data[0]!=0xA && Team!=(data[1]>>7)){ //If Header is not equal to 0xA and code was not sent from Team member...
          carriers[data[1]](data[1],data[2]); //Call function corresponding to carrier
}
      }
    }
  }
}

void parse(long unsigned int signal, byte* data){
  //parse into header, carrier, damage
  data[0] = (signal>>12); //Header
  data[1] = (signal>>8)&(0x0F); //Team, Carrier
  data[2] = signal; //Level/Value
}
void Normal(byte carrier, byte value){
  digitalWrite(RED, 1);
  bool bit_sign = value>>7;
  if(bit_sign){
    digitalWrite(RED, 1);
  }
  else{
    digitalWrite(GRN, 1);
  }
  signed byte sign = 2*(bit_sign);	//0 or 2 -> sign of value will be (-1+sign) = -1 or 1
  signed byte val = value<<1; //Grab value of code (0-127)
  Stats[0] = Stats[0] + (-1+sign)*val; //Health = Health + (-1+sign)(Value) - Add or Subtract Health
  digitalWrite(RED, 0);
  if(bit_sign){
    digitalWrite(RED, 0);
  }
  else{
    digitalWrite(GRN, 0);
  }
}/*
void pushStat(Status stat){
  for(char i=0;i<3;i++){
    if(current[i].time_in==0){
      current[i] = stat;
    }
  }
}
void popStatus(char index){
  
}
*/
void Timed(byte carrier, byte value){
  digitalWrite(BLU, 1);
  Status stat;
  stat.time_in = micros();
  stat.duration = (value&0x7F);
  stat.Carrier = (value&0x80);
  //pushStat(stat);
  digitalWrite(BLU, 0);
}
void Buf(byte carrier, byte value){
  digitalWrite(BLU, 1);
  byte sign = 2*(value>>7);      //0 or 2 -> sign of value will be (-1+sign) = -1 or 1
  byte stat = value>>6;          //Grab Stat to be added to or subtracted from
  byte val = value<<2;           //Grab value of code (0-63)
  Stats[stat] = Stats[stat] + (-1+sign)*(val);//Stat = Stat + (-1+sign)(Value) - Add or Subtract from Stat
  digitalWrite(BLU, 0);
}
void Clear(byte carrier, byte value){
  digitalWrite(GRN, 1);
  Status temp;			//Create empty Status
  temp.time_in = 0;
  for(int i = 0; i<3; i++){
    current[i] = temp;		//Clear all status conditions
  }
  digitalWrite(GRN, 0);
}
void Timed_Effect(byte Carrier, byte Value){

}
void pushStat(Status stat){
  for(char i=0;i<3;i++){
    if(current[i].time_in==0){
      current[i] = stat;
      break;
    }
  }
}
void Massive(byte carrier, byte value){
  digitalWrite(RED, 1);
  Stats[0] = Stats[0] - value;	//Health = Health - Value
  digitalWrite(RED, 0);
}
void Special(byte carrier, byte value){
  
}
void Element(byte carrier, byte value){
  
}
void LD(byte carrier, byte value){

}
void popStatus(char index){
  Status temp;
  temp.time_in = 0x00;
  current[index] = temp; 
}
void StatusConditions(){
  char empty_status = 0;
  for(char i=0; i<3; i++){			//Go through all status conditions and accumulate results
    if(current[i].time_in==0){			//If time_in==0, no status stored
      empty_status++;
      continue;
    }
    Timed_Effect(current[i].Carrier, current[i].Value);	//Accumulate Effects
    if(current[i].duration<(micros()-current[i].time_in)){//if status has been in for specified duration, remove it
      popStatus(i);
      break;
    }
  }
  statcon = 0;
}
void loop(){
  unsigned long Status_timer = 0;
  //Currently, the RF and IR Trigger pins are continuously
  //polled, but later on they will be implemented with 
  //interrupts.
  if(digitalRead(RF_trigger)==HIGH){      //If RF trigger pulled...
    RF_fire();
  }
  if(digitalRead(IR_trigger)==HIGH){      //If IR trigger pulled...
    IR_fire();
  }
  
  Get_Damage();              //Detect if IR or RF signal was received
  
  Status_timer++;
  if(statcon && Status_timer==0x0001000){              //Accumulate damage from Status Conditions
    StatusConditions();
    Status_timer=0;
  }
}
