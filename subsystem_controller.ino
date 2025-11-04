#include <Adafruit_NeoPixel.h>

#include <LiquidCrystal.h>

	// GLOBALS \\
bool baking = false;
bool prevConfirmState = HIGH;


	// CONSTANTS \\
// Counts
#define LEDCOUNT 6

// Buttons
#define CONFIRMPIN 7
#define LEDPIN 6
#define MOTORPIN 10

// Arrays
const char intro[4][16] = {\
	"Initializing",\
  	"Select number",\
  	"Bake how much?",\
  	"",\
};


	// INITIALIZE COMPONENTS \\
// Initialize LCD: (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// Initialize LED Strip: (#LEDs, Pin, Color Mode + Signal)
Adafruit_NeoPixel led(LEDCOUNT, LEDPIN, NEO_GRB + NEO_KHZ800);

	// LIBRARY-SPECIFIC CONSTANTS \\
// Colors
#define WHITE strip.Color(255, 255, 255)
#define RED strip.Color(255, 0, 0)
#define GREEN strip.Color(0, 255, 0)
#define BLUE strip.Color(0, 0, 255)

#define YELLOW strip.Color(255, 255, 0)
#define CYAN strip.Color(0, 255, 255)
#define MAGENTA strip.Color(255, 0, 255)
#define ORANGE strip.Color(255, 165, 0)
#define PURPLE strip.Color(128, 0, 128)

#define OFF strip.Color(0, 0, 0)


	// FUNCTIONS \\
// Little useful functions
bool printMessage(String msg, int line = 0, int column = 0) {
  	lcd.setCursor(column, line);  
	lcd.print("                ");
	lcd.setCursor(column, line);
	lcd.print(msg);
  	return true;
}

bool getConfirmState(){
	return !(digitalRead(CONFIRMPIN));
}

bool clearLine(int lvl=-1){
  lvl = max(lvl, 0);
  lvl = min(lvl, 1);
    
  lcd.setCursor(0, lvl);
  lcd.print("                ");
  lcd.setCursor(0, lvl);
  return true;
}

bool scrollLine(int num=2){

}

bool setColors(int a=WHITE, int b=WHITE, int c=WHITE, int d=WHITE, int e=WHITE, int f=WHITE) {
	led.SetPixelColor();
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
  pinMode(CONFIRMPIN, INPUT_PULLUP);
  pinMode(LEDPIN, OUTPUT);
  pinMode(MOTORPIN, OUTPUT);
  
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
  baking = (prevConfirmState == HIGH && getConfirmState() == LOW) ? !baking : baking;
  
  digitalWrite(LEDPIN, !baking);
  digitalWrite(MOTORPIN, !baking);
  
  prevConfirmState = getConfirmState();
  delay(50);
}
