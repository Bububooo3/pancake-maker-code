#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal.h>

/*
IMPORTANT ELECTRICAL NOTE: CONCERNING HEATER
/////////////////////////////////////////////////////
> TinkerCAD sim doesn't support AC in the way needed for the griddle
> For the real product, replace MOSFET w/ a solid-state relay (SSR)
> The DC power supply represents the AC mains
> Use relay rated >=15A @ 120V AC
> Ask mentor/electrician if unsure
/////////////////////////////////////////////////////
Step-by-step Replacement Process:
12V DC Power Suppy || 120V AC Outlet
MOSFET  ||  Solid-state relay

/////////////////////////////////////////////////////
Griddle Details (1200 Watt)
> 2 prongs/wires (non-grounded)
  - Narrow blade: Hot(live) --> often black
  - Wide blade: Neutral --> often white
  
> Hot wire carries 120V AC
  - Use an extension cord. Don't cut griddle cord directly.

*/

//	INTIALIZATION GLOBALS
bool baking = false;
bool prevConfirmState = HIGH;
String lastPrintLCD1 = "";
String lastPrintLCD2 = "";
String placeholder = "                ";
int level = 1;
int plevel = 0;


	// CONSTANTS
// Counts
#define LEDCOUNT 6

// Buttons
#define CONFIRMPIN 7
#define LEDPIN 6
#define MOTORPIN 10

// Arrays
const char intro[][16] = {\
	"Pancake maker",\
	"pAncake maker",\
	"paNcake maker",\
	"panCake maker",\
	"pancAke maker",\
	"pancaKe maker",\
	"pancakE maker",\
	"pancake Maker",\
	"pancake mAker",\
	"pancake maKer",\
	"pancake makEr",\
	"pancake makeR",\
    "pancake maker",\
    "",\
};


	// INITIALIZE COMPONENTS
// Initialize LCD: (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// Initialize LED Strip: (#LEDs, Pin, Color Mode + Signal)
Adafruit_NeoPixel led(LEDCOUNT, LEDPIN, NEO_GRB + NEO_KHZ800);

	// LIBRARY-SPECIFIC CONSTANTS
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


	// FUNCTIONS
// Little useful functions
bool printMessage(String msg, int line = 0) {
  	lcd.setCursor(0, line);
	lcd.print(placeholder);
	lcd.setCursor(0, line);
	lcd.print(msg);
  
  	if (line){
      lastPrintLCD2 = msg;
    }else{
      lastPrintLCD1 = msg;
    }
  
  	return true;
}

bool getConfirmState(){
	return !(digitalRead(CONFIRMPIN));
}

bool clearLine(int lvl=-1){
  if (lvl<0){lcd.clear(); return true;}
  lvl = constrain(lvl, 0, 1);
  printMessage(placeholder, lvl);
  return true;
}

bool setColors(int a=OFF, int b=OFF, int c=OFF, int d=OFF, int e=OFF, int f=OFF) {
	led.setPixelColor(0, a);
	led.setPixelColor(1, b);
 	led.setPixelColor(2, c);
 	led.setPixelColor(3, d);
 	led.setPixelColor(4, e);
 	led.setPixelColor(5, f);
  	led.show();
  return true;
}

bool disableLED() {
	led.setPixelColor(0, OFF);
	led.setPixelColor(1, OFF);
 	led.setPixelColor(2, OFF);
 	led.setPixelColor(3, OFF);
 	led.setPixelColor(4, OFF);
 	led.setPixelColor(5, OFF);
  	led.show();
  return true;
}

// Keyframe functions
bool requestNumPancakes(){
	return true;
}

void introductionProtocol(){
  for (int i=0; (i<(sizeof(intro)/sizeof(intro[0]))); i++) {
  	printMessage(intro[i]);
    delay(200);
  }
  
  printMessage("pancake maker", 1);
  clearLine(0);
  delay(2000);
  printMessage("Heating", 1);
  delay(1500); // <-- temporary
  
  // Heat up heating element
  //while (){
  
  
  //}
    
  printMessage("Bake how much?");
  printMessage("", 1);
}


	// RUNTIME 
// Run once on boot
void setup() {
  // Buttons
  pinMode(CONFIRMPIN, INPUT_PULLUP);
  pinMode(LEDPIN, OUTPUT);
  pinMode(MOTORPIN, OUTPUT);
  
  // LCD
  lcd.begin(16,2);
  
  // LED
  led.begin();
  led.show();
  introductionProtocol();
}

// Run repeatedly
void loop() {
  // Map 0-1028 :: 1–8
  level = map(analogRead(A0), 0, 1023, 1, 8);
  
  // LCD DISPLAY  
  // Choose # Pancakes Screen
  if (level != plevel && !baking){
  	printMessage((level>1) ? (String(level) + " Pancakes") : (String(level) + " Pancake"), 1);
    plevel = level;
  }

  if (baking && lastPrintLCD1 != "     Baking     ") {
    printMessage("     Baking     ");
    printMessage("   "+((level>1) ? (String(level) + " Pancakes") : (String(level) + " Pancake"))+"   ",1);
  }

  // Button handler  
  baking = (prevConfirmState && getConfirmState() || baking) ? true : false;
  
  // Baking System
  digitalWrite(LEDPIN, baking);
  digitalWrite(MOTORPIN, baking);
  
  prevConfirmState = getConfirmState();
  delay(150);
}
