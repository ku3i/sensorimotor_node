/*---------------------------------+
 | Jetpack Cognition Lab           |
 | Sensorimotor Firmware           |
 | Matthias Kubisch                |
 | kubisch@informatik.hu-berlin.de |
 | January 2018                    |
 +---------------------------------*/

#ifndef SUPREME_SENDBUFFER_HPP
#define SUPREME_SENDBUFFER_HPP

#include "sensorimotor_node.hpp"
#include "assert.hpp"



namespace jetpack {

template <unsigned N>
class sendbuffer {
	static const unsigned NumSyncBytes = 2;
	static const uint8_t chk_init = 0xFE; /* (0xff + 0xff) % 256 */
	uint16_t  ptr = NumSyncBytes;
	uint8_t   buffer[N];
	uint8_t   checksum = chk_init;
public:
	sendbuffer()
	{
		static_assert(N > NumSyncBytes, "Invalid buffer size.");
		for (uint8_t i = 0; i < NumSyncBytes; ++i)
			buffer[i] = 0xFF; // init sync bytes once
	}
	void add_byte(uint8_t byte) {
		assert(ptr < (N-1), 1);
		buffer[ptr++] = byte;
		checksum += byte;
	}
	void add_word(uint16_t word) {
		add_byte((word  >> 8) & 0xff);
		add_byte( word        & 0xff);
	}
	void discard(void) { ptr = NumSyncBytes; }
	void flush() {
		if (ptr == NumSyncBytes) return;
		add_checksum();
		rs485::sendmode();
		rs485::write(buffer, ptr);
		rs485::flush();   //TODO do not wait here, set an flag and check in com step to switch into recv mode again
		rs485::recvmode();
		/* prepare next */
		ptr = NumSyncBytes;
	}
	uint16_t size(void) const { return ptr; }
private:
	void add_checksum() {
		assert(ptr < N, 8);
		buffer[ptr++] = ~checksum + 1; /* two's complement checksum */
		checksum = chk_init;
	}
};

} /* namespace jetpack */

#endif /* JETPACK_SENDBUFFER_HPP */
