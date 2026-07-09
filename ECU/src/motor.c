/*
 * motor.c — DC motor control via LEDC PWM + GPIO direction pins
 *
 * Pins (from device tree):
 *   MOTOR_L_ENA  GPIO12  LEDC CH0 (PWM speed)
 *   MOTOR_L_IN1  GPIO13  GPIO Output (direction)
 *   MOTOR_L_IN2  GPIO14  GPIO Output (direction)
 *   MOTOR_R_ENA  GPIO15  LEDC CH1 (PWM speed)
 *   MOTOR_R_IN1  GPIO16  GPIO Output (direction)
 *   MOTOR_R_IN2  GPIO17  GPIO Output (direction)
 */

#include "motor.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(motor, LOG_LEVEL_INF);

/* ── Hardware bindings ─────────────────────────────────────── */

static const struct pwm_dt_spec motor_l_pwm =
	PWM_DT_SPEC_GET(DT_NODELABEL(pwm_motor_left));
static const struct pwm_dt_spec motor_r_pwm =
	PWM_DT_SPEC_GET(DT_NODELABEL(pwm_motor_right));

static const struct gpio_dt_spec motor_l_in1 =
	GPIO_DT_SPEC_GET(DT_NODELABEL(motor_l_in1), gpios);
static const struct gpio_dt_spec motor_l_in2 =
	GPIO_DT_SPEC_GET(DT_NODELABEL(motor_l_in2), gpios);
static const struct gpio_dt_spec motor_r_in1 =
	GPIO_DT_SPEC_GET(DT_NODELABEL(motor_r_in1), gpios);
static const struct gpio_dt_spec motor_r_in2 =
	GPIO_DT_SPEC_GET(DT_NODELABEL(motor_r_in2), gpios);

/* ── Public API ────────────────────────────────────────────── */

int motor_init(void){
	if(!pwm_is_ready_dt(&motor_l_pwm) ||
	    !pwm_is_ready_dt(&motor_r_pwm)){
		LOG_ERR("PWM devices not ready");
		return -ENODEV;
	}

	gpio_pin_configure_dt(&motor_l_in1, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&motor_l_in2, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&motor_r_in1, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&motor_r_in2, GPIO_OUTPUT_INACTIVE);

	/* Start stopped */
	motor_set(0, 0);

	LOG_INF("Motors initialized");
	return 0;
}

void motor_set(int16_t speed_l, int16_t speed_r){
	speed_l = CLAMP(speed_l, -255, 255);
	speed_r = CLAMP(speed_r, -255, 255);

	/* Left motor — direction */
	gpio_pin_set_dt(&motor_l_in1, speed_l >= 0 ? 1 : 0);
	gpio_pin_set_dt(&motor_l_in2, speed_l >= 0 ? 0 : 1);

	/* Right motor — direction */
	gpio_pin_set_dt(&motor_r_in1, speed_r >= 0 ? 1 : 0);
	gpio_pin_set_dt(&motor_r_in2, speed_r >= 0 ? 0 : 1);

	/* PWM duty: map |speed| (0-255) → pulse width (0–period_ns) */
	uint32_t pulse_l =
		(motor_l_pwm.period * (uint32_t)abs(speed_l)) / 255U;
	uint32_t pulse_r =
		(motor_r_pwm.period * (uint32_t)abs(speed_r)) / 255U;

	pwm_set_pulse_dt(&motor_l_pwm, pulse_l);
	pwm_set_pulse_dt(&motor_r_pwm, pulse_r);
}
