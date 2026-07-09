#ifndef IMU_H
#define IMU_H

#include <stdint.h>

/**
 * @brief Initialize the IMU (MPU6050) and start its 100Hz reading thread.
 * @return 0 on success, negative error code on failure.
 */
int imu_init(void);

#endif /* IMU_H */
