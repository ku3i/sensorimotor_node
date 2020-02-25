/*---------------------------------+
 | Jetpack Cognition Lab           |
 | Sensorimotor Node Firmware      |
 | Matthias Kubisch                |
 | kubisch@informatik.hu-berlin.de |
 | January 15th 2020               |
 +---------------------------------*/

#ifndef JETPACK_SENSORIMOTOR_CORE_HPP
#define JETPACK_SENSORIMOTOR_CORE_HPP

#include <Arduino.h>
#include <PWMServo.h>
#include <Adafruit_NeoPixel.h>
#include "jcl_capsense.hpp"
#include "rangef.h"

#include "sensorimotor_node.hpp"
#include "assert.hpp"

namespace jetpack {

CapSense cap0 = CapSense(CAPSEND,CAPRET0);
CapSense cap1 = CapSense(CAPSEND,CAPRET1);


/*
  TODOs:
  * adc in freewheel mode

*/

/* WARNING: Do NOT you the common Servo.h library,
 * the ISR takes ~15µs and trashes the 10µs bytes
 * of the 1Mbaud communication loop.
 */


class Sensors {
public:
    uint8_t position[4] = {0,0,0,0};
    uint8_t luminous[2] = {0,0};
    uint8_t capacity[2] = {0,0};
    uint16_t distance = 0;


    Rangefinder rangef;
    //TODO: MPU

    Sensors() { }

    void init() { rangef.init(); }

    uint8_t readpin(uint8_t pin) { return analogRead(pin) >> 2; } // 10 -> 8 bit

	  void step(void)
	  {
        position[0] = readpin(POTI_0);
        position[1] = readpin(POTI_1);
        position[2] = readpin(POTI_2);
        position[3] = readpin(POTI_3);

        luminous[0] = readpin(BRGT_0);
        luminous[1] = readpin(BRGT_1);

        capacity[0] = 127*cap0.step() + 128;
        capacity[1] = 127*cap1.step() + 128;

        rangef.step();
        distance = rangef.dx;
	}
};



class NodeServo {
  uint8_t pin = 0;
  PWMServo motor;
  bool is_enabled = false;

public:

  NodeServo(uint8_t pin) : pin(pin) { pinMode(pin, INPUT_PULLUP);}
  void set_pwm(uint8_t dc) { motor.write(dc); }
  void enable() { if (is_enabled) return; motor.attach(pin); is_enabled = true; }
  void disable() { if (!is_enabled) return; motor.detach(); pinMode(pin, INPUT_PULLUP); is_enabled = false;}
};


class NodeEsc {
  uint8_t pin = 0;
  PWMServo motor;
  bool is_enabled = false;

public:

  NodeEsc(uint8_t pin) : pin(pin) {
    // arming
    motor.attach(pin, 500, 1000);
    motor.write(0);
  }

  void set_pwm(uint8_t dc) { motor.write(dc); }

  void enable() {
    if (is_enabled) return;
    motor.attach(pin, 990, 1500);
    is_enabled = true;
  }

  void disable() { if (!is_enabled) return; motor.detach(); pinMode(pin, INPUT_PULLUP); is_enabled = false;}
};


class NodePix {
  uint8_t pin = 0;
  Adafruit_NeoPixel strip;

public:

  NodePix(uint8_t pin)
  : pin(pin)
  , strip(constants::num_neopix, pin, NEO_RGB + NEO_KHZ800)
  {}

  void init(void) { strip.begin(); strip.show(); }

  void set_color(uint8_t val, uint8_t px = 0xff) {
    if (px < constants::num_neopix)
      strip.setPixelColor(px, strip.Color(val, val, val));
    else if (0xff == px)
    {
          strip.fill(strip.Color(val, val, val), 0, constants::num_neopix);
    }
  }

  void step() { strip.show(); }

  void disable(void) { strip.clear(); }
};

class sensorimotor_core {

  bool enabled;

  Sensors          sensors;
  NodeServo        srv;
  //NodeEsc          esc;
  NodePix          pix;

  uint8_t          target[4];

  uint8_t          watchcat = 0;
  //uint8_t const    def_pwm = 90;

public:

  sensorimotor_core()
  : enabled(false)
  , target()
  , sensors()
  , srv(PWM_0)
  //, esc(PWM_1)
  , pix(PWM_2)
  {
  }

    void apply_target_values(void) {
        if (enabled) {
          srv.set_pwm(target[0]);
          //esc.set_pwm(target[1]);

          srv.enable();
          //esc.enable();

          pix.set_color(target[2]);//, target[3]);

        } else { /*disabled*/
          srv.disable();
          //esc.disable();
          pix.disable();
        }
    }

    void init(void) { sensors.init(); pix.init(); }

    void step_mot(void) {
      apply_target_values();
      pix.step();
      /* safety switchoff */
      if (watchcat < 100) watchcat++;
      else enabled = false;
    }

    void step_sen(void) {
      sensors.step();
    }

    void set_target_pwm(uint8_t pwm[4]) {
      for (uint8_t i = 0; i<4; ++i)
        target[i] = pwm[i];
    }

    void enable()  { enabled = true; watchcat = 0; }
    void disable() { enabled = false; }
    bool is_enabled() const { return enabled; }

    uint8_t  get_position(uint8_t i) const { assert(i<4, 42); return sensors.position[i]; }
    uint8_t  get_luminous(uint8_t i) const { assert(i<2, 43); return sensors.luminous[i]; }
    uint8_t  get_capacity(uint8_t i) const { assert(i<2, 44); return sensors.capacity[i]; }
    uint16_t get_distance(void)      const { return sensors.distance; }

};


} /* namespace jetpack */

#endif /* JETPACK_SENSORIMOTOR_CORE_HPP */
