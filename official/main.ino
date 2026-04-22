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
bool dispensingActive = false;                           // Init dispenser states
bool serviceMsg = false;                                 // Init service message state
bool griddleEnabled = false;                             // Init griddle state
bool griddleReady = true;                                // Init griddle state
bool griddleReadyPrev = true;                            // exactly what it sounds like
bool confirmStable = LOW;                                // more reliable input
bool cancelStable = LOW;                                 // more reliable input
bool tkA = false;                                        // is kill mode active
bool dispenserHoming = false;                            // is dispenser homing
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

unsigned long t_bake, t_kill, t_griddle, t_dispense, t_confirmDebounce, t_cancelDebounce;  // Init timers


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
    lastPrintLCD1 = "";
    lastPrintLCD2 = "";
    return true;
  }

  lvl = constrain(lvl, 0, 1);
  printMessage(placeholder, lvl);
  return true;
}


// Return whether the previous LED is the same as the current one
bool getLEDUnchanged(const uint32_t a[]) {
  return (
    prevLED[0] == a[0] && prevLED[1] == a[1] && prevLED[2] == a[2] && prevLED[3] == a[3] && prevLED[4] == a[4] && prevLED[5] == a[5] && prevLED[6] == a[6] && prevLED[7] == a[7]);
}

void setLEDChanged(const uint32_t a[]) {
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
  led.setPixelColor(6, g);
  led.setPixelColor(7, h);

  uint32_t temp[8] = { a, b, c, d, e, f, g, h };
  setLEDChanged(temp);

  led.show();
  return true;
}


