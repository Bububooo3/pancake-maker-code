#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal.h>

/*
 	  ////////////////////////////////
	 // CONCERNING HEATING ELEMENT //
	////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
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



  	  /////////////
 	 // GLOBALS //
	/////////////
///////////////////////////////////////////////////////////////////////////////////////////
bool baking = false, prevbaking = false, requesting = false, cancelMode = false;
bool confirmPressedPrev = LOW, cancelPressedPrev = LOW;
String lastPrintLCD1 = "";
String lastPrintLCD2 = "";
String placeholder = "                ";
int level = 1;
int plevel = 0;
int dispensed = 0;
unsigned long t_current, t_init;
unsigned long t_bake, t_kill;
bool tbA = false, tkA = false; // Timer [VarName] Active (gave up on finding a clever way to do it)

///////////////////////////////////////////////////////////////////////////////////////////


	  ///////////////
	 // CONSTANTS //
	///////////////
///////////////////////////////////////////////////////////////////////////////////////////
// Counts
#define LEDCOUNT 	6

// Time Lengths (milliseconds [SSe3])
#define HEATUP		2e3
#define FANTIME		2e3
#define MSGWAIT		1.5e3
#define ANIMINC 	0.25e3
#define COOKTIME 	10

// Buttons
#define CONFIRMPIN 	7
#define CANCELPIN 	2

// Appliances
#define LEDPIN 		6
#define MOTORPIN 	10
#define GRIDPIN 	8
#define FANPIN		9		


// Fixed-Size Arrays
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

///////////////////////////////////////////////////////////////////////////////////////////




   	  ///////////////////////////
  	 // INITIALIZE COMPONENTS //
 	///////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
// Initialize LCD: (RS, E, D4, D5, D6, D7)
// Initialize LED Strip: (#LEDs, Pin, Color Mode + Signal)

LiquidCrystal lcd(12, 11, 5, 4, 3, 13);
Adafruit_NeoPixel led(LEDCOUNT, LEDPIN, NEO_GRB + NEO_KHZ800);
// ^ (Doesn't work in TinkerCAD sim)

///////////////////////////////////////////////////////////////////////////////////////////



	  ////////////////////////////////
	 // LIBRARY-SPECIFIC CONSTANTS //
	////////////////////////////////
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
const uint32_t warning[6] = {YELLOW, ORANGE, YELLOW, ORANGE, YELLOW, ORANGE};
const uint32_t danger[6] = {RED, RED, RED, RED, RED, RED};
const uint32_t ready[6] = {GREEN, GREEN, GREEN, GREEN, GREEN, GREEN};
const uint32_t bakingA[6] = {WHITE, YELLOW, WHITE, WHITE, WHITE, WHITE};
const uint32_t bakingB[6] = {WHITE, WHITE, WHITE, YELLOW, WHITE, WHITE};
const uint32_t bakingC[6] = {WHITE, WHITE, WHITE, WHITE, WHITE, YELLOW};

////////////////////////////////////////////////////////////////////////////////////////



   	  /////////////////
  	 /// FUNCTIONS ///
 	/////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// Macros
#define getArraySize(a) 	(sizeof(a) / sizeof((a)[0]))	

// Center/truncate string (16 characters)
String center(String msg) {
	return (msg.length()<=16) ? placeholder.substring(0,(16-msg.length())/2)+msg : msg.substring(0,16);
}


// Display string on LCD
bool printMessage(String msg, int line = 0) {
  	line = constrain(line, 0, 1);
  	lcd.setCursor(0, line);
	lcd.print(placeholder);
	lcd.setCursor(0, line);
	lcd.print(msg);
  
  	if (line){
      lastPrintLCD2 = msg;
    } else {
      lastPrintLCD1 = msg;
    }
  
  	return true;
}


// Clear LCD line or entire screen if no params
bool clearLine(int lvl=-1){
  if (lvl<0){lcd.clear(); return true;}
  lvl = constrain(lvl, 0, 1);
  printMessage(placeholder, lvl);
  return true;
}


// Set LED strip colors manually
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


// Set LED strip colors using a preset
bool floodColors(const uint32_t z[6]) {
  if (getArraySize(z)<6) {return false;}
  setColors(z[0],z[1],z[2],z[3],z[4],z[5]);
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
  	led.show();
  return true;
}


// Set fan motor power simply (WIRED WRONG)
void setFanPower(float i) {
	analogWrite(FANPIN, 255*constrain(i,0,1)); // automatically becomes integer
}


// Get the net difference between these two times in milliseconds
unsigned long difftime(long t1, long t2) {return abs(t1-t2);}

////////////////////////////////////////////////////////////////////////////////////////



   	  /////////////////
  	 // ABSTRACTION //
 	/////////////////
////////////////////////////////////////////////////////////////////////////////////////////
// EXTRA INTERFACE CONSTANTS (for optimization, convenience & such)
const String bMsg = center("Baking");
const String spsMsg = "Pancakes"; // The word Pancakes
const String spMsg = spsMsg.substring(0,7); // The word Pancake
const String cspsMsg = center(spsMsg); // So comprehensive 😍
const String cspMsg = center(spMsg);
const String eMsg1 = center("/   Action   \\"); // Cancel display line 1
const String eMsg2 = center("\\ Terminated /"); // Cancel display line 2
const String amt = "Auto-Mode";


// Display a message on LCD asking for # pancakes (and set requesting to true simultaneously)
bool requestNumPancakes(){
  	printMessage(center("Bake how many?"));
 	clearLine(1);
	return true;
}


// The entire warm-up process for the machine w/ animations
void introductionProtocol(){
  for (int i=0; (i<getArraySize(intro)); i++) {
  	printMessage(center(intro[i]));
    delay(ANIMINC);
  }
  
  printMessage(center("By Jesuit HS"), 1);
  delay(MSGWAIT);
  clearLine();
  
  printMessage(center("[Please wait]"), 0);
  setFanPower(1.0f);
  delay(FANTIME);
  
  digitalWrite(GRIDPIN, HIGH); // <--- This is turning on griddle
  
  // Wait for it to heat up (timing based on trial data)
  for (int i=0; (i<(HEATUP/ANIMINC)); i++) {
    int prxy = (i%getArraySize(heatingAnim)) + 1;
  	printMessage(center(heatingAnim[prxy]),1);
    delay(ANIMINC);
  }
  
  printMessage(center("[Complete]"),1);
  delay(MSGWAIT);
  clearLine();
}


// Dispense pancakes fxn
void dispense() {

}


// Update per heartbeat fxn
void update(){
  	bool confirmPressed = (digitalRead(CONFIRMPIN) == LOW);
	bool cancelPressed  = (digitalRead(CANCELPIN) == LOW);
  
  	// Set them for next time
    tbA = baking;
  	tkA = cancelMode;
  
  	t_bake = (tbA) ? t_bake : 0.0L;
  	t_kill = (tkA) ? t_kill : 0.0L;  	
	
  	// Button handler
  	baking = ((confirmPressedPrev && !confirmPressed) || baking) ? true : false;
  
  	// Confirm button pressed
  	if (requesting && !confirmPressedPrev && confirmPressed){
      requesting = false;
      t_bake = t_current;
      clearline();
    }
  
  	// Cancel button pressed
  	if (cancelPressed && !cancelPressedPrev){
        baking = false;
      	requesting = !requesting;
      
   		clearLine();
      	cancelMode = true;
      
      	Serial.println("CANCELLED");
      	t_kill = t_current;
 	 }
  
  	// Baking System
  	digitalWrite(LEDPIN, baking);
  	digitalWrite(MOTORPIN, baking);
  	
	confirmPressedPrev = confirmPressed;
  	cancelPressedPrev = cancelPressed;
  	prevbaking = baking;
  	
  	// for debugging stuff
  /*
  	Serial.println("======================");
  	Serial.println("Timers");
  	Serial.print("| B: "); Serial.println(t_bake);
    Serial.print("| T: "); Serial.println(t_kill);
    Serial.print("| C: "); Serial.println(t_current); 
    Serial.println("======================");
  	Serial.println("Buttons");
  	Serial.print("| Confirm: "); Serial.println(confirmPressed);
  	Serial.print("| Prev-Confirm: "); Serial.println(confirmPressedPrev);
  	Serial.print("| Baking: "); Serial.println(baking);
  	Serial.print("| Requesting: "); Serial.println(requesting);*/

  	t_current = (millis() - t_init)/1000; // Increment current time separately
}


