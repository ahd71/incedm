// Interactive numerical controlled egg-decorating machine
// (c) Hellstrand 2014

#include <Servo.h> 
#include <Stepper.h>

// mechanical properties
#define servo_up_angle 65
#define servo_down_angle 110
#define rotation_backlash_compensation_steps 12
#define stepsPerRevolution 2060

// object initialization
Stepper stepperRotationAxis(stepsPerRevolution, 8, 10, 9, 11);            
Stepper stepperXaxis(stepsPerRevolution, 2, 4, 3, 5);            
Servo myservo;

// jog and 
int steps_jog_rotate = 36; 
int steps_jog_arm = 10;
int multiplierPeriodsPerRev = 4;  // used to define scale
int multiplierPatternWidth = 10;  // used to define scale
#define default_param_b 5
#define default_param_c 2

// variables
int current_pen_pos = servo_up_angle;
String inputString = "";         // a string to hold incoming serial data
boolean stringComplete = false;  // whether the string is complete (ended by any new line charactar)
boolean current_direction_is_forward = true;


void setup() {

  stepperRotationAxis.setSpeed(8);
  stepperXaxis.setSpeed(3);
  myservo.attach(6);

  pen_up();

  Serial.begin(115200);

  Serial.println("Interactive numerical controlled egg decorating machine - (c) Hellstrand 2014");
  Serial.println("*****************************************************************************");
  Serial.println();
  Serial.println("Direct Control (no enter/carriage return required, but accepted)");
  Serial.println("-----------------------------------------------------------------------------");
  Serial.println();
  Serial.println(" '4' : left");
  Serial.println(" '6' : right");
  Serial.println(" '8' : forward rotation");
  Serial.println(" '2' : backward rotation");
  Serial.println(" '9' : pen up");
  Serial.println(" '3' : pen down");
  Serial.println(" '*' : 360 degrees turn without drawing");
  Serial.println(" '-' : clear buffer (to abondon unsent pattern commands)");
  Serial.println();
  Serial.println("Pattern mode");
  Serial.println("-----------------------------------------------------------------------------");
  Serial.println();
  Serial.println("Syntax: '/abc' - where abc is three numerical one digit values");
  Serial.println();
  Serial.println("    param a: pattern");
  Serial.println("     '1' : Line (does not require parameter b and c)");
  Serial.println("     '2' : Square");
  Serial.println("     '3' : Pulse");
  Serial.println("     '4' : Saw");
  Serial.println("     '5' : Triangle");
  Serial.println("     '6' : Cosinus");
  Serial.println();
  Serial.println("    param b: frequency (optional, default value is 5)");
  Serial.println("     '1'..'9' : Periods per quarter of a revolution");
  Serial.println();
  Serial.println("    param c: width (optional, default value is 2)");
  Serial.println("     '1'..'9' : where 1 is narrow and 9 is wide");
  Serial.println();
  Serial.println("-----------------------------------------------------------------------------");

  inputString.reserve(200);
}


void pen_down() {
  int pos;
  for(pos = current_pen_pos; pos < servo_down_angle; pos += 1)
  { 
    myservo.write(pos);
    delay(10);
  } 
  current_pen_pos = pos;
}


void pen_up() {
  int pos;
  for(pos = current_pen_pos; pos>=servo_up_angle; pos-=1)
  {                                
    myservo.write(pos);          
    delay(10);                   
  } 
  current_pen_pos = pos;
}


void move_arm(long steps) {
  stepperXaxis.step(steps);
}


void rotate_axis(long steps) {
  boolean  new_direction_is_forward = steps>0;
  if (new_direction_is_forward & !current_direction_is_forward) { 
    steps += rotation_backlash_compensation_steps;
  }
  if (!new_direction_is_forward & current_direction_is_forward) { 
    steps -= rotation_backlash_compensation_steps;
  }
  stepperRotationAxis.step(-steps);
  current_direction_is_forward = new_direction_is_forward; 
}


