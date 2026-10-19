
#include <stdint.h>

/* =========================================================
   STM32F446RE - Servo SG90
   Pin de señal: PA6 (TIM3_CH1)
   Frecuencia PWM: 50 Hz
   Movimiento: 15° -> 165° -> 15°
   ========================================================= */

/* Direcciones base */
#define RCC_BASE       0x40023800UL
#define GPIOA_BASE     0x40020000UL
#define TIM3_BASE      0x40000400UL

/* Registros RCC */
#define RCC_AHB1ENR    (*(volatile uint32_t *)(RCC_BASE + 0x30UL))
#define RCC_APB1ENR    (*(volatile uint32_t *)(RCC_BASE + 0x40UL))

/* Registros GPIOA */
#define GPIOA_MODER    (*(volatile uint32_t *)(GPIOA_BASE + 0x00UL))
#define GPIOA_AFRL     (*(volatile uint32_t *)(GPIOA_BASE + 0x20UL))

/* Registros TIM3 */
#define TIM3_CR1       (*(volatile uint32_t *)(TIM3_BASE + 0x00UL))
#define TIM3_EGR       (*(volatile uint32_t *)(TIM3_BASE + 0x14UL))
#define TIM3_CCMR1     (*(volatile uint32_t *)(TIM3_BASE + 0x18UL))
#define TIM3_CCER      (*(volatile uint32_t *)(TIM3_BASE + 0x20UL))
#define TIM3_PSC       (*(volatile uint32_t *)(TIM3_BASE + 0x28UL))
#define TIM3_ARR       (*(volatile uint32_t *)(TIM3_BASE + 0x2CUL))
#define TIM3_CCR1      (*(volatile uint32_t *)(TIM3_BASE + 0x34UL))

/* Parámetros del servo */
#define SERVO_MIN_US   1100U
#define SERVO_MAX_US   1900U

#define ANGLE_MIN      15
#define ANGLE_MAX      165
#define ANGLE_STEP     2

/*
 * Retardo aproximado por software.
 * Su duración depende de la frecuencia del reloj
 * y de las opciones de compilación.
 */
static void delay(volatile uint32_t count)
{
    while (count > 0U)
    {
        count--;
    }
}

/* Configuración del PWM y del pin PA6 */
static void servo_init(void)
{
    /* Habilitar relojes de GPIOA y TIM3 */
    RCC_AHB1ENR |= (1UL << 0);
    RCC_APB1ENR |= (1UL << 1);

    /* Esperar a que se apliquen los relojes */
    (void)RCC_AHB1ENR;
    (void)RCC_APB1ENR;

    /* PA6 en modo función alternativa (10) */
    GPIOA_MODER &= ~(3UL << (6U * 2U));
    GPIOA_MODER |=  (2UL << (6U * 2U));

    /* PA6: función alternativa AF2 = TIM3_CH1 */
    GPIOA_AFRL &= ~(0xFUL << (6U * 4U));
    GPIOA_AFRL |=  (2UL << (6U * 4U));

    /*
     * Configuración para reloj TIM3 de 16 MHz:
     * PSC = 15  -> contador a 1 MHz (1 us por cuenta)
     * ARR = 19999 -> periodo de 20 ms (50 Hz)
     */
    TIM3_CR1 = 0U;
    TIM3_PSC = 15U;
    TIM3_ARR = 19999U;

    /* Canal 1: PWM modo 1 y precarga habilitada */
    TIM3_CCMR1 &= ~(7UL << 4U);
    TIM3_CCMR1 |=  (6UL << 4U);
    TIM3_CCMR1 |=  (1UL << 3U);

    /* Habilitar salida del canal 1 */
    TIM3_CCER |= (1UL << 0);

    /* Posición inicial: pulso de 1500 us */
    TIM3_CCR1 = 1500U;

    /* Aplicar configuración de registros */
    TIM3_EGR = 1UL;

    /* Habilitar precarga del periodo y arrancar TIM3 */
    TIM3_CR1 |= (1UL << 7U);
    TIM3_CR1 |= (1UL << 0U);
}

/* Convertir ángulo en ancho de pulso */
static void servo_set_angle(int angle)
{
    uint32_t pulse;

    /* Limitar el ángulo al intervalo permitido */
    if (angle < 0)
    {
        angle = 0;
    }

    if (angle > 180)
    {
        angle = 180;
    }

    /*
     * 0°   -> 1100 us
     * 90°  -> 1500 us
     * 180° -> 1900 us
     */
    pulse = SERVO_MIN_US +
            ((uint32_t)angle *
             (SERVO_MAX_US - SERVO_MIN_US)) / 180U;

    TIM3_CCR1 = pulse;
}

int main(void)
{
    int angle = ANGLE_MIN;
    int direction = 1;

    servo_init();
    servo_set_angle(angle);

    while (1)
    {
        /* Mantener cada posición un instante */
        delay(30000U);

        /* Avanzar o retroceder en el barrido */
        angle += direction * ANGLE_STEP;

        /* Llegada al extremo superior */
        if (angle >= ANGLE_MAX)
        {
            angle = ANGLE_MAX;
            direction = -1;
        }
        /* Llegada al extremo inferior */
        else if (angle <= ANGLE_MIN)
        {
            angle = ANGLE_MIN;
            direction = 1;
        }

        servo_set_angle(angle);
    }
}
