#include <LiquidCrystal.h>
#include <AccelStepper.h>

/*
  PINS

  LCD <Resistor>:
	VSS 	→ 	GND
	VDD		→ 	5V
	V0		→ 	GND

	RS		→		12
	E		→		11

	D4		→		5
	D5		→		4
	D6		→		3
	D7		→		13

	A		→		<220 Ω>		→		5V
	K		→		GND


  Knob Potentiometer:
	GND		→		GND
	SIG		→		A0
	VCC		→		5V


  Conveyor Driver <Conveyor Stepper Motor Pin>:
	EN		→		24
	STEP	→		22
	DIR		→		26

	GND.2	→		GND

	2B		→		<B->
	2A		→		<A->
	1A		→		<A+>
	1B		→		<B+>

	VDD		→		5V
	GND.1	→		GND


  Dispensing Driver <Dispensing Stepper Motor Pin>:
	EN		→		33
	STEP	→		35
	DIR		→		37
	GND.2	→		GND

	2B		→		<B->
	2A		→		<A->
	1A		→		<A+>
	1B		→		<B+>

	VDD		→		5V
	GND.1	→		GND


  Confirm Push Button <Resistor>:
	1:1 L	→		7
	1:2 R →		<1 KΩ>	→		GND


  Cancel Push Button <Resistor>:
	1:1 L	→		2
	1:2 R →		<1 KΩ>	→		GND
*/



/////////////
// GLOBALS //
/////////////
///////////////////////////////////////////////////////////////////////////////////////////
bool baking = false, requesting = false, cancelMode = false, ready = false;
bool confirmPressedPrev = LOW, cancelPressedPrev = LOW;
bool griddleEnabled = false, griddleReady = true, heatOnBoot = false, serviceMsg = false;
String lastPrintLCD1 = "";
String lastPrintLCD2 = "";
String placeholder = "                ";
int level = 1;
int plevel = 0;
int dispensed = 0;
unsigned long t_bake, t_kill, t_griddle, t_dispense;
bool tbA = false, tkA = false; // Timer [VarFirstChar] Active (gave up on finding a clever way to do it)

///////////////////////////////////////////////////////////////////////////////////////////


///////////////
// CONSTANTS //
///////////////
///////////////////////////////////////////////////////////////////////////////////////////
// Counts
#define LEDCOUNT 		8

// Time Lengths (milliseconds)
#define HEATUP					2e3
#define COOLDOWN				2e3
#define KILLTIMEOUT 		60e3
#define MSGWAIT					1.5e3
#define ANIMINC 				0.25e3
#define COOKTIME 				10e3

// Step pulse rate (steps/second)
#define	MAXSTEP					800.0
#define	CONVEYORSTEP		800.0
#define DISPENSERSPEED	800.0
#define DISPENSERACCEL	400.0

// Step positions (Usually 1.8°/step, 200 steps/revolution)	← <Research more based on specific motor>
#define DISPENSEROPEN		0
#define DISPENSERCLOSE	0

// Dispenser states
#define OPENING					true
#define CLOSING					false

// Buttons
#define CONFIRMPIN 			7
#define CANCELPIN 			2



// Fixed-Size Arrays
const char intro[][16] = {\
                          "pancake maker", \
                          "pAncake maker", \
                          "paNcake maker", \
                          "panCake maker", \
                          "pancAke maker", \
                          "pancaKe maker", \
                          "pancakE maker", \
                          "pancake Maker", \
                          "pancake mAker", \
                          "pancake maKer", \
                          "pancake makEr", \
                          "pancake makeR", \
                          " ", \
                         };

const char heatingAnim[][16] = {\
                                "heating", \
                                "heating.", \
                                "heating..", \
                                "heating...", \
                                "heating", \
                               };
///////////////////////////////////////////////////////////////////////////////////////////




///////////////////////////
// INITIALIZE COMPONENTS //
///////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
// Initialize LCD: (RS, E, D4, D5, D6, D7)
// Initialize stepper motors: (driver, STEP, DIR)

LiquidCrystal lcd(12, 11, 5, 4, 3, 13);
AccelStepper conveyor(AccelStepper::DRIVER, 22, 26);
AccelStepper dispenser(AccelStepper::DRIVER, 35, 37);

///////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////
// LIBRARY-SPECIFIC CONSTANTS //
////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
// Dispenser
bool dispenseState = OPENING, dispensingActive = false;
long dispenseTarget = DISPENSERCLOSE;

////////////////////////////////////////////////////////////////////////////////////////



/////////////////
/// FUNCTIONS ///
/////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// Macros
#define getArraySize(a) 	(sizeof(a) / sizeof((a)[0]))

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
const String bMsg = center("Baking");
const String spsMsg = "Pancakes"; // The word Pancakes
const String spMsg = spsMsg.substring(0, 7); // The word Pancake
const String cspsMsg = center(spsMsg); // So comprehensive 😍
const String cspMsg = center(spMsg);
const String eMsg1 = center("/   Action   \\"); // Cancel display line 1
const String eMsg2 = center("\\ Terminated /"); // Cancel display line 2
const String amt = "Auto-Mode";
const String rMsg1 = center("Pancakes");
const String rMsg2 = center("Ready");


