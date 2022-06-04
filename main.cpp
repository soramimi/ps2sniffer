
// Quckey3 - Copyright (C) 2020 S.Fuchita (@soramimi_jp)

/* USB Keyboard Firmware code for generic Teensy Keyboards
 * Copyright (c) 2012 Fredrik Atmer, Bathroom Epiphanies Inc
 * http://bathroomepiphanies.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "lcd.h"
#include "usb.h"
#include <avr/interrupt.h>
#include <string.h>
#include "waitloop.h"

#define CLOCK 16000000UL
#define SCALE 125
static unsigned short _scale = 0;
//static unsigned long _system_tick_count;
//static unsigned long _tick_count;
//static unsigned long _time_s;
//static unsigned short _time_ms = 0;
uint8_t interval_1ms_flag = 0;
ISR(TIMER0_OVF_vect, ISR_NOBLOCK)
{
//	_system_tick_count++;
	_scale += 16;
	if (_scale >= SCALE) {
		_scale -= SCALE;
//		_tick_count++;
//		_time_ms++;
//		if (_time_ms >= 1000) {
//			_time_ms = 0;
//			_time_s++;
//		}
		interval_1ms_flag = 1;
	}
}

extern "C" void led(uint8_t f)
{
	if (f) {
		PORTB |= 0x01;
	} else {
		PORTB &= ~0x01;
	}
}






extern "C" uint8_t usb_read_available_();
extern "C" uint8_t usb_read_byte_();

uint8_t data_tx_buffer[64];
uint8_t data_tx_buffer_i;
uint8_t data_tx_buffer_n;

uint8_t data_rx_buffer[256];
int data_rx_buffer_i;
int data_rx_buffer_n;

extern "C" void clear_buffers()
{
	data_tx_buffer_i = 0;
	data_tx_buffer_n = 0;
	data_rx_buffer_i = 0;
	data_rx_buffer_n = 0;
}

void usb_poll_tx()
{
	uint8_t tmp[TX_EP_SIZE];
	while (1) {
		int n = data_tx_buffer_n;
		n = n < sizeof(tmp) ? n : sizeof(tmp);
		if (n == 0) break;
		for (uint8_t i = 0; i < data_tx_buffer_n; i++) {
			tmp[i] = data_tx_buffer[data_tx_buffer_i];
			data_tx_buffer_i = (data_tx_buffer_i + 1) % sizeof(data_tx_buffer);
		}
		usb_data_tx(tmp, n);
		data_tx_buffer_n -= n;
	}
}


static inline void usb_poll_rx()
{
	int space = sizeof(data_rx_buffer) - 1 - data_rx_buffer_n;
	while (space > 0) {
		uint8_t tmp[16];
		int n = sizeof(tmp);
		n = usb_data_rx(tmp, n < space ? n : space);
		if (n == 0) break;
		for (uint8_t i = 0; i < n; i++) {
			int j = (data_rx_buffer_i + data_rx_buffer_n) % sizeof(data_rx_buffer);
			data_rx_buffer[j] = tmp[i];
			data_rx_buffer_n++;
		}
		space -= n;
	}
}

void usb_poll()
{
	usb_poll_tx();
	usb_poll_rx();
}

static inline int usb_read_available()
{
	return data_rx_buffer_n + usb_read_available_();
}

static inline uint8_t usb_read_byte()
{
	for (int i = 0; i < 2; i++) {
		if (data_rx_buffer_n > 0) {
			uint8_t c = data_rx_buffer[data_rx_buffer_i];
			data_rx_buffer_i = (data_rx_buffer_i + 1) % sizeof(data_rx_buffer);
			data_rx_buffer_n--;
			return c;
		}
		usb_poll_rx();
	}
	return 0;
}

void usb_write_byte(char c)
{
//	return;

	while (1) {
		if (data_tx_buffer_n < sizeof(data_tx_buffer)) {
			int8_t i = (data_tx_buffer_i + data_tx_buffer_n) % sizeof(data_tx_buffer);
			data_tx_buffer[i] = c;
			data_tx_buffer_n++;
			if (data_tx_buffer_n >= TX_EP_SIZE) {
				usb_poll_tx();
			}
			return;
		}
		usb_poll();
	}
}





void keyboard_setup();
void ps2_loop();

#ifdef LCD_ENABLED

enum {
	LCD_NONE = 0,
	LCD_HOME = 0x0c,
};

uint8_t lcd_queue[128];
uint8_t lcd_head = 0;
uint8_t lcd_size = 0;

extern "C" void lcd_putchar(uint8_t c)
{
	cli();
	if (lcd_size < 128 && c != 0) {
		lcd_queue[(lcd_head + lcd_size) & 0x7f] = c;
		lcd_size++;
	}
	sei();
}

extern "C" void lcd_print(char const *p)
{
	while (*p) {
		lcd_putchar(*p++);
	}
}

extern "C" void lcd_puthex8(uint8_t v)
{
	static char hex[] = "0123456789ABCDEF";
	lcd_putchar(hex[(v >> 4) & 0x0f]);
	lcd_putchar(hex[v & 0x0f]);
}

extern "C" void lcd_home()
{
	lcd_putchar(LCD_HOME);
}

static uint8_t lcd_popfront()
{
	uint8_t c = 0;
	cli();
	if (lcd_size > 0) {
		c = lcd_queue[lcd_head];
		lcd_head = (lcd_head + 1) & 0x7f;
		lcd_size--;
	}
	sei();
	return c;
}

#endif //  LCD_ENABLED


void putchar(uint8_t c)
{
	usb_write_byte(c);
}

void print(char const *p)
{
	while (*p) {
		putchar(*p);
		p++;
	}
}

void print_hex(uint8_t c)
{
	static char hex[] = "0123456789ABCDEF";
	putchar(hex[(c >> 4) & 0x0f]);
	putchar(hex[c & 0x0f]);
}

void print_crlf()
{
	putchar('\r');
	putchar('\n');
}

void report_host_to_device(uint8_t c)
{
	print("H ");
	print_hex(c);
	print(" ->    D");
	print_crlf();

}

void report_device_to_host(uint8_t c)
{
	print("H    <- ");
	print_hex(c);
	print(" D");
	print_crlf();
}

void setup()
{
	// 16 MHz clock
	CLKPR = 0x80; CLKPR = 0;
	// Disable JTAG
	MCUCR |= 0x80; MCUCR |= 0x80;

	PORTB = 0x00;
	PORTC = 0x00;
	DDRB = 0x01;
	DDRC = 0x04;

	TCCR0B = 0x02; // 1/8 prescaling
	TIMSK0 |= 1 << TOIE0;

	usb_init();
	while (!is_usb_configured()) {
		msleep(10);
	}

	keyboard_setup();

#ifdef LCD_ENABLED
	lcd::init();
	lcd::clear();
	lcd::home();
	lcd_print("Quckey3");
#endif
}

void loop()
{
	ps2_loop();
}

int main()
{
	setup();
	sei();
	while (1) {
		usb_poll();
#ifdef LCD_ENABLED
		uint8_t c = lcd_popfront();
		if (c == LCD_NONE) {
			loop();
		} else if (c < ' ') {
			if (c == LCD_HOME) {
				lcd::home();
			}
		} else {
			lcd::putchar(c);
		}
#else
		loop();
#endif
	}
}

