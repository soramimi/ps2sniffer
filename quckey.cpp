
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "queue16.h"
#include "ps2.h"
#include "ps2if.h"
#include "waitloop.h"
#include <stdlib.h>
#include "lcd.h"

typedef struct Queue16 Queue;
#define qinit(Q) q16init(Q)
#define qpeek(Q) q16peek(Q)
#define qget(Q) q16get(Q)
#define qput(Q,C) q16put(Q,C)
#define qunget(Q,C) q16unget(Q,C)

extern "C" void led(uint8_t f);

extern uint8_t interval_1ms_flag; // 1ms interval event

struct PS2IF {
	AbstractPS2IO *io;
	uint16_t input_bits;
	uint16_t output_bits;
	uint8_t timeout;
	Queue input_queue;
	Queue output_queue;
	Queue event_queue_l;
	Queue event_queue_h;
};

PS2DeviceIO ps2d_io;
PS2HostIO ps2h_io;

PS2IF ps2h;
PS2IF ps2d;

uint8_t countbits(uint16_t c)
{
	uint8_t i;
	for (i = 0; c; c >>= 1) {
		if (c & 1) i++;
	}
	return i;
}

bool ps2d_next_output(PS2IF *dev, uint8_t c)
{
	uint16_t d = c;
	if (!(countbits(d) & 1)) d |= 0x100;	// make odd parity
	d = (d | 0x600) << 1;
	//
	// d = 000011pdddddddd0
	//
	//                    ^ start bit
	//            ^^^^^^^^  data
	//           ^          odd parity
	//          ^           stop bit
	//         ^            reply from keyboard (ack bit)
	//
	cli();
	if (dev->input_bits) {
		sei();
		return false;
	}
	dev->output_bits = d;
	dev->io->set_clock_0();	// I/O inhibit, trigger interrupt
	dev->io->set_data_0();	// start bit
	sei();
	wait_40us();
	wait_40us();
	dev->io->set_clock_1();

	dev->timeout = 0;

	return true;
}

bool pc_send(PS2IF *host, unsigned char c)
{
	unsigned char i;
	unsigned short bits;

	bits = c & 0xff;
	if (!(countbits(bits) & 1)) bits |= 0x100;
	bits = (bits | 0x200) << 1;

	for (i = 0; i < 11; i++) {
		if (!pc_get_clock()) break;
		if (bits & 1)	host->io->set_data_1();
		else			host->io->set_data_0();
		bits >>= 1;
		wait_15us();
		if (!host->io->get_clock()) break;
		host->io->set_clock_0();
		wait_40us();
		host->io->set_clock_1();
		wait_15us();
	}
	host->io->set_data_1();

	return i == 11;
}

int pc_recv(PS2IF *host)
{
	int i;
	unsigned short bits;

	if (host->io->get_data()) return -1;
	if (!host->io->get_clock()) return -1;

	bits = 0;
	for (i = 0; i < 100; i++) {
		host->io->set_clock_0();
		wait_40us();
		host->io->set_clock_1();
		wait_15us();
		wait_15us();
		if (i < 9) {
			bits >>= 1;
			if (host->io->get_data()) bits |= 0x100;
		} else {
			if (host->io->get_data()) {	// stop bit ?
				break;
			}
		}
	}
	host->io->set_clock_0();
	host->io->set_data_0();
	wait_40us();
	host->io->set_data_1();
	host->io->set_clock_1();

	if (i != 9)
		return -1;							// framing error

	if (!(countbits(bits & 0x1ff) & 1))
		return -1;							// parity error

	return bits & 0xff;
}

inline void pc_put(PS2IF *host, unsigned char c)
{
	qput(&host->output_queue, c & 0xff);
}

inline int pc_get(PS2IF *host)
{
	int c = qget(&host->input_queue);
	return c;
}

inline void kb_put(PS2IF *dev, uint8_t c)
{
	qput(&dev->output_queue, c & 0xff);
}

