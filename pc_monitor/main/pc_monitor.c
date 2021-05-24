#include "epd_driver.h"
#include "pc_monitor.h"
#include "i2s_data_bus.h"
#include "rmt_pulse.h"

#include "display_ops.h"

#include "esp_assert.h"
#include "esp_heap_caps.h"
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "xtensa/core-macros.h"
#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#define EPD_LINE_BYTES EPD_WIDTH / 4
#define MULTITASK 1

extern uint8_t ready1[6];
uint8_t *current_chunk;
//uint8_t *dma_buffer;

//extern uint8_t *fc0, *fc1 , *fc2 , *fc3 , *fc4 , *fc5 , *fc6 , *fc7 , *fc8 , *fc9 ;

uint8_t *get_current_chunk_ptr(int chunk_number, struct context ctx)
{
  switch (chunk_number)
  {
  case 0:
    return fc0;
  case 1:
    return fc1;
  case 2:
    return fc2;
  case 3:
    return fc3;
  case 4:
    return fc4;
  case 5:
    return fc5;
  case 6:
    return fc6;
  case 7:
    return fc7;
  case 8:
    return fc8;
  case 9:
    return fc9;
  }
  return fc0;
}

void IRAM_ATTR switch_framebuffer(struct context ctx)
{
  if (ctx.current_buffer == 0)
    ctx.current_buffer = 1;
  else if (ctx.current_buffer == 1)
    ctx.current_buffer = 0;
}

void IRAM_ATTR pc_monitor_feed_display_with_skip(int total_lines_changed, int prev_total_lines_changed, struct context ctx)
{
  long time2 = xTaskGetTickCount();
  int framebuffer_cycles_final;
  if (ctx.mouse_moved == 1 && total_lines_changed < ctx.framebuffer_cycles_2_threshold && prev_total_lines_changed < ctx.framebuffer_cycles_2_threshold)
    framebuffer_cycles_final = ctx.framebuffer_cycles_2;
  else
    framebuffer_cycles_final = ctx.framebuffer_cycles;
  for (int i = 0; i < framebuffer_cycles_final; i++)
  {

    epd_start_frame();

    for (int h = 0; h < ctx.nb_chunks; h++)
    {
      current_chunk = get_current_chunk_ptr(h, ctx);
      int offset = (h * ctx.nb_rows_per_chunk);
      if (ctx.enable_skipping == 1 && total_lines_changed < ctx.epd_skip_threshold) //only skip the rows that haven't changed if they are less in number than the specified threshold
      {
        for (int g = 0; g < ctx.nb_rows_per_chunk; g++)
        {
          switch (line_changed[offset + g])
          {
          case 1:
            memcpy(epd_get_current_buffer(), current_chunk + (g * EPD_LINE_BYTES), EPD_LINE_BYTES);

            epd_output_row(draw_rmt_times[h]);
            break;

          case 0:
            epd_skip();
            //   memcpy(epd_get_current_buffer(), current_chunk + (g * EPD_LINE_BYTES), EPD_LINE_BYTES);
            //   epd_output_row(ctx.rmt_high_time);
            break;
          }
        }
      }
      else
      {
        for (int g = 0; g < ctx.nb_rows_per_chunk; g++)
        {
          memcpy(epd_get_current_buffer(), current_chunk + (g * EPD_LINE_BYTES), EPD_LINE_BYTES);
          epd_output_row(draw_rmt_times[h]);
        }
      }
    }
    epd_end_frame();

    ctx.frame_counter++;
  }
  printf("draw time: %lu\n", xTaskGetTickCount() - time2);
}

