/*---------------------------------+
 | Jetpack Cognition Lab           |
 | Sensorimotor Node Firmware      |
 | Matthias Kubisch                |
 | kubisch@informatik.hu-berlin.de |
 | January 2020                    |
 +---------------------------------*/

#ifndef SUPREME_COMMUNICATION_HPP
#define SUPREME_COMMUNICATION_HPP


#include <avr/eeprom.h>
#include "assert.hpp"
#include "sendbuffer.hpp"
#include "sensorimotor_node.hpp"

//TODO: clear recv buffer after timeout

namespace jetpack {

template <typename CoreType>//, typename ExternalSensorType>
class communication_ctrl {
public:
  /* The sensorimotor node does only communicate
   * via raw data. How the bytes are handled is user defined. */
	enum command_id_t {
		no_command,
		ping,
		ping_response,
		set_id,
		set_id_response,
    data_request,
    data_response,
    data_set,
	};

	enum command_state_t {
		  syncing
		, awaiting
		, get_id
		, reading
		, eating
		, verifying
		, pending
		, finished
		, error
    , ignore_cmd
	};

private:
	CoreType&                    ux;

	uint8_t                      recv_buffer = 0;
	uint8_t                      recv_checksum = 0;
	sendbuffer<32>               send;

	uint8_t                      motor_id = 127; // set to default
	uint8_t                      target_id = 127;

	command_id_t                 cmd_id    = no_command;
	command_state_t              cmd_state = syncing;
	unsigned int                 cmd_bytes_received = 0;

	bool                         led_state = false;
	bool                         sync_state = false;

	uint8_t                      num_bytes_read = 0;
	uint16_t                     errors = 0;

  int                          exp_num_recv_bytes = 0;
  uint8_t                      dat[256];                      


public:

	communication_ctrl(CoreType& ux)
	: ux(ux)
	{
		read_id_from_EEPROM();
	}

	void read_id_from_EEPROM() {
		eeprom_busy_wait();
		uint8_t read_id = eeprom_read_byte((uint8_t*)23);
		if (read_id) /* MSB is set, check if this id was written before */
			motor_id = read_id & 0x7F;
	}

	void write_id_to_EEPROM(uint8_t new_id) {
		eeprom_busy_wait();
		eeprom_write_byte((uint8_t*)23, (new_id | 0x80));
	}

	inline
	bool byte_received(void) {
		bool result = rs485::read(recv_buffer);
		if (result)
			recv_checksum += recv_buffer;
		return result;
	}

	command_state_t get_state()    const { return cmd_state; }
	uint8_t         get_motor_id() const { return motor_id; }
	uint16_t        get_errors()   const { return errors; }

	command_state_t waiting_for_id()
	{
		if (recv_buffer > 127) return error;
		switch(cmd_id)
		{
			/* single byte commands */
			case data_request:
			case ping:
				return (motor_id == recv_buffer) ? verifying : eating;

			/* multi-byte commands */
			case data_set:
			case set_id:
				return (motor_id == recv_buffer) ? reading : eating;

			/* responses */
			case ping_response:           return eating;
			case set_id_response:         return eating;
			case data_response:           return eating;

			default: /* unknown command */ break;
		}
		assert(false, 3);
		return finished;
	}

	void prepare_data_response(void)
	{
    const uint8_t exp_num_data_bytes = 11;
    
		send.add_byte(0xC1); /* 0101.0001 */
		send.add_byte(motor_id);
    send.add_byte(exp_num_data_bytes); // N

    send.add_byte(num_bytes_read);
    
		send.add_byte(ux.get_position(0));
		send.add_byte(ux.get_position(1));
		send.add_byte(ux.get_position(2));
		send.add_byte(ux.get_position(3));

		send.add_byte(ux.get_luminous(0));
    send.add_byte(ux.get_luminous(1));

		send.add_byte(ux.get_capacity(0));
		send.add_byte(ux.get_capacity(1));

		send.add_word(ux.get_distance());

    assert(send.size() == exp_num_data_bytes + 5, 0x7F); // 2 sync + cmd + id + N
	}

