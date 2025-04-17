#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/adc.h"
#include "font.h"
#include "ssd1306.h"
#include "ws2812.pio.h"

#define botao_a 5
#define botao_b 6
#define matriz_led 7
#define green_led 11
#define blue_led 12
#define red_led 13
#define I2C_SDA 14
#define I2C_SCL 15
#define I2C_PORT i2c1
#define endereco 0x3c
#define adc_channel_0  0
#define adc_channel_1  1
#define botao_j 22
#define matriz_led_pins 25
#define VRX 26
#define VRY 27

typedef struct{
    bool press1;
    bool press2;
    bool press3;
} Botestado;
Botestado B = {false, false, false};

void ledinit();
void botinit(uint8_t x);
void led_lig_des(uint8_t bot);
void gpio_irq_handler(uint gpio, uint32_t events);

int main(){

    stdio_init_all();
    ledinit();
    botinit(5);
    botinit(6);
    botinit(22);
    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}

void ledinit(){
    for(uint8_t i = 11; i < 14; i++){
        gpio_init(i);
        gpio_set_dir(i, GPIO_OUT);
        gpio_put(i, 0);
    }
}
void botinit(uint8_t x){
    gpio_init(x);
    gpio_set_dir(x, GPIO_IN);
    gpio_pull_up(x);

}

void gpio_irq_handler(uint gpio, uint32_t events){
    
}

void led_lig_des(uint8_t bot){
gpio_set_irq_enabled_with_callback (bot, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
}