void IRAM_ATTR pc_monitor_feed_display(int total_lines_changed, int prev_total_lines_changed, struct context ctx)
{
  long time2 = xTaskGetTickCount();
  int framebuffer_cycles_final;
  if (ctx.mouse_moved == 1 && total_lines_changed < ctx.framebuffer_cycles_2_threshold && prev_total_lines_changed < ctx.framebuffer_cycles_2_threshold)
    framebuffer_cycles_final = ctx.framebuffer_cycles_2;
  else
    framebuffer_cycles_final = ctx.framebuffer_cycles;

  for (int i = 0; i < framebuffer_cycles_final; i++)
  {

    epd_start_frame();

    for (int h = 0; h < ctx.nb_chunks; h++)
    {
      current_chunk = get_current_chunk_ptr(h, ctx);
      int offset = (h * ctx.nb_rows_per_chunk);

      for (int g = 0; g < ctx.nb_rows_per_chunk; g++)
      {
        memcpy(epd_get_current_buffer(), current_chunk + (g * EPD_LINE_BYTES), EPD_LINE_BYTES);
        epd_output_row(ctx.rmt_high_time);
      }
    }
    epd_end_frame();

    ctx.frame_counter++;
  }
  printf("draw time: %lu\n", xTaskGetTickCount() - time2);
}

