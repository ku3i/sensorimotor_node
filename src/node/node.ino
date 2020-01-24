#include "communication.hpp"
#include "sensorimotor_node.hpp"
#include "core.hpp"



#define WAIT_US 10000 /* 1kHz loop */

/* timing */
unsigned long timestamp = 0;
unsigned long cycles = 0;

/* button state */
bool pressed = false;
volatile bool paused = true;



typedef jetpack::sensorimotor_core<jetpack::PWM_Servo> core_t;

core_t core;

jetpack::communication_ctrl<core_t> com(core);


void setup() {
  Serial.begin(rs485::baudrate);

  button::begin();
  led::begin();
  rs485::begin();
  core.init_sensors();
  
}

void loop() {

  core.step_sen();
  com.step();
  core.step_mot();
 
  //supreme::adc::restart();
  ++cycles;


  /* loop delay, wait until timer signals next 1ms slot is done */
  do {
    com.step();
  } while(micros() - timestamp < WAIT_US);
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
