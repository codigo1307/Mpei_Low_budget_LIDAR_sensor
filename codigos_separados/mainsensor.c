
#include <stdint.h>

/* =========================================================
   PROYECTO: Prueba HC-SR04
   MCU: STM32F446RE
   TRIG: PA1
   ECHO: PA0 (con divisor de voltaje)
   UART2: PA2 (TX), PA3 (RX)
   Temporizador: TIM2 a 1 MHz
   ========================================================= */

/* Direcciones base */
#define RCC_BASE       0x40023800UL
#define GPIOA_BASE     0x40020000UL
#define TIM2_BASE      0x40000000UL
#define USART2_BASE    0x40004400UL

/* RCC */
#define RCC_AHB1ENR    (*(volatile uint32_t *)(RCC_BASE + 0x30UL))
#define RCC_APB1ENR    (*(volatile uint32_t *)(RCC_BASE + 0x40UL))

/* GPIOA */
#define GPIOA_MODER    (*(volatile uint32_t *)(GPIOA_BASE + 0x00UL))
#define GPIOA_IDR      (*(volatile uint32_t *)(GPIOA_BASE + 0x10UL))
#define GPIOA_BSRR     (*(volatile uint32_t *)(GPIOA_BASE + 0x18UL))
#define GPIOA_AFRL     (*(volatile uint32_t *)(GPIOA_BASE + 0x20UL))

/* TIM2 */
#define TIM2_CR1       (*(volatile uint32_t *)(TIM2_BASE + 0x00UL))
#define TIM2_CNT       (*(volatile uint32_t *)(TIM2_BASE + 0x24UL))
#define TIM2_PSC       (*(volatile uint32_t *)(TIM2_BASE + 0x28UL))
#define TIM2_ARR       (*(volatile uint32_t *)(TIM2_BASE + 0x2CUL))
#define TIM2_EGR       (*(volatile uint32_t *)(TIM2_BASE + 0x14UL))

/* USART2 */
#define USART2_SR      (*(volatile uint32_t *)(USART2_BASE + 0x00UL))
#define USART2_DR      (*(volatile uint32_t *)(USART2_BASE + 0x04UL))
#define USART2_BRR     (*(volatile uint32_t *)(USART2_BASE + 0x08UL))
#define USART2_CR1     (*(volatile uint32_t *)(USART2_BASE + 0x0CUL))

/* Pines */
#define TRIG_PIN       1U
#define ECHO_PIN       0U
#define UART_TX_PIN    2U
#define UART_RX_PIN    3U

/* Parámetros */
#define DETECTION_LIMIT_CM  100U
#define ECHO_TIMEOUT_US     30000U

/* ---------------------------------------------------------
   Temporizador: cuenta microsegundos
   Supone reloj TIM2 de 16 MHz al arrancar con HSI.
   --------------------------------------------------------- */
static void timer_init(void)
{
    RCC_APB1ENR |= (1UL << 0);

    TIM2_CR1 = 0U;
    TIM2_PSC = 15U;
    TIM2_ARR = 0xFFFFFFFFUL;
    TIM2_EGR = 1U;
    TIM2_CNT = 0U;
    TIM2_CR1 = 1U;
}

static void delay_us(uint32_t us)
{
    uint32_t start = TIM2_CNT;

    while ((uint32_t)(TIM2_CNT - start) < us)
    {
        /* Esperar */
    }
}

/* ---------------------------------------------------------
   UART2: 115200 baudios, 8 bits, sin paridad, 1 stop
   --------------------------------------------------------- */
static void uart_init(void)
{
    /* Habilitar GPIOA y USART2 */
    RCC_AHB1ENR |= (1UL << 0);
    RCC_APB1ENR |= (1UL << 17);

    /* PA2 y PA3 en modo función alternativa */
    GPIOA_MODER &= ~((3UL << (UART_TX_PIN * 2U)) |
                     (3UL << (UART_RX_PIN * 2U)));

    GPIOA_MODER |=  ((2UL << (UART_TX_PIN * 2U)) |
                     (2UL << (UART_RX_PIN * 2U)));

    /* AF7 para USART2 en PA2 y PA3 */
    GPIOA_AFRL &= ~((0xFUL << (UART_TX_PIN * 4U)) |
                    (0xFUL << (UART_RX_PIN * 4U)));

    GPIOA_AFRL |=  ((7UL << (UART_TX_PIN * 4U)) |
                    (7UL << (UART_RX_PIN * 4U)));

    /* Deshabilitar mientras se configura */
    USART2_CR1 = 0U;

    /* 16 MHz / 115200 ≈ 139 */
    USART2_BRR = 139U;

    /* Habilitar transmisor, receptor y USART */
    USART2_CR1 = (1UL << 3U) |
                 (1UL << 2U) |
                 (1UL << 13U);
}

