#include "esp_attr.h"
#include <stdbool.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "xtensa/core-macros.h"

#define DEBUG_MSGs 2
volatile int is_connected;
volatile int mirroring_active;
volatile int renderer_chunk_counter;
volatile int downloader_chunk_counter;
volatile unsigned long renderer_frame_counter;
volatile unsigned long downloader_frame_counter;
volatile int current_buffer;
volatile int renderer_busy;
volatile int downloader_busy;
volatile int stop;
volatile int need_to_refresh;


int16_t prev_total_lines_changed;
volatile uint8_t mouse_moved;
int download_size;
int per_frame_wifi_settings_size;
uint16_t *settings;

int width_resolution, height_resolution;

int total_nb_pixels, eink_framebuffer_size, chunk_size, nb_chunks, nb_rows_per_chunk;

int framebuffer_cycles; // sets the number of times to write the current framebuffer to the screen
int rmt_high_time;      // defined in rmt_pulse.h, a higher value makes blacks blacker and whites whiter
int framebuffer_cycles_2, framebuffer_cycles_2_threshold;
int enable_skipping, epd_skip_threshold, epd_skip_mouse_only;
int esp32_multithread;
int selective_compression;
int extra_bytes;
int nb_draws;
int nb_rmt_times;
int mode;
uint8_t need_to_extract;
int frame_counter;

uint8_t *compressed_chunk;
uint8_t *chunk_lenghts;
int32_t *chunk_lenghts_int;
uint8_t *line_changed;
int16_t *total_lines_changed;
uint8_t *array_with_zeros;
uint8_t *draw_black_bytes;
uint8_t *draw_white_bytes;
uint8_t **framebuffer_chunks;
uint8_t *second_framebuffer;
uint8_t *compressed_chunk;
uint8_t *where_to_download;
uint16_t *draw_rmt_times;
uint8_t *per_frame_wifi_settings;
uint8_t *fc0, *fc1, *fc2, *fc3, *fc4, *fc5, *fc6, *fc7, *fc8, *fc9;
uint8_t ready0[6];
volatile uint8_t busy[6];  
volatile uint8_t clear[2];

SemaphoreHandle_t zero_sema;
SemaphoreHandle_t one_sema;
SemaphoreHandle_t semas[2];
SemaphoreHandle_t begin;
TaskHandle_t render_task_handle;

/**
 * Write the decompressed buffers to the display 
 */
void IRAM_ATTR pc_monitor_feed_display(int total_lines_changed);

void IRAM_ATTR pc_monitor_feed_display_with_skip(int total_lines_changed);

uint8_t *get_current_chunk_ptr(int chunk_number);

/**
 * Write the decompressed buffers to the display while the next one is being downloaded and extracted. Experimental, for testing only.
 */
void IRAM_ATTR pc_monitor_feed_display_multithreaded_v1();

void IRAM_ATTR pc_monitor_feed_display_multithreaded_v2();

void IRAM_ATTR signal_245_fifo(const int sock);

void IRAM_ATTR pc_monitor_feed_display_multithreaded_v1_one_chunk();
int IRAM_ATTR switch_framebuffer_n(int n);
void IRAM_ATTR switch_framebuffer();
int back_buffer();
