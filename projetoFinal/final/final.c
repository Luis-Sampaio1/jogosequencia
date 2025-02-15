#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "ws2818b.pio.h"

// Definições dos pinos
#define LED_COUNT 25          // Número de LEDs na matriz
#define LED_PIN 7             // GPIO conectado à matriz de LEDs
#define BUTTON_A 5            // GPIO conectado ao Botão A
#define BUTTON_B 6            // GPIO conectado ao Botão B
#define BUZZER_PIN 21         // GPIO conectado ao buzzer
#define BRIGHTNESS 20         // Intensidade dos LEDs (0 a 255)

// Estrutura para representar um pixel (LED)
struct pixel_t {
    uint8_t G, R, B;
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t;

// Variáveis globais
npLED_t leds[LED_COUNT];      // Array para armazenar o estado dos LEDs
PIO np_pio;                   // PIO usado para controlar a matriz de LEDs
uint sm;                      // State Machine do PIO

// Função para inicializar a matriz de LEDs
void npInit(uint pin) {
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;
    sm = pio_claim_unused_sm(np_pio, false);
    if (sm < 0) {
        np_pio = pio1;
        sm = pio_claim_unused_sm(np_pio, true);
    }
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);
    for (uint i = 0; i < LED_COUNT; ++i) {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
}

// Função para definir a cor de um LED
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
    leds[index].R = (r * BRIGHTNESS) / 150;
    leds[index].G = (g * BRIGHTNESS) / 150;
    leds[index].B = (b * BRIGHTNESS) / 150;
}

// Função para apagar todos os LEDs
void npClear() {
    for (uint i = 0; i < LED_COUNT; ++i)
        npSetLED(i, 0, 0, 0);
}

// Função para atualizar a matriz de LEDs
void npWrite() {
    for (uint i = 0; i < LED_COUNT; ++i) {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
}

// Função para inicializar o PWM no pino do buzzer
void pwm_init_buzzer(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 1.0f); // Ajusta divisor de clock
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(pin, 0); // Desliga o PWM inicialmente
}

// Função para tocar uma nota com frequência e duração específicas
void play_tone(uint pin, uint frequency, uint duration_ms) {
    uint slice_num = pwm_gpio_to_slice_num(pin);
    uint32_t clock_freq = clock_get_hz(clk_sys);
    uint32_t top = clock_freq / frequency - 1;

    pwm_set_wrap(slice_num, top);
    pwm_set_gpio_level(pin, top * 3 / 4); // 75% de duty cycle

    sleep_ms(duration_ms);

    pwm_set_gpio_level(pin, 0); // Desliga o som após a duração
    sleep_ms(50); // Pausa entre notas
}

// Função para mostrar todos os LEDs em uma cor específica
void show_color(uint8_t r, uint8_t g, uint8_t b) {
    for (int i = 0; i < LED_COUNT; i++) {
        npSetLED(i, r, g, b);
    }
    npWrite();
    sleep_ms(1000);  // Mantém os LEDs acesos por 1 segundo
    npClear();
    npWrite();
}

// Função para mostrar a sequência de cores
void show_sequence(int sequence[], int length) {
    for (int i = 0; i < length; i++) {
        npClear();
        if (sequence[i] == 0) {
            show_color(0, 0, 50);  // Azul (Botão A)
        } else {
            show_color(50, 0, 0);  // Vermelho (Botão B)
        }
        sleep_ms(500);  // Intervalo entre as cores
    }
}

// Função para ler a entrada do jogador
void get_player_input(int player_sequence[], int length) {
    for (int i = 0; i < length; i++) {
        while (true) {
            if (!gpio_get(BUTTON_A)) {  // Botão A pressionado
                player_sequence[i] = 0; // Azul
                printf("Botão A pressionado\n"); // Log para depuração
                play_tone(BUZZER_PIN, 500, 100); // Feedback sonoro
                sleep_ms(200); // Debounce
                break;
            } else if (!gpio_get(BUTTON_B)) {  // Botão B pressionado
                player_sequence[i] = 1; // Vermelho
                printf("Botão B pressionado\n"); // Log para depuração
                play_tone(BUZZER_PIN, 500, 100); // Feedback sonoro
                sleep_ms(200); // Debounce
                break;
            }
        }
    }
}

// Função para verificar se a sequência do jogador está correta
bool check_sequence(int sequence[], int player_sequence[], int length) {
    for (int i = 0; i < length; i++) {
        if (sequence[i] != player_sequence[i]) {
            return false;  // Erro na sequência
        }
    }
    return true;  // Sequência correta
}

int main() {
    // Inicialização dos GPIOs
    stdio_init_all();
    npInit(LED_PIN);
    npClear();

    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);

    // Inicializa o buzzer
    pwm_init_buzzer(BUZZER_PIN);

    // Inicialização do gerador de números aleatórios
    srand(time(NULL));

    // Variáveis do jogo
    int sequence[100];  // Armazena a sequência de cores
    int level = 1;      // Nível atual do jogo

    while (true) {
        // Gera uma nova sequência aleatória
        for (int i = 0; i < level; i++) {
            sequence[i] = rand() % 2;  // 0 = Azul (A), 1 = Vermelho (B)
        }

        // Mostra a sequência ao jogador
        printf("Sequência gerada: ");
        for (int i = 0; i < level; i++) {
            printf("%s ", sequence[i] == 0 ? "A" : "B");
        }
        printf("\n");

        show_sequence(sequence, level);

        // Lê a entrada do jogador
        int player_sequence[level];
        get_player_input(player_sequence, level);

        printf("Sequência do jogador: ");
        for (int i = 0; i < level; i++) {
            printf("%s ", player_sequence[i] == 0 ? "A" : "B");
        }
        printf("\n");

        // Verifica se a sequência está correta
        if (check_sequence(sequence, player_sequence, level)) {
            printf("Acertou! Avançando para o nível %d\n", level + 1);
            play_tone(BUZZER_PIN, 1000, 200);  // Som de acerto
            show_color(0, 50, 0);             // Mostra verde na matriz de LEDs
            level++;                          // Aumenta a dificuldade
        } else {
            printf("Errou! Reiniciando o jogo.\n");
            play_tone(BUZZER_PIN, 100, 500);  // Som de erro
            show_color(50, 0, 0);            // Mostra vermelho na matriz de LEDs
            level = 1;                        // Reinicia o jogo
        }
    }
}