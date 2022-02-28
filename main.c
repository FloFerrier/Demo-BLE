#include "main.h"
#include "rn4871_defs.h"

#define TRANSPARENT_UART true

// needed by FreeRTOS
uint32_t SystemCoreClock;

static char pBufferConsoleDebug[BUFFER_CONSOLE_LEN_MAX+1] = "";
static char pBufferUartTx[BUFFER_CONSOLE_LEN_MAX+1] = "";
static char pBufferBleRx[BUFFER_UART_LEN_MAX+1] = "";
static char pBufferBleTx[BUFFER_UART_LEN_MAX+1] = "";

EventGroupHandle_t xEventsBleUart;

QueueHandle_t xQueueConsoleDebug;
QueueHandle_t xQueueBleUartRx;
QueueHandle_t xQueueBleUartTx;

static enum rn4871_cmd_e _current_cmd = CMD_NONE;
static bool _stream_open = false;

extern void usart3_isr(void);
static int8_t rn4871_uart_tx(uint8_t *pBuffer, uint16_t buffer_size);
static void rn4871_send_cmd(enum rn4871_cmd_e cmd);
static void rn4871_process_resp(const char *buffer);

void vTaskConsoleDebug(void *pvParameters);
void vTaskBluetooth(void *pvParameters);
void vTaskSensor(void *pvParameters);

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

    /* BLE UART Tx and Rx init */
    rcc_periph_clock_enable(RCC_GPIOC);
    gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO10 | GPIO11);
    gpio_set_af(GPIOC, GPIO_AF7, GPIO10 | GPIO11);

    rcc_periph_clock_enable(RCC_USART3);
    nvic_disable_irq(NVIC_USART3_IRQ);
    usart_disable_rx_interrupt(USART3);
    usart_disable(USART3);
    usart_set_baudrate(USART3, 115200);
    usart_set_databits(USART3, 8);
    usart_set_stopbits(USART3, USART_STOPBITS_1);
    usart_set_mode(USART3, USART_MODE_TX_RX);
    usart_set_parity(USART3, USART_PARITY_NONE);
    usart_set_flow_control(USART3, USART_FLOWCONTROL_NONE);
    nvic_set_priority(NVIC_USART3_IRQ, 255);
    usart_enable_rx_interrupt(USART3);
    usart_enable(USART3);
    nvic_enable_irq(NVIC_USART3_IRQ);

    /* Tasks creation */
    xTaskCreate(vTaskBluetooth,
        (const char *)"Bluetooth",
        configMINIMAL_STACK_SIZE * 2,
        NULL,
        tskIDLE_PRIORITY + 2,
        NULL);

    xTaskCreate(vTaskSensor,
        (const char *)"Sensor",
        configMINIMAL_STACK_SIZE * 2,
        NULL,
        tskIDLE_PRIORITY + 2,
        NULL);

    xTaskCreate(vTaskConsoleDebug,
        (const char *)"Console",
        configMINIMAL_STACK_SIZE * 2,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL);

    /* Queues creation */
    xQueueBleUartRx = xQueueCreate(10, sizeof(char) * BUFFER_UART_LEN_MAX);
    xQueueBleUartTx = xQueueCreate(10, sizeof(char) * BUFFER_UART_LEN_MAX);
    xQueueConsoleDebug = xQueueCreate(10, sizeof(char) * BUFFER_CONSOLE_LEN_MAX);

    /* Events creations */
    xEventsBleUart = xEventGroupCreate();

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
    vsnprintf(pBufferConsoleDebug, BUFFER_CONSOLE_LEN_MAX, format, args);
    xQueueSend(xQueueConsoleDebug, pBufferConsoleDebug, 100);
    va_end(args);
    va_end(args);
}

void vTaskConsoleDebug(void *pvParameters)
{
    (void) pvParameters;

    while(1)
    {
        xQueueReceive(xQueueConsoleDebug, pBufferUartTx, portMAX_DELAY);
        uint32_t buffer_size = 0;
        buffer_size = (uint16_t)strnlen(pBufferUartTx, (size_t)BUFFER_CONSOLE_LEN_MAX);
        for(uint32_t i = 0; i < buffer_size; i++)
        {
            usart_send_blocking(USART2, (uint16_t)pBufferUartTx[i]);
        }
    }
}

void usart3_isr(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    static volatile uint16_t c = 0;
    static uint16_t i = 0;
    static uint8_t cnt_delim_status = 0;

    c = usart_recv(USART3);
    pBufferBleRx[i++] = (char)c;
    if(c == RN4871_DELIMITER_STATUS)
    {
        ++cnt_delim_status;
    }

    if((c == '>') || (cnt_delim_status == 2))
    {
        pBufferBleRx[i] = 0;
        i = 0;
        cnt_delim_status = 0;
        xQueueSendFromISR(xQueueBleUartRx, pBufferBleRx, &xHigherPriorityTaskWoken);
        xEventGroupSetBitsFromISR(xEventsBleUart, FLAG_RN4871_RX, &xHigherPriorityTaskWoken);
    }
    else if(i >= (BUFFER_UART_LEN_MAX - 1))
    {
        i = 0;
    }
}

static int8_t rn4871_uart_tx(uint8_t *pBuffer, uint16_t buffer_size)
{
    if(NULL != pBuffer)
    {
        for(uint16_t i = 0; i < buffer_size; i++)
        {
            usart_send_blocking(USART3, (uint16_t)pBuffer[i]);
        }
        return 0;
    }
    return -1;
}

