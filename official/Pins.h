#ifndef PINS_H
#define PINS_H

// Button pins
const int CONFIRMPIN = 7; // Confirm
const int CANCELPIN = 2; // Cancel


// Appliance pins
const int LEDPIN = 6; // For the NeoPixel strip
const int GRIDPIN = 8; // For the SSR controlling the griddle


// Conveyor pins
const int CONVEYORPIN_EN = 24; // Enable
const int CONVEYORPIN_STEP = 22; // Step
const int CONVEYORPIN_DIR = 26; // Direction


// Dispenser pins
const int DISPENSERPIN_EN = 33; // Enable
const int DISPENSERPIN_STEP = 35; // Step
const int DISPENSERPIN_DIR = 37; // Direction


// Fan pins <TODO>
const int COOLINGPIN_EN = 27; // Enable

#endif
