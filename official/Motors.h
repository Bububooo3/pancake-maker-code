#ifndef MOTORS_H
#define MOTORS_H

// Step pulse rate (steps/second)
unsigned long MAXSTEP = 800.0;

unsigned long CONVEYORSTEP = 800.0;

const bool CONVEYOR_INVERT = true;

unsigned long DISPENSERSPEED = 800.0;

unsigned long DISPENSERACCEL = 400.0;

// Step positions (Usually 1.8°/step, 200 steps/revolution)
const int DISPENSEROPEN = 0;

const int DISPENSERCLOSE = 0;

#endif
