#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal_I2C.h>
#include <AccelStepper.h>
#include <Wire.h>

#include "Animations.h"
#include "Pins.h"
#include "Motors.h"
#include "Times.h"
#include "States.h"



/////////////////
/// VARIABLES ///
/////////////////
///////////////////////////////////////////////////////////////////////////////////////////
// Booleans //
bool confirmPressedPrev = LOW, cancelPressedPrev = LOW;  // Init button states
bool dispenseState = OPENING, dispensingActive = false;  // Init dispenser states
bool serviceMsg = false;                                 // Init service message state
bool griddleEnabled = false;                             // Init griddle state
bool griddleReady = true;                                // Init griddle state
bool heatOnBoot = false;                                 // Config

// Strings //
String lastPrintLCD1 = "";
String lastPrintLCD2 = "";
String placeholder = "                ";

// State Machine //
Status status = STATUS_REQUEST;

// Numbers //
int level = 1;      // Init # pancakes to make
int plevel = 0;     // Init previous # pancakes to make
int dispensed = 0;  // Init # pancakes dispensed

unsigned long t_bake, t_kill, t_griddle, t_dispense;  // Init timers
long dispenseTarget = DISPENSERCLOSE;                 // Init dispenser target position


//////////////////
/// COMPONENTS ///
//////////////////
///////////////////////////////////////////////////////////////////////////////////////////
// Initialize LCD: (Address, Column, Rows)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Initialize LED Strip: (#LEDs, Pin, Color Mode + Signal)
Adafruit_NeoPixel led(8, LEDPIN, NEO_GRB + NEO_KHZ800);

// Initialize stepper motors: (driver, STEP, DIR)
AccelStepper conveyor(AccelStepper::DRIVER, CONVEYORPIN_STEP, CONVEYORPIN_DIR);
AccelStepper dispenser(AccelStepper::DRIVER, DISPENSERPIN_STEP, DISPENSERPIN_DIR);
/* AccelStepper fan(AccelStepper::DRIVER, 25, 23); */

///////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////
/// LIBRARY-DEPENDENT CONSTANTS ////
////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
// Colors
#define WHITE led.Color(255, 255, 255)
#define RED led.Color(255, 0, 0)
#define GREEN led.Color(0, 255, 0)
#define BLUE led.Color(0, 0, 255)
#define YELLOW led.Color(255, 255, 0)
#define CYAN led.Color(0, 255, 255)
#define MAGENTA led.Color(255, 0, 255)
#define ORANGE led.Color(255, 165, 0)
#define PURPLE led.Color(128, 0, 128)
#define OFF 0

// Presets
const uint32_t warning[8] = { YELLOW, ORANGE, YELLOW, ORANGE, YELLOW, ORANGE, YELLOW, ORANGE };
const uint32_t danger[8] = { RED, RED, RED, RED, RED, RED, RED, RED };
const uint32_t rqst[8] = { PURPLE, MAGENTA, PURPLE, MAGENTA, PURPLE, MAGENTA, PURPLE, MAGENTA };
const uint32_t dormant[8] = { BLUE, WHITE, BLUE, WHITE, BLUE, WHITE, BLUE, WHITE };
const uint32_t off[8] = { OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF };
const uint32_t valid[8] = { GREEN, GREEN, GREEN, GREEN, GREEN, GREEN, GREEN, GREEN };
uint32_t prevLED[8] = { OFF, OFF, OFF, OFF, OFF, OFF, OFF, OFF };

////////////////////////////////////////////////////////////////////////////////////////



/////////////////
/// FUNCTIONS ///
/////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// Macros
#define getArraySize(a) (sizeof(a) / sizeof((a)[0]))

// Center/truncate string (16 characters)
String center(String msg) {
  return (msg.length() <= 16) ? placeholder.substring(0, (16 - msg.length()) / 2) + msg : msg.substring(0, 16);
}


// Display string on LCD
bool printMessage(String msg, int line = 0) {
  if (line) {
    if (lastPrintLCD2 == msg) {
      return false;
    }
  } else {
    if (lastPrintLCD1 == msg) {
      return false;
    }
  }

  line = constrain(line, 0, 1);
  lcd.setCursor(0, line);
  lcd.print(placeholder);
  lcd.setCursor(0, line);
  lcd.print(msg);

  if (line) {
    lastPrintLCD2 = msg;
  } else {
    lastPrintLCD1 = msg;
  }

  return true;
}


