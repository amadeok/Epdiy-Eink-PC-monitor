#include "pc_monitor.h"
#include "epdiy.h"
#include "i2s_data_bus.h"
#include "render_i2s.h"
#include "rmt_pulse.h"
// #include "display_ops.h"
// #include "ed097oc4.h"
#include <string.h>
#include "esp_assert.h"
#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_types.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "xtensa/core-macros.h"

#include <lwip/netdb.h>
#include <stdio.h>
#include "esp_system.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
//  #include "app_utils.h"
// #include "temp.h"
#include "lut.h"//temp

float getsecsfloat()
{
  int64_t milliseconds = esp_timer_get_time();

  double seconds = (double)milliseconds / 1000000.0;
  return seconds;
}

static const epd_ctrl_state_t NoChangeState_pc = {0};


//  #if USING_TEMP_FUNCTIONS == 1
// #define  i2s_output_row i2s_output_row_
// #define  i2s_start_frame i2s_start_frame_
// #define  i2s_end_frame i2s_end_frame_
// #define  epd_renderer_init_basic__ epd_renderer_init_basic_

// #else
// #define  i2s_output_row i2s_output_row
// #define  i2s_start_frame i2s_start_frame
// #define  i2s_end_frame i2s_end_frame
// #define  epd_renderer_init_basic__ epd_renderer_init_basic
// #endif

#define EPD_LINE_BYTES DISPLAY.width / 4

long previousT = 0;
long currentT = 0;

uint32_t skipping;


static void IRAM_ATTR i2s_output_row_pc(uint32_t output_time_dus) {
    while (i2s_is_busy() || rmt_busy()) {
    };

    const EpdBoardDefinition* epd_board = epd_current_board();
    epd_ctrl_state_t* ctrl_state = epd_ctrl_state();
    epd_ctrl_state_t mask = NoChangeState_pc;


    ctrl_state->ep_sth = true;
    ctrl_state->ep_latch_enable = true;
    mask.ep_sth = true;
    mask.ep_latch_enable = true;
    epd_board->set_ctrl(ctrl_state, &mask);

    mask = NoChangeState_pc;
    ctrl_state->ep_latch_enable = false;
    mask.ep_latch_enable = true;
    epd_board->set_ctrl(ctrl_state, &mask);

    if (epd_get_display()->display_type == DISPLAY_TYPE_ED097TC2) {
        pulse_ckv_ticks(output_time_dus, 1, false);
    } else {
        pulse_ckv_ticks(output_time_dus, 50, false);
    }

    i2s_start_line_output();
    i2s_switch_buffer();
}

// float getsecsfloat() {return 1.0;}

static void IRAM_ATTR i2s_write_row_(uint32_t output_time_dus)
{
    i2s_output_row_pc(output_time_dus);
    skipping = 0;
}

static void IRAM_ATTR i2s_skip_row_(uint8_t pipeline_finish_time)
{
    int line_bytes = DISPLAY.width / 4;
    // output previously loaded row, fill buffer with no-ops.
    if (skipping < 2)
    {
        memset((void *)i2s_get_current_buffer(), 0x00, line_bytes);
        i2s_output_row_pc(pipeline_finish_time);
    }
    else
    {
        //   printf("-----> pulse_ckv %d\n", skip_count_);
        if (epd_get_display()->display_type == DISPLAY_TYPE_ED097TC2)
        {
            pulse_ckv_ticks(5, 5, false);
        }
        else
        {
            // According to the spec, the OC4 maximum CKV frequency is 200kHz.
            pulse_ckv_ticks(45, 5, false);
        }
    }
    skipping++;
}

