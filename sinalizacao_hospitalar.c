#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
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

typedef struct{ // estrutura responsável por gerir os botões e a ativação e desativação do pwm.
    volatile bool press1;
    volatile bool press2;
    volatile bool press3;
    volatile bool impedir;
} Botestado;
Botestado B = {false, false, false, false};

typedef struct{ // estrutura responsável por regular os valores utilizados no pwm.
    float dc;
    float cd;
    bool direcao;
} pwm_struct;
pwm_struct p = {55.8, 64.0, true};

typedef struct pixeis { // estrutura empregada ao configurar a matriz de leds.
    uint8_t G, R, B;
  } pixeis;
pixeis leds[matriz_led_pins];
  // Variáveis utilizadas
struct repeating_timer timer_pwm;
struct repeating_timer timer_adc;
uint8_t slice;
uint16_t vrx_value = 0;
uint16_t vry_value = 0;
ssd1306_t ssd;
PIO pio; 
uint sm;
uint16_t media_adc;

void ledinit(); //inicialização do led
void botinit(); //inicialização dos botões
void led_lig_des(); // Responsável por inserir as interrupções.
void gpio_irq_handler(uint gpio, uint32_t events); // Conteúdo das interrupções.
int pwm_setup(); // Configuraçãod o PWM.
void buzzcontrol_on(); // Controle de ativação do PWM.
void buzzcontrol_off(); // Controle de desativação do PWM.
void som(); // Configuração do som reproduzido pelo PWM.
bool repeating_timer_callback1(struct repeating_timer *t); // Timer utilizado na configuração de ativação do PWM.
void adcinit(); // Inicialização do conversor.
uint16_t media(uint8_t channel); // Responsável pela obtenção da média da leitura do ADC a fim de ter uma maior precisão.
void i2cinit(); // Inicialização do i2c.
void oledinit(); // Configuração do display.
void oleddis(); // Definição do que irá aparecer no display.
void minit(uint pin); // Configuração da Matriz de led.
void setled(const uint index, const uint8_t r, const uint8_t g, const uint8_t b); // Definindo como a matriz de led irá selecionar os leds.
void mdisplay(); // Responsável por ativar os devidos leds de acordo com as informações recebidas.
void led_clear_a(); // Limpar o símbolo utilizado pela matriz ativada no botão a.
void led_clear_b(); // Limpar o símbolo utilizado pela matriz ativada no botão b.
void press_a(); // Apresentar o símbolo utilizado pela matriz ativada no botão a.
void press_b(); // Apresentar o símbolo utilizado pela matriz ativada no botão b.
void reading_adc(); // Apresentar a leitura do conversor para permitir análises de funcionamento.

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
        reading_adc();
        (B.press1) ? press_a():led_clear_a();
        (B.press2) ? press_b():led_clear_b();
        if(B.impedir){
            B.impedir = false;
            if(B.press3){
                buzzcontrol_on();
                add_repeating_timer_ms(50, repeating_timer_callback1, NULL, &timer_pwm);
            }
            else {
                cancel_repeating_timer(&timer_pwm);
                buzzcontrol_off();
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

        if(gpio == botao_a && (current_time - lastA > 5000)){
            B.press1 = !B.press1;
            B.press2 = false;
            B.impedir = true;
            B.press3 = false;
            gpio_put(green_led, B.press1);
            gpio_put(blue_led, B.press2);
            gpio_put(red_led, B.press3);
            lastA = current_time;
        }

        if(((gpio == botao_b && B.press1) && !B.press3) && (current_time - lastB > 5000)){
            B.press2 = !B.press2;
            gpio_put(blue_led, B.press2);
            lastB = current_time;
        }

        if((gpio == botao_j && B.press1 && B.press2) && (current_time - lastJ > 5000)){
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

bool repeating_timer_callback1(struct repeating_timer *t){
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
    media_adc = 0;
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
    const uint8_t digit_leds[] = {17, 13, 12, 11, 7};
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
    const uint8_t digit_leds[] = {17, 13, 12, 11, 7};
    size_t a = sizeof(digit_leds) / sizeof(digit_leds[0]);
    for (size_t i = 0; i < a; ++i) {
        setled(digit_leds[i], 1, 0, 0);
    }
        mdisplay();   
}

void reading_adc(){
    uint16_t reading1 = media(adc_channel_0);
    uint16_t reading2 = media(adc_channel_1);
    printf("\nLeitura dos eixos Y e X: %d | %d", reading1, reading2);
}