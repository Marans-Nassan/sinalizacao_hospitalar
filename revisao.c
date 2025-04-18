#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
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
    volatile bool impedir;
} Botestado;
Botestado B = {false, false, false, false};

typedef struct{
    float dc;
    float cd;
    bool direcao;
} pwm_struct;
pwm_struct p = {55.8, 64.0, true};

typedef struct pixeis {
    uint8_t G, R, B;
  } pixeis;
pixeis leds[matriz_led_pins];
  
struct repeating_timer timer;
uint8_t slice;
uint16_t vrx_value = 0;
uint16_t vry_value = 0;
ssd1306_t ssd;
PIO pio; 
uint sm;

void ledinit();
void botinit();
void led_lig_des();
void gpio_irq_handler(uint gpio, uint32_t events);
int pwm_setup();
void buzzcontrol_on();
void buzzcontrol_off();
void som();
bool repeating_timer_callback(struct repeating_timer *t);
void adcinit();
uint16_t media(uint8_t channel);
void i2cinit();
void oledinit();
void oleddis();
void minit(uint pin);
void setled(const uint index, const uint8_t r, const uint8_t g, const uint8_t b);
void mdisplay();
void led_clear_a();
void led_clear_b();
void press_a();
void press_b();

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
    minit(matriz_led);

    while (true) {
        vry_value = 4095 - media(adc_channel_0); //Potênciometro está invertido.
        vrx_value = media(adc_channel_1);
        oleddis();
        (B.press1) ? press_a():led_clear_a();
        (B.press2) ? press_b():led_clear_b();
        if(B.impedir){
            B.impedir = false;
            if(B.press3 == true){
                buzzcontrol_on();
                add_repeating_timer_ms(50, repeating_timer_callback, NULL, &timer);
                printf("\nBotão J pressionado, luz vermelha ligada e som do PWM ligado ."); 

            }
            else {
                cancel_repeating_timer(&timer);
                buzzcontrol_off();
                printf("\nBotão J pressionado, luz vermelha desligada e som do PWM desligado.");
            }  
        }  
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
            (B.press1) ? printf("\nBotão A pressionado. Luz verde ligada e matriz ativada"): printf("\nBotao A pressionado. Luz verde desligada e matriz desativada.");
            lastA = current_time;
        }

        if(gpio == botao_b && (current_time - lastB > 300)){
            B.press2 = !B.press2;
            gpio_put(blue_led, B.press2);
            (B.press2) ? printf("\nBotão B pressionado. Luz azul ligada e matriz ativada.") : printf("\nBotão B pressionado. Luz azul desligada e matriz desativada.");
            lastB = current_time;
        }

        if(gpio == botao_j && (current_time - lastJ > 300)){
            B.press3 = !B.press3;
            B.impedir = true;
            gpio_put(red_led, B.press3);
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
void buzzcontrol_on(){
    gpio_set_function(buzz_a, GPIO_FUNC_PWM);
    pwm_set_enabled(slice, true);
}

void buzzcontrol_off(){
    pwm_set_enabled(slice, false);
    gpio_set_function(buzz_a, GPIO_FUNC_SIO);
    gpio_set_dir(buzz_a, GPIO_OUT);
    gpio_put(buzz_a, 0);
}

void som(){
    pwm_set_clkdiv(slice, p.cd);
    pwm_set_gpio_level(buzz_a, p.dc);
        if(p.direcao == true){
            p.dc += 92.07;
            p.cd -= 0.53;
                if(p.dc >= 5580) p.direcao = false;
        }
        else {
            p.dc -= 92.07;
            p.cd += 0.53;
                if(p.dc <= 55.8) p.direcao = true;
        }             
}

bool repeating_timer_callback(struct repeating_timer *t){
    som();
    return true;
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

void minit(uint pin){
uint offset = pio_add_program(pio0, &ws2812_program);
pio = pio0;

sm = pio_claim_unused_sm(pio, false);
    if(sm < 0){
        pio = pio1;
        sm = pio_claim_unused_sm(pio, true);
    }

ws2812_program_init(pio, sm, offset, pin, 800000.f);
}

void setled(const uint index, const uint8_t r, const uint8_t g, const uint8_t b){
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}

void mdisplay(){
    for (uint i = 0; i < matriz_led_pins; ++i) {
        pio_sm_put_blocking(pio, sm, leds[i].G);
        pio_sm_put_blocking(pio, sm, leds[i].R);
        pio_sm_put_blocking(pio, sm, leds[i].B);
    }
sleep_us(100); 
}

void led_clear_a(){
    const uint8_t digit_leds[] = {24, 23, 22, 21, 20, 15, 19, 14, 10, 5, 9, 4, 3, 2, 1, 0};
    size_t a = sizeof(digit_leds) / sizeof(digit_leds[0]);
    for (size_t i = 0; i < a; ++i) {
        setled(digit_leds[i], 0, 0, 0);
    }
        mdisplay();   
}

void led_clear_b(){
    const uint8_t digit_leds[] = {22, 17, 14, 13, 12, 11, 10, 7, 2};
    size_t a = sizeof(digit_leds) / sizeof(digit_leds[0]);
    for (size_t i = 0; i < a; ++i) {
        setled(digit_leds[i], 0, 0, 0);
    }
        mdisplay();   
}


void press_a(){
    const uint8_t digit_leds[] = {24, 23, 22, 21, 20, 15, 19, 14, 10, 5, 9, 4, 3, 2, 1, 0};
    size_t a = sizeof(digit_leds) / sizeof(digit_leds[0]);
    for (size_t i = 0; i < a; ++i) {
        setled(digit_leds[i], 0, 0, 1);
    }
        mdisplay();   
}

void press_b(){
    const uint8_t digit_leds[] = {22, 17, 14, 13, 12, 11, 10, 7, 2};
    size_t a = sizeof(digit_leds) / sizeof(digit_leds[0]);
    for (size_t i = 0; i < a; ++i) {
        setled(digit_leds[i], 0, 1, 0);
    }
        mdisplay();   
}