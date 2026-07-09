#ifndef UART_GATEWAY_H
#define UART_GATEWAY_H

#include <stdint.h>

/**
 * uart_gateway_init() - Set up UART2 with interrupt-driven RX.
 *
 * Must be called once before any other uart_gateway_* function.
 * Return: 0 on success, negative errno on failure.
 */
int uart_gateway_init(void);

/**
 * uart_gateway_get_cmd() - Retrieve the latest parsed motor command.
 *
 * Thread-safe (mutex protected). Returns the last command received
 * via "M,<left>,<right>\n". Values are 0,0 until first command arrives.
 *
 * @speed_l: Output — left  motor speed (-255 to +255)
 * @speed_r: Output — right motor speed (-255 to +255)
 */
void uart_gateway_get_cmd(int16_t *speed_l, int16_t *speed_r);

/**
 * uart_gateway_send_telemetry() - Transmit one telemetry frame over UART.
 *
 * Sends: "T,<front>,<back>,<left>,<right>,<enc_l>,<enc_r>\n"
 *
 * @dist_front: Front ultrasonic distance in cm
 * @dist_back:  Back  ultrasonic distance in cm
 * @dist_left:  Left  ultrasonic distance in cm
 * @dist_right: Right ultrasonic distance in cm
 * @enc_l:      Left  encoder tick count (signed)
 * @enc_r:      Right encoder tick count (signed)
 */
void uart_gateway_send_telemetry(uint32_t dist_front, uint32_t dist_back,
				 uint32_t dist_left,  uint32_t dist_right,
				 int32_t  enc_l,      int32_t  enc_r);

/**
 * uart_gateway_send_imu() - Transmit one IMU 6-axis frame over UART.
 *
 * Sends: "I,<ax>,<ay>,<az>,<gx>,<gy>,<gz>\n"
 *
 * @ax, ay, az: Accelerometer raw readings
 * @gx, gy, gz: Gyroscope raw readings
 */
void uart_gateway_send_imu(int16_t ax, int16_t ay, int16_t az,
			   int16_t gx, int16_t gy, int16_t gz);

#endif /* UART_GATEWAY_H */