// Display a message on LCD asking for # pancakes (and set requesting to true simultaneously)
bool requestNumPancakes() {
  printMessage(center("Bake how many?"));
  clearLine(1);
  return true;
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


// Just for simplicity
bool getGriddleEnabled() {
  return griddleEnabled;
}


// Update griddle per heartbeat w/o yielding program
// Returns true if griddle is fully cooled or heated
bool updateGriddle() {
  unsigned long elapsed = difftime(millis(), t_griddle);

  if (griddleEnabled) {
    // Heatup anim
    int frame = (elapsed / ANIMINC) % getArraySize(heatingAnim);
    printMessage(center(heatingAnim[frame]), 1);

    if (!griddleReady && elapsed >= HEATUP) {
      griddleReady = true;
      return true;
    }
  } else {
    // Cooldown anim
    int frame = (elapsed / ANIMINC) % getArraySize(coolingAnim);
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
  bool cancelPressed  = (digitalRead(CANCELPIN) == LOW);

  // Confirm button pressed
  if (!baking && !confirmPressedPrev && confirmPressed) {
    if (cancelMode) {
      cancelMode = false;
      requesting = true;
      baking = false;
      ready = false;
      t_kill = 0;
      clearLine();

    } else if (requesting) {
      requesting = false;
      baking = true;
      ready = false;

      if (!heatOnBoot) {
        setGriddleEnabled(true);
      }

      t_bake = millis();
      t_dispense = millis();

      dispensingActive = false;
      dispenseState = OPENING;
      dispenseTarget = DISPENSEROPEN;
      dispensed = 0;

      clearLine();
    } else if (ready) {
      requesting = true;
      baking = false;
    }
  }

  // Cancel button pressed
  if (cancelPressed && !cancelPressedPrev) {
    baking = false;
    setGriddleEnabled(false);
    requesting = false;
    ready = false;
    cancelMode = true;
    clearLine();
    t_kill = millis();
  }

  // Baking System
  conveyor.setSpeed((baking) ? CONVEYORSTEP : 0);
  // <disabled> fan.setSpeed((baking) ? MAXSTEP : 0);

  // Store state
  confirmPressedPrev = confirmPressed;
  cancelPressedPrev = cancelPressed;
}

void updateMotors() {
  conveyor.runSpeed(); // continuous
  // <disabled> fan.runSpeed();			// continuous
  dispenser.run();	 // position-based
  serviceMsg = (!(griddleReady) && updateGriddle());
}

// Update per heartbeat fxn
void heartbeat() {
  // Set them for next time
  tbA = baking;
  tkA = cancelMode;

  t_bake = (tbA) ? t_bake : 0.0L;
  t_dispense = (tbA) ? t_dispense : 0.0L;
  t_kill = (tkA) ? t_kill : 0.0L;

  handleButtons();
  updateMotors();

  /*
  	"I'll just use a debugger," I said with joys...
  	I was then shot 57 times

    Serial.println("======================");
    Serial.println("Timers");
    Serial.print("| B: "); Serial.println(t_bake);
    Serial.print("| T: "); Serial.println(t_kill);
    Serial.print("| C: "); Serial.println(millis());
    Serial.println("======================");
  	Serial.println("Buttons");
  	Serial.print("| Confirm: "); Serial.println(confirmPressed);
  	Serial.print("| Prev-Confirm: "); Serial.println(confirmPressedPrev);
  	Serial.print("| Baking: "); Serial.println(baking);
  	Serial.print("| Requesting: "); Serial.println(requesting);
  */

}


// The screen for asking for pancakes
void requestScreen() {
  dispensed = 0;

  // Map 0-1028 :: 1–17
  level = map(analogRead(A0), 0, 1023, 1, 17);

  // Choose # Pancakes Screen
  if (level != plevel && !baking) {
    if (level < 17) {
      printMessage(center((level > 1) ? (String(level) + " " + spsMsg) : (String(level) + " " + spMsg)), 1);
    } else {
      printMessage(center(amt), 1);
    }
    plevel = level;
  }
}


// Runs when baking is finished
void bakeComplete() {
  baking = false;
  dispenseState = CLOSING;
  dispenseTarget = DISPENSERCLOSE;
  setGriddleEnabled(false);
  requesting = false;
  ready = true;
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
  lcd.begin(16, 2);

  // LED
  led.begin();
  led.show();
  introductionProtocol();

  // Serial.begin(9600);

  // Stepper Motors
  conveyor.setMaxSpeed(CONVEYORSTEP);
  // <disabled> fan.setMaxSpeed(MAXSTEP);
  dispenser.setMaxSpeed(800);
  dispenser.setAcceleration(400);
}


// Run repeatedly
void loop() {
  // Main logic handling
  if (cancelMode) {
    if (!lastPrintLCD1.equals(eMsg1)) {
      printMessage(eMsg1);
      printMessage(eMsg2, 1);
    }

    if (difftime(millis(), t_kill) >= KILLTIMEOUT) {
      cancelMode = false;
      clearLine();
    }

  } else if (!serviceMsg) {
    if (!requesting && !baking) {
      requesting = requestNumPancakes();
    }
    if (requesting && !baking) {
      requestScreen();
    }
    if (baking) {
      bakeScreen();
    }
    if (ready) {
      readyScreen();
    }
  }

  // Light handling
  if (ready && !getLEDChanged(valid)) {
    floodColors(valid);
  } else if (requesting && !getLEDChanged(rqst)) {
    floodColors(rqst);
  } else if (baking && !getLEDChanged(warning)) {
    floodColors(warning);
  } else if (cancelMode && !getLEDChanged(danger)) {
    floodColors(danger);
  } else if (!getLEDChanged(dormant)) {
    floodColors(dormant);
  }

  heartbeat(); // Updates button states & time trackers

  // delay(150);
}
