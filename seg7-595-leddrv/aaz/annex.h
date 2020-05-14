
#pragma once

#include "io_x.h"
namespace aaz {
	
	/* perform like this:
	* if(counter == max)
	*     counter = 0;
	*
	* NORMAL USEAGE:
	*
	* if(counter_reset_when(counter, max)) {
	*     //do things when counter == max...
	* }
	* 
	* use this in case you forget to reset counter.
	* takes some function call cost of course.
	*/
	bool counter_reset_when(volatile uint8_t &counter_out, uint8_t max);
}