#include "buzzer_controller.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/dac.h"
#include "driver/timer.h"
#include "esp_log.h"

/*  The timer ISR has an execution time of 5.5 micro-seconds(us).
    Therefore, a timer period less than 5.5 us will cause trigger the interrupt watchdog.
    7 us is a safe interval that will not trigger the watchdog. No need to customize it.
*/

#define WITH_RELOAD 1
#define TIMER_INTR_US 7 // Execution time of each ISR interval in micro-seconds
#define TIMER_DIVIDER 16
#define POINT_ARR_LEN 200 // Length of points array
#define AMP_DAC 255       // Amplitude of DAC voltage. If it's more than 256 will causes dac_output_voltage() output 0.
#define VDD 3300          // VDD is 3.3V, 3300mV
#define CONST_PERIOD_2_PI 6.2832    // Two pi is needed to construct a sine wave
#define SEC_TO_MICRO_SEC(x) ((x) / 1000 / 1000)                   // Convert second to micro-second
//#define UNUSED_PARAM __attribute__((unused))                      // A const period parameter which equals 2 * pai, used to calculate raw dac output value.
#define TIMER_TICKS (TIMER_BASE_CLK / TIMER_DIVIDER)              // TIMER_BASE_CLK = APB_CLK = 80MHz
#define ALARM_VAL_US SEC_TO_MICRO_SEC(TIMER_INTR_US *TIMER_TICKS) // Alarm value in micro-seconds

#define DAC_CHAN CONFIG_DAC_CHANNEL // DAC_CHANNEL_1 (GPIO25) by default

static int raw_val[POINT_ARR_LEN];  // Used to store raw values
static int volt_val[POINT_ARR_LEN]; // Used to store voltage values(in mV)

// Tag used for logging
static const char *TAG = "Passive buzzer controller";

// Global frequency variable
static volatile int frequency = 0;

// Variables used to keep track of the wave selected
static volatile int OUTPUT_POINT_NUM = 0; // The number of output wave points.
static volatile int g_index = 0;


static void calculate_output_point_num()
{
    OUTPUT_POINT_NUM = (int)(1000000 / (TIMER_INTR_US * frequency) + 0.5);
}

/* Timer interrupt service routine */
static void IRAM_ATTR timer0_ISR(void *ptr)
{
    timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);
    timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_0);

    int *head = (int *)ptr;

    /* DAC output ISR has an execution time of 4.4 us*/
    if (g_index >= OUTPUT_POINT_NUM)
        g_index = 0;
    dac_output_voltage(DAC_CHAN, *(head + g_index));
    g_index++;
}

/* Timer group0 TIMER_0 initialization */
static void buzzer_timer_init(int timer_idx, bool auto_reload)
{
    esp_err_t ret;
    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .intr_type = TIMER_INTR_LEVEL,
        .auto_reload = auto_reload,
    };

    ret = timer_init(TIMER_GROUP_0, timer_idx, &config);
    ESP_ERROR_CHECK(ret);
    ret = timer_set_counter_value(TIMER_GROUP_0, timer_idx, 0x00000000ULL);
    ESP_ERROR_CHECK(ret);
    ret = timer_set_alarm_value(TIMER_GROUP_0, timer_idx, ALARM_VAL_US);
    ESP_ERROR_CHECK(ret);
    ret = timer_enable_intr(TIMER_GROUP_0, TIMER_0);
    ESP_ERROR_CHECK(ret);
    /* Register an ISR handler */
    timer_isr_register(TIMER_GROUP_0, timer_idx, timer0_ISR, (void *)raw_val, ESP_INTR_FLAG_IRAM, NULL);
}

// Calculate the raw values of the wave form and then convert them to the corresponding voltage
static void prepare_data(int pnt_num)
{
    // Pause the timer to prevent any errors thrown because we are edditing this part
    timer_pause(TIMER_GROUP_0, TIMER_0);
    for (int i = 0; i < pnt_num; i++)
    {
#ifdef CONFIG_WAVEFORM_SINE
        raw_val[i] = (int)((sin(i * CONST_PERIOD_2_PI / pnt_num) + 1) * (double)(AMP_DAC) / 2 + 0.5);
#elif CONFIG_WAVEFORM_TRIANGLE
        raw_val[i] = (i > (pnt_num / 2)) ? (2 * AMP_DAC * (pnt_num - i) / pnt_num) : (2 * AMP_DAC * i / pnt_num);
#elif CONFIG_WAVEFORM_SAWTOOTH
        raw_val[i] = (i == pnt_num) ? 0 : (i * AMP_DAC / pnt_num);
#elif CONFIG_WAVEFORM_SQUARE
        raw_val[i] = (i < (pnt_num / 2)) ? AMP_DAC : 0;
#endif
        volt_val[i] = (int)(VDD * raw_val[i] / (float)AMP_DAC);
    }
    // Start the timer again
    timer_start(TIMER_GROUP_0, TIMER_0);
}

// Method to log everyting about the new frequency calculations
static void log_info(void)
{
    ESP_LOGI(TAG, "DAC output channel: %d", DAC_CHAN);
    if (DAC_CHAN == DAC_CHANNEL_1)
    {
        ESP_LOGI(TAG, "GPIO:%d", GPIO_NUM_25);
    }
    else
    {
        ESP_LOGI(TAG, "GPIO:%d", GPIO_NUM_26);
    }
#ifdef CONFIG_WAVEFORM_SINE
    ESP_LOGI(TAG, "Waveform: SINE");
#elif CONFIG_WAVEFORM_TRIANGLE
    ESP_LOGI(TAG, "Waveform: TRIANGLE");
#elif CONFIG_WAVEFORM_SAWTOOTH
    ESP_LOGI(TAG, "Waveform: SAWTOOTH");
#elif CONFIG_WAVEFORM_SQUARE
    ESP_LOGI(TAG, "Waveform: SQUARE");
#endif

    ESP_LOGI(TAG, "Frequency(Hz): %d", frequency);
    ESP_LOGI(TAG, "Output points num: %d\n", OUTPUT_POINT_NUM);
}

// Method used to set the frequency
void set_frequency(int _frequency)
{
    frequency = _frequency;
    // Recalculate the output numbers
    calculate_output_point_num();

    // Assert if the sine wave can be generated
    assert(OUTPUT_POINT_NUM <= POINT_ARR_LEN);

    // Log the details of the outcome
    log_info();

    // Start the sine wave at index 0
    g_index = 0;
    // Calculate the real output voltages
    prepare_data(OUTPUT_POINT_NUM);
}

// Method to initialize the buzzer
void buzzer_init()
{
    // Initialize the buzzer timer
    buzzer_timer_init(TIMER_0, WITH_RELOAD);

    // Check if the dac output is enabled
    ESP_ERROR_CHECK(dac_output_enable(DAC_CHAN));
}