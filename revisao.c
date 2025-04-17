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
    volatile bool press1;
    volatile bool press2;
    volatile bool press3;
} Botestado;
Botestado B = {false, false, false};

void ledinit();
void botinit();
void led_lig_des();
void gpio_irq_handler(uint gpio, uint32_t events);

int main(){

    stdio_init_all();
    ledinit();
    botinit();
    led_lig_des();

    while (true) {
        printf("\nHello, world!\n");
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
void botinit(){
    const int botoes[3] = {botao_a, botao_b, botao_j};
        for(uint8_t i = 0; i < 3; i++){
            gpio_init(botoes[i]);
            gpio_set_dir(botoes[i], GPIO_IN);
            gpio_pull_up(botoes[i]);
        }      
   
}

void led_lig_des(){
    const uint8_t bots[3] = {botao_a, botao_b, botao_j};
     for(uint8_t i = 0; i < 3; i++){
         gpio_set_irq_enabled_with_callback (bots[i], GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
     }
 
 }

void gpio_irq_handler(uint gpio, uint32_t events){

    uint64_t current_time = to_us_since_boot(get_absolute_time()) /1000;
    static uint64_t lastA = 0, lastB = 0, lastJ = 0;

        if(gpio == botao_a && (current_time - lastA > 300)){
            B.press1 = !B.press1;
            gpio_put(green_led, B.press1);
            (B.press1 == true) ? printf("\nBotão A pressionado e luz verde ligada"): printf("\nBotao A pressionado e luz verde desligada.");
            lastA = current_time;
        }

        if(gpio == botao_b && (current_time - lastB > 300)){
            B.press2 = !B.press2;
            gpio_put(blue_led, B.press2);
            (B.press2 == true) ? printf("\nBotão B pressionado e luz azul ligada"): printf("\nBotão B pressionado e luz azul desligada.");
            lastB = current_time;
        }

        if(gpio == botao_j && (current_time - lastJ > 300)){
            B.press3 = !B.press3;
            gpio_put(red_led, B.press3);
            (B.press3 == true) ? printf("\nBotão J pressionado e luz vermelha ligada."): printf("\nBotão J pressionado e luz vermelha desligada");
            lastJ = current_time;
        }  
}
