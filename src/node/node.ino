#include "communication.hpp"
#include "sensorimotor_node.hpp"
#include "core.hpp"


#define IDLE_TIMEOUT_US 20000 /* 50Hz main loop in idle */

/* timing */
unsigned long timestamp = 0;
unsigned long cycles = 0;

/* button state */
bool pressed = false;
volatile bool paused = true;

typedef jetpack::sensorimotor_core core_t;

core_t core;

jetpack::communication_ctrl<core_t> com(core);


void setup() {
  Serial.begin(rs485::baudrate);

  button::init();
  led   ::init();
  rs485 ::init();
  core   .init();
  pinMode(3, OUTPUT);
}

void loop() {

  digitalWrite(3, HIGH);
  core.step_sen();

  digitalWrite(3, LOW);
  delayMicroseconds(2);
  digitalWrite(3, HIGH);

  core.step_mot();
  digitalWrite(3, LOW);
  ++cycles;


  /* loop delay, wait until timer signals next 1ms slot is done */
  do {
    com.step();
    delayMicroseconds(1);
  } while(!com.check_and_reset_loop_sync() && (micros() - timestamp < IDLE_TIMEOUT_US));
  timestamp = micros();
}


/*
  // pause, if button was pressed before and then released
  bool state = button::pressed();
  if (pressed and !state)
  {
    if (paused) {
      paused = false;
      led::off();
      //DO stuff here
    } else {
      paused = true;
      led::on();
      //DO stuff here
    }
  }
  pressed = state;

*/
