// MicroStack version:1.1
// Microscope focus knob driver for Renzo
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <IRremote.h>

//int GND_PIN = 8;
int SHUTTER_PIN1 = 12;
int SHUTTER_PIN2 = 13;
int BTN_UP = 10;
int BTN_DN = 11;

int motorPin1 = 4;    // Blue   - 28BYJ48 pin 1
int motorPin2 = 5;    // Pink   - 28BYJ48 pin 2
int motorPin3 = 6;    // Yellow - 28BYJ48 pin 3
int motorPin4 = 7;    // Orange - 28BYJ48 pin 4

int motorSpeed = 2400;  //variable to set stepper speed

int count = 0;          // count of steps made
int countsperrev = 512; // number of steps per full revolution

//check stepping methods
//A full step sequence is 1A-1B-2A-2B, or the higher torque 1A1B - 2A1B - 2A2B - 1A2B.
//A half step sequence is 1A - 1A1B - 1B - 2A1B - 2A - 2A2B - 2B - etc
int lookup[8] = {B01000, B01100, B00100, B00110, B00010, B00011, B00001, B01001};//half step


//Stacker parameters
bool runnin = false;
bool mirrormode=false;//mirror Up first, then shoot
byte menu = 0; // 1-> steps, 2-> exposure, 3-> double shot (mirror up, then shoot)
//int tpulse = 500; //ms for nikon shutter pulse
int tpulse = 250;

int steps = 8; //512 steps for a full revolution
int exposuretime = 1; //in Sec
int stepcount = 0;
int shotcount = 0;

IRrecv irrecv(9);
decode_results results;

//LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);

char arr1[] = "MICROSTACK      "; //the string to print on the LCD
char arr2[] = "READY           "; //the string to print on the LCD
int tim = 500; //the value of delay time

void setup()
{
  //pinMode(GND_PIN, OUTPUT);
  //digitalWrite(GND_PIN,0);

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DN, INPUT_PULLUP);

  pinMode(SHUTTER_PIN1, OUTPUT);
  pinMode(SHUTTER_PIN2, OUTPUT);
  digitalWrite(SHUTTER_PIN1, 1);//by defaut ?
  digitalWrite(SHUTTER_PIN2, 0); //GND


  //declare the motor pins as outputs
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(motorPin3, OUTPUT);
  pinMode(motorPin4, OUTPUT);
  motorSpeed = 9600;  //slow
  //motorSpeed = 2400;

  lcd.init();
  lcd.backlight(); //open the backlight
  lcd.setCursor(0, 0);
  lcd.print(arr1);
  lcd.setCursor(0, 1);
  //delay(500);
  lcd.print("READY");

  Serial.begin(9600);
  irrecv.enableIRIn(); // Start the IR receiver
}


//////////////////////////////////////////////////////////////////////////////
//set pins to ULN2003 high in sequence from 1 to 4
//delay "motorSpeed" between each pin setting (to determine speed)
void anticlockwise()
{
  for (int i = 0; i < 8; i++)
  {
    setOutput(i);
    delayMicroseconds(motorSpeed);
  }
}

void clockwise()
{
  for (int i = 7; i >= 0; i--)
  {
    setOutput(i);
    delayMicroseconds(motorSpeed);
  }
}

void rest() {
  //rest the motor
  digitalWrite(motorPin1, 0);
  digitalWrite(motorPin2, 0);
  digitalWrite(motorPin3, 0);
  digitalWrite(motorPin4, 0);
}

//stepper driver
void setOutput(int out)
{
  digitalWrite(motorPin1, bitRead(lookup[out], 0));
  digitalWrite(motorPin2, bitRead(lookup[out], 1));
  digitalWrite(motorPin3, bitRead(lookup[out], 2));
  digitalWrite(motorPin4, bitRead(lookup[out], 3));
}


void printButton(int val)
{

  /*
     BTN 1 - 16753245
     BTN 2 - 16736925
     BTN 3 - 16769565
     BTN 4 - 16720605
     BTN 5 - 16712445
     BTN 6 - 16761405
     BTN 7 - 16769055
     BTN 8 - 16754775
     BTN 9 - 16748655
     BTN 0 - 16750695
     BTN * - 16738455
     BTN # - 16756815
     BTN UP  - 16718055
     BTN DN  - 16730805
     BTN LFT - 16716015
     BTN RGT - 16734885
     BTN OK  -  16726215
  */
  lcd.setCursor(0, 1);
  switch (val) {
    case 16753245: lcd.print("BTN #1"); 
      shoot();//take a pic
      break;
    case 16736925: lcd.print("BTN #2"); break;
    case 16769565: lcd.print("BTN #3"); break;
    case 16720605: lcd.print("BTN #4"); break;
    case 16712445: lcd.print("BTN #5"); break;
    case 16761405: lcd.print("BTN #6"); break;
    case 16769055: lcd.print("BTN #7"); break;
    case 16754775: lcd.print("BTN #8"); break;
    case 16748655: lcd.print("BTN #9"); break;
    case 16750695: lcd.print("BTN #0"); break;
    case 16738455: lcd.print("BTN * ");
      stopit();
      menu++;
      if (menu > 3)menu = 0;
      break;

    case 16756815:
      //lcd.print("BTN # ");
      stopit();
      break;

    case 16718055:
      //lcd.print("BTN UP");
      runnin = false;
      motorSpeed = 1200; //fast
      for (int i = 0; i < 256; i++) {
        clockwise();
      }
      rest();
      break;

    case 16730805:
      //lcd.print("BTN DN");
      runnin = false;
      motorSpeed = 1200; //fast
      for (int i = 0; i < 256; i++) {
        anticlockwise();
      }
      rest();
      break;

    case 16716015://decrement value
      //lcd.print("BTN LF");
      switch (menu) {
        case 1:
          if (steps > 1)steps--;
          break;
        case 2:
          if (exposuretime > 1)exposuretime--;
          break;
         case 3:
          mirrormode=!mirrormode;
          break;
      }
      updateStatus();
      break;

    case 16734885://increment value
      switch (menu) {
        case 1:
          steps++;
          break;
        case 2:
          exposuretime++;
          break;
         case 3:
          mirrormode=!mirrormode;
          break;
      }
      
      updateStatus();
      break;

    case 16726215:
      //lcd.print("BTN OK");
      if (menu > 0) {
        menu = 0;
        return;
      }
      startit();
      break;

    default:
      lcd.print("??? ??"); break;

  }
}


