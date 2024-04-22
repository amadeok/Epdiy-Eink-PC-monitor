#include "epd_driver.h"
#include "pc_monitor.h"
#include "i2s_data_bus.h"
#include "rmt_pulse.h"

//#include "display_ops.h"
#include "ed097oc4.h"
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
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#define EPD_LINE_BYTES EPD_WIDTH / 4
#define MULTITASK 0

extern uint8_t ready1[6];
uint8_t *current_chunk;
long previousT = 0;
long currentT = 0;
//uint8_t *dma_buffer;

//extern uint8_t *fc0, *fc1 , *fc2 , *fc3 , *fc4 , *fc5 , *fc6 , *fc7 , *fc8 , *fc9 ;

 static uint8_t IRAM_ATTR *get_current_chunk_ptr(int chunk_number)
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

// int IRAM_ATTR switch_framebuffer_n(int n)
// {
//   if (n == 0)
//     n = 1;
//   else if (n == 1)
//     n = 0;
//   return n;
// }
// void IRAM_ATTR switch_framebuffer()
// {
//   if (current_buffer == 0)
//     current_buffer = 1;
//   else if (current_buffer == 1)
//     current_buffer = 0;
//   return current_buffer;
// }
// int back_buffer()
// {
//   int n = 0;
//   if (current_buffer == 1)
//     n = 0;
//   else if (current_buffer == 0)
//     n = 1;
//   return n;
// }

void IRAM_ATTR pc_monitor_feed_display_with_skip(per_frame_settings* f_i) //frame_info
{

  long time2 = xTaskGetTickCount();

  for (int i = 0; i < f_i->rmt_high_times_n; i++)
  {
    int skipped = 0;

    int rmt_time = f_i->rmt_high_times[i];
#if DEBUG_MSGs == 3
    ESP_LOGI("F", " fc: %d | rmt: %d | ", frame_counter, rmt_time);
#endif

#if DEBUG_MSGs == 1
    printf("#framebuffer_cycle: %d,  frame_counter: %d , rmt timing: %d , mode %d #\n", i, frame_counter, rmt_time, f_i->mode);
#endif

    epd_start_frame();

    for (int h = 0; h < nb_chunks; h++)
    {
      current_chunk = get_current_chunk_ptr(h);
   //   int offset = (h * nb_rows_per_chunk);
      if (enable_skipping == 1 && f_i->total_lines_changed < epd_skip_threshold) //only skip the rows that haven't changed if they are less in number than the specified threshold
      {
        for (int g = 0; g < nb_rows_per_chunk; g++)
        {
          switch (f_i->line_changed[g])
          {
          case 1:
            memcpy(epd_get_current_buffer(), current_chunk + (g * EPD_LINE_BYTES), EPD_LINE_BYTES);

            epd_output_row(rmt_time);
            break;

          case 0:
            epd_skip();
            
           // skipped++;
            //   memcpy(epd_get_current_buffer(), current_chunk + (g * EPD_LINE_BYTES), EPD_LINE_BYTES);
            //   epd_output_row(rmt_high_time);
            break;
          }
        }
      }
      else
      {
        for (int g = 0; g < nb_rows_per_chunk; g++)
        {
          memcpy(epd_get_current_buffer(), current_chunk + (g * EPD_LINE_BYTES), EPD_LINE_BYTES);
          epd_output_row(rmt_time);
        }
      }
    }
    //printf("skipped %d\n", 825-skipped );
    epd_end_frame();

    // frame_counter++;
  }
        previousT = currentT;
      currentT =  xTaskGetTickCount();
      long delta = currentT - previousT;
#if DEBUG_MSGs == 3
      ESP_LOGI("F", "Draw time: %lu %4.3f", currentT - time2, 1000/(float)delta);
#endif

  // printf("Draw time: %lu\n", xTaskGetTickCount() - time2);
}

uint32_t skipping;

static void write_row(uint32_t output_time_dus) {
  skipping = 0;
  epd_output_row(output_time_dus);
}

void IRAM_ATTR skip_row_(uint8_t pipeline_finish_time) {
  // output previously loaded row, fill buffer with no-ops.
  if (skipping < 2) {
    memset(epd_get_current_buffer(), 0, EPD_LINE_BYTES);
    epd_output_row(pipeline_finish_time);
  } else {
    epd_skip();
  }
  skipping++;
}

