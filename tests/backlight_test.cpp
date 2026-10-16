#include "../firmware/main/backlight.hpp"
#include "driver/ledc.h"
#include <cassert>
#include <iostream>
namespace { int timers=0,channels=0,sets=0,updates=0,failure=0;uint32_t duty=0; }
int ledc_timer_config(const ledc_timer_config_t* c){++timers;assert(c->speed_mode==LEDC_LOW_SPEED_MODE && c->timer_num==LEDC_TIMER_0 && c->freq_hz==5000 && c->duty_resolution==LEDC_TIMER_12_BIT && c->clk_cfg==LEDC_AUTO_CLK);return failure==1?ESP_FAIL:ESP_OK;}
int ledc_channel_config(const ledc_channel_config_t* c){++channels;assert(c->gpio_num==21 && c->speed_mode==LEDC_LOW_SPEED_MODE && c->channel==LEDC_CHANNEL_0 && c->timer_sel==LEDC_TIMER_0 && c->intr_type==LEDC_INTR_DISABLE && c->hpoint==0 && c->duty<=4095);duty=c->duty;return failure==2?ESP_FAIL:ESP_OK;}
int ledc_set_duty(int mode,int channel,uint32_t value){++sets;assert(mode==LEDC_LOW_SPEED_MODE && channel==LEDC_CHANNEL_0 && value<=4095);duty=value;return failure==3?ESP_FAIL:ESP_OK;}
int ledc_update_duty(int mode,int channel){++updates;assert(mode==LEDC_LOW_SPEED_MODE && channel==LEDC_CHANNEL_0);return failure==4?ESP_FAIL:ESP_OK;}
int main(){
 using namespace deskduo;
 assert(display_set_brightness(50)==ESP_ERR_INVALID_STATE);
 assert(backlight_init(4)==ESP_ERR_INVALID_ARG && timers==0);
 failure=1;assert(backlight_init(5)==ESP_FAIL && channels==0);
 failure=2;assert(backlight_init(5)==ESP_FAIL);
 failure=0;assert(backlight_init(5)==ESP_OK && duty==205);
 for(int percent:{5,50,100}){
   assert(backlight_init(percent)==ESP_OK);
   const auto expected=(percent*4095+50)/100;assert(duty==static_cast<uint32_t>(expected));
   const auto oldSets=sets,oldUpdates=updates;
   for(int i=0;i<20;++i)assert(display_set_brightness(percent)==ESP_OK);
   assert(oldSets==sets && oldUpdates==updates);
 }
 assert(display_set_brightness(50)==ESP_OK && duty==2048);
 for(int bad:{0,4,101,255})assert(display_set_brightness(bad)==ESP_ERR_INVALID_ARG);
 for(int stage:{3,4}){
   failure=stage;assert(display_set_brightness(5)==ESP_FAIL);
   failure=0;auto old=sets;assert(display_set_brightness(5)==ESP_OK && sets==old+1 && duty==205);
   assert(display_set_brightness(50)==ESP_OK);
 }
 std::cout<<"Production backlight: saved 5/50/100, GPIO21 LEDC configuration, rounded duty, no redundant apply, errors retry passed\n";
}
