#include "bananakit.h"
#ifdef ENABLE_WEATHER_MODULE

#include <string.h>
#include <stdlib.h>
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SparkFunCCS811.h>
#include "callstack.h"
#include "menu.h"
#include "bananakit_io.h"
#include "banana_string.h"
#include "bananakit_misc.h"
#include "weather_module.h"
#define SEALEVELPRESSURE_HPA (1013.25)
#define BME280_ADDR 0x76
#define CCS811_ADDR 0x5A

static void read_bme280_data(void);
static void read_ccs811_data(void);
static void lcd_refresh(void);

static Adafruit_BME280 BME280_Sensor;
static CCS811 CCS811_Sensor(CCS811_ADDR);

typedef struct {
    struct {
        uint8_t bme280_ready: 1;
        uint8_t ccs811_ready: 1;
    } status;

    float bme280_temprature;
    float bme280_pressure;
    float bme280_humidity;
    float bme280_altitude;

    uint16_t eco2_val;
    uint16_t tvoc_val;
} weather_sensor_reading_t;

weather_sensor_reading_t Weather; 

extern callstack_t Callstack;
extern bananakit_io_t IO;

void weather_module_init(void) {
    if(BME280_Sensor.begin(BME280_ADDR)) {
        Weather.status.bme280_ready = 1;
    } else {
        Weather.status.bme280_ready = 0;
    }

    if(CCS811_Sensor.begin()) {
        Weather.status.ccs811_ready = 1;
    } else {
        Weather.status.ccs811_ready = 0;
    }
}

node_status_t weather_module_update(void) {
    node_status_t next_status = NODE_RUNNING;

    read_bme280_data();
    read_ccs811_data();
    lcd_refresh();

    switch(IO.keypress) {
        case PIC_4:
            next_status = NODE_EXITING;
            break;

        default:
            break;
    }

    delay(1000);
    return next_status;
}

void weather_module_resume(void) {
}

void weather_module_exit(void) {
}

static void read_bme280_data(void) {
    if(Weather.status.bme280_ready) {
        Weather.bme280_temperature = BME280_Sensor.readTemperature();
        Weather.bme280_pressure = BME280_Sensor.readPressure() / 100.0F;
        Weather.bme280_altitude = BME280_Sensor.readAltitude(SEALEVELPRESSURE_HPA);
        Weather.bme280_humidity = BME280_Sensor.readHumidity();
    }
}

static void read_ccs811_data(void) {
    if(Weather.status.ccs811_ready && CCS811_Sensor.dataAvailable()) {
        CCS811_Sensor.readAlgorithmResults();
        Weather.eco2_val = CCS811_Sensor.getCO2();
        Weather.tvoc_val = CCS811_Sensor.getTVOC();
    }
}

#define ALIGN_BUF_SIZE 21
static void lcd_refresh(void) {
    char floatbuf[LCD_BUF_SIZE];
    char align_buf[ALIGN_BUF_SIZE];
    IO.lcd_clear_callback();

    // Line 0:
    float2str(Weather.bme280_temperature, floatbuf, LCD_BUF_SIZE, 1);
    snprintf(
        IO.lcd_buf,
        LCD_BUF_SIZE,
        "T:%s",
        floatbuf
    );
    float2str(Weather.bme280_humidity, floatbuf, LCD_BUF_SIZE, 2);
    snprintf(
        align_buf,
        ALIGN_BUF_SIZE,
        "H:%s",
        floatbuf
    );
    right_align_overlay(align_buf, IO.lcd_buf, LCD_BUF_SIZE);
    IO.flags = LCD_REFRESH_LINE0;
    IO.lcd_refresh_callback();

    // Line 1:
    float2str(Weather.bme280_pressure, floatbuf, LCD_BUF_SIZE, 2);
    snprintf(
        IO.lcd_buf,
        LCD_BUF_SIZE,
        "P:%shPa",
        floatbuf
    );
    float2str(Weather.bme280_altitude, floatbuf, LCD_BUF_SIZE, 2);
    snprintf(
        align_buf,
        ALIGN_BUF_SIZE,
        "A:%sm",
        floatbuf
    );
    right_align_overlay(align_buf, IO.lcd_buf, LCD_BUF_SIZE);
    IO.flags = LCD_REFRESH_LINE1;
    IO.lcd_refresh_callback();

    // Line 2:
    snprintf(
        IO.lcd_buf,
        LCD_BUF_SIZE,
        "eCO2:%dPPM",
        Weather.eco2_val
    );
    float2str(Weather.bme280_altitude, floatbuf, LCD_BUF_SIZE, 2);
    snprintf(
        align_buf,
        ALIGN_BUF_SIZE,
        "tVOC:%dPPB",
        Weather.tvoc_val
    );
    right_align_overlay(align_buf, IO.lcd_buf, LCD_BUF_SIZE);
    IO.flags = LCD_REFRESH_LINE2;
    IO.lcd_refresh_callback();
}

#endif