void IRAM_ATTR pc_monitor_feed_display_with_skip_mt()
{
  int current_buf_index;

  while (1)
  {
#if DEBUG_MSGs == 3
    ESP_LOGI("F", " %-30s %-3s %4.3f", "waiting for frame ", "", getsecs());
#endif
    xQueueReceive(queue, &current_buf_index, portMAX_DELAY);
#if DEBUG_MSGs == 3
    ESP_LOGI("F", " %-30s %-3d %4.3f", "start", current_buf_index, getsecs());
#endif
    if (stop == 1)
    {
      ESP_LOGI("F", "terminating feed task ");
      vTaskDelete(NULL);
    }
    per_frame_settings *f_i = &per_frame_settings_arr[current_buf_index];
    
    // int total_lines_changed_ = EPD_HEIGHT;
    long time2 = xTaskGetTickCount();
    
    for (int i = 0; i < f_i->rmt_high_times_n; i++)
    {
      int skipped = 0;

      int rmt_time = f_i->rmt_high_times[i];

#if DEBUG_MSGs == 1
      printf("#framebuffer_cycle: %d,  frame_counter: %d , rmt timing: %d , mode %d #\n", i, frame_counter, rmt_time, f_i->mode);
#endif
    //  epd_start_frame();

      current_chunk = get_current_chunk_ptr(current_buf_index);

      if (enable_skipping == 1) //  && f_i->total_lines_changed < epd_skip_threshold only skip the rows that haven't changed if they are less in number than the specified threshold
      {
        for (int g = 0; g < nb_rows_per_chunk; g++)
        {
          if (f_i->line_changed[g] == 0)
          {
            skip_row_(rmt_time);
            continue;
          }

          memcpy(epd_get_current_buffer(), current_chunk + (g * EPD_LINE_BYTES), EPD_LINE_BYTES);
          write_row(rmt_time);

          // switch (f_i->line_changed[g])
          // {
          // case 1:
          //  memcpy(epd_get_current_buffer(), current_chunk + (g * EPD_LINE_BYTES), EPD_LINE_BYTES);
          //  write_row(rmt_time);
          // //  epd_output_row(rmt_time);
          // //  taskYIELD();
          //   //         printf("%d %d|", g, f_i->line_changed[g]);
          //   break;
          // case 0:
          //   skip_row(rmt_time);
          // //  epd_skip();
          //   ///  skipped++;
          //   //   memcpy(epd_get_current_buffer(), current_chunk + (g * EPD_LINE_BYTES), EPD_LINE_BYTES);
          //   //   epd_output_row(rmt_high_time);
          //   break;
          // }
        }
      }
      else
      {
        for (int g = 0; g < nb_rows_per_chunk; g++)
        {
          // if (g % 40 == 0)
          // printf("output row %3d\n", g);
          //    memcpy(epd_get_current_buffer(), current_chunk + (g * EPD_LINE_BYTES), EPD_LINE_BYTES);
          epd_output_row(rmt_time);
          //  vTaskDelay(1);
          //      taskYIELD();
        }
      }
      if (!skipping)
      { // Since we "pipeline" row output, we still have to latch out the last      // row.
        write_row(f_i->rmt_high_times[i]);
      }
      //      printf(" %d %d\n",  skipped, 825-skipped );

      epd_end_frame();
      // printf("skipped %d\n", 825 - skipped);

      // frame_counter++;
    }

#if DEBUG_MSGs == 4
    previousT = currentT;
    currentT = xTaskGetTickCount();
    long delta = currentT - previousT;
    ESP_LOGI("F", " Draw time: %lu %4.3f", currentT - time2, 1000 / (float)delta);
#endif
#if DEBUG_MSGs == 3
    ESP_LOGI("F", " %-30s %-3d %4.3f", "end", current_buf_index, getsecs());
#endif
    if (!stop)
      xQueueSend(buffer_queue[current_buf_index], &current_buf_index, portMAX_DELAY);
    else
    {
      printf("terminating feed task \n");
      vTaskDelete(NULL);
    }
  }
}

// void IRAM_ATTR pc_monitor_feed_display(int total_lines_changed)
// {
//   long time2 = xTaskGetTickCount();
//   int f_i->rmt_high_times_n;
//   if (mouse_moved == 1 && total_lines_changed < framebuffer_cycles_2_threshold)
//     f_i->rmt_high_times_n = framebuffer_cycles_2;
//   else
//     f_i->rmt_high_times_n = framebuffer_cycles;

//   for (int i = 0; i < f_i->rmt_high_times_n; i++)
//   {

//     epd_start_frame();

//     for (int h = 0; h < nb_chunks; h++)
//     {
//       current_chunk = get_current_chunk_ptr(h);
//       int offset = (h * nb_rows_per_chunk);

