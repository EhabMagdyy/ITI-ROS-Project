#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include <stdint.h>

/** Sensor index constants */
#define US_FRONT  0
#define US_BACK   1
#define US_LEFT   2
#define US_RIGHT  3
#define US_COUNT  4

/**
 * ultrasonic_init() - Configure TRIG outputs, ECHO input-capture ISRs,
 *                     and start the four trigger threads.
 *
 * Return: 0 on success, negative errno on failure.
 */
int ultrasonic_init(void);

/**
 * ultrasonic_get_dist() - Read latest distance measurement for a sensor.
 *
 * @idx: Sensor index (US_FRONT / US_BACK / US_LEFT / US_RIGHT)
 *
 * Return: Distance in cm, or 999 if out of range / timeout.
 */
uint32_t ultrasonic_get_dist(int idx);

#endif /* ULTRASONIC_H */
