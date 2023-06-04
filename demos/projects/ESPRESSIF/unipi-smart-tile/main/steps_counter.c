/* Copyright 2023 Luca Ceragioli l.ceragioli@sssup.it */
#include "steps_counter.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_err.h"

#include "driver/gptimer.h"
#include "driver/i2c.h"

#define ABS(x)  ( ((x) > 0) ? (x) : -(x) )

/* Algorithm constants: */
#define ALPHA                          0.90f
#define CALIBRATION_TICKS           1000
#define PEAK_DETECT_THSLD           1000.0f
#define PEAKS_MAX                      2
#define PEAK_DETECT_DISABLE_MS       500

/* I2C master conf */
#define I2C_MASTER_SCL_IO           26
#define I2C_MASTER_SDA_IO           25
#define I2C_MASTER_NUM              0
#define I2C_MASTER_FREQ_HZ          400000
#define I2C_MASTER_TIMEOUT_MS       1000

/* MPU6050 constants */
#define MPU6050_SENSOR_ADDR                 0x68
#define MPU6050_ACCEL_XOUT                  0x3B
#define MPU6050_ACCEL_YOUT                  0x3D
#define MPU6050_ACCEL_ZOUT                  0x3F
#define MPU6050_TEMP_OUT                    0x41

#define MPU6050_PWR_MGMT_1_REG_ADDR         0x6B
#define MPU6050_PWR_MGMT_1_VALUE            0x00

#define MPU6050_ACCEL_CONFIG_REG_ADDR       0x1C
#define MPU6050_ACCEL_CONFIG_2G_VALUE       (0 << 3)
#define MPU6050_ACCEL_CONFIG_4G_VALUE       (1 << 3)
#define MPU6050_ACCEL_CONFIG_8G_VALUE       (2 << 3)
#define MPU6050_ACCEL_CONFIG_16G_VALUE      (3 << 3)

typedef uint32_t time_ms_t;

void steps_counter_task(void * unused_ptr);
void accel_init();
int16_t accel_read_z();
bool steps_counter_ISR_task(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx);
void steps_counter_algorithm_run();

static SemaphoreHandle_t semaphore;
static TaskHandle_t steps_counter_task_handle;
static gptimer_handle_t gptimer = NULL;
static volatile int32_t steps_counter;
static int32_t steps_counter_tmp;
static time_ms_t algo_time_ms;
static time_ms_t counter;
static float average;
static int32_t calibration_ticks;
static uint32_t peaks_cnt = 0;

static esp_err_t mpu6050_register_read(uint8_t reg_addr, uint8_t *data, size_t len){
    return i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_SENSOR_ADDR, &reg_addr, 1, data, len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

static esp_err_t mpu6050_register_write_byte(uint8_t reg_addr, uint8_t data){
    int ret;
    uint8_t write_buf[2] = {reg_addr, data};
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_SENSOR_ADDR, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    return ret;
}

static esp_err_t i2c_master_init(void){
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
}

/* Returns time in millis for algorithm evaluation */
static time_ms_t get_time_ms(){
    return algo_time_ms;
}

esp_err_t steps_counter_init(){
    /* Resets algorithm's internal variables */
    steps_counter = 0;
    steps_counter_tmp = 0;
    peaks_cnt = 0;
    algo_time_ms = 0;
    counter = 0;
    average = 0;
    calibration_ticks = CALIBRATION_TICKS;

    accel_init();

    /* Create gptimer with resolution of 1us */
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1 * 1000 * 1000,
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    /* Create a periodic alarm every 1000us */
    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .alarm_count = 1000,
        .flags.auto_reload_on_alarm = true,
    };
    gptimer_event_callbacks_t cb = {
        .on_alarm = steps_counter_ISR_task,
    };

    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cb, NULL));
    ESP_ERROR_CHECK(gptimer_enable(gptimer));

    /* Create binary semaphore to synch between producer and consumer tasks */
    semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(semaphore);

    /* Create producer task */
    BaseType_t retval = xTaskCreate(
                 steps_counter_task,            /* Function that implements the task. */
                 "StepsCounterTask",            /* Text name for the task - only used for debugging. */
                 5000,                           /* Size of stack (in words, not bytes) to allocate for the task. */
                 NULL,                          /* Task parameter - not used in this case. */
                 tskIDLE_PRIORITY + 1,          /* Task priority, must be between 0 and configMAX_PRIORITIES - 1. */
                 &steps_counter_task_handle );  /* Used to pass out a handle to the created task */

   return (retval == pdPASS) ? ESP_OK : ESP_FAIL;
}