static void uart_send_char(char c)
{
    while ((USART2_SR & (1UL << 7U)) == 0U)
    {
        /* Esperar registro de transmisión vacío */
    }

    USART2_DR = (uint32_t)c;
}

static void uart_send_string(const char *text)
{
    while (*text != '\0')
    {
        uart_send_char(*text);
        text++;
    }
}

static void uart_send_uint(uint32_t value)
{
    char digits[10];
    uint32_t i = 0U;

    if (value == 0U)
    {
        uart_send_char('0');
        return;
    }

    while (value > 0U && i < sizeof(digits))
    {
        digits[i] = (char)('0' + (value % 10U));
        value /= 10U;
        i++;
    }

    while (i > 0U)
    {
        i--;
        uart_send_char(digits[i]);
    }
}

/* ---------------------------------------------------------
   HC-SR04
   --------------------------------------------------------- */
static void sensor_init(void)
{
    /* PA0 como entrada */
    GPIOA_MODER &= ~(3UL << (ECHO_PIN * 2U));

    /* PA1 como salida */
    GPIOA_MODER &= ~(3UL << (TRIG_PIN * 2U));
    GPIOA_MODER |=  (1UL << (TRIG_PIN * 2U));

    /* TRIG inicialmente en LOW */
    GPIOA_BSRR = (1UL << (TRIG_PIN + 16U));
}

/*
 * Espera a que ECHO alcance el nivel solicitado.
 * Devuelve 1 si lo consigue y 0 si vence el tiempo.
 */
static uint32_t wait_echo_level(uint32_t high, uint32_t timeout_us)
{
    uint32_t start = TIM2_CNT;

    while (1)
    {
        uint32_t level = (GPIOA_IDR >> ECHO_PIN) & 1UL;

        if (level == high)
        {
            return 1U;
        }

        if ((uint32_t)(TIM2_CNT - start) >= timeout_us)
        {
            return 0U;
        }
    }
}

/*
 * Devuelve la distancia en cm.
 * Devuelve 0 si no se recibe un eco válido.
 * Las distancias válidas se consideran de 2 a 400 cm.
 */
static uint32_t measure_distance_cm(void)
{
    uint32_t pulse_start;
    uint32_t pulse_width;
    uint32_t distance_cm;

    /* Asegurar TRIG en LOW antes de comenzar */
    GPIOA_BSRR = (1UL << (TRIG_PIN + 16U));
    delay_us(3U);

    /* Pulso de disparo de al menos 10 us */
    GPIOA_BSRR = (1UL << TRIG_PIN);
    delay_us(10U);
    GPIOA_BSRR = (1UL << (TRIG_PIN + 16U));

    /* Esperar inicio del eco */
    if (wait_echo_level(1U, ECHO_TIMEOUT_US) == 0U)
    {
        return 0U;
    }

    pulse_start = TIM2_CNT;

    /* Esperar final del eco */
    if (wait_echo_level(0U, ECHO_TIMEOUT_US) == 0U)
    {
        return 0U;
    }

    pulse_width = (uint32_t)(TIM2_CNT - pulse_start);

    /*
     * HC-SR04:
     * distancia en cm ≈ duración del eco en us / 58
     */
    distance_cm = pulse_width / 58U;

    if (distance_cm < 2U || distance_cm > 400U)
    {
        return 0U;
    }

    return distance_cm;
}

/* ---------------------------------------------------------
   Programa principal
   --------------------------------------------------------- */
int main(void)
{
    uint32_t distance_cm;

    timer_init();
    uart_init();
    sensor_init();

    delay_us(100000U);

    uart_send_string("\r\n");
    uart_send_string("================================\r\n");
    uart_send_string("   PRUEBA SENSOR HC-SR04\r\n");
    uart_send_string("   STM32F446RE - UART2\r\n");
    uart_send_string("================================\r\n");
    uart_send_string("Umbral de deteccion: 100 cm\r\n");
    uart_send_string("Acerca un objeto al sensor.\r\n\r\n");

    while (1)
    {
        distance_cm = measure_distance_cm();

        if (distance_cm == 0U)
        {
            uart_send_string("SIN ECO: no hay medicion valida\r\n");
        }
        else
        {
            uart_send_string("Distancia: ");
            uart_send_uint(distance_cm);
            uart_send_string(" cm | ");

            if (distance_cm <= DETECTION_LIMIT_CM)
            {
                uart_send_string("DETECTADO\r\n");
            }
            else
            {
                uart_send_string("FUERA DEL UMBRAL\r\n");
            }
        }

        /* Pausa entre mediciones */
        delay_us(200000U);
    }
}
