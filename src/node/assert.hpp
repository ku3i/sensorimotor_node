/*---------------------------------+
 | Jetpack Cognition Lab           |
 | Sensorimotor Node Firmware      |
 | Matthias Kubisch                |
 | kubisch@informatik.hu-berlin.de |
 | Januar 2020                     |
 +---------------------------------*/

#ifndef JETPACK_COMMON_ASSERT_HPP
#define JETPACK_COMMON_ASSERT_HPP

#include "sensorimotor_node.hpp"

namespace jetpack {

void blink(uint8_t code) {
	for (uint8_t i = 0; i < 8; ++i)
	{
		if ((code & (0x1 << i)) == 0)
		{
			led::on();
            delay(250); //ms
		} else {
			led::on(16);
			delay(250); //ms
		}
		led::off();
		delay(250); //ms
	}
}


void assert(bool condition, uint8_t code = 0) {
	if (condition) return;
	led::off();
	while(1) {
		blink(code);
		delay(1000);
	}
}

} /* namespace supreme */

#endif /* SUPREME_COMMON_ASSERT_HPP */
