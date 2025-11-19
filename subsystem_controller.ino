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
  - We can control T using PID , maybe if we have time
  	^ (Not a necessity)
    ^ (Using rate of change, current T, target T, & time passed)

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
bool baking = false, prevbaking = false, requesting = false, cancelMode = false;
bool prevConfirmState = HIGH, prevCancelState = HIGH;
String lastPrintLCD1 = "";
String lastPrintLCD2 = "";
String placeholder = "                ";
int level = 1;
int plevel = 0;
auto t_init = millis();
unsigned long t_bake, t_vent, t_current, t_terminated;


	// CONSTANTS
// Counts
#define LEDCOUNT 	6

// Time Lengths (milliseconds)
#define HEATUP		2e3
#define ANIMINC 	25e1
#define COOKTIME 	60e3

// Buttons
// Imagine confirm green & cancel red
#define CONFIRMPIN 	7
#define CANCELPIN 	9

// Appliances
#define LEDPIN 		6
#define MOTORPIN 	10
#define GRIDPIN 	8

const char intro[][16] = {\
	"pancake maker",\
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
    " ",\
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
// ^ (Doesn't work in TinkerCAD sim)

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

// Presets
const uint32_t warning[6] = {YELLOW, ORANGE, YELLOW, ORANGE, YELLOW, ORANGE};
const uint32_t danger[6] = {RED, RED, RED, RED, RED, RED};
const uint32_t ready[6] = {GREEN, GREEN, GREEN, GREEN, GREEN, GREEN};
const uint32_t bakingA[6] = {WHITE, YELLOW, WHITE, WHITE, WHITE, WHITE};
const uint32_t bakingB[6] = {WHITE, WHITE, WHITE, YELLOW, WHITE, WHITE};
const uint32_t bakingC[6] = {WHITE, WHITE, WHITE, WHITE, WHITE, YELLOW};

	// FUNCTIONS
// Little useful functions
#define getArraySize(a) 	(sizeof(a) / sizeof((a)[0]))	
#define getConfirmState() 	(!(digitalRead(CONFIRMPIN)))
#define getCancelState() 	(!(digitalRead(CANCELPIN)))

String center(String msg) {
	return (msg.length()<=16) ? placeholder.substring(0,(16-msg.length())/2)+msg : msg.substring(0,17);
}

bool printMessage(String msg, int line = 0) {
  	line = constrain(line, 0, 1);
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

bool floodColors(auto z){
  for (int i=0; i<getArraySize(z); i++) {
  	led.setPixelColor(i, z[i]);
  }
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
  	printMessage("Bake how much?");
 	printMessage("", 1);
	return true;
}

void introductionProtocol(){
  for (int i=0; (i<getArraySize(intro)); i++) {
  	printMessage(intro[i]);
    delay(ANIMINC);
  }
  
  printMessage("By Jesuit HS", 1);
  clearLine(0);
  delay(1500);
  printMessage("[Please wait]", 0);
  
  digitalWrite(GRIDPIN, HIGH); // <--- This is turning on griddle
  
  // Wait for it to heat up (timing based on trial data)
  for (int i=0; (i<(HEATUP/ANIMINC)); i++) {
    int prxy = (i%getArraySize(heatingAnim)) + 1;
  	printMessage(heatingAnim[prxy],1);
    delay(ANIMINC);
  }
  
  printMessage("[Complete]");
  printMessage("", 1);
}

void update(){
    // Button handler  
  	baking = (requesting && prevConfirmState && getConfirmState() || baking) ? true : false;
  	if (requesting && prevConfirmState && getConfirmState() || baking){requesting=false;}
  	t_bake = ((!requesting)&& baking) ? millis() : t_bake; // THIS IS WHERE WE LEFT OFF <-- love it
  
  	// Baking System
  	digitalWrite(LEDPIN, baking);
  	digitalWrite(MOTORPIN, baking);
  	
	prevConfirmState = getConfirmState();
  	prevCancelState = getCancelState();
  	prevbaking = baking;
  
    // Handle cancel button cases (on pressed or released)
  	if (!cancelMode && (getCancelState() != prevCancelState)){
    	cancelMode = true;
        if (baking) {
    		baking = false;
      		requesting = true;
    	} else if (requesting) {
    		requesting = false;
    	}
      
   		clearLine();
  }
}

	// EXTRA INTERFACE CONSTANTS (for optimization, convenience & such)
const String bMsg = center("Baking");
const String spsMsg = "Pancakes"; // The word Pancakes
const String spMsg = spsMsg.substring(0,7); // The word Pancake
const String eMsg1 = center("/   Action   \\"); // Cancel display line 1
const String eMsg2 = center("\\ Terminated /"); // Cancel display line 2
const String amt = "Auto-Mode";

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
  if (cancelMode) {goto onCancel;}

  t_current = millis();
  
  if (!requesting && !baking){requesting = requestNumPancakes();}
  if (requesting && !baking){
  	// Map 0-1028 :: 1–17
  	level = map(analogRead(A0), 0, 1023, 1, 17);
    
  	// Choose # Pancakes Screen
  	if (level != plevel && !baking){
    	if (level<17){
  			printMessage((level>1) ? (String(level) + " " + spsMsg) : (String(level) + " " + spMsg), 1);
    	} else {
    		printMessage(amt, 1);
    	}
    	plevel = level;
  	}
  }
  
  if (!lastPrintLCD1.equals(bMsg) && baking) {
    printMessage(bMsg);
    floodColors(warning); // Figure out color scene
  } else if (baking) {
    
    // printMessage(center("["+String(float(t_bake/10)/100.0))+"]", 1);
    
    // Start baking here (dispensing)
    // Figure out how to detect when we're out of batter too
    if (lastPrintLCD2.equals( ( )||( ) )) { // <---LEFTOFF HERE 11/19/25
    	if (level<17){
  			printMessage(center(((level>1) ? (String(level) + " " + spsMsg) : (String(level) + " " + spMsg))),1);
    	} else {
    		printMessage(center(spsMsg),1);
    	}
    }
  }
  
  update(); // Updates button states
  
  onCancel: if (cancelMode){
  	if (!lastPrintLCD1.equals(eMsg1)){
  		printMessage(eMsg1);
    	printMessage(eMsg2,1);
  	}
  }
  
  delay(150);
}
