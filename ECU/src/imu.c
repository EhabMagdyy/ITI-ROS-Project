/*
 * imu.c — MPU6050 6-axis IMU reader
 *
 * Runs a dedicated thread at 100 Hz (10ms) that reads raw 
 * accelerometer and gyroscope data over I2C0 and streams it
 * to UART using uart_gateway_send_imu().
 */

#include "imu.h"
#include "uart_gateway.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(imu, LOG_LEVEL_INF);

#define MPU6050_ADDR 0x68

static const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));

#define IMU_THREAD_STACK_SIZE 2048
#define IMU_THREAD_PRIORITY   5

K_THREAD_STACK_DEFINE(imu_stack, IMU_THREAD_STACK_SIZE);
static struct k_thread imu_thread_data;

/* Helper to write a single register to MPU6050 */
static int mpu_write_reg(uint8_t reg, uint8_t val)
{
	uint8_t buf[2] = {reg, val};
	return i2c_write(i2c_dev, buf, 2, MPU6050_ADDR);
}

static void imu_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (1) {
		uint8_t reg = 0x3B; /* ACCEL_XOUT_H */
		uint8_t data[14];

		/* Read 14 bytes (Accel, Temp, Gyro) in one burst */
		int ret = i2c_write_read(i2c_dev, MPU6050_ADDR, &reg, 1, data, 14);
		if (ret == 0) {
			int16_t ax = (data[0] << 8) | data[1];
			int16_t ay = (data[2] << 8) | data[3];
			int16_t az = (data[4] << 8) | data[5];
			
			/* Skip temperature: data[6], data[7] */
			
			int16_t gx = (data[8] << 8)  | data[9];
			int16_t gy = (data[10] << 8) | data[11];
			int16_t gz = (data[12] << 8) | data[13];

			uart_gateway_send_imu(ax, ay, az, gx, gy, gz);
		} else {
			LOG_WRN("IMU I2C read failed: %d", ret);
		}

		k_msleep(10); /* 100 Hz */
	}
}

int imu_init(void)
{
	if (!device_is_ready(i2c_dev)) {
		LOG_ERR("I2C0 device not ready");
		return -ENODEV;
	}

	/* 1. Device Reset */
	mpu_write_reg(0x6B, 0x80);
	k_msleep(100);

	/* 2. Wake up (Clear sleep bit) */
	mpu_write_reg(0x6B, 0x00);
	k_msleep(100);

	/* 3. Set Gyro Full Scale (+/- 500 deg/s) */
	mpu_write_reg(0x1B, 0x08);

	/* 4. Set Accel Full Scale (+/- 4g) */
	mpu_write_reg(0x1C, 0x08);

	/* 5. Verify WHO_AM_I */
	uint8_t reg = 0x75;
	uint8_t who = 0;
	if (i2c_write_read(i2c_dev, MPU6050_ADDR, &reg, 1, &who, 1) == 0) {
		LOG_INF("MPU6050 WHO_AM_I = 0x%02X", who);
		if (who != 0x68) {
			LOG_WRN("Unexpected WHO_AM_I (expected 0x68)");
		}
	} else {
		LOG_ERR("Failed to read WHO_AM_I");
		return -EIO;
	}

	/* Spawn 100Hz IMU reader thread */
	k_thread_create(&imu_thread_data,
			imu_stack,
			IMU_THREAD_STACK_SIZE,
			imu_thread,
			NULL, NULL, NULL,
			IMU_THREAD_PRIORITY,
			0,
			K_NO_WAIT);

	LOG_INF("IMU initialized (100 Hz streaming)");
	return 0;
}
