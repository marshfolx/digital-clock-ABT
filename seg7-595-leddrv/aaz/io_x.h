
#pragma once


extern "C" {
	#include <avr/io.h>
	#include <avr/sfr_defs.h>
	#include <stdint.h>
}

#ifndef PIN_USE
	#define PIN_USE constexpr auto
#endif

namespace aaz {
	
	inline void setpin(uint8_t pin) {
		PORTB |= _BV(pin);
	}

	inline void clrpin(uint8_t pin) {
		PORTB &= ~_BV(pin);
	}

	inline void toggle_pin(uint8_t pin) {
		PINB |= _BV(pin);
	}
	
	inline bool test_pin(uint8_t pin) {
		return bit_is_set(PINB, pin);
	}
	
	inline void disable_pullup() {
		MCUCR |= _BV(PUD);
	}
	
	//¡ù default
	inline void enable_pullup() {
		MCUCR &= _BV(PUD);
	}
	
	
	//these red underline should be ignored,
	//avrgcc works fine with these templates.
		
	template<typename T0>
	constexpr uint8_t calc_port_cfg(T0 pin) {
		return _BV(pin);
	}
	
	template <typename T0, typename... Ts>
	constexpr uint8_t calc_port_cfg(T0 pin, Ts... args) {
		return _BV(pin) | calc_port_cfg(args...);
	}
	
	template<typename... Ts>
	inline void setpins(Ts... pins) {
		PORTB |= calc_port_cfg(pins...);
	}
	
	template<typename... Ts>
	inline void clrpins(Ts... pins) {
		PORTB &= calc_port_cfg(pins...);
	}		

	template<typename... Ts>
	inline void toggle_pins(Ts... pins) {
		PINB |= calc_port_cfg(pins...);
	}


	//set specified pins output in DDRB
	template <typename... Ts>
	inline void set_pins_out(Ts... pins) {
		DDRB |= calc_port_cfg(pins...);
	}
	
	//set specified pins input in DDRB
	template <typename... Ts>
	inline void clr_pins_out(Ts... pins) {
		DDRB &= ~calc_port_cfg(pins...);
	}
	
	//set specified pins output, clr others.
	template <typename... Ts>
	inline void set_ddr(Ts... pins) {
		DDRB = calc_port_cfg(pins...);
	}
	
	
	constexpr uint8_t high_half(uint8_t b) {
		return (b & 0xf0) >> 4;
	}
	
	constexpr uint8_t low_half(uint8_t b) {
		return b & 0x0f;
	}
	
	inline uint8_t bit_flip(uint8_t &b, uint8_t pos) {
		return b ^= (1 << pos);
	}
}