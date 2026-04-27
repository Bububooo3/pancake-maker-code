#ifndef TIMES_H
#define TIMES_H

// (Miliseconds)
const unsigned long HEATUP = 40e3;  // Time it takes for the griddle to turn on

const unsigned long COOLDOWN = 40e3;  // Time it takes for the griddle to turn off

const unsigned long KILLTIMEOUT = 60e3;  // Maximum time allowed to remain in terminated state

const unsigned long MSGWAIT = 1.5e3;  // Intro message display time

const unsigned long ANIMINC = 0.25e3;  // Animation speed

const unsigned long COOKTIME = 80e3;  // Time for one pancake on the belt

const unsigned long DEBOUNCE = .025e3;  // Time before buttons sink input again after pressed

const unsigned long AUTOTIMEOUT = COOKTIME * 30; // It is what it sounds like
#endif
