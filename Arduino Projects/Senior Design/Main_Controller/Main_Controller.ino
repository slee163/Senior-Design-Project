#include <Servo.h>

#define FORWARD  1
#define RIGHT    2
#define LEFT     3
#define HALT     0
#define REVERSE  4

#define NUM_SENSORS 4

#define CAM_LEFT    2
#define CAM_RIGHT   1
#define CAM_CENTER  3

const unsigned char TRIGGERS[] = {A5, A4, A3, A2};
const unsigned char ECHOS[] = {13, 12, 8, 10};

const unsigned char MOTOR_L[] = {11, 7};
const unsigned char MOTOR_R[] = {5, 4};

const unsigned char SPEEDS[] = {150, 150, 200, 255};

const unsigned char SPEED_SENSE_L = 3;
const unsigned char SPEED_SENSE_R = 2;

const unsigned char OVERRIDE = A0;

Servo CAM_SERVO;

unsigned int rpm;
volatile byte pulses;
unsigned long timeold;

unsigned int pulses_per_turn = 20;

enum auto_state {A_INACTIVE, A_STOPPED, A_MOVE_UP, A_TURNING} a_state;
enum manuel_state {M_INACTIVE, M_ACTIVE} m_state;
enum control_state {C_IDLE, C_AUTO, C_MANUAL} c_state;

char serial_data = 0;
unsigned char servo_pos = 90;

/*debug flag*/
bool debug = true;  //true for ascii, false for actual char in binary

bool debug_ultrasonic = false;
bool debug_navigation = false;
bool debug_state = false;
bool debug_motor = false;

void speed_sense_r_isr()
{
  pulses++;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  int i = 0;
  
  for(i = 0; i < NUM_SENSORS; i++) {
    pinMode(ECHOS[i], INPUT);
    pinMode(TRIGGERS[i], OUTPUT);
  }
  
  for(i = 0; i < 2; i++) {
    pinMode(MOTOR_R[i], OUTPUT);
    pinMode(MOTOR_L[i], OUTPUT);
  }
  
  pinMode(OVERRIDE, INPUT_PULLUP);
  
  pinMode(SPEED_SENSE_L, INPUT);
  pinMode(SPEED_SENSE_R, INPUT);
  
  //attachInterrupt(0, speed_sense_r_isr, FALLING);
  pulses = 0;
  rpm = 0;
  timeold = 0;
  
  CAM_SERVO.attach(9);

  a_state = A_INACTIVE;
  m_state = M_INACTIVE;
  c_state = C_MANUAL;
  
  if(debug_ultrasonic || debug_navigation || debug_state || debug_motor)
  debug = true;
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
  digitalWrite(TRIGGERS[i], LOW);   //ensures off
  delayMicroseconds(2);             //moment of delay
  digitalWrite(TRIGGERS[i], HIGH);  //set high to pulse
  delayMicroseconds(10);            //needs to be on at least 10 uS for effective pulse
  digitalWrite(TRIGGERS[i], LOW);   //turn off
  
  //Echo
  duration = pulseIn(ECHOS[i], HIGH, 5000);  //calculates how long its receiving the pulse
  if (duration == 0)
    return 100;
  else
    return uStoCM(duration);              //calculate result into cm
}

void set_motors_manual(unsigned char velocity){
  unsigned char direct = velocity & 0x07;
  unsigned char magnitude = (velocity & 0x18) >> 3;
  unsigned char l_speed = SPEEDS[magnitude];
  unsigned char r_speed = SPEEDS[magnitude];
  unsigned char l_direct = LOW;
  unsigned char r_direct = LOW;
  
  switch(direct) {
    case HALT:
      l_speed = 0;
      r_speed = 0;
      l_direct = LOW;
      r_direct = LOW;
      break;
    case REVERSE:
      l_direct = LOW;
      r_direct = LOW;
      break;
    case RIGHT:
      l_speed = 255 - l_speed;
      l_direct = HIGH;
      r_direct = LOW;  
      break;
    case LEFT:
      r_speed = 255 - r_speed;
      l_direct = LOW;
      r_direct = HIGH;
      break;
    case FORWARD:
      l_speed = 255 - l_speed;
      r_speed = 255 - r_speed;
      l_direct = HIGH;
      r_direct = HIGH;
    default:
      l_speed = 0;
      r_speed = 0;
      l_direct = LOW;
      r_direct = LOW;
      break;
  }
  if(!debug_motor) {
    analogWrite(MOTOR_L[0], l_speed);
    analogWrite(MOTOR_R[0], r_speed);
    digitalWrite(MOTOR_L[1], l_direct);
    digitalWrite(MOTOR_R[1], r_direct);
  }
  else {
    Serial.print("Speed L: "); Serial.print(l_speed);
    Serial.print("  Direction L: "); Serial.print(l_direct);
    Serial.print("  Speed R: "); Serial.print(r_speed);
    Serial.print("  Direction R: "); Serial.println(r_direct);
  }
    
}

