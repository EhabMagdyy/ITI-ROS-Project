/*
 * main.c — Main ECU Telemetry & Control Loop
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "motor.h"
#include "encoder.h"
#include "ultrasonic.h"
#include "imu.h"
#include "uart_gateway.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void){
	LOG_INF("Robot Car ECU starting...");

	if(motor_init() < 0){
		LOG_ERR("Failed to initialize motors");
		return -1;
	}

	if(encoder_init() < 0){
		LOG_ERR("Failed to initialize encoders");
		return -1;
	}

	if(ultrasonic_init() < 0){
		LOG_ERR("Failed to initialize ultrasonics");
		return -1;
	}

	if(imu_init() < 0){
		LOG_ERR("Failed to initialize IMU");
		return -1;
	}


	if(uart_gateway_init() < 0){
		LOG_ERR("Failed to initialize UART Gateway");
		return -1;
	}

	LOG_INF("All modules initialized. Starting 10 Hz telemetry loop.");

	while(1){
		/* 1. Retrieve latest command from UART gateway */
		int16_t speed_l = 0;
		int16_t speed_r = 0;
		uart_gateway_get_cmd(&speed_l, &speed_r);

		/* 2. Update motor PWMs */
		motor_set(speed_l, speed_r);

		/* 3. Retrieve latest sensor data */
		uint32_t dist_front = ultrasonic_get_dist(US_FRONT);
		uint32_t dist_back  = ultrasonic_get_dist(US_BACK);
		uint32_t dist_left  = ultrasonic_get_dist(US_LEFT);
		uint32_t dist_right = ultrasonic_get_dist(US_RIGHT);

		int32_t ticks_l = encoder_get_left();
		int32_t ticks_r = encoder_get_right();

		/* 4. Transmit status frame */
		uart_gateway_send_telemetry(
			dist_front, dist_back, dist_left, dist_right,
			ticks_l, ticks_r
		);

		/* 5. Sleep until next cycle(100 ms = 10 Hz) */
		k_msleep(100);
	}

	return 0;
}
