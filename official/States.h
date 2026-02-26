#ifndef STATES_H
#define STATES_H

// System states
enum Status {
  STATUS_EMPTY,
  STATUS_REQUEST,
  STATUS_BAKE,
  STATUS_CANCEL,
  STATUS_READY
};

// Dispenser states
const int OPENING = true;
const int CLOSING = false;

#endif
