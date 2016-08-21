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

#ifndef OTB_I2C_H
#define OTB_I2C_H

typedef struct otb_i2c_ads_samples
{
  otb_conf_ads *ads;
  
  int time;
  
  volatile os_timer_t timer;

  uint16_t next_sample;
  
  uint8_t type;

  uint16_t dupes;

} otb_i2c_ads_samples;

#define OTB_I2C_ADS_MAX_SAMPLES 1024
typedef struct otb_i2c_ads_rms_samples
{
  otb_i2c_ads_samples hdr;
  
  int16_t samples[OTB_I2C_ADS_MAX_SAMPLES];

  uint16_t final_val;
  
} otb_i2c_ads_rms_samples;

typedef struct otb_i2c_ads_nonrms_samples
{
  otb_i2c_ads_samples hdr;

  int16_t samples[OTB_I2C_ADS_MAX_SAMPLES];

  int16_t final_val;
  
} otb_i2c_ads_nonrms_samples;

void i2c_master_gpio_init(void);
void i2c_master_init(void);

#define i2c_master_wait    os_delay_us
void i2c_master_stop(void);
void i2c_master_start(void);
void i2c_master_setAck(uint8 level);
uint8 i2c_master_getAck(void);
uint8 i2c_master_readByte(void);
void i2c_master_writeByte(uint8 wrdata);

bool i2c_master_checkAck(void);
void i2c_master_send_ack(void);
void i2c_master_send_nack(void);

#define OTB_I2C_MQTT_ERROR_LEN 256
#ifdef OTB_I2C_C
char otb_i2c_mqtt_error[OTB_I2C_MQTT_ERROR_LEN];
#else
extern char otb_i2c_mqtt_error[];
#endif // OTB_I2C_C

void otb_i2c_ads_disable_all_timers(void);
void otb_i2c_ads_on_timer(void *arg);
void otb_ads_build_msb_lsb_conf(otb_conf_ads *ads, uint8 *msb, uint8 *lsb);
bool otb_ads_configure(otb_conf_ads *ads);
void otb_ads_initialize(void);
void otb_i2c_ads_init_samples(otb_conf_ads *ads, otb_i2c_ads_samples *samples);
void otb_i2c_ads_on_timer(void *arg);
void otb_i2c_ads_sample_timer(void *arg);
void otb_i2c_ads_start_sample(otb_i2c_ads_samples *samples);
bool otb_i2c_ads_read_rms_sample(otb_i2c_ads_samples *samples);
bool otb_i2c_ads_read_nonrms_sample(otb_i2c_ads_samples *samples);
uint16_t otb_i2c_ads_finish_rms_samples(otb_i2c_ads_samples *samples);
int16_t otb_i2c_ads_finish_nonrms_samples(otb_i2c_ads_samples *samples);
bool otb_i2c_ads_get_sample(otb_i2c_ads_samples *samples);
void otb_i2c_ads_finish_sample(otb_i2c_ads_samples *samples);
extern bool otb_i2c_init();
char *otb_i2c_mqtt_error_write(char *error);
bool otb_i2c_mqtt_get_addr(char *byte, uint8 *addr);
bool otb_i2c_mqtt_get_num(char *byte, int *num);
bool otb_i2c_ads_get_binary_val(char *byte, uint8_t *num);
extern void otb_i2c_mqtt(char *, char **);
void otb_i2c_ads_mqtt(char *cmd, char **sub_cmd);
bool otb_i2c_test(uint8 addr);
bool otb_i2c_ads_set_cont_mode(uint8 addr, uint8 msb, uint8 lsb);
bool otb_i2c_ads_set_read_mode(uint8 addr, uint8 mode);
bool otb_i2c_ads_read(uint8 addr, int16_t *val);
bool otb_i2c_ads_range(uint8 addr, int num, int16_t *result, int *time_taken);
bool otb_i2c_ads_rms_range(uint8 addr, int num, uint16_t *result, int *time_taken);
bool otb_i2c_ads_conf_get_addr(uint8_t addr, otb_conf_ads **ads);
bool otb_i2c_ads_conf_get_loc(char *loc, otb_conf_ads **ads);
int8_t otb_i2c_ads_conf_field_match(char *field);
void otb_i2c_ads_conf_set(char *addr, char *field, char *value);
void otb_i2c_ads_conf_get(char *addr, char *field, char *field2);
unsigned long isqrt(unsigned long x);
bool otb_i2c_write_reg(uint8_t reg);
bool otb_i2c_write_val(uint8_t val);
void otb_i2c_bus_start();
void otb_i2c_bus_stop();
bool otb_i2c_bus_call(uint8_t addr, bool read);
bool otb_i2c_write_one_reg(uint8_t addr, uint8_t reg, uint8_t val);
bool otb_i2c_write_reg_seq(uint8_t addr, uint8_t reg, uint8_t count, uint8_t *val);
bool otb_i2c_read_one_reg(uint8_t addr, uint8_t reg, uint8_t *val);
bool otb_i2c_read_reg_seq(uint8_t addr, uint8_t reg, uint8_t count, uint8_t *val);
bool otb_i2c_read_one_val(uint8_t addr, uint8_t *val);
bool otb_i2c_write_one_val(uint8_t addr, uint8_t val);

#define OTB_I2C_ADC_GAIN_VALUES 8
#ifndef OTB_I2C_C  
extern float otb_i2c_ads_gain_to_v[OTB_I2C_ADC_GAIN_VALUES];
#else
float otb_i2c_ads_gain_to_v[OTB_I2C_ADC_GAIN_VALUES] = 
{
  // From ADS1115 data sheet:
  // Gain amplifier setting - valid values 0-7
  // 000 : FS = ±6.144V(1)
  // 001 : FS = ±4.096V(1)
  // 010 : FS = ±2.048V (default)
  // 011 : FS = ±1.024V
  // 100 : FS = ±0.512V
  // 101 : FS = ±0.256V
  // 110 : FS = ±0.256V
  // 111 : FS = ±0.256V

  6.144,
  4.096,
  2.048,
  1.024,
  0.512,
  0.256,
  0.256,
  0.256,
};
#endif //OTB_I2C_C

// Store off whether I2C already initialized
#ifndef OTB_I2C_C
extern bool otb_i2c_initialized;
#else
bool otb_i2c_initialized = FALSE;
#endif // OTB_I2C_C


#endif // OTB_I2C_H
