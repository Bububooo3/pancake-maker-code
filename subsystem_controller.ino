#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal.h>

/*
IMPORTANT ELECTRICAL NOTE: CONCERNING HEATING ELEMENT
/////////////////////////////////////////////////////
> TinkerCAD sim doesn't support AC in the way needed for the griddle
> For the real product, replace MOSFET w/ a solid-state relay (SSR)
> The DC power supply represents the AC mains
> Use SSR rated >=15A @ 120V AC
> Ask mentor/electrician if unsure

/////////////////////////////////////////////////////
Reference Link
> https://shorturl.fm/pwcKO

/////////////////////////////////////////////////////
Step-by-step Replacement Process:
12V DC Power Suppy || 120V AC Outlet
MOSFET			   ||  Solid-state relay
  - Cut only hot wire of extension cord
  - On the SSR, connect one end to COM & other to NO
  - Griddle turns on when relay is on & off when relay is off
  - DO NOT CUT NEUTRAL WIRE

/////////////////////////////////////////////////////
Griddle Details (1200 Watt)
> 2 prongs/wires (non-grounded)
  - Narrow blade: Hot(live) --> often black
  - Wide blade: Neutral --> often white
> Hot wire carries 120V AC
  - Use an extension cord. Don't cut griddle cord directly.
  - Cut cord jacket carefully & strip hot wire after cutting it.
  
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

// Time Lengths (milliseconds)
#define HEATUP 10000
#define ANIMINC 200

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

const char heatingAnim[][16] = {\
	"heating",\
	"heating.",\
	"heating..",\
	"heating...",\
    "heating",\
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

// Arrays
const uint32_t warning[6] = {YELLOW, ORANGE, YELLOW, ORANGE, YELLOW, ORANGE};
const uint32_t danger[6] = {RED, RED, RED, RED, RED, RED};
const uint32_t ready[6] = {GREEN, GREEN, GREEN, GREEN, GREEN, GREEN};
const uint32_t bakingA[6] = {WHITE, YELLOW, WHITE, WHITE, WHITE, WHITE};
const uint32_t bakingB[6] = {WHITE, WHITE, WHITE, YELLOW, WHITE, WHITE};
const uint32_t bakingC[6] = {WHITE, WHITE, WHITE, WHITE, WHITE, YELLOW};

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

bool setColors(uint32_t z[]={}, int a=OFF, int b=OFF, int c=OFF, int d=OFF, int e=OFF, int f=OFF) {
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
    delay(ANIMINC);
  }
  
  printMessage("By Jesuit HS", 1);
  clearLine(0);
  delay(5000);
  printMessage("[Please wait]", 0);
  
  // Turn on relay here (heat up griddle)
  
  for (int i=0; (i<(sizeof(heatingAnim)/sizeof(heatingAnim[0]))*(HEATUP/ANIMINC)); i++) {
  	printMessage(heatingAnim[(i%(sizeof(heatingAnim)/sizeof(heatingAnim[0]))) + 1],1);
    delay(ANIMINC);
  }
  
  printMessage("[Complete]")
  printMessage("", 1);
  
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
