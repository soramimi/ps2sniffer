
#include <avr/io.h>

void pc_set_clock_0()
{
	PORTD |= 0x20;
}

void pc_set_clock_1()
{
	PORTD &= ~0x20;
}

void pc_set_data_0()
{
	PORTD |= 0x80;
}

void pc_set_data_1()
{
	PORTD &= ~0x80;
}

bool pc_get_clock()
{
	return (PIND >> 4) & 1;
}

bool pc_get_data()
{
	return (PIND >> 6) & 1;
}


void kb0_set_clock_0()
{
	PORTD |= 0x02;
}

void kb0_set_clock_1()
{
	PORTD &= ~0x02;
}

void kb0_set_data_0()
{
	PORTD |= 0x08;
}

void kb0_set_data_1()
{
	PORTD &= ~0x08;
}

bool kb0_get_clock()
{
	return (PIND >> 0) & 1;
}

bool kb0_get_data()
{
	return (PIND >> 2) & 1;
}

#if 0

void kb1_set_clock_0()
{
	PORTD |= 0x20;
}

void kb1_set_clock_1()
{
	PORTD &= ~0x20;
}

void kb1_set_data_0()
{
	PORTD |= 0x80;
}

void kb1_set_data_1()
{
	PORTD &= ~0x80;
}

bool kb1_get_clock()
{
	return (PIND >> 4) & 1;
}

bool kb1_get_data()
{
	return (PIND >> 6) & 1;
}

#endif
