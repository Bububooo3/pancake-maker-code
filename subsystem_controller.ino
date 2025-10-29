#include <LiquidCrystal_I2C.h>

// Initialize LCD: (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

	// CONSTANTS \\


	// FUNCTIONS \\
// Little useful functions


void printMessage(String msg, int line = 0, int column = 0) {
  	lcd.setCursor(column, line);  
	lcd.print("        ");
	lcd.setCursor(column, line);
	lcd.print(msg);
}

void clearLine(int lvl=-1){
  lvl = max(lvl, 0)
  lvl = min(lvl, 1)
    
  lcd.setCursor(0, lvl);
  lcd.print("        ");
  lcd.setCursor(0, lvl);
}

// Keyframe functions
void requestNumPancakes(){

}
// void 

	// RUNTIME \\
  
void setup() {
  lcd.begin(16, 2); // Initialize 16x2 LCD
  lcd.print("Level:");     // Title on first row
}

void loop() {  
  // Map 0-1028 :: 1–8
  int level = map(analogRead(A0), 0, 1023, 1, 8);

  // Display on LCD
  clearLine(1);
  lcd.setCursor(0, 1);
  lcd.print(level);

  delay(150);              // Small delay for readability
}