// Clear LCD line or entire screen if no params
bool clearLine(int lvl = -1) {
  if (lvl < 0) {
    lcd.clear();
    return true;
  }
  lvl = constrain(lvl, 0, 1);
  printMessage(placeholder, lvl);
  return true;
}


// Return whether the previous LED is the same as the current one
bool getLEDChanged(uint32_t a[]) {
  return (
    prevLED[0] == a[0] && prevLED[1] == a[1] && prevLED[2] == a[2] && prevLED[3] == a[3] && prevLED[4] == a[4] && prevLED[5] == a[5]);
}

void setLEDChanged(uint32_t a[]) {
  prevLED[0] = a[0];
  prevLED[1] = a[1];
  prevLED[2] = a[2];
  prevLED[3] = a[3];
  prevLED[4] = a[4];
  prevLED[5] = a[5];
  prevLED[6] = a[6];
  prevLED[7] = a[7];
}


// Set LED strip colors manually
bool setColors(int a = OFF, int b = OFF, int c = OFF, int d = OFF, int e = OFF, int f = OFF, int g = OFF, int h = OFF) {
  led.setPixelColor(0, a);
  led.setPixelColor(1, b);
  led.setPixelColor(2, c);
  led.setPixelColor(3, d);
  led.setPixelColor(4, e);
  led.setPixelColor(5, f);
  led.setPixelColor(6, e);
  led.setPixelColor(7, f);

  uint32_t temp[8] = { a, b, c, d, e, f, g, h };
  setLEDChanged(temp);

  led.show();
  return true;
}


// Set LED strip colors using a preset
bool floodColors(const uint32_t z[8]) {
  if (getArraySize(z) < 6) {
    return false;
  }
  setColors(z[0], z[1], z[2], z[3], z[4], z[5], z[6], z[7]);
  return true;
}


// Disable LED strip lights efficiently
bool disableLED() {
  led.setPixelColor(0, OFF);
  led.setPixelColor(1, OFF);
  led.setPixelColor(2, OFF);
  led.setPixelColor(3, OFF);
  led.setPixelColor(4, OFF);
  led.setPixelColor(5, OFF);
  led.setPixelColor(6, OFF);
  led.setPixelColor(7, OFF);
  led.show();
  return true;
}


// Set fan motor pulse rate
void setFanPower(float i) {
  // <disabled> fan.setSpeed(MAXSTEP * constrain(i, 0, 1));
}


// Get the net difference between these two times in milliseconds
unsigned long difftime(long t1, long t2) {
  return abs(t2 - t1);
}

////////////////////////////////////////////////////////////////////////////////////////



/////////////////
// ABSTRACTION //
/////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// EXTRA INTERFACE CONSTANTS (for optimization, convenience & such)
const String spMsg = "Pancake";     // Word to show for a single pancake
const String spsMsg = spMsg + "s";  // Word to show for >1 pancakes
const String amt = "Auto-Mode";     // Word to show for auto mode

const String bMsg = center("Baking");

const String eMsg1 = center("Action");      // Cancel display line 1
const String eMsg2 = center("Terminated");  // Cancel display line 2

const String rMsg1 = center("Pancakes");
const String rMsg2 = center("Ready");

// QoL method
bool isActive(Status check) {
  return (status == check);
}

void setActive(Status check) {
  status = check;
}

// Display a message on LCD asking for # pancakes (and set requesting to true simultaneously)
void requestNumPancakes() {
  setActive(STATUS_REQUEST);
  printMessage(center("Bake how many?"));
  clearLine(1);
}


// Set whether the griddle should be heating up or cooling down or neither
void setGriddleEnabled(bool enabled) {
  if (griddleEnabled == enabled) {
    return;
  }

  digitalWrite(GRIDPIN, enabled);
  griddleEnabled = enabled;
  griddleReady = false;
  t_griddle = millis();
}