	command_state_t process_command()
	{
		switch(cmd_id)
		{
			case data_request:
				ux.disable();
				prepare_data_response();
				break;

			case data_set:
				ux.set_target_pwm(dat);
				ux.enable();
				prepare_data_response();
				break;

			case ping:
				send.add_byte(0xE1); /* 1110.0001 */
				send.add_byte(motor_id);
				break;

			case set_id:
				write_id_to_EEPROM(target_id);
				read_id_from_EEPROM();
				send.add_byte(0x71); /* 0111.0001 */
				send.add_byte(motor_id);
				break;

			default: /* unknown command */
				assert(false, 2);
				break;

		} /* switch cmd_id */

		return finished;
	}


	/* handle multi-byte commands */
	command_state_t waiting_for_data()
	{
		switch(cmd_id)
		{
			case data_set:
				if (-1 == exp_num_recv_bytes) {
				  exp_num_recv_bytes = recv_buffer;
          return reading;
				}
        else {
          dat[num_bytes_read++] = recv_buffer;
          return (num_bytes_read < exp_num_recv_bytes) ? reading : verifying;  
        }

			case set_id:
				if (recv_buffer < 128) {
					target_id = recv_buffer;
					return verifying;
				}
				else return error;

			default: /* unknown command */ break;
		}
		assert(false, 4);
		return finished;
	}

	command_state_t eating_others_data()
	{
		++num_bytes_read;
		switch(cmd_id)
		{
			case data_request:
			case ping:
			case ping_response:
			case set_id_response:
				assert(num_bytes_read == 1, 77);
				return finished;

			case set_id:
				return (num_bytes_read <  2) ? eating : finished;

		  case data_set:
			case data_response:
        if (-1 == exp_num_recv_bytes)
          exp_num_recv_bytes = recv_buffer;
				return (num_bytes_read < exp_num_recv_bytes+1) ? eating : finished;

			default: /* unknown command */ break;
		}
		assert(false, 5);
		return finished;
	}

	command_state_t verify_checksum()
	{
		return (recv_checksum == 0) ? pending : error;
	}

	command_state_t get_sync_bytes()
	{
		if (recv_buffer != 0xFF) {
			sync_state = false;
			return finished;
		}

		if (sync_state) {
			sync_state = false;
			return awaiting;
		}

		sync_state = true;
		return syncing;
	}

	command_state_t search_for_command()
	{
		switch(recv_buffer)
		{
      case 0xE0: /* 1110.0000 */ cmd_id = ping;                    break;
      case 0xE1: /* 1110.0001 */ cmd_id = ping_response;           break;

      case 0x70: /* 0111.0000 */ cmd_id = set_id;                  break;
      case 0x71: /* 0111.0001 */ cmd_id = set_id_response;         break;
  
			case 0xC0: /* 1100.0000 */ cmd_id = data_request;            break;
			case 0xC1: /* 1100.0001 */ cmd_id = data_response;           break;

      case 0x55: /* 0101.0101 */ cmd_id = data_set;                break;

			default: /* unknown command */
				return ignore_cmd;

		} /* switch recv_buffer */

		return get_id;
	}

	/* return code true means continue processing, false: wait for next byte */
	bool receive_command()
	{
  //led::on();
		switch(cmd_state)
		{
			case syncing:
				if (not byte_received()) return false;
				cmd_state = get_sync_bytes();
				break;

			case awaiting:
				if (not byte_received()) return false;
				cmd_state = search_for_command();
				break;

			case get_id:
				if (not byte_received()) return false;
				cmd_state = waiting_for_id();
				break;

			case reading:
				if (not byte_received()) return false;
				cmd_state = waiting_for_data();
				break;

			case eating:
				if (not byte_received()) return false;
				cmd_state = eating_others_data();
				break;

			case verifying:
				if (not byte_received()) return false;
				cmd_state = verify_checksum();
				break;

			case pending:
				cmd_state = process_command();
				break;

			case finished: /* cleanup, prepare for next message */
				send.flush();
				cmd_id = no_command;
				cmd_state = syncing;
				num_bytes_read = 0;
				recv_checksum = 0;
        exp_num_recv_bytes = -1;
				assert(sync_state == false, 55);
				/* anything else todo? */
				break;

			case error:
				if (errors < 0xffff) ++errors;
				led::on();
				send.discard();
				cmd_state = finished;
				break;

      case ignore_cmd:
        cmd_state = finished;
        break;
        
			default: /* unknown command state */
				assert(false, 17);
				break;

		} /* switch cmd_state */
    //led::off();
		return true; // continue
	}

	inline
	void step() {
		while(receive_command());
   //led::off();
	}
};

} /* namespace supreme */

#endif /* SUPREME_COMMUNICATION_HPP */
