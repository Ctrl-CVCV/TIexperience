/*
 * MSPM0G3519 and TJC screen number synchronization over UART4.
 *
 * MCU -> screen: n0.val=<value> FF FF FF
 * Screen -> MCU: V=<value>\n
 * USER short press increments; long press decrements and repeats.
 */

#include <string.h>

#include "ti_msp_dl_config.h"
#include "BSP/UART_DMA/uart_dma.h"
#include "BSP/SPI0_OLED/spi0_oled.h"

#define VALUE_MIN       0
#define VALUE_MAX       999
#define KEY_SCAN_MS     20U
#define KEY_LONG_MS     1000U
#define KEY_REPEAT_MS   200U
#define SCREEN_SYNC_MS  1000U

volatile int16_t g_value = 0;
volatile bool g_value_changed = true;
volatile uint32_t g_sys_ms = 0U;
volatile uint32_t nowtime = 0U;
volatile bool g_ctrl_tick = false;

static bool key_is_pressed(void)
{
    return DL_GPIO_readPins(Key_PORT, Key_User_PIN) == 0U;
}

static char *append_decimal(char *dst, int16_t value)
{
    char reverse[8];
    uint16_t number;
    uint8_t count = 0U;

    if (value < 0) {
        *dst++ = '-';
        number = (uint16_t)(-value);
    } else {
        number = (uint16_t)value;
    }

    if (number == 0U) {
        reverse[count++] = '0';
    }
    while (number > 0U) {
        reverse[count++] = (char)('0' + number % 10U);
        number /= 10U;
    }
    while (count > 0U) {
        *dst++ = reverse[--count];
    }
    return dst;
}

static void format_oled_value(char *dst, int16_t value)
{
    char *end;

    if (value < VALUE_MIN) {
        value = VALUE_MIN;
    } else if (value > VALUE_MAX) {
        value = VALUE_MAX;
    }

    end = append_decimal(dst, value);
    while (end < dst + 3) {
        *end++ = ' ';
    }
    *end = '\0';
}

static bool parse_screen_value(const char *line, int16_t *value)
{
    uint16_t parsed = 0U;
    const char *cursor;

    if (line == NULL || value == NULL || line[0] != 'V' || line[1] != '=') {
        return false;
    }

    cursor = line + 2;
    if (*cursor < '0' || *cursor > '9') {
        return false;
    }

    while (*cursor >= '0' && *cursor <= '9') {
        if (parsed <= VALUE_MAX) {
            parsed = (uint16_t)(parsed * 10U + (uint16_t)(*cursor - '0'));
        }
        cursor++;
    }

    if (*cursor == '\r') {
        cursor++;
    }
    if (*cursor != '\n' && *cursor != '\0') {
        return false;
    }

    *value = (parsed > VALUE_MAX) ? VALUE_MAX : (int16_t)parsed;
    return true;
}

static void handle_screen_line(const char *line)
{
    int16_t received;

    if (parse_screen_value(line, &received)) {
        g_value = received;
        g_value_changed = true;
    }
}

static void key_scan(uint32_t now_ms)
{
    static uint32_t last_scan_ms;
    static uint32_t pressed_at_ms;
    static uint32_t last_repeat_ms;
    static bool was_pressed;
    bool pressed;

    if (now_ms - last_scan_ms < KEY_SCAN_MS) {
        return;
    }
    last_scan_ms = now_ms;
    pressed = key_is_pressed();

    if (pressed && !was_pressed) {
        pressed_at_ms = now_ms;
        last_repeat_ms = 0U;
    } else if (pressed && was_pressed) {
        if (now_ms - pressed_at_ms >= KEY_LONG_MS) {
            if (last_repeat_ms == 0U ||
                now_ms - last_repeat_ms >= KEY_REPEAT_MS) {
                if (g_value > VALUE_MIN) {
                    g_value--;
                    g_value_changed = true;
                }
                last_repeat_ms = now_ms;
            }
        }
    } else if (!pressed && was_pressed) {
        if (now_ms - pressed_at_ms < KEY_LONG_MS && g_value < VALUE_MAX) {
            g_value++;
            g_value_changed = true;
        }
    }

    was_pressed = pressed;
}

static void send_value_to_screen(void)
{
    static char tx_buffer[20];
    char *cursor = tx_buffer;

    memcpy(cursor, "n0.val=", 7U);
    cursor += 7;
    cursor = append_decimal(cursor, g_value);
    *cursor++ = (char)0xFF;
    *cursor++ = (char)0xFF;
    *cursor++ = (char)0xFF;

    if (uart4_dma_send(
            (const uint8_t *)tx_buffer, (uint16_t)(cursor - tx_buffer))) {
        g_value_changed = false;
    }
}

void TIMER_7_INST_IRQHandler(void)
{
    switch (DL_TimerG_getPendingInterrupt(TIMER_7_INST)) {
    case DL_TIMER_IIDX_ZERO:
        DL_TimerG_clearInterruptStatus(
            TIMER_7_INST, DL_TIMERG_INTERRUPT_ZERO_EVENT);
        g_sys_ms += 5U;
        nowtime += 5U;
        g_ctrl_tick = true;
        break;
    default:
        break;
    }
}

int main(void)
{
    uint32_t oled_last_ms = 0U;
    uint32_t sync_last_ms = 0U;

    SYSCFG_DL_init();

    DL_SPI_disable(SPI_0_INST);
    DL_SPI_setBitRateSerialClockDivider(SPI_0_INST, 39U);
    DL_SPI_enable(SPI_0_INST);

    OLED_Init();
    OLED_Clear();
    uart4_dma_init();

    {
        DL_TimerG_TimerConfig timer_config = {
            .period = 49999U,
            .timerMode = DL_TIMER_TIMER_MODE_PERIODIC,
            .startTimer = DL_TIMER_START,
        };
        DL_TimerG_initTimerMode(TIMER_7_INST, &timer_config);
        DL_TimerG_enableInterrupt(
            TIMER_7_INST, DL_TIMERG_INTERRUPT_ZERO_EVENT);
        NVIC_SetPriority(TIMER_7_INST_INT_IRQN, 2U);
        NVIC_EnableIRQ(TIMER_7_INST_INT_IRQN);
    }

    while (1) {
        uint32_t now;

        while (!g_ctrl_tick) {}
        g_ctrl_tick = false;
        now = g_sys_ms;

        key_scan(now);

        while (uart4_rx_line_available()) {
            char line[32];
            uart4_rx_read_line(line, sizeof(line));
            handle_screen_line(line);
        }

        if (now - oled_last_ms >= 100U) {
            char oled_value[4];
            oled_last_ms = now;
            format_oled_value(oled_value, g_value);
            OLED_ShowString(50U, 3U, (u8 *)oled_value);
        }

        if (now - sync_last_ms >= SCREEN_SYNC_MS) {
            sync_last_ms = now;
            g_value_changed = true;
        }

        if (g_value_changed && !uart4_dma_is_busy()) {
            send_value_to_screen();
        }
    }
}
