
#include <stdint.h>

/* RCC: control de relojes */
#define RCC_BASE       0x40023800UL
#define RCC_AHB1ENR    (*(volatile uint32_t *)(RCC_BASE + 0x30UL))
#define RCC_APB1ENR    (*(volatile uint32_t *)(RCC_BASE + 0x40UL))

/* GPIOA */
#define GPIOA_BASE     0x40020000UL
#define GPIOA_MODER    (*(volatile uint32_t *)(GPIOA_BASE + 0x00UL))
#define GPIOA_AFRL     (*(volatile uint32_t *)(GPIOA_BASE + 0x20UL))

/* TIM3 */
#define TIM3_BASE      0x40000400UL
#define TIM3_CR1       (*(volatile uint32_t *)(TIM3_BASE + 0x00UL))
#define TIM3_EGR       (*(volatile uint32_t *)(TIM3_BASE + 0x14UL))
#define TIM3_CCMR1     (*(volatile uint32_t *)(TIM3_BASE + 0x18UL))
#define TIM3_CCER      (*(volatile uint32_t *)(TIM3_BASE + 0x20UL))
#define TIM3_PSC       (*(volatile uint32_t *)(TIM3_BASE + 0x28UL))
#define TIM3_ARR       (*(volatile uint32_t *)(TIM3_BASE + 0x2CUL))
#define TIM3_CCR1      (*(volatile uint32_t *)(TIM3_BASE + 0x34UL))

static void delay(volatile uint32_t count)
{
    while (count > 0U) {
        count--;
    }
}

static void servo_init(void)
{
    /* Habilitar relojes de GPIOA y TIM3 */
    RCC_AHB1ENR |= (1U << 0);
    RCC_APB1ENR |= (1U << 1);

    /* PA6 en modo función alternativa: MODER6 = 10 */
    GPIOA_MODER &= ~(3U << 12);
    GPIOA_MODER |=  (2U << 12);

    /* PA6: AF2, correspondiente a TIM3_CH1 */
    GPIOA_AFRL &= ~(0xFU << 24);
    GPIOA_AFRL |=  (2U << 24);

    /* Detener el temporizador mientras lo configuramos */
    TIM3_CR1 = 0U;

    /*
     * Reloj del temporizador: 16 MHz
     * PSC = 15 -> contador de 1 MHz (1 us por cuenta)
     * ARR = 19999 -> período de 20 ms
     */
    TIM3_PSC = 15U;
    TIM3_ARR = 19999U;
    TIM3_CCR1 = 1500U;

    /*
     * CH1 en PWM modo 1:
     * OC1M = 110, OC1PE = 1
     */
    TIM3_CCMR1 &= ~(7U << 4);
    TIM3_CCMR1 |=  (6U << 4);
    TIM3_CCMR1 |=  (1U << 3);

    /* Habilitar salida del canal 1 */
    TIM3_CCER &= ~(1U << 1);
    TIM3_CCER |=  (1U << 0);

    /* Cargar registros de precarga */
    TIM3_EGR = 1U;

    /* Habilitar precarga de ARR y arrancar contador */
    TIM3_CR1 |= (1U << 7);
    TIM3_CR1 |= (1U << 0);
}

int main(void)
{
    servo_init();

    while (1) {
        /* Posición aproximada izquierda */
        TIM3_CCR1 = 1100U;
        delay(1000000U);

        /* Posición aproximada central */
        TIM3_CCR1 = 1500U;
        delay(1000000U);

        /* Posición aproximada derecha */
        TIM3_CCR1 = 1900U;
        delay(1000000U);
    }
}
