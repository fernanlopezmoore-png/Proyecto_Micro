#ifndef SOUND_H // Evita incluir este archivo mas de una vez.
#define SOUND_H // Activa la proteccion de inclusion.

#include <stdint.h> // Permite usar uint8_t y uint32_t.
#include "stm32f3xx.h" // Da acceso a los registros del STM32F303.

static uint8_t decimalToUint32(const char *text, uint32_t *value) // Convierte texto decimal a entero.
{ // Inicia la funcion de conversion.
    uint32_t result = 0; // Guarda el numero que se va formando.
    uint32_t digit; // Guarda cada digito encontrado.
    if (text == NULL || *text == '\0' || value == NULL) // Rechaza datos vacios o punteros invalidos.
        return 0; // Indica que la conversion fallo.
    while (*text != '\0') // Recorre el texto hasta el final.
    { // Inicia el procesamiento de un caracter.
        if (*text < '0' || *text > '9') // Verifica que el caracter sea un digito decimal.
            return 0; // Rechaza letras y otros caracteres.
        digit = (uint32_t)(*text - '0'); // Convierte el caracter a su valor numerico.
        if (result > (0xFFFFFFFFu - digit) / 10u) // Detecta si el numero excederia 32 bits.
            return 0; // Rechaza el numero demasiado grande.
        result = result * 10u + digit; // Agrega el digito al resultado.
        text++; // Avanza al siguiente caracter.
    } // Termina el recorrido del texto.
    *value = result; // Entrega el numero convertido.
    return 1; // Indica que la conversion fue correcta.
} // Termina decimalToUint32.

static void initSound(void) // Prepara el pin del buzzer y el temporizador TIM3.
{ // Inicia la configuracion del sonido.
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN; // Enciende el reloj del puerto GPIOB.
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN; // Enciende el reloj de TIM3.
    GPIOB->BSRR = 1u << 20; // Coloca PB4 en nivel bajo para iniciar en silencio.
    GPIOB->MODER &= ~(3u << 8); // Limpia los bits de modo de PB4.
    GPIOB->MODER |= 1u << 8; // Configura PB4 inicialmente como salida normal.
    GPIOB->AFR[0] &= ~(15u << 16); // Limpia la funcion alternativa de PB4.
    GPIOB->AFR[0] |= 2u << 16; // Selecciona la funcion alternativa AF2 de TIM3.
    TIM3->CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1PE; // Prepara PWM en el canal 1.
    TIM3->CR1 |= TIM_CR1_ARPE; // Permite actualizar el periodo de forma ordenada.
    TIM3->CCER = 0; // Deja desactivada la salida del temporizador.
} // Termina initSound.

static void muteSound(void) // Apaga el buzzer.
{ // Inicia la rutina para silenciar.
    TIM3->CR1 &= ~TIM_CR1_CEN; // Detiene TIM3.
    TIM3->CCER &= ~TIM_CCER_CC1E; // Desactiva el canal de salida 1.
    GPIOB->BSRR = 1u << 20; // Fuerza PB4 a nivel bajo.
    GPIOB->MODER = (GPIOB->MODER & ~(3u << 8)) | (1u << 8); // Devuelve PB4 al modo de salida normal.
} // Termina muteSound.

static uint8_t playSound(uint32_t frequency) // Genera sonido con la frecuencia indicada en hertz.
{ // Inicia la rutina de sonido.
    uint32_t timerClock; // Guarda la frecuencia del reloj de TIM3.
    uint32_t prescaler; // Guarda el divisor de reloj de TIM3.
    uint32_t period; // Guarda la cantidad de pulsos por ciclo.
    uint32_t apb1Prescaler; // Guarda la configuracion del divisor APB1.

    if (frequency == 0u) // Verifica si se pidio SOUND 0.
    { // Inicia el caso de silencio.
        muteSound(); // Apaga el buzzer.
        return 1; // Indica que la orden se ejecuto correctamente.
    } // Termina el caso de silencio.

    SystemCoreClockUpdate(); // Actualiza el valor conocido del reloj del microcontrolador.
    timerClock = SystemCoreClock; // Parte de la frecuencia del reloj principal.
    apb1Prescaler = (RCC->CFGR >> 8) & 7u; // Lee el divisor APB1 configurado.
    if (apb1Prescaler > 4u) // Comprueba si hay que ajustar el reloj del timer.
        timerClock >>= apb1Prescaler - 4u; // Ajusta la frecuencia efectiva de TIM3.
    if (frequency > timerClock / 2u) // Verifica que haya al menos dos pulsos por ciclo.
        return 0; // Indica que la frecuencia solicitada es demasiado alta.

    period = timerClock / frequency; // Calcula el periodo antes de escoger el divisor.
    prescaler = (period - 1u) / 65536u + 1u; // Ajusta el periodo al rango de 16 bits.
    period = timerClock / (prescaler * frequency); // Calcula el periodo final de TIM3.

    TIM3->CR1 &= ~TIM_CR1_CEN; // Detiene TIM3 mientras cambia sus valores.
    TIM3->PSC = prescaler - 1u; // Programa el divisor de reloj.
    TIM3->ARR = period - 1u; // Programa la duracion de un ciclo.
    TIM3->CCR1 = period / 2u; // Produce una onda cuadrada cercana al 50 por ciento.
    TIM3->EGR = TIM_EGR_UG; // Aplica los nuevos valores al temporizador.
    TIM3->CNT = 0; // Reinicia el contador del temporizador.
    GPIOB->MODER = (GPIOB->MODER & ~(3u << 8)) | (2u << 8); // Conecta PB4 a la funcion alternativa.
    TIM3->CCER |= TIM_CCER_CC1E; // Activa la salida del canal 1.
    TIM3->CR1 |= TIM_CR1_CEN; // Inicia TIM3 y el sonido.
    return 1; // Indica que el sonido se activo correctamente.
} // Termina playSound.

// En main.c, junto a los otros encabezados, esta esta linea:
// #include "Sound.h"
// En main(), despues de iniciar USART1, esta esta linea:
// initSound();
// En executeCommand(), estas son las lineas que reconocen SOUND:
// else if (strcmp(command, "SOUND") == 0)
// {
//     a = strtok(NULL, " \t");
//     b = strtok(NULL, " \t");
//     if (a == NULL || b != NULL || !decimalToUint32(a, &data))
//     {
//         USART1_String("Error: use SOUND freq en hertz\r\n");
//         return;
//     }
//     if (!playSound(data))
//     {
//         USART1_String("Error: frecuencia demasiado alta\r\n");
//         return;
//     }
//     USART1_String(data == 0u ? "Buzzer silenciado\r\n" : "Sonido activado\r\n");
// }
// En executeCommand(), estas son las lineas que reconocen MUTE:
// else if (strcmp(command, "MUTE") == 0)
// {
//     if (strtok(NULL, " \t") != NULL)
//     {
//         USART1_String("Error: use MUTE\r\n");
//         return;
//     }
//     muteSound();
//     USART1_String("Buzzer silenciado\r\n");
// }

#endif // Termina la proteccion de inclusion de Sound.h.
