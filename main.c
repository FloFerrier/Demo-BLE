#include "main.h"

// needed by FreeRTOS
uint32_t SystemCoreClock;

static char pBufferConsoleDebug[BUFFER_CONSOLE_LEN_MAX+1] = "";

void console_debug(const char *format, ...);

void vTaskBluetooth(void *pvParameters);

int main(void)
{
    /* Clock settings */
    rcc_clock_setup_pll(rcc_hse_8mhz_3v3);
    SystemCoreClock = rcc_ahb_frequency;
    systick_set_frequency(configTICK_RATE_HZ, SystemCoreClock);

    /* Console init */
    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2);
    gpio_set_af(GPIOA, GPIO_AF7, GPIO2);
    rcc_periph_clock_enable(RCC_USART2);
    usart_disable(USART2);
    usart_set_baudrate(USART2, 115200);
    usart_set_databits(USART2, 8);
    usart_set_stopbits(USART2, USART_STOPBITS_1);
    usart_set_mode(USART2, USART_MODE_TX_RX);
    usart_set_parity(USART2, USART_PARITY_NONE);
    usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);
    usart_enable(USART2);

    /* Tasks creation */
    xTaskCreate(vTaskBluetooth,
        (const char *)"Bluetooth",
        configMINIMAL_STACK_SIZE * 2,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL);

    taskENABLE_INTERRUPTS();
    vTaskStartScheduler();

    while(1)
    {
        // should never fall here
    };

    return 0;
}

void console_debug(const char *format, ...)
{
    va_list args;
    va_start (args, format);
    uint32_t buffer_size = 0;
    buffer_size = vsnprintf(pBufferConsoleDebug, BUFFER_CONSOLE_LEN_MAX, format, args);
    for(uint32_t i = 0; i < buffer_size; i++)
    {
        usart_send_blocking(USART2, (uint16_t)pBufferConsoleDebug[i]);
    }
    va_end(args);
}

void vTaskBluetooth(void *pvParameters)
{
    (void)pvParameters;

    while(1)
    {
        console_debug("[BLE] Bluetooth task\r\n");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
