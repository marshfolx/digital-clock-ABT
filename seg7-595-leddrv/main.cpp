/* ATtiny13A spec
*    ROM                   1k Byte
*    RAM                   64 Byte
*    GP REG                32 Byte
*
*    MAX CALL STACK DEPTH  32          :  additional 2-byte RAM should be reserved for interrupt routine, thus max available CALL STACK DEPTH - 1.
*/

#define F_CPU (1200000UL)  //1.2Mhz

extern "C" {
	#include <avr/interrupt.h>
	#include <avr/sleep.h>
	#include <avr/pgmspace.h>
	#include <avr/cpufunc.h>
	#include <avr/wdt.h>
	#include <util/delay.h>
	#include <avr/interrupt.h>
	#include <stdint.h>
	#include <assert.h>
}

#include "aaz/aaz.h"
#include "aaz/annex.h"

/* 595 与 DS1302 共用SCLK RCLK/CE DS 三个串行数据传输引脚。
   DS1302 在CE 拉高期间进行数据传输，595 则在RCLK（CE）上升沿时进行一次输出更新，
   这个差异是 RCLK/CE 实现复用的基础：
   595 动态驱动数码管显示时RCLK/CE 只会被短暂拉高以产生一次上升沿，随后立刻拉低，
   并且对595 发出RCLK 上升沿时，DS，SCLK 不会发生操作，因此RCLK/CE 正脉冲不会干扰DS 1302的运作。
   操作DS1302 拉高CE 时会引发595 的一次更新，但此时595 内部寄存器的值并没有被更改，所以不会出现显示错误，
   随后对DS1302 的读写操作会向595 发送错误数据，但正常运行时只会从DS1302 读取一字节分钟数据，
   CE 上升沿只发生一次，错误数据不会被显示。
*/
PIN_USE  SCLK     = PORTB2;
PIN_USE  RCLK_595 = PORTB4;
PIN_USE  DS       = PORTB0;

//PIN_USE  CE_1302  = PORTB4;
PIN_USE  CE_1302  = PORTB4;

//DS_IN read external level after DS is set, otherwise DS_IN is pulled low.
PIN_USE  DS_IN    = PINB0;
PIN_USE  KEY_IN   = PINB3;    //ADC input for key reading.

namespace shiftdrv {
	//led or seg7 led driver using 595,
	//functions can also be used in serial communication to other chip.
	//before sending bits, configure SCLK, RCLK, DS pin as output, keep SCLK, RCLK at low level.
	
	//rclk positive pulse
	inline void rclk_ppulse() {
		aaz::setpin(RCLK_595);
		_NOP();
		aaz::clrpin(RCLK_595);
		//_NOP();
	}

	inline void sclk_ppulse() {
		aaz::setpin(SCLK);
		_NOP();
		//_delay_us(1);
		aaz::clrpin(SCLK);
		//_NOP();
	}
	
	//output a byte form MSB to LSB, each bit write at a positive pulse
	void msb_shift_out(uint8_t a) {
		for(uint8_t i = 8; i; --i) {
			if (a & 0x80)
				aaz::setpin(DS);
			else
				aaz::clrpin(DS);
			a <<= 1;
			sclk_ppulse();
		}
	}
	
	//output a byte form LSB to MSB, each bit write at a positive pulse
	void lsb_shift_out(uint8_t a) {
		for(uint8_t i = 8; i; --i) {
			if (a & 0x01)
				aaz::setpin(DS);
			else
				aaz::clrpin(DS);
			a >>= 1;
			sclk_ppulse();
			//_delay_us(1);
		}
	}	

	inline void double_byte_shift_lsb(uint8_t a, uint8_t b) {
		lsb_shift_out(a);
		lsb_shift_out(b);
	}
}


namespace rtcdrv {
	//RTC DS1302 driver
	inline void start_transfer() {
		aaz::setpin(CE_1302);
		_NOP();
	}
	
	inline void end_transfer() {
		aaz::clrpin(CE_1302);
		_NOP();
	}
	
	//scope guard
	class RtcSession {
	public:
		RtcSession() {
			start_transfer();
		}
		~RtcSession() {
			end_transfer();
		}
	};
	
	/* addr is the command byte, not the exact register address.
	   therefore the addr differs in read and write operation, 
	   read DS1302 datasheet Table3 for detail.
	*/
	void single_write(uint8_t addr, uint8_t data) {
		RtcSession rs;
		shiftdrv::lsb_shift_out(addr);
		shiftdrv::lsb_shift_out(data);
	}
	
