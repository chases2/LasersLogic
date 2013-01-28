/*
Lasers and Logic - Motion Tracker Turret, Simple Design
1/27/13
The Simple Motion Tracker Turret...
*/
#include VirtualWire.h

//PINS//
int Motor_A = 0;  		//Connected to pin 3A on the H-bridge
int Motor_B = 1;			//Connected to pin 4A on the H-bridge
int Motor_EN = 11;		//Connected to M_EN (motor enable) on the H-bridge

int Ranger1 = 3;			//Connected to Ranger 1 Data Line
int Ranger2 = 4;			//Connected to Ranger 2 Data Line
int Ranger3 = 5;			//Connected to Ranger 3 Data Line

int Hit = 2;				//Connected to HIT line on IR receiver - will vector
					// to ISR after changing from LOW->HIGH logic
//TX, D7 line connected to IR LED
//RX, D6 line connected to IR Receiver pin 3

int POT = A0;			//Connected to position potentiometer

//VARIABLES//
float position;			//The position of the mount is determined by the
					// input from a potentiometer which rotates with
					// the mount.
int Health = 10;

void setup(){
	Serial.begin(9600);
	
	pinMode(Motor_A, OUTPUT);
	digitalWrite(Motor_A, HIGH);
	pinMode(Motor_B, OUTPUT);
	digitalWrite(Motor_B,LOW);
	pinMode(Motor_EN, OUTPUT);
	digitalWrite(Motor_EN, LOW);
	
	//Configure external interrupt
	pinMode(2, INPUT);			//D2 will trigger the interrupt
	//enable pin change interrupt 2: 
	//Change on any enabled PCINT[23:16] will cause interrupt
	//PCICR = Pin change interrupt control register
	PCICR |= (1 << PCIE2);      
     
	//enable Interrupt on specific pin
	//PCMSK2 = Pin change Mask register
	PCMSK2 |= (1 << PCINT18); 	//enable PinChangeInterrupt on PCINT18
	//PCINT18 = pin nr.2 = PD2 = D2

	pinMode(7, INPUT);
	pinMode(6, INPUT);
	// Bits per sec
	vw_setup(2000);
	// pin 2 is used as the transmit data out into the TX
	// Link module, change this to suit your needs.
	vw_set_tx_pin(7);
}

ISR(PCINT2_vect){  //InterruptService-Routine, called by the Interruptvektor PCINT_vect
	while(pin==1);    //debouncing
	Health--;
	if(Health ==0)
		delay(60000);	
}

void ping(int ranger){
	// The PING is triggered by a HIGH pulse of 2 or more microseconds.
	// Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
	pinMode(ranger,OUTPUT);
	digitalWrite(ranger, LOW);
	delayMicroseconds(2);
	digitalWrite(ranger, HIGH);
	delayMicroseconds(5);
	digitalWrite(ranger, LOW);
	// The same pin is used to read the signal from the PING))): a HIGH
	// pulse whose duration is the time (in microseconds) from the sending
	// of the ping to the reception of its echo off of an object.  
	pinMode(ranger, INPUT);
	duration[ranger-3] = pulseIn(ranger, HIGH);
	// convert the time into a distance
	inches[ranger-3] = microsecondsToInches(duration[ranger-3]);
	cm[ranger-3] = microsecondsToCentimeters(duration[ranger-3]);

	Serial.print(inches[ranger-3]);
	Serial.print("in, ");
	Serial.print(cm[ranger-3]);
	Serial.print("cm");
	Serial.println();
}

long microsecondsToInches(long microseconds)
{
	// According to Parallax's datasheet for the PING))), there are
	// 73.746 microseconds per inch (i.e. sound travels at 1130 feet per
	// second).  This gives the distance travelled by the ping, outbound
	// and return, so we divide by 2 to get the distance of the obstacle.
	// See: http://www.parallax.com/dl/docs/prod/acc/28015-PING-v1.3.pdf
	return microseconds / 74 / 2;
}
 
long microsecondsToCentimeters(long microseconds)
{
	// The speed of sound is 340 m/s or 29 microseconds per centimeter.
	// The ping travels out and back, so to find the distance of the
	// object we take half of the distance travelled.
	return microseconds / 29 / 2;
}

void move(){
	if(position==0.0){
		digitalWrite(Motor_A, LOW);
		digitalWrite(Motor_B, HIGH);
	}
	if(position==5.0){
		digitalWrite(Motor_A, HIGH);
		digitalWrite(Motor_B, LOW);
	}
	digitalWrite(Motor_EN, HIGH);
	delay(1);
	digitalWrite(Motor_EN, LOW);
}

void get_position(){
	int sensorValue = analogRead(POT);
	position = sensorValue * (5.0/1023.0);
}

void fire(){
	byte msg[2];
	msg[0] = '50';
	msg[1] = '50';
	vw_send(msg, 2);
	// Wait for message to finish
	vw_wait_tx();
}

void track(){
	boolean MEN, M2A, M1A;
	char counter = 1;
	MEN = 1;
	MA = 1;
	MB = 1;
	while(!(M1A==0 && M2A==0 && MEN==0)){
		if(!(counter%6)){
			fire();
			counter = 1;
		}
		MEN = (0x00 && ping(1));			//Will be either 0 or 1
		MA = (0x00 && ping(0));
		MB = (0x00 && ping(2));
		
		digitalWrite(Motor_EN, !MEN);
		digitalWrite(Motor_A, MA);
		digitalWrite(Motor_B, MB);
		
		counter++;
	}
	digitalWrite(Motor_EN, LOW);
	digitalWrite(Motor_A, 1);
	digitalWrite(Motor_B, 0);
}

void loop(){
	get_position();
	move();
	track();
}
