
#ifndef QUEUE16_H
#define QUEUE16_H

#include <stdint.h>

struct Queue16 {
	uint8_t pos;
	uint8_t len;
	uint8_t buf[16];
};

extern void q16init(struct Queue16 *q);
extern int q16peek(struct Queue16 *q);
extern int q16get(struct Queue16 *q);
extern void q16put(struct Queue16 *q, uint8_t c);
extern void q16unget(struct Queue16 *q, uint8_t c);

#endif

