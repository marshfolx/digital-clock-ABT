
#pragma once

#include "io_x.h"

extern "C" {
	#include <avr/wdt.h>
}


/////////////Watchdog

namespace aaz {
	namespace wdt {
		enum class wdt_prescaler : uint8_t {
			//2k cycles
			cycle_16ms = 0x00,
			//4k          8k            16k           32k
			cycle_32ms,  cycle_64ms,   cycle_125ms,  cycle_250ms,
			//64k         128k       256k
			cycle_500ms, cycle_1s, cycle_2s,
			//512k
			cycle_4s = 0x20,
			//1024k
			cycle_8s = 0x21,
		};
		
		//when you use wdt as a normal timer,
		//place wdt reset statement at the first in ISR to avoid unexpected behavior.
		enum class wdt_mode : uint8_t {
			interrupt = _BV(WDTIE),
			reset = _BV(WDE),
			interrupt_reset = _BV(WDTIE) | _BV(WDE),
		};
		
		//'cli()' may be needed before call this
		inline void mute() {
			//wdt_reset();
			WDTCR |= (_BV(WDCE) | _BV(WDE));    //start timed sequence
			WDTCR = 0x00;
		}
		
		//may be necessary at the start of program
		inline void after_sys_reset() {
			MCUSR &= ~_BV(WDRF);
			//watchdog_mute();
		}
		
		//'cli()' may be needed before call this
		inline void run(wdt_mode m, wdt_prescaler p) {
			wdt_reset();
			WDTCR |= (_BV(WDCE) | _BV(WDE));    //start timed sequence
			WDTCR = static_cast<uint8_t>(m) | static_cast<uint8_t>(p);
		}
		
		inline void reset() {
			wdt_reset();
		}
		
		//enable interrupt individually
		//normally no need to call this
		inline void enable_interrupt() {
			WDTCR |= _BV(WDTIE);
		}
		
		//disable interrupt individually
		//in case you want to disable interrupt in interrupt mode.
		inline void disable_interrupt() {
			WDTCR &= ~_BV(WDTIE);
		}
	}
}