	void single_read(uint8_t addr, uint8_t &data_out) {
		RtcSession rs;
		shiftdrv::lsb_shift_out(addr);
		
		aaz::clr_pins_out(DS);
		for(uint8_t i = 8; i; --i) {
			data_out >>= 1;
			if(aaz::test_pin(DS_IN))    //read DS at negative edge.
				data_out |= 0x80;
			
			shiftdrv::sclk_ppulse(); 
		}
		aaz::set_pins_out(DS);
	}
	
	void read_hour(uint8_t &h_out) {
		single_read(0x85, h_out);
	}
	
	void write_hour(uint8_t h) {
		single_write(0x84, h);
	}
	
	void read_minute(uint8_t &m_out) {
		single_read(0x83, m_out);
	}
	
	void write_minute(uint8_t m) {
		single_write(0x82, m);
	}
	
	//set second register to 0 and reset countdown chain, 
	//other register operation should be done in 1 second.
	inline void reset_second() {
		single_write(0x80, 0x00);
	}
	
	inline void clr_write_protection() {
		single_write(0x8e, 0x00);
	}
	
	inline void set_write_protection() {
		single_write(0x8e, 0x80);
	}
}

/* 程序流程
*  -复位
*  -时钟设置模式
*  ---读取RTC
*  ---响应按键，读取ADC 输入
*  ---设置RTC，退出设置模式
*  -显示循环
*  ---watchdog 定时采样RTC
*  ---动态数码管显示
*
*  
*  程序复位后进入时钟设置模式
*  首先读取RTC 中小时H 和分钟M 并显示，此时点击按键A 进入编辑，点击按键B 退出设置模式，进入显示循环。
*  无操作一分钟左右自动退出设置模式，这是考虑到单片机意外复位的情况，但是为了发现并处理意外复位问题，或许应当保持在设置模式，并在设置模式中添加醒目的提示。
*  
*  进入编辑后，共有四位数值需要编辑，分别为[小时], [AM/PM], [分钟-十位], [分钟-个位], 其中小时为16 进制显示。
*  点击按键A 完成一位编辑并切换到下一位，点击按键B 返回上一位，在第四位编辑后，点击按键A，退出设置模式，时钟设置将被应用到RTC。
*  设置RTC 时秒数值将被清零，因此设置时钟时，可等待外部时间秒归零时退出设置模式。
*  
*  使用按键T 编辑数值，数字循环自增。A B T 三个按键均通过ADC 实现，不同按键输入不同电压，从而实现所有按键复用一条输入引脚。
*  没有按键按下时，ADC 输入被拉到Vcc。
*  轮询ADSC， ADC 工作在单次模式，CPU 循环动态驱动数码管，并检查输入电压。
*  
*  进入显示循环后，使用Watchdog 定时读取RTC，获取分钟M 数值，与前一次获取的M 数值比较，当两次结果不同时，假定此时分钟+1，当M 从59 跳到0 时，小时H 自增。
*  
*  RTC 按12小时模式运行，分钟数值在RTC 中，十位和个位分成两位BCD 分别在高四位和低四位存储，单片机内部为便于转换数码管驱动码，将两位BCD 分离到两个字节存储。
*/

struct {
	//MSB of hour is 12-hour mode flag, which is set to one.
	uint8_t hour = 0x83;
	// only low 4 bit is valid, minute number is represented by two BCD.
	uint8_t minute_ten = 0x01;
	uint8_t minute_one = 0x05;
	
} clk_cache; // = {0x83, 0x01, 0x05};

//bit at pm_mark_pos of hour indicates pm(1)/am(0)
constexpr int PM_MARK_POS = 5;


//convert hour number from two BCD to hex format
//only affect low 4 bit, high half as-is.
void hour_bcd_to_hex() {
	//clk_cache.hour = low_half(h);
	if(clk_cache.hour & 0x10)
		clk_cache.hour += 10;
}

uint8_t hour_hex_to_bcd(uint8_t h) {
	if(aaz::low_half(h) > 9) {
		h -= 10;
		h |= 0x10;
	}
	return h;
}

//add 12-hour mode mark
inline void hour_mark_12() {
	clk_cache.hour |= 0x80;
}

