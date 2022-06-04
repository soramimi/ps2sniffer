
#include "queue16.h"

void q16init(struct Queue16 *q)
{
	q->pos = 0;
	q->len = 0;
}

int q16peek(struct Queue16 *q)
{
	return q->len > 0 ? q->buf[q->pos] : -1;
}

int q16get(struct Queue16 *q)
{
	int c = q16peek(q);
	if (c >= 0) {
		q->pos = (q->pos + 1) % sizeof(q->buf);
		q->len--;
	}
	return c;
}

void q16put(struct Queue16 *q, uint8_t c)
{
	if (q->len < sizeof(q->buf)) {
		q->buf[(q->pos + q->len) % sizeof(q->buf)] = c;
		q->len++;
	}
}

void q16unget(struct Queue16 *q, uint8_t c)
{
	if (q->len < sizeof(q->buf)) {
		q->pos = (q->pos + sizeof(q->buf) - 1) % sizeof(q->buf);
		q->buf[q->pos] = c;
		q->len++;
	}
}