// The screen for asking for pancakes
void requestScreen() {
  	// Map 0-1028 :: 1–17
  	level = map(analogRead(A0), 0, 1023, 1, 17);
    
  	// Choose # Pancakes Screen
  	if (level != plevel && !baking){
    	if (level<17){
  			printMessage(center((level>1) ? (String(level) + " " + spsMsg) : (String(level) + " " + spMsg)), 1);
    	} else {
    		printMessage(center(amt), 1);
    	}
    	plevel = level;
  	}
}


// The screen that shows when bancakes are paking
void bakeScreen() {
  if (!lastPrintLCD1.equals(bMsg) && baking) {
    printMessage(bMsg);
  } else if (baking) {
    
    if (difftime(t_bake, t_current)>COOKTIME) {
    	dispense();
    }
    
    // Time display for debugging
    // printMessage(center("["+String(float(TIMEVARHERE/10)/100.0))+"]", 1);
    
    // Start baking here (dispensing)
    // Figure out how to detect when we're out of batter too
    auto temporaryPrint = (level<17) ? center(((level>1) ? (String(level) + " " + spsMsg) : (String(level) + " " + spMsg))) : spsMsg;
    if (!lastPrintLCD2.equals(temporaryPrint)) {
    	printMessage(temporaryPrint,1);
    }
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
  pinMode(FANPIN, OUTPUT);
  pinMode(LEDPIN, OUTPUT);
  pinMode(MOTORPIN, OUTPUT);
  
  // LCD
  lcd.begin(16,2);
  
  // LED
  led.begin();
  led.show();
  introductionProtocol();
  
  // Timer
  t_init = millis();
  Serial.begin(9600);
}


// Run repeatedly
void loop() {  
  if (cancelMode) {
  	if (!lastPrintLCD1.equals(eMsg1)){
  		printMessage(eMsg1);
    	printMessage(eMsg2,1);
  	}
    
    
  } else {
  	if (!requesting && !baking){requesting = requestNumPancakes(); Serial.println("a");}
  	if (requesting && !baking){requestScreen(); Serial.println("b");}
  	if (baking) {bakeScreen(); Serial.println("c");}
    
    
  }
  
  
  update(); // Updates button states & time trackers
  
  delay(150);
}