// Update griddle per heartbeat w/o yielding program
// Returns true if griddle is fully cooled or heated
bool updateGriddle() {
  unsigned long elapsed = difftime(millis(), t_griddle);

  if (griddleEnabled) {
    // Heatup anim
    int frame = static_cast<int>(elapsed / ANIMINC) % getArraySize(heatingAnim);
    printMessage(center(heatingAnim[frame]), 1);

    if (!griddleReady && elapsed >= HEATUP) {
      griddleReady = true;
      return true;
    }
  } else {
    // Cooldown anim
    int frame = static_cast<int>(elapsed / ANIMINC) % getArraySize(coolingAnim);
    printMessage(center(coolingAnim[frame]), 1);

    if (!griddleReady && elapsed >= COOLDOWN) {
      griddleReady = true;
      return true;
    }
  }

  return false;
}

// The entire warm-up process for the machine w/ animations
void introductionProtocol() {
  for (int i = 0; (i < getArraySize(intro)); i++) {
    printMessage(center(intro[i]));
    delay(ANIMINC);
  }

  printMessage(center("By Jesuit HS"), 1);
  delay(MSGWAIT);
  clearLine();

  setFanPower(1.0f);

  if (heatOnBoot) {
    setGriddleEnabled(true);
  }

  printMessage(center("[Complete]"), 1);
  delay(MSGWAIT);
  clearLine();
}


// Dispense pancakes fxn
void dispense() {
  if (!dispensingActive) {
    dispensingActive = true;
    dispenser.moveTo(DISPENSEROPEN);
  }

  dispenser.run();

  if (dispenser.distanceToGo() == 0 && dispenser.currentPosition() == DISPENSEROPEN) {
    dispenser.moveTo(DISPENSERCLOSE);
  }

  if (dispenser.distanceToGo() == 0 && dispenser.currentPosition() == DISPENSERCLOSE) {
    dispensingActive = false;
  }
}


// Handle the confirm/cancel button logic
void handleButtons() {
  bool confirmPressed = (digitalRead(CONFIRMPIN) == LOW);
  bool cancelPressed = (digitalRead(CANCELPIN) == LOW);

  // Confirm button pressed
  if (!confirmPressedPrev && confirmPressed) {
    switch (status) {
      case STATUS_CANCEL:
        setActive(STATUS_REQUEST);
        t_kill = 0;
        clearLine();
        break;

      case STATUS_REQUEST:
        level = map(analogRead(A0), 0, 1023, 1, 17);
        if (!heatOnBoot) setGriddleEnabled(true);

        t_bake = millis();
        t_dispense = millis();

        dispensingActive = false;
        dispenseState = OPENING;
        dispenseTarget = DISPENSEROPEN;
        dispensed = 0;

        clearLine();
        setActive(STATUS_BAKE);
        break;

      case STATUS_READY:
        setActive(STATUS_REQUEST);
        break;
    }
  }

  // Cancel button pressed
  if (cancelPressed && !cancelPressedPrev) {
    setActive(STATUS_CANCEL);
    setGriddleEnabled(false);
    clearLine();
    t_kill = millis();
  }

  // Baking System
  conveyor.setSpeed(isActive(STATUS_BAKE) ? CONVEYORSTEP : 0);
  // <disabled> fan.setSpeed(isActive(STATUS_BAKE) ? MAXSTEP : 0);

  // Store state
  confirmPressedPrev = confirmPressed;
  cancelPressedPrev = cancelPressed;
}

void updateMotors() {
  conveyor.runSpeed();  // continuous
  // <disabled> fan.runSpeed();			// continuous
  dispenser.run();  // position-based
  serviceMsg = (!(griddleReady) && updateGriddle());
}

// Update per heartbeat fxn
void heartbeat() {
  t_bake = isActive(STATUS_BAKE) ? t_bake : 0.0L;
  t_dispense = isActive(STATUS_BAKE) ? t_dispense : 0.0L;
  t_kill = isActive(STATUS_CANCEL) ? t_kill : 0.0L;

  handleButtons();
  updateMotors();
}