static void rn4871_send_cmd(enum rn4871_cmd_e cmd)
{
    switch(cmd)
    {
        case CMD_MODE_QUIT:
        case CMD_REBOOT:
        case CMD_DUMP_INFOS:
        case CMD_GET_DEVICE_NAME:
        case CMD_GET_VERSION:
        case CMD_CLEAR_ALL_SERVICES:
        {
            snprintf(pBufferBleTx, BUFFER_UART_LEN_MAX, "%s\r\n", TABLE_COMMAND[cmd]);
            xQueueSend(xQueueBleUartTx, pBufferBleTx, 100);
            xEventGroupSetBits(xEventsBleUart, FLAG_RN4871_TX);
            break;
        }
        case CMD_SET_SERVICES:
        {
            /* Enable Transparent UART */
            snprintf(pBufferBleTx, BUFFER_UART_LEN_MAX, "%s,C0\r\n", TABLE_COMMAND[cmd]);
            xQueueSend(xQueueBleUartTx, pBufferBleTx, 100);
            xEventGroupSetBits(xEventsBleUart, FLAG_RN4871_TX);
            break;
        }
        case CMD_CREATE_PRIVATE_SERVICE:
        {
            break;
        }
        case CMD_CREATE_PRIVATE_CHARACTERISTIC:
        {
            break;
        }
        case CMD_SERVER_WRITE_CHARACTERISTIC:
        {
            break;
        }
        case CMD_SERVER_READ_CHARACTERISTIC:
        {
            break;
        }
        default:
        break;
    }
    _current_cmd = cmd;
}

static void rn4871_process_resp(const char *buffer)
{
    if(NULL != buffer)
    {
        enum rn4871_cmd_e cmd = _current_cmd;
        if((strstr(buffer, "CMD>") != NULL) || (strstr(buffer, "REBOOT") != NULL))
        {
            switch(cmd)
            {
                case CMD_MODE_ENTER:
                    rn4871_send_cmd(CMD_DUMP_INFOS);
                    break;
                case CMD_DUMP_INFOS:
                    rn4871_send_cmd(CMD_GET_DEVICE_NAME);
                    break;
                case CMD_GET_DEVICE_NAME:
                    rn4871_send_cmd(CMD_GET_VERSION);
                    break;
                case CMD_GET_VERSION:
                    if(TRANSPARENT_UART)
                        rn4871_send_cmd(CMD_SET_SERVICES);
                    else
                        rn4871_send_cmd(CMD_CLEAR_ALL_SERVICES);
                    break;
                case CMD_SET_SERVICES:
                    rn4871_send_cmd(CMD_REBOOT);
                    break;
                case CMD_REBOOT:
                    _current_cmd = CMD_NONE;
                    break;
                default:
                    _current_cmd = CMD_NONE;
                    break;
            }
        }
        else if(strstr(buffer, "STREAM_OPEN") != NULL)
        {
            _stream_open = true;
        }
        else if(strstr(buffer, "DISCONNECT") != NULL)
        {
            _stream_open = false;
        }
    }
}

void vTaskBluetooth(void *pvParameters)
{
    (void)pvParameters;

    static char pBufferRx[BUFFER_UART_LEN_MAX+1] = "";
    static char pBufferTx[BUFFER_UART_LEN_MAX+1] = "";

    /* Send fake data for testing if command mode is available */
    rn4871_uart_tx((uint8_t*)"\r\n", 2);
    if(pdPASS != xQueueReceive(xQueueBleUartRx, pBufferRx, pdMS_TO_TICKS(100)))
    {
        console_debug("Tx <- $$$\r\n");
        rn4871_uart_tx((uint8_t*)"$", 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        rn4871_uart_tx((uint8_t*)"$$", 2);
        _current_cmd = CMD_MODE_ENTER;
    }
    else
    {
        rn4871_send_cmd(CMD_DUMP_INFOS);
    }
    xEventGroupClearBits(xEventsBleUart, FLAG_RN4871_RX);

    while(1)
    {
        EventBits_t uxBits = xEventGroupWaitBits(xEventsBleUart, FLAG_RN4871_RX | FLAG_RN4871_TX, pdFALSE, pdFALSE, portMAX_DELAY);
        if(0 != (uxBits & FLAG_RN4871_RX))
        {
            xEventGroupClearBits(xEventsBleUart, FLAG_RN4871_RX);
            if(pdPASS == xQueueReceive(xQueueBleUartRx, pBufferRx, 100))
            {
                uint16_t buffer_size = (uint16_t)strnlen(pBufferRx, (size_t)BUFFER_UART_LEN_MAX);
                console_debug("[BLE] Rx [%d] %s\r\n", buffer_size, pBufferRx);
                rn4871_process_resp(pBufferRx);
            }
        }
        else if(0 != (uxBits & FLAG_RN4871_TX))
        {
            xEventGroupClearBits(xEventsBleUart, FLAG_RN4871_TX);
            if(pdPASS == xQueueReceive(xQueueBleUartTx, pBufferTx, 100))
            {
                uint16_t buffer_size = (uint16_t)strnlen(pBufferTx, (size_t)BUFFER_UART_LEN_MAX);
                console_debug("[BLE] Tx [%d] %s\r\n", buffer_size, pBufferTx);
                rn4871_uart_tx((uint8_t*)pBufferTx, buffer_size);
            }
        }
    }
}

void vTaskSensor(void *pvParameters)
{
    (void)pvParameters;

    static char pData[255] = "";
    static int cnt = 0;

    while(1)
    {
        if(_stream_open)
        {
            snprintf(pData, 255, "Client %d", cnt++);
            console_debug("[SENSOR] BLE Send %s\r\n", pData);
            xQueueSend(xQueueBleUartTx, pData, 100);
            xEventGroupSetBits(xEventsBleUart, FLAG_RN4871_TX);
        }
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}