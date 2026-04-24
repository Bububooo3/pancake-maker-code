#ifndef MOTORS_H
#define MOTORS_H

// Step pulse rate (steps/second)
const float MAXSTEP = 800.0;

const float CONVEYORSTEP = 800.0;

const bool CONVEYOR_INVERT = true;

const bool DISPENSER_INVERT = false;

const float DISPENSERSPEED = 80.0;

const float DISPENSERACCEL = 800.0;



// Step position offsets (Usually 1.8°/step, 200 steps/revolution)
const int DISPENSEROPEN = -60 * (5.0/9.0); // degrees * -5/9
const int DISPENSERCLOSE = 0;

#endif
