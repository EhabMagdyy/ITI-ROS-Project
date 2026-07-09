#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

/**
 * encoder_init() - Configure quadrature encoder GPIO interrupts.
 *
 * Both A and B channels of each encoder are set up with both-edge
 * interrupts to achieve X4 quadrature decoding.
 *
 * Return: 0 on success, negative errno on failure.
 */
int encoder_init(void);

/**
 * encoder_get_left()  - Read left  wheel tick counter (signed).
 * encoder_get_right() - Read right wheel tick counter (signed).
 *
 * Positive = forward, negative = reverse.
 * Counters are never reset automatically; call encoder_reset() if needed.
 */
int32_t encoder_get_left(void);
int32_t encoder_get_right(void);

/**
 * encoder_reset() - Zero both tick counters.
 */
void encoder_reset(void);

#endif /* ENCODER_H */
