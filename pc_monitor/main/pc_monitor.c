#include "epd_driver.h"
#include "pc_monitor.h"

#include "ed097oc4.h"

#include "esp_assert.h"
#include "esp_heap_caps.h"
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "xtensa/core-macros.h"
#include <string.h>

#define EPD_LINE_BYTES EPD_WIDTH / 4

int nb_times_to_write_framebuffer = 3; // sets the number of times to write the current framebuffer to the screen
int rmt_high_time = 150;   // defined at in rmt_pulse.h, a higher value makes blacks blacker and whites whiter
int rmt_low_time = 1;

extern int nb_chunks;            // number of pieces into which divide the framebuffer (for multiprocessing)
extern int nb_rows_per_chunk; 
int frame_counter = 0;

volatile int renderer_chunk_counter;
volatile int downloader_chunk_counter;

uint8_t **framebuffer_chunks; //array of pointers pointing to the decompressed chunks of framebuffer
//uint8_t *dma_buffer;

void IRAM_ATTR pc_monitor_feed_display()
{
  long time2 = xTaskGetTickCount();
  for (int i = 0; i < nb_times_to_write_framebuffer; i++)
  {

    epd_start_frame();

    for (int h = 0; h < nb_chunks; h++)
    {
      for (int g = 0; g < nb_rows_per_chunk; g++)
      {
        memcpy(epd_get_current_buffer(), framebuffer_chunks[h] + (g * EPD_LINE_BYTES), EPD_LINE_BYTES);
        epd_output_row(rmt_high_time);
         //   pc_monitor_output_row(rmt_high_time, rmt_low_time, framebuffer_chunks[h] + (g * EPD_LINE_BYTES), dma_buffer);
      }
    }
    epd_end_frame();

    frame_counter++;
  }
  int time3 = (xTaskGetTickCount() - time2);

  printf("draw time: %d\n", time3);
}

void IRAM_ATTR pc_monitor_feed_display_multithreaded()
{

  while (1)
  {

    renderer_chunk_counter = 0;
    while (renderer_chunk_counter == downloader_chunk_counter)
    {
    };
    long time2 = xTaskGetTickCount();
    epd_start_frame();

    for (int b = 0; b < nb_rows_per_chunk; b++)
    {
      memcpy(epd_get_current_buffer(), framebuffer_chunks[0] + (b * EPD_LINE_BYTES), EPD_LINE_BYTES);
      epd_output_row(rmt_high_time);
      // output_row(rmt_high_time, 1, framebuffer_chunks[0] + b * EPD_LINE_BYTES);
    }
    for (int g = 0; g < nb_rows_per_chunk * 4; g++)
      epd_skip();
    epd_end_frame();

    renderer_chunk_counter++;

    for (int h = 0; h <nb_chunks-1; h++)
    {

      while (renderer_chunk_counter == downloader_chunk_counter)
      {
      };

      epd_start_frame();
      for (int l = 0; l < (h + 1); l++)
      {
        for (int g = 0; g < nb_rows_per_chunk; g++)
          epd_skip();
      }
      for (int g = 0; g < nb_rows_per_chunk; g++)
      {
        memcpy(epd_get_current_buffer(), framebuffer_chunks[h] + (g * EPD_LINE_BYTES), EPD_LINE_BYTES);
        epd_output_row(rmt_high_time);
        //  output_row(rmt_high_time, 1, framebuffer_chunks[h + 1] + (g * EPD_LINE_BYTES));
      }
      for (int l = 0; l <nb_chunks-1 - (h + 1); l++)
      {
        for (int g = 0; g < nb_rows_per_chunk; g++)
          epd_skip();
      }
      epd_end_frame();

      renderer_chunk_counter++;
    }
    renderer_chunk_counter = 0;

    int time3 = (xTaskGetTickCount() - time2);

    printf("draw time: %d\n", time3);
    //  printf("r: %d lp = %d \n", time3, frame_counter);

    frame_counter++;
  }
}
// void IRAM_ATTR pc_monitor_init()
// {

//   epd_base_init(EPD_WIDTH);
//   dma_buffer = epd_get_current_buffer();
// }

// void IRAM_ATTR output_row(uint32_t output_time_dus,uint32_t rmt_low_time, uint8_t *current_row)
// {
//  memcpy(dma_buffer, current_row, EPD_LINE_BYTES);
//   while (i2s_is_busy() || rmt_busy())
//   {
//   };
//

//   latch_row();
// #if defined(CONFIG_EPD_DISPLAY_TYPE_ED097TC2) ||
//     defined(CONFIG_EPD_DISPLAY_TYPE_ED133UT2)
//   pulse_ckv_ticks(output_time_dus, 1, false);
// #else
//   pulse_ckv_ticks(output_time_dus, 50, false);
// #endif

//   i2s_start_line_output();
// }x 