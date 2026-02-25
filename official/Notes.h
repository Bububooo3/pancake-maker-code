/*
  PINS

  LCD w/ I2C Backpack:
	VCC 	→ 	5V
  GND 	→ 	GND
  SDA 	→ 	20
  SCL 	→ 	21


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


  Cooling Driver <Cooling Stepper Motor Pin>:
	EN		→		27
	STEP	→		25
	DIR		→		23
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


  Confirm Push Button:
	1:1 L	→		7
	1:2 R	→		GND


  Cancel Push Button:
	1:1 L	→		2
	1:2 R	→		GND
*/

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
