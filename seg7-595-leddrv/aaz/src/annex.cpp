
#include "../annex.h"
//#include <stdint.h>

bool aaz::counter_reset_when(volatile uint8_t &counter_out, uint8_t max) {
	if(counter_out == max) {
		counter_out = 0;
		return true;
	}
	return false;
}