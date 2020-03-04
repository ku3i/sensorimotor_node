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
#include "neopixel.hpp"
#include "jcl_capsense.hpp"
#include "rangef.h"
#include "adc.hpp"
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

    void init() {
        supreme::adc::init();
        supreme::adc::restart();
        rangef.init();
    }

    //uint8_t readpin(uint8_t pin) { return analogRead(pin) >> 2; } // 10 -> 8 bit

    void step(void)
    {
        position[0] = supreme::adc::result[supreme::adc::poti_0] >> 2;
        position[1] = supreme::adc::result[supreme::adc::poti_1] >> 2;
        position[2] = supreme::adc::result[supreme::adc::poti_2] >> 2;
        position[3] = supreme::adc::result[supreme::adc::poti_3] >> 2;

        luminous[0] = supreme::adc::result[supreme::adc::brgt_0] >> 2;
        luminous[1] = supreme::adc::result[supreme::adc::brgt_1] >> 2;

        supreme::adc::restart();

        digitalWrite(3, LOW);
        delayMicroseconds(2);
        digitalWrite(3, HIGH);

        capacity[0] = 127*cap0.step() + 128;

        digitalWrite(3, LOW);
        delayMicroseconds(2);
        digitalWrite(3, HIGH);

        capacity[1] = 127*cap1.step() + 128;

        digitalWrite(3, LOW);
        delayMicroseconds(2);
        digitalWrite(3, HIGH);

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

    NodeEsc(uint8_t pin)
    : pin(pin)
    {
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

    void disable() {
        if (!is_enabled) return;
        motor.detach();
        pinMode(pin, INPUT_PULLUP);
        is_enabled = false;
    }
};


//TODO: this needs to be improved
template <unsigned DATA_PIN>
class NodePix {
public:

    NodePix()
    {
        ledsetup();
    }

    void init(void) { showColor(64, 32, 64); delay(500); showColor(0, 0, 0); }

    void set_color(uint8_t val) { showColor(val, val, val); }

    void step() {
        /* remember time of last write and wait until latch time,
         * otherwise too frequent calls of step will not work.
         * Alternatively use: show() (which basically waits the needed time. */
    }

    void disable(void) { set_color(0); }

};

class sensorimotor_core {

    bool enabled;

    Sensors          sensors;
    NodeServo        srv;
    //NodeEsc          esc;
    NodePix<PWM_2>   pix;

    uint8_t          target[4];
    uint8_t          watchcat = 0;

public:

    sensorimotor_core()
    : enabled(false)
    , target()
    , sensors()
    , srv(PWM_0)
    //, esc(PWM_1)
    , pix()
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

