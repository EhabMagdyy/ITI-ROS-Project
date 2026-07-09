#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

/**
 * motor_init() - Configure PWM and direction GPIO pins for both motors.
 *
 * Must be called once before motor_set().
 * Return: 0 on success, negative errno on failure.
 */
int motor_init(void);

/**
 * motor_set() - Apply speed and direction to both motors.
 *
 * @speed_l: Left  motor speed (-255 = full reverse, +255 = full forward, 0 = stop)
 * @speed_r: Right motor speed (-255 = full reverse, +255 = full forward, 0 = stop)
 *
 * Values outside [-255, 255] are clamped automatically.
 */
void motor_set(int16_t speed_l, int16_t speed_r);

#endif /* MOTOR_H */
