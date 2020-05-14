
#pragma once

#include "io_x.h"
#include "timer0.h"

extern "C" {
	#include <assert.h>
}

namespace aaz{
	namespace wavegen {
		//        ///Wave gen Util - CTC(50% square wave) - PWM
		
		/*OC0A => PB0, OC0B => PB1
		* set timer0 value to 0 first.
		*
		* freq_ctc          = f_cpu / ( 2 * timer0_clkdiv * (1 + OCR0x) );
		*
		* freq_buffered_ctc = f_cpu / ( timer0_clkdiv * 256 )             (WGM 3);
		*                   = f_cpu / ( timer0_clkdiv * (1 + OCR0A) )     (WGM 7);
		*
		* freq_fast_pwm     = f_cpu / ( timer0_clkdiv * 256 )             (WGM 3);
		*                   = f_cpu / ( timer0_clkdiv * (1 + OCR0A) )     (WGM 7 - ocr0a as TOP);
		*
		* duty_ratio_fast   = (OCR0x + 1) / (256)
		*                   = (OCR0B + 1) / (OCR0A + 1)
		*
		* freq_center_pwm   = f_cpu / ( timer0_clkdiv * 510 )             (WGM 1);
		*                   = f_cpu / ( timer0_clkdiv * 2 * OCR0A )       (WGM 5 - ocr0a as TOP);
		*
		* duty_ratio_center = OCR0x / 255
		*                   = OCR0B / OCR0A
		*
		* call ctc_xxxx_at / pwm_xxxx_at to config, then enable_oc0x_output to start wave output,
		* output wont apply when the pin is in input mode.
		*
		* pwm duty ratio and ctc frequency setting depending on runtime data
		* cannot use these utility functions,
		* as the calculate functions are all constexpr which requires constant arguments.
		*/
		
		/* 	时钟分频器与比较器值同时影响输出频率，而时钟分频器的设置具有全局影响力，
		*	因此应当手动设置，并配合参数上下限 assert。
		*   同时waveform gen mode 也是全局性的设置，由OCR0A/B 共享，因此也需要显式设置，不能和OCR0x 设置捆绑
		*/

		inline void clear_wavegen() {
			t0::set_mode_normal();
		}
		
		inline void clear_oc0x_output() {
			DDRB &= ~(_BV(PB0) | _BV(PB1));
		}
		
		constexpr uint8_t calc_oc0x_ddr_cfg(bool oc0a_out, bool oc0b_out) {
			return (oc0a_out ? _BV(PB0) : 0) | (oc0b_out ? _BV(PB1) : 0);
		}
		
		inline void enable_oc0x_output(bool oc0a_out, bool oc0b_out=false) {
			DDRB |= calc_oc0x_ddr_cfg(oc0a_out, oc0b_out);
		}
		
		//    ///CTC
		constexpr float max_ctc_frequency(t0::timer0_clkdiv ckdv) {
			return t0::calc_freq(ckdv) / 2;
		}
		
		constexpr uint8_t calc_ctc_initval(float freq, t0::timer0_clkdiv ckdv) {
			return static_cast<uint8_t>(calc_freq(ckdv) / (2 * freq) - 1);
		}
		
		inline void ctc_oc0a_at(float freq, t0::timer0_clkdiv ckdv) {
			assert(freq <= max_ctc_frequency(ckdv));
			OCR0A = calc_ctc_initval(freq, ckdv);
		}
		
		inline void ctc_config_oc0a(float freq, t0::timer0_clkdiv ckdv) {
			ctc_oc0a_at(freq, ckdv);
			t0::set_oc0a_mode(t0::compare_output_mode::toggle);
		}
		
		inline void ctc_oc0b_at(float freq, t0::timer0_clkdiv ckdv) {
			assert(freq <= max_ctc_frequency(ckdv));
			OCR0B = calc_ctc_initval(freq, ckdv);
		}
		
		inline void ctc_config_oc0b(float freq, t0::timer0_clkdiv ckdv) {
			ctc_oc0b_at(freq, ckdv);
			t0::set_oc0b_mode(t0::compare_output_mode::toggle);
		}
		
		//     ///PWM - fast
		// may be useful when printed by compiler.
		constexpr float calc_pwm_fast_frequency(t0::timer0_clkdiv ckdv) {
			return t0::calc_freq(ckdv) / 256;
		}
		
		constexpr uint8_t calc_duty_ratio_fast(float duty_ratio) {
			return ( (256 * duty_ratio > 1) ? (static_cast<uint8_t>(256 * duty_ratio) - 1) : 0);
		}
		
		inline void pwm_oc0a_at(float duty_ratio) {
			t0::set_ocr0a_val(calc_duty_ratio_fast(duty_ratio));
		}
		
		inline void pwm_config_oc0a(float duty_ratio) {
			pwm_oc0a_at(duty_ratio);
			t0::set_oc0a_mode(t0::compare_output_mode::clear);
		}

		inline void pwm_oc0b_at(float duty_ratio) {
			t0::set_ocr0b_val(calc_duty_ratio_fast(duty_ratio));
		}

		inline void pwm_config_oc0b(float duty_ratio) {
			pwm_oc0b_at(duty_ratio);
			t0::set_oc0b_mode(t0::compare_output_mode::clear);
		}
		
		//oc0a pwm toggle 模式50% 方波，具有比较器值缓冲。
		inline void pwm_ctc_config_oc0a_at() {
			t0::set_oc0a_mode(t0::compare_output_mode::toggle);
			t0::set_ocr0a_val(127);
		}
		
		//     ///PWM - center
		// TODO: functions for Fast PWM may be compatible with Center Aligned PWM
		//       except calc_duty_ratio_xxxx probably.
		
		//may be useful when printed by compiler.
		constexpr float calc_pwm_center_frequency(t0::timer0_clkdiv ckdv) {
			return calc_freq(ckdv) / 510;
		}
	}
}