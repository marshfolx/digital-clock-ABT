
#pragma once

#include "io_x.h"

extern "C" {
	#include <avr/sleep.h>
}


namespace aaz {
	//////////Clock and Power Mgr
	enum class sys_clkdiv : uint8_t {
		div_1 = 0x00, 
		div_2, div_4, div_8, div_16,
		div_32, div_64, div_128, div_256,
	};

	inline void set_sys_clk_prescaler(sys_clkdiv ckdv) {
		CLKPR = 0x80;
		CLKPR = static_cast<uint8_t>(ckdv);
	}

	enum class sleep_mode_enum : uint8_t {
		idle = 0x00,
		adc_noise_reduction, power_down,
	};

	inline void set_sleep_mode_as(sleep_mode_enum mode) {
		MCUCR = ((MCUCR & ~(_BV(SM0) | _BV(SM1))) | static_cast<uint8_t>(mode));
	}

	inline void sleep() {
		MCUCR |= _BV(SE);
		sleep_cpu();
		MCUCR &= ~_BV(SE);
	}

	inline void shutdown_adc() {
		PRR |= _BV(PRADC);
	}

	inline void wake_up_adc() {
		PRR &= ~_BV(PRADC);
	}

	inline void shutdown_timer0() {
		PRR |= _BV(PRTIM0);
	}

	inline void wake_up_timer0() {
		PRR &= ~_BV(PRTIM0);
	}
}