void draw_pattern(int pattern_id, int number_of_periods, int pattern_width ) {

  int direction;
  int xpos_wanted = 0;
  int xpos_current = 0;
  int deltasteps = 0;

  const float pi = 3.141592;

  switch(pattern_id) {
  case 1: // line
    Serial.print("Line");
    break;
  case 2: // square
    Serial.print("Square");
    break;
  case 3: // pulse
    Serial.print("Pulse");
    break;
  case 4: // saw
    Serial.print("Saw");
    break;
  case 5: // triangle
    Serial.print("Triangle");
    break;
  case 6: // cosinus
    Serial.print("Cosinus");
    break;
  default:
    Serial.print("Default (Line)");
  }

  Serial.print(" (");
  Serial.print(number_of_periods);
  Serial.print(" periods of width ");
  Serial.print(pattern_width);
  Serial.println(")");

  pen_down();

  int steps_per_period = stepsPerRevolution/number_of_periods;
  for (int i=0; i<number_of_periods; i++) {

    for (int j=0; j<steps_per_period; j++) {

      switch(pattern_id) {
      case 1: // line
        xpos_wanted = 0;
        break;
      case 2: // square
        if((float(j)/(float)steps_per_period)>0.5) { 
          xpos_wanted = pattern_width; 
        } 
        else { 
          xpos_wanted = 0; 
        }
        break;
      case 3: // pulse
        if((float(j)/(float)steps_per_period)>0.8) { 
          xpos_wanted = pattern_width; 
        } 
        else { 
          xpos_wanted = 0; 
        }
        break;
      case 4: // saw
        xpos_wanted = float(j)/(float)steps_per_period*(float)pattern_width;
        break;
      case 5: // triangle
        xpos_wanted = (-abs((float(j)/(float)steps_per_period*(float)2*pi)-pi)+pi)/pi*(float)pattern_width; 
        break;
      case 6: // sinus
        xpos_wanted = ((cos((float(j)/(float)steps_per_period*(float)2*pi)+pi))+(float)1)*(float)pattern_width/(float)2;
        break;
      default:
        xpos_wanted = 0;
      }

      deltasteps = xpos_wanted-xpos_current;
      move_arm(deltasteps);
      xpos_current += deltasteps;

      rotate_axis(1);

    }
  }

  move_arm(-xpos_current); // go back to start pos if required

  pen_up(); 

}


void loop() {

  if (stringComplete) {

    Serial.println(inputString); 

    char p0=inputString.charAt(0);
    char p1=inputString.charAt(1)-'0';
    char p2=inputString.charAt(2)-'0';
    char p3=inputString.charAt(3)-'0';

    if (p2<1 or p2>9) {
      p2 = default_param_b;
    }
    if (p3<1 or p3>9) {
      p3 = default_param_c;
    }

    if (p0=='/' and inputString.length()>2) { 
      draw_pattern(p1, p2*multiplierPeriodsPerRev, p3*multiplierPatternWidth); 
    }

    // clear the string:
    inputString = "";
    stringComplete = false;
  }  

}

void serialEvent() {

  while (Serial.available()) {

    char inChar = (char)Serial.read(); 

    if (inputString=="") {  // empty string, could be a "direct mode command"

      switch (inChar) {

      case '9': 
        pen_up();
        break;

      case '3': 
        pen_down();
        break;

      case '4': 
        move_arm(-steps_jog_arm);
        break;

      case '6': 
        move_arm(steps_jog_arm);
        break;

      case '2': 
        rotate_axis(-steps_jog_rotate);
        break;

      case '8': 
        rotate_axis(steps_jog_rotate);
        break;

      case '*': 
        rotate_axis(stepsPerRevolution);
        break;
      default:
        inputString += inChar; // unhandled keypress when string is empty, add to string
      }
    } 

    else { // building message and appending to string

      inputString += inChar; // additional characters to be added to string that was already started
    }

    if (inChar == '-') { // clear buffer
      inputString = "";
    } 

    if (inChar == '\n' || inChar == '\r') { // string completed, send for execution
      stringComplete = true;
    } 
  }
}



