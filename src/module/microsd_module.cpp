#include "bananakit.h"
#ifdef ENABLE_MICROSD_MODULE

#include <string.h>
#include <stdlib.h>
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include "callstack.h"
#include "menu.h"
#include "bananakit_io.h"
#include "banana_string.h"
#include "bananakit_misc.h"
#include "module/microsd_module.h"

static int sd_pre_check_info(void);
static void display_sd_info(void);

extern callstack_t Callstack;
extern bananakit_io_t IO;
Sd2Card SD_Card;
SdVolume Volume;
SdFile RootFile;
static int8_t SD_isinit;
static int8_t Vol_isinit;

void microsd_module_init(void) {
    SD_isinit = 0;
    Vol_isinit = 0;

    if(SD_Card.init(SPI_HALF_SPEED, MICROSD_CS_PIN)) {
        SD_isinit = 1;
        if(Volume.init(SD_Card)) {
            Vol_isinit = 1;
        }
    }

    display_sd_info();
}

node_status_t microsd_module_update(void) {
    node_status_t next_status = NODE_RUNNING;

    switch(IO.keypress) {
        case PIC_4:
            next_status = NODE_EXITING;
            break;

        default:
            break;
    }

    delay(100);
    return next_status;
}

void microsd_module_resume(void) {

}

void microsd_module_exit(void) {

}

static int sd_pre_check_info(void) {
    if(!SD_isinit) {
        snprintf(
            IO.lcd_buf,
            LCD_BUF_SIZE,
            "SD init fail"
        );
        IO.flags = LCD_REFRESH_LINE0;
        IO.lcd_refresh_callback();
        return 0;
    }
    if(!Vol_isinit) {
        snprintf(
            IO.lcd_buf,
            LCD_BUF_SIZE,
            "Vol init fail"
        );
        IO.flags = LCD_REFRESH_LINE0;
        IO.lcd_refresh_callback();
        return 0;
    }
    return 1;
}

static void display_sd_info(void) {
    int offset;
    uint32_t volume_size;

    if(!sd_pre_check_info()) {
        return;
    }

    // Line0: "Type:SD1 FS:FAT32"
    offset = 0;
    offset += snprintf(
        IO.lcd_buf + offset,
        LCD_BUF_SIZE - offset,
        "Type:"
    );
    switch(SD_Card.type()) {
        case SD_CARD_TYPE_SD1:
            offset += snprintf(
                IO.lcd_buf + offset,
                LCD_BUF_SIZE - offset,
                "SD1"
            );
            break;
        case SD_CARD_TYPE_SD2:
            offset += snprintf(
                IO.lcd_buf + offset,
                LCD_BUF_SIZE - offset,
                "SD2"
            );
            break;
        case SD_CARD_TYPE_SDHC:
            offset += snprintf(
                IO.lcd_buf + offset,
                LCD_BUF_SIZE - offset,
                "SDHC"
            );
            break;
        default:
            offset += snprintf(
                IO.lcd_buf + offset,
                LCD_BUF_SIZE - offset,
                "?"
            );
            break;
    }
    offset += snprintf(
        IO.lcd_buf+offset,
        LCD_BUF_SIZE-offset,
        " FS:FAT%d",
        Volume.fatType()
    );
    IO.flags = LCD_REFRESH_LINE0;
    IO.lcd_refresh_callback();

    // Line1: "Clusters:123"
    offset = 0;
    offset += snprintf(
        IO.lcd_buf,
        LCD_BUF_SIZE-offset,
        "Clusters:%u",
        Volume.clusterCount()
    );
    IO.flags = LCD_REFRESH_LINE1;
    IO.lcd_refresh_callback();

    // Line2: "Blocks:456"
    offset = 0;
    offset += snprintf(
        IO.lcd_buf,
        LCD_BUF_SIZE-offset,
        "Blocks:%u",
        Volume.blocksPerCluster()
    );
    IO.flags = LCD_REFRESH_LINE2;
    IO.lcd_refresh_callback();

    // Line3: "VolSZ(MB):4096"
    offset = 0;
    volume_size = Volume.blocksPerCluster() * Volume.clusterCount();
    volume_size /= 2; // to KB
    volume_size /= 1024; // to MB
    offset += snprintf(
        IO.lcd_buf,
        LCD_BUF_SIZE-offset,
        "VolSZ(MB):%u",
        volume_size
    );
    IO.flags = LCD_REFRESH_LINE3;
    IO.lcd_refresh_callback();
}

#endif
