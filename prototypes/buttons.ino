#include <Adafruit_NeoPixel.h>
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
// Time Lengths (milliseconds)
#define MSGWAIT				1.5e3
#define ANIMINC 			0.25e3

// Buttons
#define CONFIRMPIN 			7
#define CANCELPIN 			2


// Fixed-Size Arrays
const char intro[][16] = {\
                          "Button tester", \
                          "bUtton tester", \
                          "buTton tester", \
                          "butTon tester", \
                          "buttOn tester", \
                          "buttoN tester", \
                          "button Tester", \
                          "button tEster", \
                          "button teSter", \
                          "button tesTer", \
                          "button testEr", \
                          "button testeR", \
                          "button tester", \
                         };
///////////////////////////////////////////////////////////////////////////////////////////




///////////////////////////
// INITIALIZE COMPONENTS //
///////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
// Initialize LCD: (RS, E, D4, D5, D6, D7)

LiquidCrystal lcd(12, 11, 5, 4, 3, 13);
///////////////////////////////////////////////////////////////////////////////////////////



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

// The entire warm-up process for the machine w/ animations
void introductionProtocol() {
  for (int i = 0; (i < getArraySize(intro)); i++) {
    printMessage(center(intro[i]));
    delay(ANIMINC);
  }

  printMessage(center("[prototype]"), 1);
  delay(MSGWAIT);
  clearLine();

  printMessage(center("[Complete]"), 1);
  delay(MSGWAIT);
  clearLine();
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

      t_bake = millis();
      t_dispense = millis();

      dispensingActive = false;
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
    requesting = false;
    ready = false;
    cancelMode = true;
    clearLine();
    t_kill = millis();
  }

  // Store state
  confirmPressedPrev = confirmPressed;
  cancelPressedPrev = cancelPressed;
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

  // LCD
  lcd.begin(16, 2);

  introductionProtocol();
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

  heartbeat(); // Updates button states & time trackers
}