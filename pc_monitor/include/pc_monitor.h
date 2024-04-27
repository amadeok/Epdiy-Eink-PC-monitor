#include "esp_attr.h"
#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "xtensa/core-macros.h"
#include "epdiy.h"
#include "epd_internals.h"
#include "render_context.h"
//#include "render_i2s.h"
#define USING_TEMP_FUNCTIONS 0

#define BOARD epd_board_v4
#define DISPLAY ED097TC2
//#define WAVEFORM EPD_BUILTIN_WAVEFORM we are not using waveforms

#define WIFI_SSID "TIM-22836756"
#define WIFI_PASS "yyHZdybbxsHRErFT69mP3dLK"
// #define WIFI_SSID "wifi_ssid"
// #define WIFI_PASS "wifi_password"


#ifndef min_
#define min_(a, b) ((a) < (b) ? (a) : (b))
#endif

// const EpdBoardDefinition *epd_board_;
// const EpdBoardDefinition *display_;

// const EpdDisplay_t *epd_get_display_();
// const EpdBoardDefinition *epd_current_board_();

// display = DISPLAY;

#define DEBUG_MSGs 4

RenderContext_t render_context_;

volatile unsigned long renderer_frame_counter, downloader_frame_counter;
volatile int stop, clearing;
volatile unsigned long tr0, tr1, td0, td1;

// volatile uint8_t mouse_moved;

int width_resolution, height_resolution;

int total_nb_pixels, eink_framebuffer_size;

int enable_skipping, epd_skip_threshold, epd_skip_mouse_only;
int esp32_multithread;
int selective_compression;
int extra_bytes;

int refresh_on_startup;
int draw_black_on_startup;
int frame_counter;

uint8_t *compressed_chunk;
uint8_t *array_with_zeros;
uint8_t *draw_black_bytes;
uint8_t *draw_white_bytes;
uint8_t **framebuffer_chunks;
uint8_t *compressed_chunk;
uint8_t *per_frame_wifi_settings_buffer;
uint8_t ready0[6];
volatile uint8_t clear[2];


SemaphoreHandle_t begin;

typedef struct {
    int signal;
    volatile int mouse_moved;
    int mode;
    int do_full_refresh;
    int16_t rmt_high_times[100];
    int16_t rmt_high_times_n;
    char * type;
    char * notes;
    int wifi_transfer_size;
    int framebuffer_data_pos;
    int framebuffer_data_size;
    int line_changed_pos;
    int draw_count;
    int nb_draws;
    int total_lines_changed;
    int need_to_extract;
    uint8_t *line_changed;
    uint8_t * frame_buffer;
} per_frame_settings;

#define QUEUE_LENGTH 1
#define ITEM_SIZE sizeof(int)
#define BUFFERED_FRAMES_N  1
#define  REQUIRED_BUFFERS_N BUFFERED_FRAMES_N*2
volatile QueueHandle_t buffer_queue[REQUIRED_BUFFERS_N];
QueueHandle_t queue;
volatile int switcher;

static IRAM_ATTR int get_prev_index(int index){
    int pi = index -1;
    if (pi < 0)
        return REQUIRED_BUFFERS_N -1;
    else return pi;
}
per_frame_settings per_frame_settings_arr [REQUIRED_BUFFERS_N];


void IRAM_ATTR pc_monitor_feed_display_with_skip(per_frame_settings* f_i, bool print_times);

void IRAM_ATTR pc_monitor_feed_display_with_skip_mt();

void print_per_frame_settings(per_frame_settings *settings);
float getsecsfloat();

//TEMPORARY:
// void epd_clear_area_cycles_(EpdRect area, int cycles, int cycle_time);

// void epd_clear_area_(EpdRect area);

// void epd_clear_();

// void epd_control_reg_init_();

// void epd_poweron_();
// void epd_poweroff_();
// void IRAM_ATTR epd_push_pixels_i2s_(RenderContext_t *ctx, EpdRect area, short time, int color);

// EpdRect epd_full_screen_();
//TEMPORARY


//  #if USING_TEMP_FUNCTIONS == 1
// // #define  epd_clear_area_cycles__ epd_clear_area_cycles_
// // #define  epd_clear_area__ epd_clear_area_
// // #define  epd_clear__ epd_clear_
// // #define  epd_control_reg_init__ epd_control_reg_init_
// // #define  epd_poweron__ epd_poweron_
// // #define  epd_clear_area__ epd_clear_area_
// // #define  epd_poweroff__ epd_poweroff_
// // #define  epd_push_pixels_i2s__ epd_push_pixels_i2s_
// // #define epd_get_display  epd_get_display_
// // #define  epd_current_board__  epd_current_board_

// #else
// #define  epd_clear_area_cycles__ epd_clear_area_cycles
// #define  epd_clear_area__ epd_clear_area
// #define  epd_clear__ epd_clear
// #define  epd_control_reg_init__ epd_control_reg_init
// #define  epd_poweron__ epd_poweron
// #define  epd_clear_area__ epd_clear_area
// #define  epd_poweroff__ epd_poweroff
// #define  epd_push_pixels_i2s__ epd_push_pixels
// #define epd_get_display epd_get_display
// #define  epd_current_board__ epd_current_board
// #endif


