
#pragma once

#include "io_x.h"

extern "C" {
	#include <assert.h>
}

#ifndef F_CPU
# warning "F_CPU should be defined to use timer0 related calculate functions."
#endif

/////////////Timer0

namespace aaz {
	namespace t0 {
		//div_256 = 0x84 ==> 256 = 2^8,
		//only low half will actually be assigned to register.
		//high half represents the clock divide count which = 2^n.
		enum class timer0_clkdiv : uint8_t {
			div_1    = 0x01,
			div_8    = 0x32,
			div_64   = 0x63,
			div_256  = 0x84,
			div_1024 = 0xa5,
		};
		
		enum class waveform_mode : uint8_t {
			normal = 0x00,
			pwm_center,    //center aligned or 'phase correct'
			ctc,           //Clear Timer on Compare match
			pwm_edge,      //edge aligned or 'fast'
		};
		
		enum class compare_output_mode : uint8_t {
			disconnect = 0x00,
			toggle,
			clear,
			set,
		};
		
		//oc0a[PB0] compare output mode
		inline void set_oc0a_mode(compare_output_mode m) {
			TCCR0A &= 0b00111111;
			TCCR0A |= static_cast<uint8_t>(m) << 6;
		}
		
		//oc0b[PB1] compare output mode
		inline void set_oc0b_mode(compare_output_mode m) {
			TCCR0A &= 0b11001111;
			TCCR0A |= static_cast<uint8_t>(m) << 4;
		}
		
		//OCR0x is double-buffed under PWM mode
		inline void set_ocr0a_val(uint8_t val) {
			OCR0A = val;
		}
		
		inline void set_ocr0b_val(uint8_t val) {
			OCR0B = val;
		}
		
		inline void set_mode_normal() {
			TCCR0A &= ~(_BV(WGM01) | _BV(WGM00));
			TCCR0B &= ~_BV(WGM02);
		}
		
		inline void set_waveform_mode(waveform_mode m, bool pwm_with_ocra_as_top = false) {
			//branch will be optimized and function will be inline
			//when arguments is  compile-time constant.
			TCCR0A &= ~(_BV(WGM01) | _BV(WGM00));
			if(m != waveform_mode::normal)
				TCCR0A |= static_cast<uint8_t>(m);
			
			if(pwm_with_ocra_as_top) {
				TCCR0B |= _BV(WGM02);
			}
			else {
				TCCR0B &= ~_BV(WGM02);
			}
		}
		
		inline void stop() {
			TCCR0B &= ~(_BV(CS02) | _BV(CS01) | _BV(CS00));
		}
		
		/* clock prescaler must be set explicitly,
		*  no RAM space will be used to store prescaler setting,
		*  user should keep it in mind.
		*/
		inline void start_at(timer0_clkdiv ckdv) {
			TCCR0B &= ~(_BV(CS02) | _BV(CS01) | _BV(CS00));
			TCCR0B |= low_half(static_cast<uint8_t>(ckdv));
		}

		inline void enable_overflow_interrupt() {
			TIMSK0 |= _BV(TOIE0);
		}
		
		inline void disable_overflow_interrupt() {
			TIMSK0 &= ~_BV(TOIE0);
		}
		
		constexpr uint8_t calc_timer0_intmask(bool timer0_ovf, bool compare_match_a, bool compare_match_b) {
			return (timer0_ovf ? _BV(TOIE0) : 0) | (compare_match_a ? _BV(OCIE0A) : 0) | (compare_match_b ? _BV(OCIE0B) : 0);
		}
		
		inline void set_interrupt_mask(bool timer0_ovf, bool compare_match_a = false, bool compare_match_b = false) {
			TIMSK0 = calc_timer0_intmask(timer0_ovf, compare_match_a, compare_match_b);
		}
		
		inline void set_val(uint8_t init_val) {
			TCNT0 = init_val;
		}
		
		constexpr uint32_t calc_prescale(timer0_clkdiv ckdv) {
			return 1 << high_half(static_cast<uint8_t>(ckdv));
		}
		
		//return as (XXXX)Hz
		constexpr uint32_t calc_freq(timer0_clkdiv ckdv) {
			return F_CPU / calc_prescale(ckdv);
		}
		
		//return as millisecond
		constexpr float calc_period(timer0_clkdiv ckdv) {
			return 1000.0 / calc_freq(ckdv);
		}
		
		constexpr float calc_max_duration(timer0_clkdiv ckdv) {
			return calc_period(ckdv) * 256;
		}
		
		//call calc_max_timer0_duration() before calc init value.
		constexpr uint8_t calc_init_val(float ms, timer0_clkdiv ckdv) {
			return static_cast<uint8_t>(ms / calc_period(ckdv));
		}
		
		//set timer0 initial value base on counting duration and F_CPU
		inline void set_duration(uint16_t ms, timer0_clkdiv ckdv) {
			assert(ms < calc_max_duration(ckdv));
			set_val(calc_init_val(ms, ckdv));
		}
		
		
		/////////////Timer0 Prescaler
		inline void reset_prescaler() {
			GTCCR |= _BV(PSR10);
		}
		
		//halt timer0
		inline void freeze() {
			GTCCR |= _BV(TSM);
		}
		
		inline void freeze_and_reset() {
			GTCCR |= _BV(TSM) | _BV(PSR10);
		}
		
		//restart timer0
		inline void unfreeze() {
			GTCCR &= ~_BV(TSM);
		}
	}
}