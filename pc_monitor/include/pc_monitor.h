#include "esp_attr.h"
#include <stdbool.h>
#include <stdint.h>



/**
 * Write the decompressed buffers to the display 
 */
void IRAM_ATTR pc_monitor_feed_display(int total_lines_changed, int prev_total_lines_changed, int prev_total_lines_changed_2);

void IRAM_ATTR pc_monitor_feed_display_with_skip(int total_lines_changed, int prev_total_lines_changed, int prev_total_lines_changed_2);

uint8_t *get_current_chunk_ptr(int chunk_number);

/**
 * Write the decompressed buffers to the display while the next one is being downloaded and extracted. Experimental, for testing only.
 */
void IRAM_ATTR pc_monitor_feed_display_multithreaded_v1();

void IRAM_ATTR pc_monitor_feed_display_multithreaded_v2();

void IRAM_ATTR signal_245_fifo(const int sock);

void IRAM_ATTR pc_monitor_feed_display_multithreaded_v1_one_chunk();