void IRAM_ATTR pc_monitor_feed_display_with_skip(per_frame_settings* f_i, bool print_times)// frame_info
{
    long time2 = xTaskGetTickCount();

    for (int i = 0; i < f_i->rmt_high_times_n; i++)
    {
        uint64_t frame_start = esp_timer_get_time() / 1000;

        int rmt_time = f_i->rmt_high_times[i];
        
#if DEBUG_MSGs == 3
        // if (print_times)
        //     ESP_LOGI("F", " fc: %d | rmt: %d | ", frame_counter, rmt_time);
#endif

#if DEBUG_MSGs == 1
        if (print_times)
            printf(  "#framebuffer_cycle: %d,  frame_counter: %d , rmt timing: %d , mode %d #\n", i,   frame_counter, rmt_time, f_i->mode);
#endif

        i2s_start_frame();

        if (enable_skipping == 1 && f_i->total_lines_changed < epd_skip_threshold) // only skip the rows that haven't changed if they are less in  number than the specified threshold
        {
            for (int g = 0; g < DISPLAY.height; g++)
            {
                //  taskYIELD(); testing for multithreading

                if (f_i->line_changed[g] == 0)
                {
                    // skip_line = g;
                     i2s_skip_row_(rmt_time);
                    continue;
                }

                 memcpy(   i2s_get_current_buffer(), f_i->frame_buffer + (g * EPD_LINE_BYTES),     EPD_LINE_BYTES);
                 i2s_write_row_(rmt_time);
            }
        }
        else
        {
            for (int g = 0; g < DISPLAY.height; g++)
            {
                 memcpy(    i2s_get_current_buffer(), f_i->frame_buffer + (g * EPD_LINE_BYTES),     EPD_LINE_BYTES);
                 i2s_write_row_(rmt_time);
            }
        }
        if (!skipping)
             i2s_write_row_(f_i->rmt_high_times[i]);

         i2s_end_frame();
        uint64_t frame_end = esp_timer_get_time() / 1000;
        // if (frame_end - frame_start < MINIMUM_FRAME_TIME)
        //   //  vTaskDelay(min_(MINIMUM_FRAME_TIME - (frame_end - frame_start), MINIMUM_FRAME_TIME));

        // frame_counter++;
    }

#if DEBUG_MSGs == 3 || DEBUG_MSGs == 4
    if (print_times)
    {
        previousT = currentT;
        currentT = xTaskGetTickCount();
        long delta = currentT - previousT;
        ESP_LOGI("F", "Draw time: %lu %4.3f | dc: %2d/%2d", currentT - time2, 1000 / (float)delta, f_i->draw_count, f_i->nb_draws);
    }
#endif
    printf("Draw time: %lu\n", xTaskGetTickCount() - time2);
}