inline void toggle_pm_mark() {
	clk_cache.hour = aaz::bit_flip(clk_cache.hour, PM_MARK_POS);
	
}

inline bool at_pm() {
	return static_cast<bool>(clk_cache.hour & (1 << PM_MARK_POS));
}

//number encoding for common anode   0     1     2     3     4     5     6     7     8     9     A     b     C    pm    am
const uint8_t seg7_tbl[] PROGMEM = {0x03, 0x9f, 0x25, 0x0d, 0x99, 0x49, 0x41, 0x1f, 0x01, 0x09, 0x11, 0xc1, 0x63, 0x31, 0x11,};     //reversed, LSB to MSB
//const uint8_t seg7_tbl[] PROGMEM = {0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82, 0xf8, 0x80, 0x90, 0x88, 0x83, 0xc6, 0x8c, 0x88};    //MSB to LSB
constexpr uint8_t SEG7_CODE_HIDE = 0xff;
constexpr auto PM_SIGN_POS = (sizeof(seg7_tbl) - 2);
constexpr auto AM_SIGN_POS = (sizeof(seg7_tbl) - 1);


inline uint8_t seg7_code_of(uint8_t pos) {
	return pgm_read_byte(&seg7_tbl[pos]);
}

uint8_t seg7_display_cache[4] = {0x03, 0x31, 0x03, 0x03};

constexpr uint8_t NUM_POS_HOUR = 3;
constexpr uint8_t NUM_POS_SIGN = 2;
constexpr uint8_t NUM_POS_MINUTE_TEN = 1;
constexpr uint8_t NUM_POS_MINUTE_ONE = 0;
constexpr uint8_t MAX_NUM_POS = NUM_POS_HOUR;

void display_cache_update() {
	if(at_pm())
		seg7_display_cache[NUM_POS_SIGN] = seg7_code_of(PM_SIGN_POS);
	else
		seg7_display_cache[NUM_POS_SIGN] = seg7_code_of(AM_SIGN_POS);
	
	seg7_display_cache[NUM_POS_HOUR] = seg7_code_of(aaz::low_half(clk_cache.hour));
	seg7_display_cache[NUM_POS_MINUTE_TEN] = seg7_code_of(clk_cache.minute_ten);
	seg7_display_cache[NUM_POS_MINUTE_ONE] = seg7_code_of(aaz::low_half(clk_cache.minute_one));
}

void time_number_inc(uint8_t pos) {
	switch(pos) {
		case (NUM_POS_MINUTE_ONE):
			if(clk_cache.minute_one == 9) {
				clk_cache.minute_one = 0;
			}
			else {
				++clk_cache.minute_one;
				break;
			}
		case (NUM_POS_MINUTE_TEN):
			if(clk_cache.minute_ten == 5) {
				clk_cache.minute_ten = 0;
			}
			else {
				++clk_cache.minute_ten;
				break;
			}
		case (NUM_POS_HOUR):
			if(aaz::low_half(clk_cache.hour) == 12) {
				clk_cache.hour &= 0xf0;
				clk_cache.hour |= 0x01;
			}
			else {
				++clk_cache.hour;
				break;
			}
		case (NUM_POS_SIGN):
			toggle_pm_mark();
			break;
	}
}

void minute_inc() {
	time_number_inc(NUM_POS_MINUTE_ONE);
}

void load_clk() {
	rtcdrv::read_hour(clk_cache.hour);
	hour_bcd_to_hex();
	rtcdrv::read_minute(clk_cache.minute_one);
	clk_cache.minute_ten = aaz::high_half(clk_cache.minute_one);
	clk_cache.minute_one &= 0x0f;
	display_cache_update();
}

void upload_clk_config() {
	rtcdrv::reset_second();
	rtcdrv::write_hour(hour_hex_to_bcd(clk_cache.hour));
	rtcdrv::write_minute(clk_cache.minute_ten << 4 | clk_cache.minute_one);
}

void sync_time() {
	uint8_t tmp;
	rtcdrv::read_minute(tmp);
	tmp &= 0x0f;
	if(tmp != clk_cache.minute_one) {
		minute_inc();
		display_cache_update();
	}
}

/*
void display() {
	for(uint8_t i = 0, mask = 0x80; i != 4; ++i, mask >>= 1) {
		shiftdrv::double_byte_shift_lsb(seg7_display_cache[i], mask);
		shiftdrv::rclk_ppulse();
	}
}
*/

