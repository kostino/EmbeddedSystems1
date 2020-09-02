#ifndef TIMER_H
#define TIMER_H

typedef struct {
  int period; // Time between calls of TimerFcn in ms
  int tasks_to_execute; // How many times TimerFcn is ran
  int start_delay; // Seconds before first start of TimerFcn
  void (*start_fcn)(); // "Constructor" function for timer objects, init data needed for TimerFnc
  void (*stop_fcn)(); // Function that runs after last call of TimerFcn
  void (*timer_fcn)(void*); // Function called at the start of each period
  void (*error_fcn)(); // Function called if queeu becomes full
  void *user_data; // Pointer to user data
} timer;

void start(timer *t);
void startat(timer *t, int year, int month, int day, int h, int min, int sec);

#endif
