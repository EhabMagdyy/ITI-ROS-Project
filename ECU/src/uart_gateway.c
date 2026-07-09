/*
 * uart_gateway.c — UART Communication Gateway
 *
 * Supported commands (RX):
 *   "PING\n"          → replies "PONG\n"  (connectivity test)
 *   "M,<L>,<R>\n"    → sets motor speeds, e.g. "M,200,-150\n"
 *
 * Telemetry (TX, 10 Hz via uart_gateway_send_telemetry):
 *   "T,<front>,<back>,<left>,<right>,<enc_l>,<enc_r>\n"
 */

#include "uart_gateway.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LOG_MODULE_REGISTER(uart_gateway, LOG_LEVEL_INF);

/* ── Hardware bindings ─────────────────────────────────────── */
static const struct device *const uart2_dev =
	DEVICE_DT_GET(DT_NODELABEL(uart2));

/* ── State ─────────────────────────────────────────────────── */
static int16_t cmd_speed_l;
static int16_t cmd_speed_r;
K_MUTEX_DEFINE(cmd_mutex);

#define UART_RX_BUF_LEN 64
static uint8_t rx_buf[UART_RX_BUF_LEN];
static int     rx_idx;

/* ── Internal TX helper (used inside ISR-safe context too) ─── */
static void send_raw(const char *str)
{
	while (*str) {
		uart_poll_out(uart2_dev, (uint8_t)(*str++));
	}
}

/* ── Receive Callback ──────────────────────────────────────── */
static void uart_cb(const struct device *dev, void *user_data){
	if(!uart_irq_update(dev)){
		return;
	}

	if(!uart_irq_rx_ready(dev)){
		return;
	}

	uint8_t c;

	while (uart_fifo_read(dev, &c, 1) == 1) {
		if (c == '\n' || c == '\r') {
			if (rx_idx > 0) {
				rx_buf[rx_idx] = '\0';

				/* ── PING test command ─────────────────── */
				if (strcmp((char *)rx_buf, "PING") == 0) {
					send_raw("PONG\n");

				/* ── Motor speed command ───────────────── */
				} else if (rx_idx >= 5 &&
					   rx_buf[0] == 'M' &&
					   rx_buf[1] == ',') {
					int l = 0, r = 0;

					if (sscanf((char *)&rx_buf[2],
						    "%d,%d", &l, &r) == 2) {
						k_mutex_lock(&cmd_mutex, K_NO_WAIT);
						cmd_speed_l = (int16_t)l;
						cmd_speed_r = (int16_t)r;
						k_mutex_unlock(&cmd_mutex);
					}
				}
			}
			rx_idx = 0;
		} else if (rx_idx < UART_RX_BUF_LEN - 1) {
			rx_buf[rx_idx++] = c;
		}
	}
}

int uart_gateway_init(void)
{
	if (!device_is_ready(uart2_dev)) {
		LOG_ERR("UART2 device not ready");
		return -ENODEV;
	}

	uart_irq_callback_user_data_set(uart2_dev, uart_cb, NULL);
	uart_irq_rx_enable(uart2_dev);

	/* Send boot banner so the laptop knows the ECU is alive */
	send_raw("ECU_READY\n");

	LOG_INF("UART Gateway initialized (115200 baud)");
	return 0;
}

void uart_gateway_get_cmd(int16_t *speed_l, int16_t *speed_r){
	if(speed_l == NULL || speed_r == NULL){
		return;
	}

	k_mutex_lock(&cmd_mutex, K_FOREVER);
	*speed_l = cmd_speed_l;
	*speed_r = cmd_speed_r;
	k_mutex_unlock(&cmd_mutex);
}

void uart_gateway_send_telemetry(uint32_t dist_front, uint32_t dist_back, uint32_t dist_left,  uint32_t dist_right, int32_t  enc_l, int32_t  enc_r){
	char tx_buf[96];

	snprintf(tx_buf, sizeof(tx_buf), "T,%u,%u,%u,%u,%d,%d\n", (unsigned)dist_front, (unsigned)dist_back, (unsigned)dist_left, (unsigned)dist_right, enc_l, enc_r);

	char *p = tx_buf;
	while(*p){
		uart_poll_out(uart2_dev, (uint8_t)(*p++));
	}
}

void uart_gateway_send_imu(int16_t ax, int16_t ay, int16_t az,
			   int16_t gx, int16_t gy, int16_t gz)
{
	char tx_buf[64];
	snprintf(tx_buf, sizeof(tx_buf), "I,%d,%d,%d,%d,%d,%d\n",
		 ax, ay, az, gx, gy, gz);
	// LOG_INF("Sending IMU data, ax=%d, ay=%d, az=%d, gx=%d, gy=%d, gz=%d",
	// 	ax, ay, az, gx, gy, gz);

	char *p = tx_buf;
	while (*p) {
		uart_poll_out(uart2_dev, (uint8_t)(*p++));
	}
}