void display_with_hide(uint8_t hide_pos) {
	for(uint8_t i = 0, mask = 0x80; i != 4; ++i, mask >>= 1) {
		if(i == hide_pos)
			shiftdrv::double_byte_shift_lsb(SEG7_CODE_HIDE, mask);
		else
			shiftdrv::double_byte_shift_lsb(seg7_display_cache[i], mask);
			
		shiftdrv::rclk_ppulse();
	}
}

inline void display() {
	display_with_hide(0xff);
}


volatile uint8_t timer_interrupt_counter = 0;
volatile bool blink_flag = true;

ISR(iv_wdt) {
	aaz::wdt::reset();
	timer_interrupt_counter++;
	blink_flag = !blink_flag;
	aaz::adc::start();
}

enum class key_code: uint8_t {
	no_key = 0x0,
	key_a,
	key_b,
	key_t,
};

volatile bool key_tapped = false;
volatile key_code key_last = key_code::no_key;
volatile key_code key = key_code::no_key;

ISR(iv_adc) {
	//only use 8-bit of adc result (left aligned), thus the range is [0 - 255]
	//threshold voltage level of each key at 3.3V Vcc:
	//== key		|  threshold volt	| adc result ==
	//  no_key		|     > 2.4			|    185
	//    A			|     < 2.4			|    185
	//    B			|     < 1.6			|    124
	//    T			|     < 0.8			|     62
	
	uint8_t r = ADCH;
	key_last = key;
	if(r < 62) {    //T
		key = key_code::key_t;
	}
	else if(r < 124) {  // B
		key = key_code::key_b;
	}
	else if(r < 185) {    //A
		key = key_code::key_a;
	}		
	else{
		key = key_code::no_key;
	}
	
	if(key != key_last && key_last == key_code::no_key)
		key_tapped = true;
}


void time_edit() {
	int8_t editing_pos = 0;    // editing position at the four values ([ hour | AM/PM | minute_ten | minute_one ])
	constexpr uint8_t editing_blink_time = 10;    //16ms * 31 ≈ 0.5s
	
	while(true) {
		if(timer_interrupt_counter > editing_blink_time)    //number at editing position blink over time.
			display_with_hide(editing_pos);
		else
			display();
	
		aaz::counter_reset_when(timer_interrupt_counter, editing_blink_time * 2);
		
		if(key_tapped) {
			key_tapped = false;
			
			switch(key) {
				case (key_code::key_a):    //A
					if(++editing_pos > MAX_NUM_POS) {
						//send time config
						hour_mark_12();
						upload_clk_config();
						return;
					}
					break;
				case (key_code::key_b):    //B
					if(--editing_pos < 0) {
						//skip_menu
						load_clk();
						return;
					}
					break;
				case (key_code::key_t):    //T
					time_number_inc(editing_pos);
					display_cache_update();
					break;
				
				case (key_code::no_key):
					;
			}
		}
	}
}


int main() {
	using namespace aaz;
	
	wdt::after_sys_reset();
	set_ddr(SCLK, RCLK_595, DS, CE_1302);
	
	//F_CPU = 1.2Mhz  F_ADC = 1200 / 4 = 300kHz
	adc::set_admux(adc::adc_mux::pb3);
	adc::set_align_left();
	//adc::set_prescaler(adc_clkdiv::div_4);
	//adc::enable_interrupt();
	//adc::enable();
	//equivalent operation to the three commented statements above
	adc::set_and_enable(adc::adc_clkdiv::div_4, true);
			
	load_clk();
	rtcdrv::clr_write_protection();
	wdt::run(wdt::wdt_mode::interrupt, wdt::wdt_prescaler::cycle_16ms);
	sei();
	time_edit();
	rtcdrv::set_write_protection();
	
	cli();
	//wdt::mute();
	wdt::run(wdt::wdt_mode::interrupt, wdt::wdt_prescaler::cycle_250ms);
	adc::disable();
	sei();
	
	// NORMAL CLOCK routine
	//AM/PM mark blink overtime, clock sync with ds1302 several times a minute.

	while(true) {
		if(blink_flag)
			display();
		else
			display_with_hide(NUM_POS_SIGN);

		if(counter_reset_when(timer_interrupt_counter, 25)) {
			sync_time();
		}
	}
	
}
