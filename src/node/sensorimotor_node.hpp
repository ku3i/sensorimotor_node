/*---------------------------------+
 | Jetpack Cognition Lab           |
 | Sensorimotor Node Firmware      |
 | Matthias Kubisch                |
 | kubisch@informatik.hu-berlin.de |
 | January 15th 2020               |
 +---------------------------------*/

#ifndef JETPACK_SENSORIMOTOR_NODE_HPP
#define JETPACK_SENSORIMOTOR_NODE_HPP

#include <Arduino.h>

/* motors */
#define PWM_0 9
#define PWM_1 10
#define PWM_2 5
#define PWM_3 6

#define BUZ 3

/* potis */
#define POTI_0 A0
#define POTI_1 A1
#define POTI_2 A2
#define POTI_3 A3

/* luminous */
#define BRGT_0 A6
#define BRGT_1 A7

/* capsense */
#define CAPSEND 2
#define CAPRET0 8
#define CAPRET1 12

namespace constants {
  const uint8_t num_neopix = 16;
}
/*
    use new command codes, that do not interfere with sensorimotors

    sensors:
    + poti 0 2byte each
    + poti 1
    + poti 2
    + poti 3

    + MPU 12 bytes 6x2
    + Tof 2 bytes
    + capsense 4 bytes 2x2

    + light 2x2

*/

float readpin(uint8_t pin) { return analogRead(pin) / 1023.f; }

namespace led
{
    const uint8_t led_pin = 11;

    void on(uint8_t pwm = 255) {
        if (pwm == 255)
            digitalWrite(led_pin, HIGH);
        else
            analogWrite(led_pin, pwm);
    }


    void off() { digitalWrite(led_pin, LOW); }

    void init() {
        pinMode(led_pin, OUTPUT);
        off();
    }
} /* namespace led */


namespace button
{
    const uint8_t button_pin = 4;

    void init() {
        pinMode(button_pin, INPUT_PULLUP);
    }

    bool pressed()
    {
        static int integ = 0;
        bool buttonstate = !digitalRead(button_pin);
        integ += buttonstate ? 1 : -1;
        integ = constrain(integ, 0, 10);
        return integ>=5;
    }

} /* namespace button */

namespace rs485
{

    const uint8_t drive_enable = 13; // DE
    const uint8_t read_disable = 7;  // NRE
    const int baudrate = 1000000; // 1 Mbaud


    void sendmode() {
        digitalWrite(drive_enable, HIGH);
        digitalWrite(read_disable, HIGH);
    }

    void recvmode() {
        digitalWrite(drive_enable, LOW);
        digitalWrite(read_disable, LOW);
    }

    void init() {
        pinMode(drive_enable, OUTPUT);
        pinMode(read_disable, OUTPUT);
        Serial.begin(1000000);
        recvmode();
    }

    void flush() { Serial.flush(); }

    void write(const uint8_t* buffer, uint16_t N) {
        for (uint16_t i = 0; i < N; ++i)
            Serial.write(buffer[i]);
    }

    bool read(uint8_t& buffer) {
        if (Serial.available() > 0) {
            //led::on();
            buffer = Serial.read();
            //led::off();
            return true;
        } else {
            return false;           
        }
    }


}; /* namespace rs485 */


namespace jetpack {

class SensorimotorNode {
public:
};




} /* namespace jetpack */

#endif /* JETPACK_SENSORIMOTOR_NODE_HPP */