void set_motors_auto(unsigned char direct){
  if(!debug_motor) {
    if(direct == FORWARD){
      digitalWrite(MOTOR_R[0],LOW);
      digitalWrite(MOTOR_R[1],HIGH);
      digitalWrite(MOTOR_L[0],LOW);
      digitalWrite(MOTOR_L[1],HIGH);
    }
    else if(direct == LEFT){
      digitalWrite(MOTOR_R[0],LOW);
      digitalWrite(MOTOR_R[1],HIGH);
      digitalWrite(MOTOR_L[0],HIGH);
      digitalWrite(MOTOR_L[1],LOW);
    }
    else if(direct == RIGHT){
      digitalWrite(MOTOR_R[0],HIGH);
      digitalWrite(MOTOR_R[1],LOW);
      digitalWrite(MOTOR_L[0],LOW);
      digitalWrite(MOTOR_L[1],HIGH);
    }
    else if(direct == REVERSE){
       digitalWrite(MOTOR_R[0],HIGH);
      digitalWrite(MOTOR_R[1],LOW);
      digitalWrite(MOTOR_L[0],HIGH);
      digitalWrite(MOTOR_L[1],LOW);
    }
    else{
      digitalWrite(MOTOR_R[0],LOW);
      digitalWrite(MOTOR_R[1],LOW);
      digitalWrite(MOTOR_L[0],LOW);
      digitalWrite(MOTOR_L[1],LOW);
    }
  } 
}

void set_servos(unsigned char input) {
  unsigned char servo_action = (input & 0x60) >> 5;
    switch (servo_action) {
      case CAM_RIGHT:
        if(servo_pos > 10)
          servo_pos -= 5;
        break;
        
      case CAM_LEFT:
        if(servo_pos < 170)
          servo_pos += 5;
        break;
        
      case CAM_CENTER:
        servo_pos = 90;
        break;
        
      default:
        break;
    }
        
      if(servo_action != 0)
        CAM_SERVO.write(servo_pos);
}

unsigned char auto_sm(unsigned char last_dir)
{
  static unsigned char halt_count = 0;
  unsigned char direct = last_dir;
  unsigned char cm_array[4];
  unsigned char i = 0;
  
  if(a_state != A_INACTIVE) {
    for(i = 0; i < NUM_SENSORS; i++)
      cm_array[i] = loop_pulse(i);
  }
  //Verify both frontal readings are correct (within +/- 2 of each other
  //if(constrain(up1, up2 - 2, up2 + 2) != up1)
    //return direct;

  switch(a_state){
    case A_INACTIVE:
      if (c_state == C_AUTO)
        a_state = A_STOPPED;
      else
        a_state = A_INACTIVE;
    case A_STOPPED:
      if( c_state != C_AUTO) {
        a_state = A_INACTIVE;
        direct = HALT;
      }
      else if(min(cm_array[0], cm_array[1]) > 20){
        direct = FORWARD;
        a_state = A_MOVE_UP;
      }
      else if(cm_array[2] >= cm_array[3] && cm_array[2] > 20){
        direct = RIGHT;
        a_state = A_TURNING;
      }
      else if(cm_array[3] > cm_array[2] && cm_array[3] > 20){
        direct = LEFT;
        a_state = A_TURNING;
      }
      else{
        if(halt_count < 10){
          direct = HALT;
          a_state = A_STOPPED;
          halt_count++;
        }
        else{
          direct = RIGHT;
          a_state = A_TURNING;
          halt_count = 0;
        }
      }
      break;
    
    case A_MOVE_UP:
      if( c_state != C_AUTO) {
        a_state = A_INACTIVE;
        direct = HALT;
      }
      else if(min(cm_array[0], cm_array[1]) > 20){
        direct = FORWARD;
        a_state = A_MOVE_UP;
      }
      else{
        direct = HALT;
        a_state = A_STOPPED;
      }
      break;
      
    case A_TURNING:
      if( c_state != C_AUTO) {
        a_state = A_INACTIVE;
        direct = HALT;
      }
      else if(min(cm_array[0], cm_array[1]) > 20){
        direct = HALT;
        a_state = A_STOPPED;
      }
      break;
      
    default:
       direct = HALT;
       a_state = A_STOPPED;
       break;
  }
  
  if (debug_navigation && (direct & 0x07) != (last_dir & 0x07)) {
    Serial.print("Direction: ");
    Serial.println(direct);
  }
  
  if (debug_ultrasonic) {
    for(i = 0; i < NUM_SENSORS; i++) {
      Serial.print("Sensor ");
      Serial.print(i+1);                //debug formatting, idc
      Serial.print(":[");
      Serial.print(cm_array[i]);              //use for debugging, display char
      Serial.print("]\t");
    }
    Serial.println("");
  }
    
  if(a_state != A_INACTIVE)
    set_motors_auto(direct & 0x07);
  
  return direct;
}

