#include <Encoder.h>
#include "A4988.h"

#include <LiquidCrystal.h>
// The menu wrapper library
#include <LiquidMenu.h>

// Pin mapping for the display:
const byte LCD_RS = 12;
const byte LCD_E = 11;
const byte LCD_D4 = 8;
const byte LCD_D5 = 7;
const byte LCD_D6 = 6;
const byte LCD_D7 = 5;
//LCD R/W pin to ground
LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

const byte LCD_CONTRAST = 10;
const byte LCD_BACKLIGHT = 9;

const byte HOME_SWITCH = 18;
bool homeSwitchState = false;

const byte ENC_SW = 14;
const byte ENC_A = 2;
const byte ENC_B = 3;
int encPos = 0;

const byte STEPPER_DIR = 19;
const byte STEPPER_STEP = 17;
const byte STEPPER_EN = 4;
const uint8_t STEPPER_STEPS_PER_REV = 200;
const uint8_t STEPPER_MICROSTEPS_PER_STEP = 16;
bool stepperIsEnabled = true;
int currentPos = 0;

const long MAX_POS_IN_STEPS = 600000;
const float SCREW_PITCH_IN_MM = 1.25;
const float GEAR_RATIO = 22.0/12.0;
const float STEPS_PER_MM = (STEPPER_STEPS_PER_REV * STEPPER_MICROSTEPS_PER_STEP * GEAR_RATIO) / SCREW_PITCH_IN_MM;


/*
 * Variables used for periodic execution of code. The first one is the period
 * in milliseconds and the second one is the last time the code executed.
 */
unsigned int period_check = 1000;
unsigned long lastMs_check = 0;

unsigned int period_nextScreen = 5000;
unsigned long lastMs_nextScreen = 0;

Encoder enc(ENC_A, ENC_B);

A4988 stepper(STEPPER_STEPS_PER_REV, STEPPER_DIR, STEPPER_STEP, STEPPER_EN);

/*
 * The LiquidMenu object combines the LiquidScreen objects to form the
 * menu. Here it is only instantiated and the screens are added later
 * using menu.add_screen(someScreen_object);. This object is used to
 * control the menu, for example: menu.next_screen(), menu.switch_focus()...
 */
LiquidMenu menu(lcd);

/*
 * LiquidLine objects represent a single line of text and/or variables
 * on the display. The first two parameters are the column and row from
 * which the line starts, the rest of the parameters are the text and/or
 * variables that will be printed on the display. They can be up to four.
 */
// Here the line is set to column 1, row 0 and will print the passed
// string and the passed variable.
LiquidLine welcome_line1(1, 0, "LiquidMenu ", LIQUIDMENU_VERSION);
// Here the column is 3, the row is 1 and the string is "Hello Menu".
LiquidLine welcome_line2(3, 1, "Hello Menu");

/*
 * LiquidScreen objects represent a single screen. A screen is made of
 * one or more LiquidLine objects. Up to four LiquidLine objects can
 * be inserted from here, but more can be added later in setup() using
 * welcome_screen.add_line(someLine_object);.
 */
// Here the LiquidLine objects are the two objects from above.
LiquidScreen welcome_screen(welcome_line1, welcome_line2);

// Here there is not only a text string but also a changing integer variable.
LiquidLine analogReading_line(0, 0, "Current pos: ", currentPos);
LiquidLine analogReading_line2(0, 1, "Steps left: ", stepperIsEnabled);
LiquidScreen secondary_screen(analogReading_line, analogReading_line2);


void setup() {
  Serial.begin(115200);
  Serial.println("START");

  pinMode(HOME_SWITCH, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);

  lcd.begin(16, 2);

  // This is the method used to add a screen object to the menu.
  // menu.add_screen(welcome_screen);
  menu.add_screen(secondary_screen);
  
  pinMode(LCD_CONTRAST, OUTPUT);
  pinMode(LCD_BACKLIGHT, OUTPUT);
  analogWrite(LCD_CONTRAST, 90);
  analogWrite(LCD_BACKLIGHT, 255);

  stepper.setSpeedProfile(BasicStepperDriver::Mode::LINEAR_SPEED);
  stepper.setEnableActiveState(LOW);
  stepper.enable();
  stepper.begin(200,16);
}

void loop() {
  static bool lastHomeSwitchState = false;
  static bool lastEncSwitchState = false;
  static int lastEncPos = 0;
  homeSwitchState = digitalRead(HOME_SWITCH);
  encPos = enc.read();
  if(!digitalRead(ENC_SW))
  {
    while(!digitalRead(ENC_SW)) {}
    stepper.enable();
    stepper.startMove(-MAX_POS_IN_STEPS);
    Serial.println("Homing start");
    while(stepper.getCurrentState() != BasicStepperDriver::State::STOPPED)
    {
      if(!digitalRead(HOME_SWITCH))
      {
        Serial.println(stepper.stop());
        currentPos = 0;
      }
      stepper.nextAction();
    }
    delay(2);
  }
  if (homeSwitchState != lastHomeSwitchState || lastEncPos != encPos | lastEncSwitchState) {
    lastHomeSwitchState = homeSwitchState;
    if(lastEncPos < encPos)
    {
      currentPos += 1;
      stepper.move(10*STEPS_PER_MM);
    }
    else
    {
      if((currentPos - 1) > 0)
      {
        currentPos -= 1;
        stepper.move(-10*STEPS_PER_MM);
      }
    }
    lastEncPos = encPos;
    menu.update();
  }

  // Periodic switching to the next screen.
  // if (millis() - lastMs_nextScreen > period_nextScreen) {
  //   lastMs_nextScreen = millis();
  //   menu.next_screen();
  // }
}