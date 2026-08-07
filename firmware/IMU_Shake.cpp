#include "IMU_Shake.h"
#include <Wire.h>
#include <Arduino.h>

#define QMI8658_WHO_AM_I 0x00
#define QMI8658_CTRL1 0x02
#define QMI8658_CTRL2 0x03
#define QMI8658_CTRL7 0x08
#define QMI8658_AX_L 0x35

static uint8_t s_imu_addr = 0x6B;
static bool imu_ok = false;
static uint32_t last_shake_time = 0;

void IMU_Init() {
    // Try 0x6B first, then 0x6A
    uint8_t addrs[] = {0x6B, 0x6A};
    for (int i = 0; i < 2; i++) {
        Wire.beginTransmission(addrs[i]);
        if (Wire.endTransmission() == 0) {
            Wire.beginTransmission(addrs[i]);
            Wire.write(QMI8658_WHO_AM_I);
            if (Wire.endTransmission(false) == 0) {
                Wire.requestFrom(addrs[i], (uint8_t)1);
                if (Wire.available()) {
                    uint8_t who = Wire.read();
                    if (who == 0x05) {
                        imu_ok = true;
                        s_imu_addr = addrs[i];
                        printf("QMI8658 IMU found at 0x%02X!\n", s_imu_addr);
                        
                        // Enable Accelerometer (CTRL7 = 0x01)
                        Wire.beginTransmission(s_imu_addr);
                        Wire.write(QMI8658_CTRL7);
                        Wire.write(0x01); // Enable accel only
                        Wire.endTransmission();
                        
                        // Config Accel (CTRL2 = 0x24) -> 250Hz, +/- 2g
                        Wire.beginTransmission(s_imu_addr);
                        Wire.write(QMI8658_CTRL2);
                        Wire.write(0x24);
                        Wire.endTransmission();
                        return;
                    }
                }
            }
        }
    }
    printf("QMI8658 IMU not found.\n");
}

bool IMU_CheckShake() {
    if (!imu_ok) return false;
    
    // Read 6 bytes of accel data
    Wire.beginTransmission(s_imu_addr);
    Wire.write(QMI8658_AX_L);
    Wire.endTransmission(false);
    
    Wire.requestFrom(s_imu_addr, (uint8_t)6);
    if (Wire.available() == 6) {
        int16_t ax = Wire.read() | (Wire.read() << 8);
        int16_t ay = Wire.read() | (Wire.read() << 8);
        int16_t az = Wire.read() | (Wire.read() << 8);
        
        // Convert to 'g's roughly (scale for +/-2g is 16384 LSB/g)
        float gx = ax / 16384.0f;
        float gy = ay / 16384.0f;
        float gz = az / 16384.0f;
        
        // Magnitude squared
        float mag = (gx*gx) + (gy*gy) + (gz*gz);
        
        // Rest is approx 1.0 (Earth's gravity)
        // A shake would spike above 1.0. Let's trigger on > 1.8 (i.e. > 1.34g total) to make it easier to trigger
        if (mag > 1.8f) {
            uint32_t now = millis();
            if (now - last_shake_time > 2000) { // 2 second cooldown
                last_shake_time = now;
                return true;
            }
        }
    }
    return false;
}
