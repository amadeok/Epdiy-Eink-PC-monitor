#include "esp_attr.h"
#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "xtensa/core-macros.h"


#define DEBUG_MSGs  4

volatile int renderer_chunk_counter, downloader_chunk_counter;
volatile unsigned long renderer_frame_counter, downloader_frame_counter;
// volatile int current_buffer;
//volatile int renderer_busy, downloader_busy;
volatile int stop, clearing;
volatile unsigned long tr0, tr1, td0, td1;

// volatile uint8_t mouse_moved;


//uint16_t *settings;

int width_resolution, height_resolution;

int total_nb_pixels, eink_framebuffer_size;

//int framebuffer_cycles; // sets the number of times to write the current framebuffer to the screen
//int rmt_high_time;      // defined in rmt_pulse.h, a higher value makes blacks blacker and whites whiter
//int framebuffer_cycles_2, framebuffer_cycles_2_threshold;
int enable_skipping, epd_skip_threshold, epd_skip_mouse_only;
//int draw_white_first;
int esp32_multithread;
int selective_compression;
int extra_bytes;
//int nb_draws;
int refresh_on_startup;
int draw_black_on_startup;
//int nb_rmt_times;
//int mode;
int frame_counter;
//uint8_t need_to_extract;

uint8_t *compressed_chunk;
//uint8_t *chunk_lenghts;
//int32_t *chunk_lenghts_int;
// uint8_t *line_changed;
//int16_t *total_lines_changed;
uint8_t *array_with_zeros;
uint8_t *draw_black_bytes;
uint8_t *draw_white_bytes;
uint8_t **framebuffer_chunks;
//uint8_t *second_framebuffer;
uint8_t *compressed_chunk;
// uint8_t *where_to_download;
//uint16_t *draw_rmt_times;
uint8_t *per_frame_wifi_settings_buffer;
//uint8_t *fc0, *fc1, *fc2, *fc3, *fc4, *fc5, *fc6, *fc7, *fc8, *fc9;
uint8_t ready0[6];
volatile uint8_t clear[2];

#define k 10
#define MINIMUM_FRAME_TIME  10


SemaphoreHandle_t begin;

typedef struct {
    int signal;
    volatile int mouse_moved;
    int mode;
    int do_full_refresh;
    int16_t rmt_high_times[100];
    int16_t rmt_high_times_n;
    // int16_t rmt_high_times_aux[100];
    // int16_t rmt_high_times_aux_n;
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

void print_per_frame_settings(per_frame_settings* settings);

/**
 * Write the decompressed buffers to the display 
 */
// void IRAM_ATTR pc_monitor_feed_display(int total_lines_changed);

void IRAM_ATTR pc_monitor_feed_display_with_skip(per_frame_settings* frame_info);

void IRAM_ATTR pc_monitor_feed_display_with_skip_mt();

// uint8_t *get_current_chunk_ptr(int chunk_number);

/**
 * Write the decompressed buffers to the display while the next one is being downloaded and extracted. Experimental, for testing only.
//  */
// void IRAM_ATTR pc_monitor_feed_display_multithreaded_v1();

// void IRAM_ATTR pc_monitor_feed_display_multithreaded_v2();

// void IRAM_ATTR signal_245_fifo(const int sock);

// void IRAM_ATTR pc_monitor_feed_display_multithreaded_v1_one_chunk();
// int IRAM_ATTR switch_framebuffer_n(int n);
// void IRAM_ATTR switch_framebuffer();
int back_buffer();
float getsecs();