//       for (int g = 0; g < nb_rows_per_chunk; g++)
//       {
//         memcpy(epd_get_current_buffer(), current_chunk + (g * EPD_LINE_BYTES), EPD_LINE_BYTES);
//         epd_output_row(rmt_high_time);
//       }
//     }
//     epd_end_frame();

//     //   frame_counter++;
//   }
//   printf("draw time: %lu\n", xTaskGetTickCount() - time2);
// }

// void IRAM_ATTR pc_monitor_feed_display_multithreaded_v1()
// {
//   printf("pc_monitor_feed_display_multithreaded \n");
//   rmt_high_time = 150;
//   int sleep_time = 1;

//   xSemaphoreTake(begin, 9999999);
//   vTaskDelay(1000 / portTICK_PERIOD_MS);

//   while (1)
//   {

//     renderer_chunk_counter = 0;
//     while (renderer_chunk_counter == downloader_chunk_counter)
//     { //vTaskDelay(sleep_time / portTICK_PERIOD_MS);
//     };
//     long time2 = xTaskGetTickCount();

//     epd_start_frame();

//     for (int b = 0; b < nb_rows_per_chunk; b++)
//     {
//       memcpy(epd_get_current_buffer(), framebuffer_chunks[0] + (b * EPD_LINE_BYTES), EPD_LINE_BYTES);
//       epd_output_row(rmt_high_time);
//       // output_row(rmt_high_time, 1, framebuffer_chunks[0] + b * EPD_LINE_BYTES);
//     }
//     renderer_chunk_counter++;
//     //    printf("rend cc %d \n", downloader_chunk_counter);

//     for (int h = 0; h < nb_chunks - 1; h++)
//     {

//       while (renderer_chunk_counter == downloader_chunk_counter)
//       { //vTaskDelay(sleep_time / portTICK_PERIOD_MS);
//       };

//       for (int g = 0; g < nb_rows_per_chunk * (nb_chunks - 1); g++)
//         epd_skip();
//       for (int g = 0; g < nb_rows_per_chunk; g++)
//       {
//         memcpy(epd_get_current_buffer(), framebuffer_chunks[h + 1] + (g * EPD_LINE_BYTES), EPD_LINE_BYTES);
//         epd_output_row(rmt_high_time);
//         //  output_row(rmt_high_time, 1, framebuffer_chunks[h + 1] + (g * EPD_LINE_BYTES));
//       }

//       renderer_chunk_counter++;
//       // printf("rend cc %d \n", downloader_chunk_counter);
//     }
//     epd_end_frame();

//     renderer_chunk_counter = 0;

//     int time3 = (xTaskGetTickCount() - time2);

//     printf("draw time: %d\n", time3);
//     //  printf("r: %d lp = %d \n", time3, frame_counter);

//     //  frame_counter++;
//   }
// }

// void IRAM_ATTR pc_monitor_feed_display_multithreaded_v1_one_chunk()
// {
//   printf("pc_monitor_feed_display_multithreaded_v1_onechunk \n");
//   int rmt_time;
  
//   renderer_frame_counter = 0;

//   xSemaphoreTake(begin, 9999999);
//   vTaskDelay(1000 / portTICK_PERIOD_MS);

//   while (1)
//   {
// #if DEBUG_MSGs == 2
//     printf("r0 render loop \n");
// #endif
//     renderer_chunk_counter = 0;
//     uint8_t *ptr = NULL;
//     uint8_t *ptr_b = NULL;

//     while (renderer_frame_counter == downloader_frame_counter || clearing )
//     {
//       if (stop == 1)
//       {
//         printf("terminating render task \n");
//         vTaskDelete(NULL);
//       }
//       vTaskDelay(1 / portTICK_PERIOD_MS);
//     };

//     tr0 = xTaskGetTickCount();

//     ptr = get_current_chunk_ptr(current_buffer);

//     for (int y = 0; y < framebuffer_cycles; y++)
//     {
//       renderer_busy = 1;
//       if (mode == 10)
//         rmt_time = draw_rmt_times[frame_counter];
//       else
//         rmt_time = draw_rmt_times[y];

//       epd_start_frame();

//       for (int b = 0; b < nb_rows_per_chunk; b++)
//       {
//         memcpy(epd_get_current_buffer(), ptr + (b * EPD_LINE_BYTES), EPD_LINE_BYTES);
//         epd_output_row(rmt_time);
//         // output_row(rmt_high_time, 1, framebuffer_chunks[0] + b * EPD_LINE_BYTES);
//       }
//     }
//     epd_end_frame();
//     switch_framebuffer();
//     renderer_busy = 0;

//     tr1 = xTaskGetTickCount();

