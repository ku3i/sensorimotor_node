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
    //uint16_t mpu_x   = 0;
	  //uint16_t mpu_y   = 0;

    Rangefinder rangef;

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



class PWM_Servo {
  uint8_t pin = 0;
  PWMServo motor;
  bool is_enabled = false;

public:

  PWM_Servo(uint8_t pin) : pin(pin) {}
  void set_pwm(uint8_t dc) { motor.write(dc); }
  void enable() { if (is_enabled) return; motor.attach(pin); is_enabled = true; }
  void disable() { if (!is_enabled) return; motor.detach(); is_enabled = false;}
};




template <typename MotorDriverType>
class sensorimotor_core {

  bool enabled;

  Sensors          sensors;
  PWM_Servo        motor[4];
  uint8_t          target[4];

  uint8_t          watchcat = 0;
  uint8_t const    def_pwm = 90;

public:

  sensorimotor_core()
  : enabled(false)
  , target()
  , sensors()
  , motor({PWM_0,PWM_1,PWM_2,PWM_3})
  {
  }

    void apply_target_values(void) {
        if (enabled) {
            for (uint8_t i = 0; i<2; ++i) {
              motor[i].set_pwm(target[i]);
              motor[i].enable();
            }
        } else { /*disabled*/
          for (uint8_t i = 0; i<2; ++i) {
            motor[i].disable();
          }
        }
    }

    void init_sensors(void) { sensors.init(); }

    void step_mot(void) {
      apply_target_values();

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
