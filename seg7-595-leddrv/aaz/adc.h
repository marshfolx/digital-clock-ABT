
#pragma once

#include "io_x.h"


namespace aaz {
	namespace adc {
		//Analog to Digit Converter
		enum class adc_trigger : uint8_t {
			free_running = 0x00,
			acmp,             //analog comparator
			int0,             //external int0
			t0_oca,           //timer0_output_compare_a - timer0_compare_match_a
			t0_ovf,           //timer0_overflow
			t0_ocb,
			pcint,            //pin change interrupt
		};
		
		/* A single conversion costs around 15 ADC clock cycles,
		 * while the first conversion after ADC enabled will cost around 49 cycles,
		 * In auto triggered mode, a single conversion costs around 16 ADC cycles.
		 *
		 * official datasheet said ADC clock frequency should be at range of 50~200kHz,
		 * while if lower precision than 10-bit is acceptable, clock frequency can be higher than 200kHz
		 *
		 * if adc is clocked at 100kHz, the first conversion need around 0.5ms,
		 * and a normal conversion need 0.15ms.
		*/
		enum class adc_clkdiv : uint8_t {
			div_2 = 0x01,
			div_4, div_8, div_16,
			div_32, div_64, div_128,
		};

		enum class adc_mux : uint8_t {
			pb5 = 0x00,  //ADC0
			pb2,         //ADC1
			pb4,         //ADC2
			pb3,         //ADC3
		};

		inline void enable() {
			ADCSRA |= _BV(ADEN);
		}

		inline void disable() {
			ADCSRA &= ~_BV(ADEN);
		}

		constexpr uint8_t calc_adcsra_cfg(bool aden, adc_clkdiv ckdv, bool interrupt_enable, bool auto_trigger) {
			return ((aden) ? _BV(ADEN) : 0) | ((auto_trigger) ? _BV(ADATE) : 0) | ((interrupt_enable) ? _BV(ADIE) : 0) | static_cast<uint8_t>(ckdv);
		}

		//adc turn on and config
		inline void set_and_enable(adc_clkdiv ckdv, bool interrupt_enable = false, bool auto_trigger = false) {
			ADCSRA = calc_adcsra_cfg(true, ckdv, interrupt_enable, auto_trigger);
		}

		inline void enable_interrupt() {
			ADCSRA |= _BV(ADIE);
		}

		inline void disable_interrupt() {
			ADCSRA &= ~_BV(ADIE);
		}

		inline void set_align_left() {
			ADMUX |= _BV(ADLAR);
		}
		
		//¡ù default config
		inline void set_align_right() {
			ADMUX &= ~_BV(ADLAR);
		}
		
		inline void set_aref_internal() {
			ADMUX |= _BV(REFS0);
		}
		
		//¡ù default config
		inline void set_aref_vcc() {
			ADMUX &= ~_BV(REFS0);
		}

		inline void set_mux(adc_mux mx) {
			ADMUX &= ~(_BV(MUX1) | _BV(MUX0));
			if(mx == adc_mux::pb5)    //branch will be optimized after compile when mx is compile-time constant.
				return;
			ADMUX |= static_cast<uint8_t>(mx);
		}

		constexpr uint8_t calc_admux_cfg(adc_mux mx, bool left_align = true, bool internal_aref = false) {
			return static_cast<uint8_t>(mx) | ((left_align) ? _BV(ADLAR) : 0) | ((internal_aref) ? _BV(REFS0) : 0);
		}
		
		//use this function to config all control bit in ADMUX register in one assignment operation.
		inline void set_admux(adc_mux mx, bool left_align = true, bool internal_aref = false) {
			ADMUX = calc_admux_cfg(mx, left_align, internal_aref);
		}

		inline void set_prescaler(adc_clkdiv ckdv) {
			ADCSRA &= ~(_BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0));
			if (ckdv == adc_clkdiv::div_2)
			return;
			ADCSRA |= static_cast<uint8_t>(ckdv);
		}

		//set adc auto trigger source,
		//!!- clear ACME simultaneously -!!
		inline void set_trigger(adc_trigger tg) {
			ADCSRB = static_cast<uint8_t>(tg);
		}

		inline void enable_auto_trigger(adc_trigger tg) {
			ADCSRA |= _BV(ADATE);
			set_trigger(tg);
		}

		inline void enable_auto_trigger() {
			ADCSRA |= _BV(ADATE);
		}

		inline void disable_auto_trigger() {
			ADCSRA &= ~_BV(ADATE);
		}

		inline void enable_free_running() {
			enable_auto_trigger(adc_trigger::free_running);
		}

		inline void disable_free_running() {
			disable_auto_trigger();
		}

		inline void start() {
			ADCSRA |= _BV(ADSC);
		}
		
	}
}

