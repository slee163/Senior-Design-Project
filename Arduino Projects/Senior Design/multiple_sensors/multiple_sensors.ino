/*Ultra Sonic Sensor for multiple sensors and then transmit through USART

Circuit:
    +V at 5v
    GND at GND
    TRIG at PIN 7-
    ECHO at PIN 8+
    
    USART through PIN 0 and 1
*/

#define FORWARD  1
#define RIGHT    2
#define LEFT     3
#define HALT     4
/*constant values for PINs for Ultra Sonic*/
/*Change constant for number of sensors*/
const unsigned int num_sensors = 4;

/*Redefine Ports here*/
const unsigned int TRIG_0 = 7;    //hard code first, then devise a loop
const unsigned int TRIG_1 = 6;
const unsigned int TRIG_2 = 5;
const unsigned int TRIG_3 = 4;
const unsigned int ECHO_0 = 8;
const unsigned int ECHO_1 = 9;
const unsigned int ECHO_2 = 10;
const unsigned int ECHO_3 = 11;
unsigned char trig_arr[] = {TRIG_0, TRIG_1, TRIG_2, TRIG_3};
unsigned char echo_arr[] = {ECHO_0, ECHO_1, ECHO_2, ECHO_3};

const unsigned int MOTOR_R_F = 12;
const unsigned int MOTOR_R_B = 13;
const unsigned int MOTOR_L_F = 2;
const unsigned int MOTOR_L_B = 3;

/*debug flag*/
bool debug = true;  //true for ascii, false for actual char in binary

/*Code*/
void setup(){
  //initialize serial communication: (USART)?
  Serial.begin(9600);
  //initialize ports to output/input
  pinMode(TRIG_0, OUTPUT);  //hard code first, then devise a loop
  pinMode(TRIG_1, OUTPUT);
  pinMode(TRIG_2, OUTPUT);
  pinMode(TRIG_3, OUTPUT);
  pinMode(ECHO_0, INPUT);
  pinMode(ECHO_1, INPUT);
  pinMode(ECHO_2, INPUT);
  pinMode(ECHO_3, INPUT);
  
  pinMode(MOTOR_R_F, OUTPUT);
  pinMode(MOTOR_R_B, OUTPUT);
  pinMode(MOTOR_L_F, OUTPUT);
  pinMode(MOTOR_L_B, OUTPUT);
}

 
//CM distance calculation. FINE TUNE THIS FOR BETTER ACCURACY
unsigned char uStoCM(long uS){
  /*29us per cm*/
  unsigned long temp;
  unsigned char result;
  
  temp = uS / 29 / 2;
  if (temp >= 255){
    result = 255;
  }
  else{
    result = temp;
  }
  return result;
}

unsigned char loop_pulse(unsigned char i){  //i indicated which sensor
  long duration = 0;                //used to hold value of echo
  //Pulse
  digitalWrite(trig_arr[i], LOW);   //ensures off
  delayMicroseconds(2);             //moment of delay
  digitalWrite(trig_arr[i], HIGH);  //set high to pulse
  delayMicroseconds(10);            //needs to be on at least 10 uS for effective pulse
  digitalWrite(trig_arr[i], LOW);   //turn off
  
  //Echo
  duration = pulseIn(echo_arr[i], HIGH);  //calculates how long its receiving the pulse
  return uStoCM(duration);              //calculate result into cm
}


enum drone_state {STOPPED, MOVE_UP, TURNING} dstate;

unsigned char drone_sm(unsigned char up1, unsigned char up2, unsigned char left, unsigned char right, unsigned char last_dir)
{
  static unsigned char halt_count = 0;
  unsigned char direct = last_dir;  
  //Verify both frontal readings are correct (within +/- 2 of each other
  //if(constrain(up1, up2 - 2, up2 + 2) != up1)
    //return direct;

  switch(dstate){
    case STOPPED:
      if(min(up1,up2) > 20){
        direct = FORWARD;
        dstate = MOVE_UP;
      }
      else if(right >= left && right > 20){
        direct = RIGHT;
        dstate = TURNING;
      }
      else if(left > right && left > 20){
        direct = LEFT;
        dstate = TURNING;
      }
      else{
        if(halt_count < 10){
          direct = HALT;
          dstate = STOPPED;
          halt_count++;
        }
        else{
          direct = RIGHT;
          dstate = TURNING;
          halt_count = 0;
        }
      }
      break;
    
    case MOVE_UP:
      if(min(up1,up2) > 20){
        direct = FORWARD;
        dstate = MOVE_UP;
      }
      else{
        direct = HALT;
        dstate = STOPPED;
      }
      break;
    case TURNING:
      if(min(up1,up2) > 20){
        direct = HALT;
        dstate = STOPPED;
      }
      break;
    default:
       direct = HALT;
       dstate = STOPPED;
       break;
  }
  return direct; 
}

void set_motors(unsigned char direction){
  if(direction == FORWARD){
    digitalWrite(MOTOR_R_F,HIGH);
    digitalWrite(MOTOR_R_B,LOW);
    digitalWrite(MOTOR_L_F,HIGH);
    digitalWrite(MOTOR_L_B,LOW);
  }
  else if(direction == RIGHT){
    digitalWrite(MOTOR_R_F,LOW);
    digitalWrite(MOTOR_R_B,HIGH);
    digitalWrite(MOTOR_L_F,HIGH);
    digitalWrite(MOTOR_L_B,LOW);
  }
  else if(direction == LEFT){
    digitalWrite(MOTOR_R_F,HIGH);
    digitalWrite(MOTOR_R_B,LOW);
    digitalWrite(MOTOR_L_F,LOW);
    digitalWrite(MOTOR_L_B,HIGH);
  }
  else{
    digitalWrite(MOTOR_R_F,LOW);
    digitalWrite(MOTOR_R_B,LOW);
    digitalWrite(MOTOR_L_F,LOW);
    digitalWrite(MOTOR_L_B,LOW);
  }
}
  
//unsigned char fix_percision(unsigned char cm){
//      static unsigned char prev;
//      static unsigned char second_prev;
//      unsigned char current;
//}

unsigned char determine_direction(unsigned char up1, unsigned char up2, unsigned char left, unsigned char right){
  char direct = -1;
  
  if(min(up1,up2) > 20)
    direct = FORWARD;
  else if(right >= left && right > 20)
    direct = RIGHT;
  else if(left > right && left > 20)
    direct = LEFT;
  else
    direct = HALT;
 
  return direct;
}

   
void loop(){   //main loop
  unsigned char cm_array[4];
  unsigned char dir = HALT;
  static unsigned char last_direction = HALT;
  static drone_state dstate = STOPPED;

  for (unsigned i=0; i<num_sensors; i++){
    cm_array[i] = loop_pulse(i);
    /*
    if (!debug)
      Serial.write(cm);              //send binary to Raspberry Pi
    else{
      Serial.print("Sensor ");
      Serial.print(i+1);                //debug formatting, idc
      Serial.print(":[");
      Serial.print(cm);              //use for debugging, display char
      Serial.print("]\t");
    }*/
  }  
    dir = drone_sm(cm_array[0], cm_array[1], cm_array[2], cm_array[3], last_direction);
    
    if(dir != last_direction) {
      last_direction = dir;
      set_motors(last_direction);
     
      if(!debug)
        Serial.write(last_direction);
        
      else{
        Serial.print("Direction: ");
        Serial.print(last_direction);
        Serial.println();              //used for debugging, display char newline         
      }
    }
  delay(100);
}

  
  
  
  

