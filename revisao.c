#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
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
#define buzz_a 21
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
uint8_t slice;
uint16_t vrx_value = 0;
uint16_t vry_value = 0;
ssd1306_t ssd;

void ledinit();
void botinit();
void led_lig_des();
void gpio_irq_handler(uint gpio, uint32_t events);
int pwm_setup();
void som(uint8_t slice);
void adcinit();
uint16_t media(uint8_t channel);
void i2cinit();
void oledinit();
void oleddis();

int main(){

    stdio_init_all();
    ledinit();
    botinit();
    led_lig_des();
    adcinit();
    pwm_setup();
    slice = pwm_setup();
    i2cinit();
    oledinit();

    while (true) {
        vry_value = 4095 - media(adc_channel_0); //Potênciometro está invertido.
        vrx_value = media(adc_channel_1);
        oleddis();
        
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
            (B.press3 == true) ? printf("\nBotão J pressionado, luz vermelha ligada e som do PWM desligado ."): printf("\nBotão J pressionado, luz vermelha desligada e som do PWM ligado.");
            (B.press3 == true) ? pwm_set_enabled(slice, false): pwm_set_enabled(slice, true);
            lastJ = current_time;
        }  
}

int pwm_setup(){
    gpio_set_function(buzz_a, GPIO_FUNC_PWM);
    uint8_t slice = pwm_gpio_to_slice_num(buzz_a);
    pwm_set_clkdiv(slice, 56.0);
    pwm_set_wrap(slice, 5580);
    pwm_set_enabled(slice, true);
    return slice;
}

void som(uint8_t slice){
    static float dc = 55.8; // 1% DC
    static float cd = 64.0;
        for (dc, cd; dc <= 5580; dc+= 92.07, cd-=0.53 ){
            pwm_set_clkdiv(slice, cd);
            pwm_set_gpio_level(buzz_a, dc);
        }

        for (dc, cd; dc >= 55.8; dc-= 92.07, cd+=0.53){
            pwm_set_clkdiv(slice, cd);
            pwm_set_gpio_level(buzz_a, dc);
        }
}

void i2cinit(){
    i2c_init(I2C_PORT, 400*1000);
        for(uint8_t i = 14 ; i < 16; i++){
            gpio_set_function(i, GPIO_FUNC_I2C);
            gpio_pull_up(i);
        }
}

void adcinit(){
    adc_init();
    adc_gpio_init(VRY);
    adc_gpio_init(VRX);
}

void oledinit(){
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
    ssd1306_config(&ssd);
}
uint16_t media(uint8_t channel){
    uint16_t media_adc = 0;
    for(uint8_t i = 0; i < 10; i++){
        adc_select_input(channel);
        sleep_us(2);
        media_adc += adc_read();
    }
return media_adc / 10;
}

void oleddis(){

    uint16_t coluna_x = (vrx_value * WIDTH) / 4095;
    uint16_t linha_y = (vry_value * HEIGHT) / 4095;
    if(coluna_x > WIDTH - 8) coluna_x = WIDTH - 8;
    if(linha_y > HEIGHT - 8) linha_y = HEIGHT - 8;
    ssd1306_fill(&ssd, false);
    ssd1306_rect(&ssd, linha_y, coluna_x, 8, 8, true, true);
    ssd1306_send_data(&ssd);
}