void shoot() {

  //mirror up
  if (mirrormode) {
    digitalWrite(SHUTTER_PIN1, 0);
    delay(tpulse);
    digitalWrite(SHUTTER_PIN1, 1);
    lcd.setCursor(0, 1);
    lcd.print("MIRROR UP    ");
    delay(1000);//wait one sec to end vibration
  }
  
  //shoot
  digitalWrite(SHUTTER_PIN1, 0);
  delay(tpulse);
  digitalWrite(SHUTTER_PIN1, 1);
  // Wait `exposuretime` seconds
  lcd.setCursor(0, 1);
  lcd.print("EXPOSURE      ");
  delay(exposuretime * 1000);
  shotcount++;
}

void updateStatus() {

  if (menu == 0) {
    //ok
  } else if (menu == 1) {
    menuSteps();
    return;
  } else if (menu == 2) {
    menuExpo();
    return;
  }else if (menu == 3) {
    menuMirror();
    return;
  }

  lcd.setCursor(0, 0);
  lcd.print("STP:");
  lcd.print(steps);
  lcd.print(" EXP:");
  lcd.print(exposuretime);
  lcd.print("SEC  ");//sec

  lcd.setCursor(0, 1);

  if (runnin) {
    lcd.print(shotcount);
    lcd.print("SHOT ");
    lcd.print(stepcount);
    lcd.print("STEPS  ");

  } else {
    lcd.print(" = STOPPED =    ");
  }
}

void menuSteps() {
  lcd.setCursor(0, 0);
  lcd.print("=NUMBR OF STEPS=");
  lcd.setCursor(0, 1);
  lcd.print(steps);
  lcd.print(" (");
  float rev = 512 / steps;
  lcd.print(rev);
  lcd.print("/REVO) ");
}

void menuExpo() {
  lcd.setCursor(0, 0);
  lcd.print("=EXPOSURE TIME= ");
  lcd.setCursor(0, 1);
  lcd.print(exposuretime);
  lcd.print(" SECOND(S)   ");
}

void menuMirror() {
  lcd.setCursor(0, 0);
  lcd.print("= MIRROR MODE =  ");
  lcd.setCursor(0, 1);
  if (mirrormode) {
    lcd.print("YES            ");  
  }else{
    lcd.print("NO             ");
  }
}

void loop()
{

  //Manual mode first
  int up = digitalRead(BTN_UP);
  int dn = digitalRead(BTN_DN);

  if (up == 0) {
    if (runnin)stopit();
    //lcd.setCursor(0, 0);
    //lcd.print(" == MANUAL ==  ");
    //lcd.setCursor(0, 1);
    //lcd.print("UP        ");
    motorSpeed = 2400; //fast
    clockwise();
    return;
  } else if (dn == 0) {
    if (runnin)stopit();
    //lcd.setCursor(0, 0);
    //lcd.print(" == MANUAL == ");
    //lcd.setCursor(0, 1);
    //lcd.print("DOWN        ");
    motorSpeed = 2400; //fast
    anticlockwise();
    return;
  }



  if (irrecv.decode(&results)) {
    if (results.value == 4294967295) {
      //repeat?
    } else {
      printButton(results.value);//handle buttons
    }
    irrecv.resume(); // Receive the next value
  }



  if (runnin) {

    updateStatus();
    shoot();
    updateStatus();

    // Move it
    for (int i = 0; i < steps; i++) {
      anticlockwise();
      stepcount++;
    }
    rest();//turn motor off
    updateStatus();

    if (stepcount > 512 * 10) {
      // Maximum safe work time
      stopit();
    }

  } else {
    rest();//turn off motor
    updateStatus();
    delay(200);
  }

}

void startit() {
  if (runnin)return;
  runnin = true;
  motorSpeed = 9600;//slow
  motorSpeed = 4800;//still slow
  lcd.setCursor(0, 1);
  lcd.print("START    ");
}

void stopit() {
  runnin = false;
  stepcount = 0;
  shotcount = 0;
  //updateStatus();
}
