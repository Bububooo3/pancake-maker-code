#include <LiquidCrystal.h>

// Initialize LCD: (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

	// GLOBALS \\
const char consoleMessages[] = {\
	"Initializing",\
  	"Select number",\
  	"Bake?",\
  	"",\
};

bool baking = false;
bool prevConfirmState = HIGH;

	// CONSTANTS \\
// Buttons
const int confirmPin = 7;

// Miscellaneous
const int confirmLEDPin = 9;
const int conveyorMotorPin = 10;

	// FUNCTIONS \\
// Little useful functions

bool printMessage(String msg, int line = 0, int column = 0) {
  	lcd.setCursor(column, line);  
	lcd.print("                ");
	lcd.setCursor(column, line);
	lcd.print(msg);
  	return true;
}

bool clearLine(int lvl=-1){
  lvl = max(lvl, 0);
  lvl = min(lvl, 1);
    
  lcd.setCursor(0, lvl);
  lcd.print("                ");
  lcd.setCursor(0, lvl);
  return true;
}

bool scrollLine(){

}

// Keyframe functions
bool requestNumPancakes(){

}

void introductionProtocol(){

}

	// RUNTIME \\
// Run once on boot
void setup() {
  // Buttons
  pinMode(confirmPin, INPUT_PULLUP);
  pinMode(confirmLEDPin, OUTPUT);
  pinMode(conveyorMotorPin, OUTPUT);
  
  // LCD
  lcd.begin(16,2);
  printMessage("Bake how much?");
  printMessage("", 1);
}

// Run repeatedly
void loop() {  
  // Map 0-1028 :: 1–8
  String level = String(map(analogRead(A0), 0, 1023, 1, 8));

  // Display on LCD
  printMessage(((map(analogRead(A0), 0, 1023, 1, 8)>1) ? level + " Pancakes" : level + " Pancake"), 1);
  
  // Button handler
  int confirmState = (!(digitalRead(confirmPin)));
  
  baking = (prevConfirmState == HIGH && confirmState == LOW) ? !baking : baking;
  
  digitalWrite(confirmLEDPin, !baking);
  digitalWrite(conveyorMotorPin, !baking);
  
  prevConfirmState = confirmState;
  delay(50);
}
