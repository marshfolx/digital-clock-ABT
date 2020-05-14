
#pragma once

extern "C" {
	#include <avr/io.h>
}


//iv == interrupt vector

#define iv_adc      (ADC_vect)

#define iv_acmp_trigger      (ANA_COMP_vect)

#define iv_timer0_overflow   (TIM0_OVF_vect)
#define iv_timer0_oca        (TIM0_COMPA_vect)
#define iv_timer0_ocb        (TIM0_COMPA_vect)

#define iv_int0              (INT0_vect)
#define iv_pcint0            (PCINT0_vect)

#define iv_eeprom_ready      (EE_RDY_vect)

#define iv_wdt               (WDT_vect)