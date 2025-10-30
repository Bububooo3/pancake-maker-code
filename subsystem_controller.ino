#include <LiquidCrystal.h>

// Initialize LCD: (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

	// GLOBALS \\


	// CONSTANTS \\


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

// Keyframe functions
bool requestNumPancakes(){

}

void introductionProtocol(){

}

	// RUNTIME \\
// Run once on boot
void setup() {
  lcd.begin(16,2);
  printMessage("");
}

void loop() {  
  // Map 0-1028 :: 1–8
  String level = String(map(analogRead(A0), 0, 1023, 1, 8));

  // Display on LCD
  printMessage(level, 1);
  delay(150);
}