// Set LED strip colors using a preset
bool floodColors(const uint32_t z[8]) {
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
unsigned long difftime(unsigned long t1, unsigned long t2) {
  return (t2 >= t1) ? (t2 - t1) : (0xFFFFFFFFUL - t1 + t2 + 1);  // ~49-day lifetime of accuracy
}

////////////////////////////////////////////////////////////////////////////////////////



/////////////////
// ABSTRACTION //
/////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// EXTRA INTERFACE CONSTANTS (for optimization, convenience & such)
const String spsMsg = "Pancakes";    // Word to show for >1 pancakes
const String amt = "Auto-Mode";     // Word to show for auto mode

const String bMsg = center("Baking");

const String eMsg1 = center("Action");      // Cancel display line 1
const String eMsg2 = center("Terminated");  // Cancel display line 2

const String rMsg1 = center("Pancakes");
const String rMsg2 = center("Ready");

void bakeComplete();  // Just in case

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
  bool canWriteLCD = !isActive(STATUS_READY) && !isActive(STATUS_CANCEL);

  if (griddleEnabled) {
    if (canWriteLCD) {
      int frame = static_cast<int>(elapsed / ANIMINC) % getArraySize(heatingAnim);
      printMessage(center(heatingAnim[frame]), 1);
    }
    if (!griddleReady && elapsed >= HEATUP) {
      griddleReady = true;
      if (isActive(STATUS_BAKE)) t_dispense = millis();
      return true;
    }
  } else {
    if (canWriteLCD) {
      int frame = static_cast<int>(elapsed / ANIMINC) % getArraySize(coolingAnim);
      printMessage(center(coolingAnim[frame]), 1);
    }
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


// For handling during cancel state or errors
void homeDispenser() {
  dispensingActive = false;
  dispenserHoming = true;
  dispenser.moveTo(DISPENSERCLOSE);
}

// Dispense pancakes fxn
bool dispense() {
  if (dispenserHoming) {
    if (dispenser.distanceToGo() == 0 && dispenser.currentPosition() == DISPENSERCLOSE) {
      dispenserHoming = false;
    }
    return false;
  }

  if (!dispensingActive) return false;

  if (dispenser.distanceToGo() == 0) {
    if (dispenser.currentPosition() == DISPENSEROPEN) {
      dispenser.moveTo(DISPENSERCLOSE);
    } else if (dispenser.currentPosition() == DISPENSERCLOSE) {
      dispensingActive = false;
      return true;
    }
  }
  return false;
}


// Handle the confirm/cancel button logic
void handleButtons() {
  bool rawConfirm = (digitalRead(CONFIRMPIN) == LOW);
  bool rawCancel = (digitalRead(CANCELPIN) == LOW);

  unsigned long now = millis();

  // CONFIRM DEBOUNCE
  if (rawConfirm != confirmStable) {
    if (now - t_confirmDebounce >= DEBOUNCE) {
      confirmStable = rawConfirm;
      t_confirmDebounce = now;  // Reset only after committing
    }
  }

  // CANCEL DEBOUNCE
  if (rawCancel != cancelStable) {
    if (now - t_cancelDebounce >= DEBOUNCE) {
      cancelStable = rawCancel;
      t_cancelDebounce = now;  // Reset only after committing
    }
  }

  bool confirmPressed = confirmStable;
  bool cancelPressed = cancelStable;

  // Confirm button pressed
  if (!confirmPressedPrev && confirmPressed) {
    switch (status) {
      case STATUS_CANCEL:
        setActive(STATUS_REQUEST);
        t_kill = 0;
        tkA = false;
        clearLine();
        printMessage(center("[please wait]"));
        plevel = -1;
        break;

      case STATUS_REQUEST:
        level = map(analogRead(A0), 0, 1023, 1, 17);
        if (!heatOnBoot) setGriddleEnabled(true);

        t_bake = now;
        t_dispense = now;
        griddleReadyPrev = false;
        dispensingActive = false;
        dispenserHoming = false;
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
    homeDispenser();
    clearLine();
    t_kill = millis();
    tkA = true;
    t_confirmDebounce = millis();
  }

  // Baking System
  conveyor.setSpeed(isActive(STATUS_BAKE) ? CONVEYORSTEP : 0);
  // <disabled> fan.setSpeed(isActive(STATUS_BAKE) ? MAXSTEP : 0);

  // Store state
  confirmPressedPrev = confirmPressed;
  cancelPressedPrev = cancelPressed;
}


void updateMotors() {
  conveyor.runSpeed();
  bool cycleComplete = dispense();
  dispenser.run();

  if (!griddleReady) {
    updateGriddle();
    griddleReadyPrev = false;
  } else {
    griddleReadyPrev = true;
  }

  serviceMsg = !griddleReady;

  if (!isActive(STATUS_BAKE)) return;

  if (cycleComplete) {
    dispensed++;
    t_dispense = millis();
    if (level < 17 && dispensed >= level) {
      bakeComplete();
      return;
    }
  }

  if (level == 17 && difftime(millis(), t_bake) >= AUTOTIMEOUT) bakeComplete();
}

// Update per heartbeat fxn
void heartbeat() {
  t_bake = isActive(STATUS_BAKE) ? t_bake : 0UL;
  t_dispense = isActive(STATUS_BAKE) ? t_dispense : 0UL;
  t_kill = (isActive(STATUS_CANCEL) && tkA) ? t_kill : 0UL;

  handleButtons();
  updateMotors();
}


// The screen for asking for pancakes
void requestScreen() {
  // Map 0-1023 :: 1–17
  level = map(analogRead(A0), 0, 1023, 1, 17);

  // Choose # Pancakes Screen
  if ((abs(level - plevel) >= 1) && isActive(STATUS_REQUEST)) {
    if (level < 17) {
      printMessage(center(String(level * 2) + " " + spsMsg), 1);
    } else {
      printMessage(center(amt), 1);
    }

    plevel = level;
  }
}


// Runs when baking is finished
void bakeComplete() {
  setActive(STATUS_READY);
  setGriddleEnabled(false);
}

// Display while baking
void bakeScreen() {
  if (!griddleReadyPrev) {
    printMessage(center("Heating..."));
    return;
  }

  printMessage(bMsg);
  printMessage((level < 17) ? center(String(level * 2) + " " + spsMsg) : center(amt), 1);

  if (!dispensingActive && !dispenserHoming && difftime(millis(), t_dispense) >= COOKTIME) {
    dispensingActive = true;
    dispenser.moveTo(DISPENSEROPEN);
  }
}

// Display for when the pancakes are ready
void readyScreen() {
  printMessage(rMsg1);
  printMessage(rMsg2, 1);
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
  digitalWrite(CONVEYORPIN_DIR, ((CONVEYORINVERT) ? HIGH : LOW));
  // <disabled> fan.setMaxSpeed(MAXSTEP);
  dispenser.setMaxSpeed(DISPENSERSPEED);
  dispenser.setAcceleration(DISPENSERACCEL);

  dispenser.moveTo(DISPENSERCLOSE);
  dispenserHoming = true;
  while (dispenser.distanceToGo() != 0) dispenser.run();
  dispenserHoming = false;

  // Initialize griddle bc of heatOnBoot
  griddleReadyPrev = griddleReady;
}


// Run repeatedly
void loop() {
  heartbeat();

  // Light handling
  switch (status) {
    case STATUS_READY:
      if (getLEDUnchanged(valid)) break;
      floodColors(valid);
      break;

    case STATUS_REQUEST:
      if (getLEDUnchanged(rqst)) break;
      floodColors(rqst);
      break;

    case STATUS_BAKE:
      if (getLEDUnchanged(warning)) break;
      floodColors(warning);
      break;

    case STATUS_CANCEL:
      if (getLEDUnchanged(danger)) break;
      floodColors(danger);
      break;

    case STATUS_EMPTY:
      if (getLEDUnchanged(dormant)) break;
      floodColors(dormant);
      break;
  }

  // Main logic handling
  if (serviceMsg && !isActive(STATUS_CANCEL) && !isActive(STATUS_BAKE) && !isActive(STATUS_READY)) return;

  switch (status) {
    case STATUS_CANCEL:
      printMessage(eMsg1);
      printMessage(eMsg2, 1);

      if (tkA && difftime(millis(), t_kill) >= KILLTIMEOUT) {
        tkA = false;
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
}
