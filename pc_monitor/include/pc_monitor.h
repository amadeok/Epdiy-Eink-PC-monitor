#include "esp_attr.h"
#include <stdbool.h>
#include <stdint.h>



/**
 * Write the decompressed buffers to the display 
 */
void IRAM_ATTR pc_monitor_feed_display();

/**
 * Write the decompressed buffers to the display while the next one is being downloaded and extracted. Experimental, for testing only.
 */
void IRAM_ATTR pc_monitor_feed_display_multithreaded();

