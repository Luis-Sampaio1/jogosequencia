# Jogo de Sequência de Cores com Raspberry Pi Pico

Um jogo interativo onde o jogador deve repetir uma sequência de cores gerada aleatoriamente. Usa uma matriz de LEDs WS2818B, dois botões e um buzzer para feedback visual e sonoro.

-----
## Estrutura do Projeto

### Componentes Utilizados

-  Raspberry Pi Pico: Microcontrolador principal.
-  Matriz de LEDs WS2818B (NeoPixel)**: Exibe as cores do jogo.
-  Botões (A e B)**: Entrada do jogador.
-  Buzzer**: Feedback sonoro.
-----
## Estrutura de Dados

 ### `pixel\_t `

A estrutura pixel\_t é utilizada para armazenar as cores de cada LED na matriz. Cada LED possui três componentes de cor (vermelho, verde e azul), que podem ser configurados individualmente.

```c
typedef struct {
    uint8_t G, R, B;
} pixel_t;
````

- G (Green): Componente verde da cor.
- R (Red): Componente vermelho da cor.
- B (Blue): Componente azul da cor.

 ### `leds[LED\_COUNT]`

O array leds[] contém o estado de cada LED na matriz.

```c
pixel\_t leds[LED\_COUNT];
````
Uso: Serve como buffer temporário para armazenar as cores dos LEDs antes de enviá-las para a matriz real.

### `sequence[MAX\_SEQUENCE]`

O array sequence[] armazena a sequência de LEDs que o jogador deve memorizar.
```c
int sequence[MAX\_SEQUENCE];
````
Uso: A sequência é gerada aleatoriamente, e a dificuldade aumenta conforme o jogo avança.

### `player\_index e sequence\_length`
```c
int player\_index = 0;
int sequence\_length = 1;
````
- player\_index: Controla a posição atual do jogador na sequência.
- sequence\_length: Define o tamanho da sequência que o jogador deve memorizar.
-----
## Funções Principais

### ``npInit(uint pin)``

Inicializa o controlador PIO para os LEDs WS2812B.

```c
void npInit(uint pin) {
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;
    sm = pio_claim_unused_sm(np_pio, true);
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);
    pio_sm_set_enabled(np_pio, sm, true);
}

```

### `npSetLED(uint index, uint8\_t r, uint8\_t g, uint8\_t b)`

Define a cor de um LED na matriz.
```c
void npSetLED(uint index, uint8_t r, uint8_t g, uint8_t b) {
    if (index < LED_COUNT) {
        leds[index] = (pixel_t){g, r, b};
    }
}
```

### `npClear()`

Apaga todos os LEDs da matriz.

```c
void npClear() {
    for (uint i = 0; i < LED_COUNT; i++)
        npSetLED(i, 0, 0, 0);
}
```

### `npWrite()` 

Atualiza os LEDs com os dados armazenados no buffer.

```c
void npWrite() {
    for (uint i = 0; i < LED_COUNT; i++) {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
    sleep_us(300);
}
```

### `showSequence()`

Exibe a sequência de LEDs que o jogador deve memorizar.

```c
void showSequence() {
    npClear();
    npWrite();
    sleep_ms(500);

    for (int i = 0; i < sequence_length; i++) {
        npSetLED(sequence[i], 0, 255, 0);
        npWrite();
        sleep_ms(500);
        npClear();
        npWrite();
        sleep_ms(250);
    }
}
```

### `resetGame()`

Reinicia o jogo, gerando uma nova sequência aleatória de LEDs.

```c
void resetGame() {
    sequence_length = 1;
    player_index = 0;
    for (int i = 0; i < MAX_SEQUENCE; i++)
        sequence[i] = rand() % LED_COUNT;
    showSequence();
}
```
-----
## Como o Jogo Funciona

1. **Inicialização**: O jogo começa no nível 1, com uma sequência de 1 LED.
1. **Exibição da Sequência**: Cada LED da sequência pisca para o jogador memorizar.
1. **Entrada do Jogador**:
   1. Botão A: Representa o LED Azul.
   1. Botão B: Representa o LED Vermelho.
   1. O jogo verifica se a entrada está correta.
1. **Progressão**:
   1. Se acertar, o nível aumenta e uma nova cor é adicionada.
   1. Se errar, o jogo reinicia.
1. **Feedback**:
   1. O buzzer emite sons diferentes para acertos e erros.
   1. A matriz de LEDs exibe cores específicas para indicar sucesso (verde) ou falha (vermelho).
-----


