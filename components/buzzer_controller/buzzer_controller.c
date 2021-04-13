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
#define POINT_ARR_LEN 200                       // Length of points array
#define AMP_DAC 255                             // Amplitude of DAC voltage. If it's more than 256 will causes dac_output_voltage() output 0.
#define VDD 3300                                // VDD is 3.3V, 3300mV
#define CONST_PERIOD_2_PI 6.2832                // Two pi is needed to construct a sine wave
#define SEC_TO_MICRO_SEC(x) ((x) / 1000 / 1000) // Convert second to micro-second
//#define UNUSED_PARAM __attribute__((unused))                      // A const period parameter which equals 2 * pai, used to calculate raw dac output value.
#define TIMER_TICKS (TIMER_BASE_CLK / TIMER_DIVIDER)              // TIMER_BASE_CLK = APB_CLK = 80MHz
#define ALARM_VAL_US SEC_TO_MICRO_SEC(TIMER_INTR_US *TIMER_TICKS) // Alarm value in micro-seconds

#define DAC_CHAN CONFIG_DAC_CHANNEL // DAC_CHANNEL_1 (GPIO25) by default

static int raw_val[POINT_ARR_LEN];  // Used to store raw values
static int volt_val[POINT_ARR_LEN]; // Used to store voltage values(in mV)

// Tag used for logging
static const char *TAG = "Passive buzzer controller";

// Global frequency variable, frequency is how many times the signal goes up and down in 1 second
static volatile int frequency = 0;

// Variables used to keep track of the wave selected
static volatile int output_point_number = 0; // The number of output wave points used to construct the wave
static volatile int g_index = 0;

// Method to calculate how many points we use to generate the wave
static void calculate_output_point_num()
{
    // 1000000 is 1 second in us
    // We multiply the TIMER_INTR_US by frequency to get the amount of points we can use
    // TIMER_INTR_US is the amount of time used by 1 interupt
    // Add 0.5 to round it up
    output_point_number = (int)(1000000 / (TIMER_INTR_US * frequency) + 0.5);
}

/// This method is from the example project but i will eleborate what it does
/* Timer interrupt service routine */
static void IRAM_ATTR timer0_ISR(void *ptr)
{
    // Clear the status of the interupt
    timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);
    // Enable the interupt alarm
    timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_0);

    // Cast the ptr as head
    int *head = (int *)ptr;

    /* DAC output ISR has an execution time of 4.4 us*/
    // If the index is as large as the array size we reset the index to 0
    if (g_index >= output_point_number)
        g_index = 0;

    // Set the voltage to the selected channel, 
    // here the head is the start of the array and we add the index to get the corresponding integer
    dac_output_voltage(DAC_CHAN, *(head + g_index));

    // Go to the next index
    g_index++;
}

/// This method is from the example project but i will eleborate what it does
/* Timer group0 TIMER_0 initialization */
static void buzzer_timer_init(bool auto_reload)
{
    // Construct the configuration of the timer
    timer_config_t config = {
        .divider = TIMER_DIVIDER, // Set the devider to preset value (default 16)
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .intr_type = TIMER_INTR_LEVEL,
        .auto_reload = auto_reload,
    };

    // Initialise timer with timer index of timer_idx on group 0
    ESP_ERROR_CHECK(timer_init(TIMER_GROUP_0, TIMER_0, &config));
    // Set the counter value to 0
    ESP_ERROR_CHECK(timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0x00000000ULL));
    // Set the alarm value via the macro
    ESP_ERROR_CHECK(timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, ALARM_VAL_US));
    // Enable the intr on the tim
    ESP_ERROR_CHECK(timer_enable_intr(TIMER_GROUP_0, TIMER_0));
    /* Register an ISR handler */
    // Register the function timer0_ISR as interupt handler
    timer_isr_register(TIMER_GROUP_0, TIMER_0, timer0_ISR, (void *)raw_val, ESP_INTR_FLAG_IRAM, NULL);
}

// Calculate the raw values of the wave form and then convert them to the corresponding voltage
static void prepare_data(int pnt_num)
{
    // Pause the timer to prevent any errors thrown because we are edditing this part
    timer_pause(TIMER_GROUP_0, TIMER_0);
    for (int i = 0; i < pnt_num; i++)
    {
        // Functions to make the selected wave sort
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
    ESP_LOGI(TAG, "Output points num: %d\n", output_point_number);
}

// Method used to set the frequency
void set_frequency(int _frequency)
{
    frequency = _frequency;
    // Recalculate the output numbers
    calculate_output_point_num();

    // Assert if the wave can be generated
    assert(output_point_number <= POINT_ARR_LEN);

    // Log the details of the outcome
    log_info();

    // Start the wave at index 0
    g_index = 0;
    // Calculate the real output voltages
    prepare_data(output_point_number);
}

// Method to initialize the buzzer
void buzzer_init()
{
    // Initialize the buzzer timer
    buzzer_timer_init(WITH_RELOAD);

    // Check if the dac output is enabled
    ESP_ERROR_CHECK(dac_output_enable(DAC_CHAN));
}