inline int kb_get(PS2IF *dev)
{
	int c;
	cli();
	c = qget(&dev->input_queue);
	sei();
	return c;
}

// pin change interrupt

void intr(PS2IF *dev)
{
	if (dev->io->get_clock()) {
		sei();
	} else {
		dev->timeout = 10;	// 10ms
		if (!dev->input_bits) {
			if (dev->output_bits) {			// transmit mode
				if (dev->output_bits == 1) {
					dev->output_bits = 0;		// end transmit
					dev->timeout = 0;
				} else {
					if (dev->output_bits & 1) {
						dev->io->set_data_1();
					} else {
						dev->io->set_data_0();
					}
					dev->output_bits >>= 1;
				}
			} else {
				dev->input_bits = 0x800;		// start receive
			}
		}
		if (dev->input_bits) {
			dev->input_bits >>= 1;
			if (dev->io->get_data()) {
				dev->input_bits |= 0x800;
			}
			if (dev->input_bits & 1) {
				if (dev->input_bits & 0x800) {				// stop bit ?
					if (countbits(dev->input_bits & 0x7fc) & 1) {	// odd parity ?
						uint8_t c = (dev->input_bits >> 2) & 0xff;
						qput(&dev->input_queue, c);
					}
				}
				dev->input_bits = 0;
				dev->timeout = 0;
			}
		}
	}
}

ISR(INT0_vect)
{
	intr(&ps2d);
}

ISR(INT5_vect)
{
}

//

void report_host_to_device(uint8_t c);
void report_device_to_host(uint8_t c);

void ps2_io_handler(PS2IF *host, PS2IF *dev)
{
	int c;

	// receive from host
	c = pc_recv(host);
	if (c >= 0) {
		qput(&host->input_queue, c);
	}

	// transmit to host
	c = qget(&host->output_queue);
	if (c >= 0) {
		if (!pc_send(host, c)) {
			qunget(&host->output_queue, c);
		}
	}

	// transmit to device
	c = qget(&dev->output_queue);
	if (c >= 0) {
		if (!ps2d_next_output(dev, c)) {
			qunget(&dev->output_queue, c);
		}
	}
}

void ps2_handler(PS2IF *host, PS2IF *dev, bool timer_event_flag)
{
	int c;

	if (timer_event_flag) {

		cli();
		if (dev->timeout > 0) {
			if (dev->timeout > 1) {
				dev->timeout--;
			} else {
				dev->output_bits = 0;
				dev->input_bits = 0;
				dev->io->set_data_1();
				dev->io->set_clock_1();
				dev->timeout = 0;
			}
		}
		sei();
	}

	c = pc_get(host);
	if (c >= 0) {
		kb_put(dev, c);
		report_host_to_device(c);
	}
	c = kb_get(dev);
	if (c >= 0) {
		pc_put(host, c);
		report_device_to_host(c);
	}
}

void init_device(PS2IF *dev)
{
	qinit(&dev->event_queue_l);
	qinit(&dev->event_queue_h);
	qinit(&dev->output_queue);
	qinit(&dev->input_queue);
	dev->output_bits = 0;
	dev->input_bits = 0;
}

void init_as_ps2_device(PS2IF *d)
{
	d->io->set_clock_0();
	d->io->set_data_1();

	init_device(d);

	d->io->set_clock_1();
}

void init_as_ps2_host(PS2IF *d)
{
	init_as_ps2_device(d);
}

void keyboard_setup()
{
	ps2d.io = &ps2d_io;
	ps2h.io = &ps2h_io;

	PORTD = 0;
	DDRD = 0xaa;

	EIMSK |= 0x21;
	EICRA = 0x01;
	EICRB = 0x04;

	init_as_ps2_host(&ps2h);
	init_as_ps2_device(&ps2d);
}

void ps2_loop()
{
	cli();
	bool timerevent = interval_1ms_flag;
	interval_1ms_flag = false;
	sei();

	ps2_io_handler(&ps2h, &ps2d);
	ps2_handler(&ps2h, &ps2d, timerevent);
}





