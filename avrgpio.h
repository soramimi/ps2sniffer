#ifndef AVRGPIO_H
#define AVRGPIO_H

#include <avr/io.h>

namespace avr {
enum {
	B,
	C,
	D,
};

template <int PORT, int PIN> class GPIO {
public:
private:
	const uint8_t bit = 1 << PIN;
	bool pullup_ = false;
	void setdir(bool output)
	{
		switch (PORT) {
		case avr::B: if (output) { DDRB |= bit; } else { DDRB &= ~bit; } return;
		case avr::C: if (output) { DDRC |= bit; } else { DDRC &= ~bit; } return;
		case avr::D: if (output) { DDRD |= bit; } else { DDRD &= ~bit; } return;
		}
	}
public:
	GPIO()
	{
		setdir(false);
		setpullup(false);
	}
	void setpullup(bool pullup)
	{
		pullup_ = pullup;
		switch (PORT) {
		case avr::B: write(pullup_); return;
		case avr::C: write(pullup_); return;
		case avr::D: write(pullup_); return;
		}
	}
	bool read()
	{
		setdir(false);
		switch (PORT) {
		case avr::B: return (PINB & bit) ? 1 : 0;
		case avr::C: return (PINC & bit) ? 1 : 0;
		case avr::D: return (PIND & bit) ? 1 : 0;
		}
	}
	void write(bool b)
	{
		if (pullup_ && b) {
			setdir(false);
		} else {
			setdir(true);
		}
		switch (PORT) {
		case avr::B: if (b) { PORTB |= bit; } else { PORTB &= ~bit; } return;
		case avr::C: if (b) { PORTC |= bit; } else { PORTC &= ~bit; } return;
		case avr::D: if (b) { PORTD |= bit; } else { PORTD &= ~bit; } return;
		}
	}
};

} // namespace avr

#endif // AVRGPIO_H