void IRAM_ATTR pc_monitor_feed_display_multithreaded_v1(struct context ctx)
{
  printf("pc_monitor_feed_display_multithreaded \n");
  ctx.rmt_high_time = 150;
  int sleep_time = 1;
  while (ctx.mirroring_active == 0)
  {
    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
  while (1)
  {

    ctx.renderer_chunk_counter = 0;
    while (ctx.renderer_chunk_counter == ctx.downloader_chunk_counter)
    { //vTaskDelay(sleep_time / portTICK_PERIOD_MS);
    };
    long time2 = xTaskGetTickCount();

    epd_start_frame();

    for (int b = 0; b < ctx.nb_rows_per_chunk; b++)
    {
      memcpy(epd_get_current_buffer(), framebuffer_chunks[0] + (b * EPD_LINE_BYTES), EPD_LINE_BYTES);
      epd_output_row(ctx.rmt_high_time);
      // output_row(ctx.rmt_high_time, 1, framebuffer_chunks[0] + b * EPD_LINE_BYTES);
    }
    ctx.renderer_chunk_counter++;
    //    printf("rend cc %d \n", ctx.downloader_chunk_counter);

    for (int h = 0; h < ctx.nb_chunks - 1; h++)
    {

      while (ctx.renderer_chunk_counter == ctx.downloader_chunk_counter)
      { //vTaskDelay(sleep_time / portTICK_PERIOD_MS);
      };

      for (int g = 0; g < ctx.nb_rows_per_chunk * (ctx.nb_chunks - 1); g++)
        epd_skip();
      for (int g = 0; g < ctx.nb_rows_per_chunk; g++)
      {
        memcpy(epd_get_current_buffer(), framebuffer_chunks[h + 1] + (g * EPD_LINE_BYTES), EPD_LINE_BYTES);
        epd_output_row(ctx.rmt_high_time);
        //  output_row(ctx.rmt_high_time, 1, framebuffer_chunks[h + 1] + (g * EPD_LINE_BYTES));
      }

      ctx.renderer_chunk_counter++;
      // printf("rend cc %d \n", ctx.downloader_chunk_counter);
    }
    epd_end_frame();

    ctx.renderer_chunk_counter = 0;

    int time3 = (xTaskGetTickCount() - time2);

    printf("draw time: %d\n", time3);
    //  printf("r: %d lp = %d \n", time3, ctx.frame_counter);

    ctx.frame_counter++;
  }
}

void IRAM_ATTR pc_monitor_feed_display_multithreaded_v1_one_chunk(struct context ctx)
{
  printf("pc_monitor_feed_display_multithreaded_v1_onechunk \n");
  ctx.rmt_high_time = 150;
  int sleep_time = 2;
  while (ctx.mirroring_active == 0)
  {
    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
  while (1)
  {

    ctx.renderer_chunk_counter = 0;
    while (ctx.renderer_chunk_counter == ctx.downloader_chunk_counter)
    {
      // vTaskDelay(sleep_time / portTICK_PERIOD_MS);
    };
    long time2 = xTaskGetTickCount();
    uint8_t *ptr = NULL;

#if MULTITASK == 1
    printf("r current_buffer %d \n", ctx.current_buffer);
    if (ctx.current_buffer == 1)
      ptr = second_framebuffer;
    else if (ctx.current_buffer == 0)
      ptr = framebuffer_chunks[0];
#else
    ptr = framebuffer_chunks[0];
#endif
    for (int y = 0; y < ctx.framebuffer_cycles; y++)
    {

      epd_start_frame();

      for (int b = 0; b < ctx.nb_rows_per_chunk; b++)
      {
        memcpy(epd_get_current_buffer(), ptr + (b * EPD_LINE_BYTES), EPD_LINE_BYTES);
        epd_output_row(ctx.rmt_high_time);
        // output_row(ctx.rmt_high_time, 1, framebuffer_chunks[0] + b * EPD_LINE_BYTES);
      }
    }
    ctx.renderer_chunk_counter++;
    epd_end_frame();
    switch_framebuffer(ctx);

    ctx.renderer_chunk_counter = 0;
    printf("rend cc %d \n", ctx.downloader_chunk_counter);

    int time3 = (xTaskGetTickCount() - time2);

    printf("draw time: %d\n", time3);
    //  printf("r: %d lp = %d \n", time3, ctx.frame_counter);

    ctx.frame_counter++;
  }
}

// void IRAM_ATTR signal_245_fifo(const int sock)
// {
//   send(sock, "ready0", 6, 0);

//   recv(sock, ready1, 6, 0);
//   epd_start_frame();

//   for (int h = 0; h > 100; h++)
//   {
//     send(sock, "ready0", 6, 0);
//     vTaskDelay(100 / portTICK_PERIOD_MS);
//    // output_row_245(150);
//   }
//   epd_end_frame();
// }

void IRAM_ATTR pc_monitor_feed_display_multithreaded_v2(struct context ctx)
{
  printf("pc_monitor_feed_display_multithreaded \n");
  ctx.rmt_high_time = 150;
  int sleep_time = 1;
  while (ctx.mirroring_active == 0)
  {
    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
  while (1)
  {

    ctx.renderer_chunk_counter = 0;
    while (ctx.renderer_chunk_counter == ctx.downloader_chunk_counter)
    { //vTaskDelay(sleep_time / portTICK_PERIOD_MS);
    };
    long time2 = xTaskGetTickCount();
    for (int j = 0; j < 3; j++)
    {
      epd_start_frame();

      for (int b = 0; b < ctx.nb_rows_per_chunk; b++)
      {
        memcpy(epd_get_current_buffer(), framebuffer_chunks[0] + (b * EPD_LINE_BYTES), EPD_LINE_BYTES);
        epd_output_row(ctx.rmt_high_time);
        // output_row(ctx.rmt_high_time, 1, framebuffer_chunks[0] + b * EPD_LINE_BYTES);
      }
      epd_end_frame();
    }
    ctx.renderer_chunk_counter++;
    //    printf("rend cc %d \n", ctx.downloader_chunk_counter);

    for (int h = 0; h < ctx.nb_chunks - 1; h++)
    {

      while (ctx.renderer_chunk_counter == ctx.downloader_chunk_counter)
      { //vTaskDelay(sleep_time / portTICK_PERIOD_MS);
      };
      for (int j = 0; j < 3; j++)
      {
        epd_start_frame();

        for (int g = 0; g < ctx.nb_rows_per_chunk * (h + 1); g++)
          epd_skip();
        for (int g = 0; g < ctx.nb_rows_per_chunk; g++)
        {
          memcpy(epd_get_current_buffer(), framebuffer_chunks[h + 1] + (g * EPD_LINE_BYTES), EPD_LINE_BYTES);
          epd_output_row(ctx.rmt_high_time);
          //  output_row(ctx.rmt_high_time, 1, framebuffer_chunks[h + 1] + (g * EPD_LINE_BYTES));
        }
        epd_end_frame();
      }
      ctx.renderer_chunk_counter++;
      // printf("rend cc %d \n", ctx.downloader_chunk_counter);
    }

    ctx.renderer_chunk_counter = 0;

    int time3 = (xTaskGetTickCount() - time2);

    printf("draw time: %d\n", time3);
    //  printf("r: %d lp = %d \n", time3, ctx.frame_counter);

    ctx.frame_counter++;
  }
}
