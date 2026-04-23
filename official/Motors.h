#ifndef MOTORS_H
#define MOTORS_H

// Step pulse rate (steps/second)
unsigned long MAXSTEP = 800.0;

unsigned long CONVEYORSTEP = 800.0;

const bool CONVEYOR_INVERT = true;

const unsigned long DISPENSERSPEED = 800.0;

const unsigned long DISPENSERACCEL = 400.0;



// Step position offsets (Usually 1.8°/step, 200 steps/revolution)
const int DISPENSEROPEN = 50; // 90° (may need to invert)
const int DISPENSERCLOSE = 0;

#endif
