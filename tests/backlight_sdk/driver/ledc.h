#pragma once
#include "esp_err.h"
#include <cstdint>
constexpr int LEDC_LOW_SPEED_MODE=1,LEDC_TIMER_0=0,LEDC_CHANNEL_0=0,LEDC_TIMER_12_BIT=12,LEDC_AUTO_CLK=0,LEDC_INTR_DISABLE=0;
struct ledc_timer_config_t { int speed_mode;int duty_resolution;int timer_num;uint32_t freq_hz;int clk_cfg; };
struct ledc_channel_config_t {int gpio_num;int speed_mode;int channel;int intr_type;int timer_sel;uint32_t duty;int hpoint;};
int ledc_timer_config(const ledc_timer_config_t*);
int ledc_channel_config(const ledc_channel_config_t*);
int ledc_set_duty(int,int,uint32_t);
int ledc_update_duty(int,int);
