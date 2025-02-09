//Bibliotecas Padrão
#include <stdio.h> //Uso de malloc e free
#include <string.h> //Para as funções strlen e strcpy
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

// Bibliotecas criadas para o projeto
#include "bibliotecas/control_leds.h"
#include "bibliotecas/numeros.h"
#include "Proj_U4_COMUNICACAO.pio.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define BOTAO_A 5
#define BOTAO_B 6
#define LED_VERDE 11
#define LED_AZUL 12
#define MATRIZ 7

// por padrão stdout é `uart0`
#define UART_ID uart0
#define BAUD_RATE 115200
#define UART_TX_PIN 0
#define UART_RX_PIN 1


//Criação do prototipo da função de interrupção
static void gpio_irq_handler(uint gpio, uint32_t events);

//Objetos globais
PIO pio = pio0;
uint sm; 
volatile int contador = 0;
static volatile uint32_t last_time = 0; // Armazena o tempo do último evento (em microssegundos)



int main()
{   // Inicializa o stdio para usar o Serial Monitor
     stdio_init_all();
     char c;

     while (!stdio_usb_connected()) {
        sleep_ms(100); // Aguarde 100ms antes de verificar novamente 
    }

    //Definir o LED Azul como saída
    gpio_init(LED_AZUL);
    gpio_set_dir(LED_AZUL,GPIO_OUT);

    //Definir o LED Verde como saída
    gpio_init(LED_VERDE);
    gpio_set_dir(LED_VERDE,GPIO_OUT);

    //Definir o botão A como entrada e pull up
    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A,GPIO_IN);
    gpio_pull_up(BOTAO_A);

    //Definir o botão B como entrada e pull up
    gpio_init(BOTAO_B);
    gpio_set_dir(BOTAO_B,GPIO_IN);
    gpio_pull_up(BOTAO_B);

    //coloca a frequência de clock para 128 MHz, facilitando a divisão pelo clock
    bool ok = set_sys_clock_khz(128000, false);

    //configurações da PIO
    uint offset = pio_add_program(pio, &Proj_U4_COMUNICACAO_program);
    uint sm = pio_claim_unused_sm(pio, true);
    Proj_U4_COMUNICACAO_program_init(pio, sm, offset, MATRIZ);

    //I2C
    i2c_init(I2C_PORT, 400 * 1000); //Inicializa a comunicacao I2C a 400KHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA); // Pull up the data line
    gpio_pull_up(I2C_SCL); // Pull up the clock line

    //Display
    ssd1306_t ssd; // Inicializa a estrutura do display
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
    ssd1306_send_data(&ssd); // Envia os dados para o display
    ssd1306_fill(&ssd, false); // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_send_data(&ssd); // Envia os dados para o display

    printf("Display inicializado!\n");

    // Inicializa a UART
    uart_init(UART_ID, BAUD_RATE);

    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART); // Configura o pino 0 para TX
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART); // Configura o pino 1 para RX

    printf("UART inicializado!\n");

    //Configuração da interrupão com callback incremento
    gpio_set_irq_enabled_with_callback(BOTAO_A,GPIO_IRQ_EDGE_FALL,true, &gpio_irq_handler);
    //Configuração da interrupão com callback decremento
    gpio_set_irq_enabled_with_callback(BOTAO_B,GPIO_IRQ_EDGE_FALL,true, &gpio_irq_handler);
    
    while (true) {
        if (stdio_usb_connected()) {
        printf("Entre com um CARACTERE!\n"); //Entre com uma frase
            if(scanf("%c",&c) == 1) {
                printf("Recebido: '%c'\n", c);   
                ssd1306_fill(&ssd, false);
                ssd1306_rect(&ssd, 3, 3, 122, 58, true, false);
                ssd1306_draw_string(&ssd, "RECEBIDO:", 8, 10);
                ssd1306_draw_char(&ssd, c, 60, 30);
                ssd1306_send_data(&ssd);

               if (c >= '0' && c<= '9') {
                exibir_numero(pio,sm, (uint) (c -'0')); // Aciona a matriz WS2812 para exibir o símbolo correspondente ao número encontrado
               }

            }
        }
    }
    
}

static void gpio_irq_handler(uint gpio, uint32_t events) {
    if (gpio == BOTAO_A) {
        uint32_t now = time_us_32();
        // Se menos de 200ms se passaram desde o último evento, ignora (debounce)
        if (now - last_time < 200000) return;
        last_time = now;
        
        // Alterna o estado do LED Verde
        bool current_state = gpio_get(LED_VERDE);
        gpio_put(LED_VERDE, !current_state);

        // Registra a operação no Serial Monitor
        if (!current_state) {
            printf("Botão A pressionado: LED Verde ligado.\n");
        } else {
            printf("Botão A pressionado: LED Verde desligado.\n");
        } 
    }

    if (gpio == BOTAO_B) {
        uint32_t now = time_us_32();
        // Se menos de 200ms se passaram desde o último evento, ignora (debounce)
        if (now - last_time < 200000) return;
        last_time = now;
        
        // Alterna o estado do LED Verde
        bool current_state = gpio_get(LED_AZUL);
        gpio_put(LED_AZUL, !current_state);

        // Registra a operação no Serial Monitor
        if (!current_state) {
            printf("Botão B pressionado: LED Azul ligado.\n");
        } else {
            printf("Botão B pressionado: LED Azul desligado.\n");
        } 
    }
}