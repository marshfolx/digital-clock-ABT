#pragma once

#include "io_x.h"

extern "C" {
	#include <avr/eeprom.h>
}

namespace aaz {
	namespace eep {
		
		//note that all mode enum(s) in aaz has the default item with value of 0x0,
		//which is the default value in related control register after reset.
		enum class eeprom_write_mode: uint8_t {
			erase_and_write = 0x0,
			erase_only      = 0x10,
			write_only      = 0x20,
		};
		
		inline void set_write_mode(eeprom_write_mode m) {
			EECR &= ~(_BV(EEPM1) | _BV(EEPM0));
			if(m == eeprom_write_mode::erase_and_write)
				return;
			EECR |= static_cast<uint8_t>(m);
		}
		
		inline void enable_interrupt() {
			EECR |= _BV(EERIE);
		}

		inline void disable_interrupt() {
			EECR &= ~_BV(EERIE);
		}
		
		inline bool now_busy() {
			return static_cast<bool>(EECR & _BV(EEPE));
		}
		
		//read a byte from specific address, 
		//eeprom busy check may be needed prior.
		inline uint8_t read_at(uint8_t addr) {
			EEARL = addr;
			return EEDR;
		}
		
		//void read_duoble_byte_at(uint8_t addr, uint16_t &out_ptr) {
			//uint8_t * const tmp_ptr = reinterpret_cast<uint8_t*>(&out_ptr);
			//*(tmp_ptr) = read_at(addr);
			//*(tmp_ptr + 1) = read_at(addr + 1);
		//}
		//
		//void read_float_at(uint8_t addr, float &out_ptr) {
			//assert(sizeof(float) == 4);
			//uint8_t *tmp_ptr = reinterpret_cast<uint8_t*>(&out_ptr);
			//*(tmp_ptr) = read_at(addr);
			//*(++tmp_ptr) = read_at(++addr);
			//*(++tmp_ptr) = read_at(++addr);
			//*(++tmp_ptr) = read_at(++addr);
			//
		//}
		
		//request for a eeprom write operation at a specified address.
		//poll EEPE or use EEPROM ready interrupt to detect write completion.
		//eeprom busy check may be needed prior.
		void write_request_at(uint8_t addr) {
			EEARL = addr;
			EECR |= (_BV(EEMPE) | _BV(EEPE));
		}
		
		//read byte from eeprom data register.
		inline uint8_t get_val() {
			return EEDR;
		}
		
		//write byte to eeprom data register.
		inline void put_val(uint8_t val) {
			EEDR = val;
		}
		
		
	}
}