void IRAM_ATTR pc_monitor_feed_display_with_skip_mt()
{
    int current_buf_index;

    while (1)
    {
#if DEBUG_MSGs == 3
         ESP_LOGI("F", " %-30s %-3s %4.3f", "waiting for frame ", "", getsecsfloat());
#endif
        xQueueReceive(queue, &current_buf_index, portMAX_DELAY);
#if DEBUG_MSGs == 3
         ESP_LOGI("F", " %-30s %-3d %4.3f", "start", current_buf_index, getsecsfloat());

#endif
#if DEBUG_MSGs == 4
        long time2 = xTaskGetTickCount();
#endif
        if (stop == 1)
        {
            ESP_LOGI("F", "terminating feed task ");
            vTaskDelete(NULL);
        }
        per_frame_settings *f_i = &per_frame_settings_arr[current_buf_index];
        pc_monitor_feed_display_with_skip(f_i, false);

        //   //skip_count_ = 0;
        //     for (int i = 0; i < f_i->rmt_high_times_n; i++)
        //     {
        //       // int skipped = 0;
        //       int rmt_time = f_i->rmt_high_times[i];
        //       uint64_t frame_start = esp_timer_get_time() / 1000;
        // #if DEBUG_MSGs == 1
        //       printf("#framebuffer_cycle: %d,  frame_counter: %d , rmt timing: %d , mode %d
        //       #\n", i, frame_counter, rmt_time, f_i->mode);
        // #endif
        //       i2s_start_frame();
        //       if (enable_skipping == 1 && f_i->total_lines_changed < epd_skip_threshold) //
        //       only skip the rows that haven't changed if they are less in number than the
        //       specified threshold
        //       {
        //         for (int g = 0; g < DISPLAY.height; g++)
        //         {
        //        //   printf("i %3d sc %3d ft %3d sk %3d cond %3d \n", g,  skip_count_,
        //        rmt_time, skipping, f_i->line_changed[g] == 0);
        //           if (f_i->line_changed[g] == 0)
        //           {
        //             i2s_skip_row_(rmt_time);
        //             continue;
        //           }
        //           memcpy(i2s_get_current_buffer(), f_i->frame_buffer + (g * EPD_LINE_BYTES),
        //           EPD_LINE_BYTES); i2s_write_row_(rmt_time);
        //         }
        //       }
        //       else
        //       {
        //         for (int g = 0; g < DISPLAY.height; g++)
        //         {
        //           memcpy(i2s_get_current_buffer(), f_i->frame_buffer + (g * EPD_LINE_BYTES),
        //           EPD_LINE_BYTES);
        //           // i2s_output_row_pc(rmt_time);
        //           i2s_write_row_(rmt_time);// taskYIELD();
        //         }
        //       }
        //       if (!skipping) // Since we "pipeline" row output, we still have to latch out
        //       the last      // row.
        //         i2s_write_row_(f_i->rmt_high_times[i]);
        //       i2s_end_frame();
        //       uint64_t frame_end = esp_timer_get_time() / 1000;
        //       if (frame_end - frame_start < MINIMUM_FRAME_TIME)
        //         vTaskDelay(min_(MINIMUM_FRAME_TIME - (frame_end - frame_start),
        //         MINIMUM_FRAME_TIME));
        //     }

#if DEBUG_MSGs == 4
        previousT = currentT;
        currentT = xTaskGetTickCount();
        long delta = currentT - previousT;
        ESP_LOGI("F", " Draw time: %lu %4.3f | dc: %2d/%2d", currentT - time2, 1000 / (float)delta, f_i->draw_count, f_i->nb_draws);
#endif
#if DEBUG_MSGs == 3
         ESP_LOGI("F", " %-30s %-3d %4.3f", "end", current_buf_index, getsecsfloat());
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

void print_per_frame_settings(per_frame_settings *settings)
{
  printf("mouse_moved: %d\n", settings->mouse_moved);
  printf("mode: %d\n", settings->mode);
  printf("do_full_refresh: %d\n", settings->do_full_refresh);

  printf("rmt_high_times_n: %d\n", settings->rmt_high_times_n);

  printf("rmt_high_times: ");
  for (int i = 0; i < settings->rmt_high_times_n; i++)
  {
    printf("%d ", settings->rmt_high_times[i]);
  }
  printf("\n");

  printf("type: %s\n", settings->type);

  printf("notes: %s\n", settings->notes);
  printf("framebuffer_data_size: %d\n", settings->framebuffer_data_size);
  printf("draw_count: %d\n", settings->draw_count);
  printf("total_lines_changed: %d\n", settings->total_lines_changed);
  printf("_need_to_extract: %d\n", settings->need_to_extract);

  printf("line_changed:  %3d %3d \n", settings->line_changed[0], settings->line_changed[1]);

}


// const EpdDisplay_t *epd_get_display_()
// {
//     return display_;
// }

// const EpdBoardDefinition *epd_current_board_()
// {
//     return epd_board_;
// }

// EpdRect epd_full_screen_() {
//   EpdRect area = {.x = 0, .y = 0, .width = DISPLAY.width, .height = DISPLAY.height};
//   return area;
// }

// void IRAM_ATTR epd_push_pixels_i2s_(RenderContext_t *ctx, EpdRect area, short time, int color) {

//     int line_bytes = ctx->display_width / 4;
//     uint8_t row[line_bytes];
//     memset(row, 0, line_bytes);

//     const uint8_t color_choice[4] = {DARK_BYTE, CLEAR_BYTE, 0x00, 0xFF};
//     for (uint32_t i = 0; i < area.width; i++) {
//         uint32_t position = i + area.x % 4;
//         uint8_t mask =
//             color_choice[color] & (0b00000011 << (2 * (position % 4)));
//         row[area.x / 4 + position / 4] |= mask;
//     }
//     reorder_line_buffer((uint32_t *)row, line_bytes);

//     i2s_start_frame();

//     for (int i = 0; i < ctx->display_height; i++) {
//         // before are of interest: skip
//         if (i < area.y) {
//             i2s_skip_row_( time);
//             // start area of interest: set row data
//         } else if (i == area.y) {
//             i2s_switch_buffer();
//             memcpy((void*)i2s_get_current_buffer(), row, line_bytes);
//             i2s_switch_buffer();
//             memcpy((void*)i2s_get_current_buffer(), row, line_bytes);

//             i2s_write_row_( time * 10);
//             // load nop row if done with area
//         } else if (i >= area.y + area.height) {
//             i2s_skip_row_( time);
//             // output the same as before
//         } else {
//             i2s_write_row_( time * 10);
//         }
//     }
//     // Since we "pipeline" row output, we still have to latch out the last row.
//     i2s_write_row_( time * 10);

//     i2s_end_frame();
// }


// void epd_clear_area_cycles_(EpdRect area, int cycles, int cycle_time) {
//     const short white_time = cycle_time;
//     const short dark_time = cycle_time;

//     for (int c = 0; c < cycles; c++) {
//         for (int i = 0; i < 10; i++) {
//             epd_push_pixels_i2s_(&render_context_, area, dark_time, 0);
//         }
//         for (int i = 0; i < 10; i++) {
//             epd_push_pixels_i2s_(&render_context_, area, white_time, 1);
//         }
//         for (int i = 0; i < 2; i++) {
//             epd_push_pixels_i2s_(&render_context_, area, white_time, 2);
//         }
//     }
// }
// const int clear_cycle_time_ = 12;


// void epd_clear_area_(EpdRect area) {
//     epd_clear_area_cycles_(area, 3, clear_cycle_time_);
// }

// void epd_clear_() { epd_clear_area_(epd_full_screen_()); }

// static epd_ctrl_state_t ctrl_state_;

// void epd_control_reg_init_() {
//   ctrl_state_.ep_latch_enable = false;
//   ctrl_state_.ep_output_enable = false;
//   ctrl_state_.ep_sth = true;
//   ctrl_state_.ep_mode = false;
//   ctrl_state_.ep_stv = true;
//   epd_ctrl_state_t mask = {
//     .ep_latch_enable = true,
//     .ep_output_enable = true,
//     .ep_sth = true,
//     .ep_mode = true,
//     .ep_stv = true,
//   };

//   epd_board_->set_ctrl(&ctrl_state_, &mask);
// }

// void epd_poweron_() {
//   epd_current_board__()->poweron(&ctrl_state_);
// }

// void epd_poweroff_() {
//   epd_current_board__()->poweroff(&ctrl_state_);
// }