// #if DEBUG_MSGs == 2
//     printf("r2 fc %lu \n", renderer_frame_counter);
//     printf("r3 draw time: %lu | tr1, tr0:  %lu, %lu \n", tr1 - tr0, tr0, tr1);
// #else
//     printf("draw time: %lu\n", tr1 - tr0);
// #endif
//     renderer_frame_counter++;

//     //  printf("r: %d lp = %d \n", time3, frame_counter);
//     if (renderer_frame_counter == 4294967290)
//       renderer_frame_counter = 0;

//     // frame_counter++;
//   }
// }

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

// void IRAM_ATTR pc_monitor_feed_display_multithreaded_v2()
// {
//   printf("pc_monitor_feed_display_multithreaded \n");
//   rmt_high_time = 150;

//   xSemaphoreTake(begin, 9999999);
//   vTaskDelay(1000 / portTICK_PERIOD_MS);

//   while (1)
//   {

//     renderer_chunk_counter = 0;
//     while (renderer_chunk_counter == downloader_chunk_counter)
//     { //vTaskDelay(sleep_time / portTICK_PERIOD_MS);
//     };
//     long time2 = xTaskGetTickCount();
//     for (int j = 0; j < 3; j++)
//     {
//       epd_start_frame();

//       for (int b = 0; b < nb_rows_per_chunk; b++)
//       {
//         memcpy(epd_get_current_buffer(), framebuffer_chunks[0] + (b * EPD_LINE_BYTES), EPD_LINE_BYTES);
//         epd_output_row(rmt_high_time);
//         // output_row(rmt_high_time, 1, framebuffer_chunks[0] + b * EPD_LINE_BYTES);
//       }
//       epd_end_frame();
//     }
//     renderer_chunk_counter++;
//     //    printf("rend cc %d \n", downloader_chunk_counter);

//     for (int h = 0; h < nb_chunks - 1; h++)
//     {

//       while (renderer_chunk_counter == downloader_chunk_counter)
//       { //vTaskDelay(sleep_time / portTICK_PERIOD_MS);
//       };
//       for (int j = 0; j < 3; j++)
//       {
//         epd_start_frame();

//         for (int g = 0; g < nb_rows_per_chunk * (h + 1); g++)
//           epd_skip();
//         for (int g = 0; g < nb_rows_per_chunk; g++)
//         {
//           memcpy(epd_get_current_buffer(), framebuffer_chunks[h + 1] + (g * EPD_LINE_BYTES), EPD_LINE_BYTES);
//           epd_output_row(rmt_high_time);
//           //  output_row(rmt_high_time, 1, framebuffer_chunks[h + 1] + (g * EPD_LINE_BYTES));
//         }
//         epd_end_frame();
//       }
//       renderer_chunk_counter++;
//       // printf("rend cc %d \n", downloader_chunk_counter);
//     }

//     renderer_chunk_counter = 0;

//     int time3 = (xTaskGetTickCount() - time2);

//     printf("draw time: %d\n", time3);
//     //  printf("r: %d lp = %d \n", time3, frame_counter);

//     //   frame_counter++;
//   }
// }

float getsecs() {
    int64_t milliseconds = esp_timer_get_time();

    float seconds = (float)milliseconds / 1000000.0;
    return seconds;

}

void print_per_frame_settings(per_frame_settings* settings) {
   // printf("signal: %d\n", settings->signal);
    printf("mouse_moved: %d\n", settings->mouse_moved);
    printf("mode: %d\n", settings->mode);
    printf("do_full_refresh: %d\n", settings->do_full_refresh);
    
    printf("rmt_high_times_n: %d\n", settings->rmt_high_times_n);

    printf("rmt_high_times: ");
    for (int i = 0; i < settings->rmt_high_times_n; i++) {
        printf("%d ", settings->rmt_high_times[i]);
    }
    printf("\n");
      
    printf("type: %s\n", settings->type);

    printf("notes: %s\n", settings->notes);
   // printf("wifi_transfer_size: %d\n", settings->wifi_transfer_size);
   // printf("framebuffer_data_pos: %d\n", settings->framebuffer_data_pos);
    printf("framebuffer_data_size: %d\n", settings->framebuffer_data_size);
   // printf("line_changed_pos: %d\n", settings->line_changed_pos);
    printf("draw_count: %d\n", settings->draw_count);
    printf("total_lines_changed: %d\n", settings->total_lines_changed);
    printf("_need_to_extract: %d\n", settings->need_to_extract);
    
    printf("line_changed:  %3d %3d \n", settings->line_changed[0], settings->line_changed[1] );
    // for (int i = 0; i < settings->total_lines_changed; i++) {
    //     printf("%d ", settings->line_changed[i]);
    // }
    
  //  printf("\n");
}