/*
 * OTB-IOT - Out of The Box Internet Of Things
 *
 * Copyright (C) 2016 Piers Finlayson
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version. 
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OTB_I2C_PCA9685_H
#define OTB_I2C_PCA9685_H

// PCA9685 base address
#define OTB_I2C_PCA9685_BASE_ADDR   0x40

// PCA9685 key registers
#define OTB_I2C_PCA9685_REG_MODE1        0x00
#define OTB_I2C_PCA9685_REG_MODE2        0x01
#define OTB_I2C_PCA9685_REG_IO0_ON_L     0x06
#define OTB_I2C_PCA9685_REG_IO_ALL_ON_L  0xFA
#define OTB_I2C_PCA9685_REG_PRE_SCALE    0xFE

// PCA9685 LED offsets from IO start
#define OTB_I2C_PCA9685_REG_IO_ON_L     0x00
#define OTB_I2C_PCA9685_REG_IO_ON_H     0x01
#define OTB_I2C_PCA9685_REG_IO_OFF_L    0x02
#define OTB_I2C_PCA9685_REG_IO_OFF_H    0x03
#define OTB_I2C_PCA9685_REG_IO_LEN      0x04

// Number of IOs supported by a PCA9685
#define OTB_I2C_PCA9685_REG_IO_NUM     16

// PCA9685 mode bits
#define OTB_I2C_PCA9685_MODE_INVERT     (0b00010000 << 8)
#define OTB_I2C_PCA9685_MODE_OCH_ACK    (0b00001000 << 8)
#define OTB_I2C_PCA9685_MODE_TOTEM      (0b00000100 << 8)
#define OTB_I2C_PCA9685_MODE_OUTNE1      ???
#define OTB_I2C_PCA9685_MODE_OUTNE0      ???
#define OTB_I2C_PCA9685_MODE_RESTART     0b10000000
#define OTB_I2C_PCA9685_MODE_EXTCLOCK    0b01000000
#define OTB_I2C_PCA9685_MODE_AI_REG      0b00100000
#define OTB_I2C_PCA9685_MODE_SLEEP       0b00010000
#define OTB_I2C_PCA9685_MODE_SUB1        0b00001000
#define OTB_I2C_PCA9685_MODE_SUB2        0b00000100
#define OTB_I2C_PCA9685_MODE_SUB3        0b00000010
#define OTB_I2C_PCA9685_MODE_ALLCALL     0b00000000

#define OTB_I2C_PCA9685_IO_FULL_ON       0b0001000000000000
#define OTB_I2C_PCA9685_IO_FULL_OFF      0b0001000000000000

// PCA9685 default pre-scale (from datasheet) = 200Hz PWM
#define OTB_I2C_PCA9685_PRESCALE_DEFAULT  0x1E

#ifndef OTB_I2C_PCA9685_C


#else
bool otb_i2c_pca9685_led_on;
static volatile os_timer_t otb_i2c_pca9685_test_timer;
uint8_t otb_i2c_pca9685_test_addr;

#endif // OTB_I2C_PCA9685_C

void otb_i2c_pca9685_test_timerfunc(void);
void otb_i2c_pca9685_test_init(void);
bool otb_i2c_pca9685_led_conf(uint8_t, uint8_t led, uint16_t on, uint16_t off);
bool otb_i2c_pca9685_init(uint8_t addr);
bool otb_i2c_pca9685_read_reg(uint8_t addr, uint8_t reg, uint8_t *val);
bool otb_i2c_pca9685_set_mode(uint8_t addr, uint16_t mode);
bool otb_i2c_pca9685_write_one_reg(uint8_t addr, uint8_t reg, uint8_t val);
void otb_i2c_pca9685_start();
void otb_i2c_pca9685_stop();
bool otb_i2c_pca9685_call(uint8_t addr);
bool otb_i2c_pca9685_reg(uint8_t reg);
bool otb_i2c_pca9685_val(uint8_t val);

#endif // OTB_I2C_PCA9685_H