void steps_counter_start_ISR_task(){
    ESP_ERROR_CHECK(gptimer_start(gptimer));
}

int32_t steps_counter_get_steps(){
    int32_t val = -1;
    if (xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE){
        val = steps_counter;
        xSemaphoreGive(semaphore);
    }
    return val;
}

int steps_counter_get_data(int32_t *steps, float *accel_peak, int32_t *step_duration_ms, float *step_energy){
    if (xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE){
        *steps = steps_counter;
        *accel_peak = 1.0f;
        *step_duration_ms = 500;
        *step_energy = 0.5f;
        xSemaphoreGive(semaphore);
        return 1;
    }
    return 0;
}

void steps_counter_reset_steps(){
    if (xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE){
        steps_counter = 0;
        xSemaphoreGive(semaphore);
    }
}

void accel_init(){
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_ERROR_CHECK(mpu6050_register_write_byte(MPU6050_PWR_MGMT_1_REG_ADDR, MPU6050_PWR_MGMT_1_VALUE));
    ESP_ERROR_CHECK(mpu6050_register_write_byte(MPU6050_ACCEL_CONFIG_REG_ADDR, MPU6050_ACCEL_CONFIG_4G_VALUE));
    accel_read_z();
}

int16_t accel_read_z(){
    int16_t accelz_aux;
    uint8_t data[2];
    ESP_ERROR_CHECK(mpu6050_register_read(MPU6050_ACCEL_ZOUT, data, 2));
    accelz_aux = (((data[0] <<8)) | data[1]);
    return accelz_aux;
}

/* Action to be run on gptimer alarm. Runs in ISR context! */
bool steps_counter_ISR_task(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx){
    algo_time_ms++;

    BaseType_t pxHigherPriorityTaskWoken;
    xTaskNotifyFromISR(steps_counter_task_handle,   /* Task to notify */
                       0,                           /* Unused value */
                       eNoAction,                   /* No action performed on notification */
                       &pxHigherPriorityTaskWoken); /* Boolean value indicating whether an higher priority task has been woken */

    return pxHigherPriorityTaskWoken == pdTRUE;
}

void steps_counter_algorithm_run(){
    time_ms_t current_time = get_time_ms();

    float data = accel_read_z();
    average = ALPHA * average + data * (1-ALPHA);

    /* self-calibration */
    if( calibration_ticks ){
        calibration_ticks--;
        return;
    }

    if( ABS(data-average) > PEAK_DETECT_THSLD ){
        if( current_time > counter ){
            counter = current_time + PEAK_DETECT_DISABLE_MS;
            if( ++peaks_cnt >= PEAKS_MAX ) {
                peaks_cnt = 0;
                steps_counter_tmp++;
            }
        }
    }
}

void steps_counter_task(void * unused_ptr){
    for(;;){
        xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);

        steps_counter_algorithm_run();

        /* Increment shared variable `steps_counter` if shadow variable `steps_counter_tmp`
         * is not zero. A shadow variable is used because if the semaphore is unavailable
         * we cannot wait inside the interrupt to update the shared variable. */
        if( steps_counter_tmp > 0 ) {
            if( xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE ) {
                steps_counter += steps_counter_tmp;
                steps_counter_tmp = 0;
                xSemaphoreGive(semaphore);
            }
        }
    }
}