// The screen for asking for pancakes
void requestScreen() {
  // Map 0-1028 :: 1–17
  level = map(analogRead(A0), 0, 1023, 1, 17);

  // Choose # Pancakes Screen
  if ((abs(level - plevel) >= 1) && isActive(STATUS_REQUEST)) {
    if (level < 17) {
      printMessage(center((level > 1) ? (String(level) + " " + spsMsg) : (String(level) + " " + spMsg)), 1);
    } else {
      printMessage(center(amt), 1);
    }
    plevel = level;  // TODO Implement Auto Mode :(
  }
}


// Runs when baking is finished
void bakeComplete() {
  setActive(STATUS_READY);
  dispenseState = CLOSING;
  dispenseTarget = DISPENSERCLOSE;
  setGriddleEnabled(false);
}

// The screen that shows when bancakes are paking
void bakeScreen() {
  if (!lastPrintLCD1.equals(bMsg)) {
    printMessage(bMsg);
    printMessage((level < 17) ? center(((level > 1) ? (String(level) + " " + spsMsg) : (String(level) + " " + spMsg))) : spsMsg, 1);

  } else {
    if (griddleReady) {
      if (difftime(millis(), t_dispense) >= COOKTIME && !dispensingActive) {
        dispense();
        t_dispense = millis();
        dispensed++;
      }
    }

    auto temporaryPrint = (level < 17) ? center(((level > 1) ? (String(level) + " " + spsMsg) : (String(level) + " " + spMsg))) : spsMsg;
    if (!lastPrintLCD2.equals(temporaryPrint)) {
      printMessage(temporaryPrint, 1);
    }

    if (dispensed >= level) {
      bakeComplete();
    }
  }
}


// Display for when the pancakes are ready
void readyScreen() {
  if (!lastPrintLCD1.equals(rMsg1)) {
    printMessage(rMsg1);
  }

  if (!lastPrintLCD2.equals(rMsg2)) {
    printMessage(rMsg2, 1);
  }
}

////////////////////////////////////////////////////////////////////////////////////////



/////////////
// RUNTIME //
/////////////
////////////////////////////////////////////////////////////////////////////////////////
// Run once on boot
void setup() {
  // Pin Initialization
  pinMode(CONFIRMPIN, INPUT_PULLUP);
  pinMode(CANCELPIN, INPUT_PULLUP);
  pinMode(LEDPIN, OUTPUT);
  pinMode(GRIDPIN, OUTPUT);

  // LCD
  lcd.init();
  lcd.backlight();

  // LED
  led.begin();
  led.show();
  introductionProtocol();

  // Serial.begin(9600);

  // Stepper Motors
  conveyor.setMaxSpeed(CONVEYORSTEP);
  conveyor.setAcceleration(400);
  // <disabled> fan.setMaxSpeed(MAXSTEP);
  dispenser.setMaxSpeed(800);
  dispenser.setAcceleration(400);
}


// Run repeatedly
void loop() {
  // Main logic handling
  if (serviceMsg && !isActive(STATUS_CANCEL) && !isActive(STATUS_BAKE) && !isActive(STATUS_READY)) return;

  switch (status) {
    case STATUS_CANCEL:
      if (!lastPrintLCD1.equals(eMsg1)) {
        printMessage(eMsg1);
        printMessage(eMsg2, 1);
      }

      if (difftime(millis(), t_kill) >= KILLTIMEOUT) {
        clearLine();
        requestNumPancakes();
      }
      break;

    case STATUS_REQUEST:
      requestScreen();
      break;

    case STATUS_BAKE:
      bakeScreen();
      break;

    case STATUS_READY:
      readyScreen();
      break;

    case STATUS_EMPTY:
      requestNumPancakes();
      break;

    default:
      setActive(STATUS_EMPTY);
      break;
  }

  // Light handling
  switch (status) {
    case STATUS_READY:
      if (getLEDChanged(valid)) break;
      floodColors(valid);
      break;

    case STATUS_REQUEST:
      if (getLEDChanged(rqst)) break;
      floodColors(rqst);
      break;

    case STATUS_BAKE:
      if (getLEDChanged(warning)) break;
      floodColors(warning);
      break;

    case STATUS_CANCEL:
      if (getLEDChanged(danger)) break;
      floodColors(danger);
      break;

    case STATUS_EMPTY:
      if (getLEDChanged(dormant)) break;
      floodColors(dormant);
      break;
  }

  heartbeat();  // Updates button states & time trackers
}
