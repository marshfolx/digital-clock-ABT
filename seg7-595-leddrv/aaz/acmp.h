
#pragma once

#include "io_x.h"
#include "adc.h"


namespace aaz {
	namespace acmp {    
		//Analog Comparator
		// ac interrupt trigger mode
		enum class acmp_trigger: uint8_t {
			on_change = 0x0,
			on_falling = 0x02,
			on_rising = 0x03,
		};
		
		inline void set_mux(adc::adc_mux mx) {
			adc::set_mux(mx);
		}
		
		// enable AC(Analog Comparator) negative input MUX and select.
		// note that analog comparator mux can only be enabled when adc is disabled.
		inline void enable_mux(adc::adc_mux mx) {
			adc::set_mux(mx);
			ADCSRB |= _BV(ACME);
		}
		
		inline void disable_mux() {
			ADCSRB &= ~_BV(ACME);
		}
		
		inline void disable() {
			ACSR |= _BV(ACD);
		}
		
		inline void enable() {
			ACSR &= ~_BV(ACD);
		}
		
		inline void enable_interrupt() {
			ACSR |= _BV(ACIE);
		}
		
		inline void disable_interrupt() {
			ACSR &= ~_BV(ACIE);
		}
		
		inline void set_trigger(acmp_trigger tg) {
			ACSR &= ~(_BV(ACIS1) | _BV(ACIS0));
			if (tg == acmp_trigger::on_change)
			ACSR |= static_cast<uint8_t>(tg);
		}
		
		constexpr uint8_t calc_acsr_cfg(acmp_trigger tg, bool interrupt_enable, bool ref_bandgap, bool enable) {
			return static_cast<uint8_t>(tg) | ((interrupt_enable) ? _BV(ACIE) : 0) | ((ref_bandgap) ? _BV(ACBG) : 0) | (disable ? _BV(ACD) : 0);
		}
		
		//ACSR = Analog Comparator Control & Status Register
		//use this function to set all control bit in one assignment operation.
		inline void set_acsr(acmp_trigger tg, bool interrupt_enable = true, bool ref_bandgap = false, bool enable = true) {
			ACSR = calc_acsr_cfg(tg, interrupt_enable, ref_bandgap, enable);
		}
	}
}