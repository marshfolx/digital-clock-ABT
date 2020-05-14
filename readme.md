# Clock ABT

<p style="text-align:center">A digital clock with three buttons: A, B, and T.</p> 

![](https://raw.githubusercontent.com/marshfolx/pics/master/Photo_0514_1a-m.jpg)

## Components

1. ATtiny13A.
2. A DIY board with a push button of RESET. 
3. DS1302 RTC module.
4. 4-bit 7-segment LED module, drived by two 74xx595 chips.
5. Three additional push buttons used to config the clock, and a board to place them.

## Requirements

1. [USBasp](https://www.fischl.de/usbasp/) or any other programmer.
2. Atmel Studio 7.

## Circuit

The circuit is pretty simple, here is the simplified diagram demonstrating the connections among main components:

![](https://raw.githubusercontent.com/marshfolx/pics/master/%E6%89%B9%E6%B3%A8%202020-05-15%20013050.jpg)

## Buttons

Buttons are used to adjust time manully. 

It will enter in time edit mode after the MCU reset, use button A and B to move editing position left and right and select which bit to edit,  button T is used to increment the number at editting position.

Tiny13a has not enough IO pins to connect three buttons individually. Here is a trick which needs one wire only: the voltage divider circuit. The circuit output different voltage when a button is pressed, thus the button pressed can be figured out when the V_out  is read by ADC. The ADC input pin is labeled KEY_IN.

## DS1302 And 595

These two modules are connected together to reduce IO occupation. The principal is that the DS1302 needs a steady high signal on CE line during operation, once the signal goes low the operation is terminated. While 595 only needs a positive pulse to output data in the shift register. The pulse can be short enough to avoid triggering DS1302 and SCLK and DS can remains high or low during the pulse,  which means operation to 595 will not interfere DS1302. 

Of course the data in 595 will be messed up when transferring data with DS1302, it doesn't matters, you just write again after the transfer is done.

## Program and Display Format

Hour is displayed as a hex number, thus 'A' means 10 clock. A mark of AM/PM showed to the right, and minute is two decimal numbers.

You can see there is still a pin remains unconnected, which allows further expansion, as long as the flash space is enough ... 

![](https://raw.githubusercontent.com/marshfolx/pics/master/%E6%89%B9%E6%B3%A8%202020-05-15%20023111.jpg)

which has only 4 bytes left.

If you use ATtiny25 or higher, an alarm feature should be easily implemented.



And what is that **aaz** stuff ?  That is a thin wrap library which hides mosly all special register operaions behind inline functions with zero overhead, to cure the pain of my human memory and enhance the readiability, I hope.