void control_sm() {
  static char serial_count = 0;
  
  switch(c_state) {
    case C_IDLE:
      c_state = C_AUTO;
      break;
    case C_AUTO:
      if(Serial.available() > 0) {
        serial_count = 0;
        serial_data = Serial.read();
        if((serial_data & 0x80) != 0 )
          c_state = C_MANUAL;
      }
      else {
        serial_count++;
        if(serial_count >= 20) {
          serial_data = 0x60;
          serial_count = 0;
        }
      }
      break;
      
    case C_MANUAL:
      if(Serial.available() > 0) {
        serial_count = 0;
        serial_data = Serial.read();
        if((serial_data & 0x80) != 0 )
          c_state = C_AUTO;
      }
      else {
        serial_count++;
        if(serial_count >= 20) {
          serial_data = 0x60;
          serial_count = 0;
        }
      }
      break;
      
    default:
      c_state = C_IDLE;
  }
}

void manual_sm() {
  switch(m_state) {
    case M_INACTIVE:
      if(c_state == C_MANUAL)
        m_state = M_ACTIVE;
      else if(c_state = C_AUTO) {
        if((serial_data & 0x80) == 0)
          set_servos(serial_data);
      }
      break;
      
    case M_ACTIVE:
      if(c_state != C_MANUAL) {
        set_motors_manual(0);
        m_state = M_INACTIVE;
      }
      else {
        if((serial_data & 0x80) == 0) {
          set_motors_auto(serial_data & 0x07);
          //set_motors_manual(serial_data);
          set_servos(serial_data);
        }
      }
      break;
      
    default:
      m_state = M_INACTIVE;
      break;
  }
}

void determine_rpm() {
  rpm = (60 * 1000 / pulses_per_turn) / (millis() - timeold) * pulses;
  timeold = millis();
  pulses = 0;
  
}

void loop() {
  static unsigned char last_direction;
  static long timer = 0;
  //put your main code here, to run repeatedly:
  /*
  if (millis() - timeold >= 1000) {
    detachInterrupt(0);
    determine_rpm();
    Serial.print("RPM = ");
    Serial.println(rpm,DEC);
    attachInterrupt(0,speed_sense_r_isr,FALLING);
  }*/
  if(digitalRead(OVERRIDE) == HIGH) {
    control_sm();
    manual_sm();
    last_direction = auto_sm(last_direction);
  
    if(debug_state) {
      Serial.print("Control State:"); Serial.print(c_state);
      Serial.print("  Manual State:"); Serial.print(m_state);
      Serial.print("  Auto State:"); Serial.println(a_state);
    }

  }
  else {
    serial_data = 0x00;
    set_motors_manual(0);
    set_servos(0x60);
    c_state = C_MANUAL;
  }
  
  delay(50);
}
