/* Pc monitor application for Epdiy controller board. (work in progress)
 * More info at https://github.com/amadeok/Epdiy-PC-screen-monitor
 */

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "freertos/event_groups.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "pc_monitor.h"
#include "epdiy.h"

#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "cJSON.h"
//  #include "app_utils.h"
// #include "temp.h"
#include "render.h"

// #if USING_TEMP_FUNCTIONS == 1
// // #define  epd_full_screen epd_full_screen_
// // #define  epd_renderer_init_basic__ epd_renderer_init_basic_
// #else
// #define  epd_full_screen epd_full_screen
// #define  epd_renderer_init_basic__ epd_renderer_init_basic
// #endif

const char* transfer_uuid1 = "8fPMGCramH2aqRY2v5CGqY";
const char* transfer_uuid2 = "gizUD6hB2kxJEewtbB4MvU";
#define UUID_SIZE 22
#define TRANSFER_MESSAGE_SIZE (UUID_SIZE*2)+4

char transfer_message[TRANSFER_MESSAGE_SIZE];

#define FT245MODE 0

#define PORT 3333
int buf_size;
int sock = 0;

static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;
static const char *TAG = "pc_monitor";
char* ip_address = NULL;
const char * id_label = "id_label_not_assigned"; // top_left top_right bottom_left bottom_right
volatile bool connectedToPc = false;


// Wifi event handler
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
  switch (event->event_id)
  {

  case SYSTEM_EVENT_STA_START:
    esp_wifi_connect();
    break;

  case SYSTEM_EVENT_STA_GOT_IP:
    xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    break;

  case SYSTEM_EVENT_STA_DISCONNECTED:
    xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
    ESP_LOGI(TAG, "Wi-Fi disconnected, trying to reconnect...");
    esp_err_t err = esp_wifi_connect();
    if (err == ESP_ERR_WIFI_NOT_STARTED)
    {
      return;
    }
    ESP_ERROR_CHECK(err);
    break;

  default:
    break;
  }

  return ESP_OK;
}

void free_memory()
{
  heap_caps_free(compressed_chunk);
}

int end_session()
{
  printf("Powering off Epdiy board %d\n ", esp32_multithread);
  if (esp32_multithread == 1)
  {
    printf("sending end data to queues \n ");
    int data = -2;

    if (uxQueueMessagesWaiting(buffer_queue[0]) == 0)
      xQueueSend(buffer_queue[0], &data, portMAX_DELAY);

    if (uxQueueMessagesWaiting(buffer_queue[1]) == 0)
      xQueueSend(buffer_queue[1], &data, portMAX_DELAY);

    if (uxQueueMessagesWaiting(queue) == 0)
      xQueueSend(queue, &data, portMAX_DELAY);
    printf("sending end data to queues 2\n ");
  }
  // free_memory();
  for (int i = 0; i < REQUIRED_BUFFERS_N; i++)
  {
    heap_caps_free(per_frame_settings_arr[i].frame_buffer);
  }
  epd_poweroff();
  stop = 1;

  clearing = 0;
  memset(clear, 0, 2);
  esp_restart();
  return -1;
}

// Main task
tcpip_adapter_ip_info_t wifi_task(void *pvParameter)
{
  if (heap_caps_check_integrity_all(true) == 1)
    ESP_LOGI(TAG, "Checking heap integrity: OK ");
  else
    ESP_LOGI(TAG, "Heap is corrupted");
  // wait for connection
  printf("Main task: waiting for connection to the wifi network... \n");
  xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, 50000000);
  printf("connected!\n");

  // print the local IP address
  tcpip_adapter_ip_info_t ip_info;
  ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info));
  // memcpy(ip_address, &ip_info.ip, strlen(&ip_info.ip));
  if (ip_address == NULL)
  {
    ip_address = (char *)heap_caps_malloc(100, MALLOC_CAP_SPIRAM);
  }
  if (ip_address == NULL)
  {
    printf("ip_address ptr is null");
  }
  else
  {
    memset(ip_address, 0, 100);
    sprintf(ip_address, "%s", ip4addr_ntoa(&ip_info.ip));
  }

  printf("IP Address:  %s\n", ip4addr_ntoa(&ip_info.ip));
  printf("Subnet mask: %s\n", ip4addr_ntoa(&ip_info.netmask));
  printf("Gateway:     %s\n", ip4addr_ntoa(&ip_info.gw));
  printf("IP Address len:  %d\n", strlen(&ip_info.ip));
  return ip_info;
  // while (1)
  //  vTaskDelay(4000 / portTICK_PERIOD_MS);
}

static void  IRAM_ATTR rle_extract1(int compressed_size, uint8_t *decompressed_ptr, uint8_t *compressed)
{
  if (compressed == NULL)
    printf("compress NULL\n");
  if (decompressed_ptr == NULL)
    printf("decompressed_ptr NULL\n");

  int counter, counter2, j = 0;

  counter2 = 0, counter = 0, buf_size = 4096;
  int offset = 0;
  int8_t i = 0;
  while (counter < compressed_size)
  {
    // if (counter2 > 45000)
    // vTaskDelay(200 / portTICK_PERIOD_MS);

    i = compressed[counter];
    //  printf("counter %d, CAC: %hhx \n", counter, compressed[counter]);
    counter++;
    j = compressed[counter] & 0xff;
    // printf("counter %d, CAC: %hhx \n", counter, compressed[counter]);
    counter++;

    if (i < 0)
    {
      offset = (i + 130);
      // if (counter2 > 45000)
      //printf("offset %*d \n\n", space, offset);
      switch (j)
      {
      case 0:
        memcpy(decompressed_ptr + counter2, array_with_zeros, offset);
        counter2 += offset;
        break;
        // case 85:
        //   memcpy(decompressed_ptr + counter2, draw_black_bytes, offset);
        //   counter2 += offset;
        //   break;
        // case 170:
        //   memcpy(decompressed_ptr + counter2, draw_white_bytes, offset);
        //   counter2 += offset;
        break;
      default:
        for (int f = 0; f < i + 130; f++)
        {
          decompressed_ptr[counter2] = j;
          counter2++;
        }
        break;
      }
    }
    else if (i >= 0)
    {
      for (int f = 0; f < i; f++)
      {
        decompressed_ptr[counter2] = j;
        // if (counter2 > 45000)
        //   printf(" %*d ", space, j);
        j = compressed[counter++] & 0xff;
        counter2++;
      }
      decompressed_ptr[counter2] = j;

      counter2++;
    }
  }
}

void power_on_driver()
{
  printf("Powering on Epdiy board \n ");
  epd_poweron();
  printf("epd_poweron \n");

  volatile uint32_t t1 = xTaskGetTickCount();
  if (refresh_on_startup == -1)
  {
    epd_clear();
    printf("epd_clear\n");
  }
  else if (refresh_on_startup > 0){
    epd_clear_area_cycles(epd_full_screen(), 1, refresh_on_startup);
    printf("epd_clear_area_cycles\n");
    }
    else{
      printf("no refresh on startup\n");
    }
    /**
 * Darken / lighten an area for a given time.
 *
 * @param area: The area to darken / lighten.
 * @param time: The time in us to apply voltage to each pixel.
 * @param color: 1: lighten, 0: darken.
 */
  
  volatile uint32_t t2 = xTaskGetTickCount();
  printf("EPD clear took %dms.\n", t2 - t1);
  vTaskDelay(300 / portTICK_PERIOD_MS);
}

int send_compressed(int compressed_size) // for debugging
{

  int buf_size = 4096 * 5;
  if (compressed_size < buf_size)
    buf_size = compressed_size;
  int tott = 0, rett = 0, tot_times_t = compressed_size / buf_size;
  for (int g = 0; g < tot_times_t; g++) // g< tot_times
  {
    rett = send(sock, compressed_chunk + tott, buf_size, 0);
    tott += rett;
  }
  rett = send(sock, compressed_chunk + tott, compressed_size - tott, 0);
  return tott;
}

int send_decompressed(uint8_t *decompressed_chunck) // for debugging
{
  int buf_size2 = 4096 * 5, tot = 0, len = 0;
  do
  {
    len = send(sock, decompressed_chunck + tot, buf_size2, 0);
    //  printf("len %d, tot %d\n", len, tot);
    tot += len;
    if (eink_framebuffer_size - tot < 4096 * 6)
    {
      buf_size2 = eink_framebuffer_size - tot;
    }
  } while (tot < eink_framebuffer_size);
  return tot;
}

void print_values(int tot) // for debugging
{
  for (int h = 0; h < 10; h++)
    printf("%03d", compressed_chunk[tot + h]);
  printf("\n");

  for (int h = 0; h < 10; h++)
    printf(" %d ", (tot + h));
  printf("\n");
}

void populate_rmt_array(char* which, int16_t* whichArr, int16_t *array_size, cJSON * root){
    cJSON *numbers_array = cJSON_GetObjectItem(root, which);
    int i = 0;
    if (numbers_array != NULL && cJSON_IsArray(numbers_array))
    {
      *array_size = cJSON_GetArraySize(numbers_array);
      for (i = 0; i < *array_size; i++)
      {
        cJSON *element = cJSON_GetArrayItem(numbers_array, i);
        whichArr[i] = element->valueint;
        //per_frame_settings_arr[cur_free_buffer].rmt_high_times_main[i]  = element;
        // if (cJSON_IsNumber(element))
          //  printf("-----> %d\n", element);
      }
    }else
        printf(" numbers_array json is null\n");

    for (int i = 0; i < 100; i++ ){
      if (whichArr[i] == -1)break;
         printf("%s %d %d\n", which, i, whichArr[i]);
    }
}

per_frame_settings * populate_per_frame_settings_arr(cJSON *root_, int index)
{
  per_frame_settings *settings = &per_frame_settings_arr[index];
  // memset(settings->rmt_high_times, -1, settings->rmt_high_times_n + 1);
  //memset(settings->rmt_high_times_aux, -1, settings->rmt_high_times_aux_n + 1);

  settings->mouse_moved = cJSON_GetObjectItem(root_, "mouse_moved")->valueint;
  settings->mode = cJSON_GetObjectItem(root_, "mode")->valueint;
  settings->do_full_refresh = cJSON_GetObjectItem(root_, "do_full_refresh")->valueint;
  settings->notes = cJSON_GetObjectItem(root_, "notes")->valuestring;
  settings->need_to_extract = cJSON_GetObjectItem(root_, "need_to_extract")->valueint;
  settings->total_lines_changed = cJSON_GetObjectItem(root_, "total_lines_changed")->valueint;
  settings->framebuffer_data_size = cJSON_GetObjectItem(root_, "framebuffer_data_size")->valueint;
  settings->draw_count = cJSON_GetObjectItem(root_, "draw_count")->valueint;

  cJSON *current_draw_conf = cJSON_GetObjectItem(root_, "current_draw_conf");
  //cJSON *rmt_high_times = cJSON_GetObjectItem(current_draw_conf, "rmt_high_times");
  cJSON *draws_conf = cJSON_GetObjectItem(root_, "draws_conf");
  settings->nb_draws = cJSON_GetObjectItem(draws_conf, "nb_draws")->valueint;
 // printf("------>nb_draws %d\n", settings->nb_draws)->valueint;
  settings->type = cJSON_GetObjectItem(current_draw_conf, "type")->valuestring;

  populate_rmt_array("rmt_high_times", settings->rmt_high_times, &settings->rmt_high_times_n, current_draw_conf);

  return settings;
}


int N = 0;

void ch()
{
  if (heap_caps_check_integrity_all(true) == 1)
    ESP_LOGI(TAG, "Checking heap integrity: OK fun %d", N);
  else
    ESP_LOGI(TAG, "Heap is corrupted fun %d", N);
  N++;
}
void print_free_internal_ram(char* step){
   printf("sram step %s\n", step);
       //   ESP_LOGI("SRAM", "esp_get_free_heap_size: %d bytes", esp_get_free_heap_size());
    ESP_LOGI("SRAM", "esp_get_free_internal_heap_size: %d bytes", esp_get_free_internal_heap_size());
   //   ESP_LOGI("SRAM", "esp_get_minimum_free_heap_size: %d bytes", esp_get_minimum_free_heap_size());
}

static void  IRAM_ATTR download_and_extract(const int sock)
{
  //print_free_internal_ram("entry download_and_extract");

  uint8_t *ptr_m;

  downloader_frame_counter = 0;
  stop = 0;
  // if (esp32_multithread == 2)
  //   xSemaphoreGive(begin);
 // int cur_free_buffer = 0;

  while (1)
  {
    // int download_size;
    int per_frame_wifi_settings_size;

#if DEBUG_MSGs == 2
    printf("d0 download_and_extract loop \n");
#endif
    send(sock, "ready0", 6, 0);

    if (esp32_multithread == 1)
    {
#if DEBUG_MSGs == 3
       ESP_LOGI("D", " %-30s %-3d %4.3f", "waiting for buffer", switcher, getsecsfloat());
#endif
      xQueueReceive(buffer_queue[switcher], &switcher, portMAX_DELAY);
      // printf("D %-30s %-3d %4.3f\n", "received", switcher, getsecsfloat());
#if DEBUG_MSGs == 3
       ESP_LOGI("D", " %-30s %-3d %4.3f", "start", switcher, getsecsfloat());
#endif
    }
    unsigned long t0 = xTaskGetTickCount();

    int len = 0, tot = 0, compressed_size, buf_size = 4096 * 5;
    int delta = 0;

    recv(sock, transfer_message, TRANSFER_MESSAGE_SIZE, 0);
    for (int i = 0; i < UUID_SIZE; i++)
      if (transfer_message[i] != transfer_uuid1[i]){
        printf("WARNING TRANSFER MESSAGE UUID MISMATCH\n");
      }
    memcpy(&per_frame_wifi_settings_size, transfer_message + UUID_SIZE, 4);
    recv(sock, per_frame_wifi_settings_buffer, per_frame_wifi_settings_size, 0);
    //  printf("per_frame_wifi_settings_buffer:%d %s\n", per_frame_wifi_settings_size, per_frame_wifi_settings_buffer);
    // printf("t01 %lu \n", xTaskGetTickCount() - t0);

    // long t_f_i_0 = xTaskGetTickCount();
    cJSON *per_frame_settings_json_root = cJSON_Parse((const char *)per_frame_wifi_settings_buffer);
    memset(per_frame_wifi_settings_buffer, 0, min_(256 * 256, per_frame_wifi_settings_size+2));

    if (per_frame_settings_json_root == NULL)
    {
      const char *error_ptr = cJSON_GetErrorPtr();
      if (error_ptr != NULL)
        printf("Error before: %p\n", error_ptr);
      cJSON_Delete(per_frame_settings_json_root);
      return -1;
    }
     per_frame_settings* frame_info = populate_per_frame_settings_arr(per_frame_settings_json_root, switcher);
     //print_per_frame_settings(frame_info);
     cJSON_Delete(per_frame_settings_json_root);

    uint32_t free_sram = esp_get_free_internal_heap_size();
    if (free_sram < 10*1000  || 1)
      ESP_LOGI("SRAM", "--------------> warning low free sram: %d bytes", free_sram);

    len = recv(sock, frame_info->line_changed, height_resolution, 0);

    uint8_t *where_to_download =  frame_info->need_to_extract ? compressed_chunk : frame_info->frame_buffer ; // get_current_chunk_ptr(switcher) ;

    unsigned long t1 = xTaskGetTickCount();

    if (frame_info->framebuffer_data_size < buf_size)
      buf_size = frame_info->framebuffer_data_size;


    do
    {
      len = recv(sock, where_to_download + tot, buf_size, 0);
#if DEBUG_MSGs == 1
      printf("len %d, tot %d\n", len, tot);
#endif
      // print_values(tot);
      tot += len;
      if (len < 0)
        break;

      if (frame_info->framebuffer_data_size - tot < 4096 * 6)
      {
        buf_size = frame_info->framebuffer_data_size - tot;
      }
    } while (tot < frame_info->framebuffer_data_size);
    //printf("per_frame_wifi_settings 8\n");
    if (len < 0)
      if (end_session() == -1)
        break;

#if DEBUG_MSGs == 1
    printf("tot %d \n", tot);
#endif
    if (frame_info->need_to_extract == 1)
      rle_extract1(frame_info->framebuffer_data_size, frame_info->frame_buffer , where_to_download ); //get_current_chunk_ptr(switcher)

  
    unsigned long t2 = xTaskGetTickCount();

#if DEBUG_MSGs == 2
    ESP_LOGI("D", "2 Download and extract took : %lu | td1 td0: %lu, %lu ", td1 - td0, td0, td1);
#elif DEBUG_MSGs == 4
    unsigned long d1 = t1 - t0;
    unsigned long d2 = t2 - t1;
    unsigned long d3 = t2 - t0;
    ESP_LOGI("D", "Download and extract took: (%lu %lu) %lu, | dc: %2d/%2d", d1, d2, d3, frame_info->draw_count, frame_info->nb_draws);
#endif
    int pi = get_prev_index(switcher);
    //printf("------> cur %d, prev %d | dc %d nd %d | %d \n", switcher, pi, frame_info->draw_count, frame_info->nb_draws, frame_info->nb_draws > 1);

    if (esp32_multithread == 0)
    {

      if (frame_info->draw_count == frame_info->nb_draws - 1)
      {

        // if (frame_info->nb_draws > 1)
        //   pc_monitor_feed_display_with_skip(&per_frame_settings_arr[pi], true);
          pc_monitor_feed_display_with_skip(&per_frame_settings_arr[switcher], true);
       }
    }
    else
    {
#if DEBUG_MSGs == 3

      ESP_LOGI("D", " %-30s %-3d %4.3f", "end", switcher, getsecsfloat());
#endif
      if (frame_info->draw_count == frame_info->nb_draws - 1)
      {
        // int data = switcher;
        if (frame_info->nb_draws > 1)
          xQueueSend(queue, &pi, portMAX_DELAY);
        xQueueSend(queue, &switcher, portMAX_DELAY);
      }
    }
    switcher++;

    if (switcher >= REQUIRED_BUFFERS_N)
      switcher = 0;

    frame_counter++;
    // downloader_frame_counter = downloader_frame_counter >= 4294967290 ? 0 : downloader_frame_counter;
    // frame_counter = frame_counter == nb_draws ? 0 : frame_counter;
  }
}

void receive_settings(const int sock)
{

  printf("Receiving settings.. \n");
  int16_t settings_size[1];

  recv(sock, settings_size, 2, 0);
  printf("settings_size %d \n", settings_size[0]);

 // settings = (uint16_t *)calloc(settings_size[0], sizeof(uint16_t));
  char* json_string = (char *)calloc(settings_size[0], sizeof(char));

  int ret = recv(sock, json_string, settings_size[0], 0);
    printf("settings json string %s \n", json_string);

  // vTaskDelay(10000 / portTICK_PERIOD_MS);
  // char *json_string = "{\"name\": \"John\", \"age\": 30}";
  cJSON *root = cJSON_Parse(json_string);
  if (root == NULL)
  {
     printf("ERROR failed to parse settings json:\n");
  }
 // framebuffer_cycles = cJSON_GetObjectItem(root, "framebuffer_cycles");
  enable_skipping = cJSON_GetObjectItem(root, "enable_skipping")->valueint;
  epd_skip_threshold = cJSON_GetObjectItem(root, "epd_skip_threshold")->valueint;
  esp32_multithread = cJSON_GetObjectItem(root, "esp32_multithread")->valueint;
//  framebuffer_cycles_2 = cJSON_GetObjectItem(root, "framebuffer_cycles_2")->valueint;
 // framebuffer_cycles_2_threshold = cJSON_GetObjectItem(root, "framebuffer_cycles_2_threshold")->valueint;
 // draw_white_first = cJSON_GetObjectItem(root, "draw_white_first")->valueint;
  selective_compression = cJSON_GetObjectItem(root, "selective_compression")->valueint;
  //nb_chunks = cJSON_GetObjectItem(root, "nb_chunks")->valueint;
  //nb_draws = cJSON_GetObjectItem(root, "nb_draws")->valueint;
  //per_frame_wifi_settings_size = cJSON_GetObjectItem(root, "per_frame_wifi_settings_size")->valueint;
  refresh_on_startup = cJSON_GetObjectItem(root, "refresh_on_startup")->valueint;
  draw_black_on_startup = cJSON_GetObjectItem(root, "draw_black_on_startup")->valueint;

  printf("### Settings ### %d \n", ret);
  //printf("framebuffer_cycles %d \n", framebuffer_cycles );
  // printf("rmt_high_time %d \n", rmt_high_time = settings[1]);
  printf("enable_skipping %d \n", enable_skipping);
  printf("epd_skip_threshold %d \n", epd_skip_threshold);
  printf("esp32_multithread %d \n", esp32_multithread);

 // printf("framebuffer_cycles_2 %d \n", framebuffer_cycles_2 );
 // printf("framebuffer_cycles_2_threshold %d \n", framebuffer_cycles_2_threshold );
  //printf("draw_white_first %d \n", draw_white_first );
  printf("selective_compression %d \n", selective_compression );
 // printf("nb_draws %d \n", nb_draws);
 // printf("per_frame_wifi_settings_size %d \n", per_frame_wifi_settings_size);
  printf("refresh_on_startup %d \n", refresh_on_startup);
  printf("draw_black_on_startup %d \n", draw_black_on_startup);

  // if (nb_draws > framebuffer_cycles)
  //   nb_rmt_times = nb_draws;
  // else
  //   nb_rmt_times = framebuffer_cycles;

//  printf("nb_rmt_times %d \n", nb_rmt_times);
  printf("################# \n");
  // already_got_settings = true;
  width_resolution = DISPLAY.width;
  height_resolution = DISPLAY.height;

  total_nb_pixels = width_resolution * height_resolution;
  eink_framebuffer_size = total_nb_pixels / 4;
  //chunk_size = (eink_framebuffer_size / nb_chunks);
  //nb_rows_per_chunk = height_resolution / nb_chunks;
  extra_bytes = 50000;
  //  int free_mem = esp_get_free_heap_size();
  // ESP_LOGI(TAG, "free memory %d ", free_mem);



  compressed_chunk = (uint8_t *)heap_caps_malloc(eink_framebuffer_size*2, MALLOC_CAP_SPIRAM);
  // chunk_lenghts = (uint8_t *)heap_caps_malloc(64, MALLOC_CAP_SPIRAM);
  // chunk_lenghts_int = (int32_t *)heap_caps_malloc(nb_chunks * 64, MALLOC_CAP_SPIRAM);
  // line_changed = (uint8_t *)heap_caps_malloc(height_resolution + 2, MALLOC_CAP_SPIRAM);
  // total_lines_changed = (int16_t *)heap_caps_malloc(2, MALLOC_CAP_SPIRAM);
  // draw_rmt_times = (uint16_t *)heap_caps_malloc(nb_rmt_times * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
   per_frame_wifi_settings_buffer = (uint8_t *)heap_caps_malloc(256*256, MALLOC_CAP_SPIRAM);
    memset(per_frame_wifi_settings_buffer, 0, 256 * 256);

   for (int i = 0; i < REQUIRED_BUFFERS_N; i++)
   {
    int data = i;
     if (uxQueueMessagesWaiting(buffer_queue[i]) == 0)
       xQueueSend(buffer_queue[i], &data, portMAX_DELAY);
    //  if (uxQueueMessagesWaiting(buffer_queue[1]) == 0)
    //    xQueueSend(buffer_queue[1], &data, portMAX_DELAY);
   }
   // init_memory();
   for (int i = 0; i < REQUIRED_BUFFERS_N; i++)
   {
     per_frame_settings_arr[i].frame_buffer = (uint8_t *)heap_caps_malloc((eink_framebuffer_size + extra_bytes) * sizeof(uint8_t), MALLOC_CAP_SPIRAM);
     if (per_frame_settings_arr[i].frame_buffer == NULL)
       ESP_LOGI(TAG, "ptr %d is null ", i);
  }


  //free_mem = esp_get_free_heap_size();
  //ESP_LOGI(TAG, "free memory %d ", free_mem);
  

  ESP_LOGI(TAG, "eink_framebuffer_size %d, eink_framebuffer_size+extra_bytes %d",  eink_framebuffer_size, eink_framebuffer_size + extra_bytes);
  //memset(line_changed, 1, height_resolution + 2);
  for (int i = 0; i < 2; i++)
    memset(per_frame_settings_arr[i].line_changed, 0, height_resolution);
  

  if (esp32_multithread == 1)
  {
    // begin = xSemaphoreCreateBinary();

    xTaskCreatePinnedToCore(&pc_monitor_feed_display_with_skip_mt, "feed_display_task", 1 << 14, NULL, 10, NULL, 0); //0
    // second_framebuffer = (uint8_t *)heap_caps_malloc(chunk_size + extra_bytes, MALLOC_CAP_SPIRAM);
  }
    cJSON_Delete(root);

}

static void tcp_server_task(void *pvParameter)
{
  char addr_str[128];
  int addr_family;
  int ip_protocol;
  ESP_LOGI(TAG, "tcp_server_task");

#ifdef CONFIG_EXAMPLE_IPV4
  struct sockaddr_in dest_addr;
  dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(PORT);
  addr_family = AF_INET;
  ip_protocol = IPPROTO_IP;
  inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);
#else // IPV6
  struct sockaddr_in6 dest_addr;
  bzero(&dest_addr.sin6_addr.un, sizeof(dest_addr.sin6_addr.un));
  dest_addr.sin6_family = AF_INET6;
  dest_addr.sin6_port = htons(PORT);
  addr_family = AF_INET6;
  ip_protocol = IPPROTO_IPV6;
  inet6_ntoa_r(dest_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
#endif

  int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
  // int yes = 0;
  // int result = setsockopt(listen_sock,
  //                         IPPROTO_TCP,
  //                         TCP_NODELAY,
  //                         (char *)&yes,
  //                         sizeof(int)); // 1 - on, 0 - off
  // if (result < 0)
  //   printf("error setting tcp socket options\n");


    //     bool flag = true;
    // if (setsockopt(listen_sock, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(bool)) <0) {
    //    printf("Error setting TCP_NODELAY: \n");
    //     closesocket(listen_sock);
    //     return 1;
    // }

  if (listen_sock < 0)
  {
    ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
    goto CLEAN_UP;
    vTaskDelete(NULL);
    return;
  }
  ESP_LOGI(TAG, "Socket created");

  int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
  if (err != 0)
  {
    ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
    goto CLEAN_UP;
  }
  ESP_LOGI(TAG, "Socket bound, port %d", PORT);

  err = listen(listen_sock, 1);
  if (err != 0)
  {
    ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
    goto CLEAN_UP;
  }
  while (1)
  {
    connectedToPc = false;
    ESP_LOGI(TAG, "Socket listening");
    struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
    uint addr_len = sizeof(source_addr);
    sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);

    int nodelay = 1;
    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay)) < 0)
    {
      printf("Error setting TCP_NODELAY: \n");
      closesocket(sock);
      return 1;
    }

    // bool flag = true;
    // if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(bool)) <0) {
    //    printf("Error setting TCP_NODELAY: \n");
    //     closesocket(sock);
    //     return 1;
    //}

    // result = setsockopt(sock,
    //                     IPPROTO_TCP,
    //                     TCP_NODELAY,
    //                     (char *)&yes,
    //                     sizeof(int)); // 1 - on, 0 - off
    // if (result < 0)
    //   printf("error setting tcp socket options\n");


    if (sock < 0)
    {
      ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
      goto CLEAN_UP;
      break;
    }

    // Convert ip address to string
    if (source_addr.sin6_family == PF_INET)
    {
      inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
    }
    else if (source_addr.sin6_family == PF_INET6)
    {
      inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
    }
    ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);
    connectedToPc = true;
    //  if (already_got_settings == false)

    receive_settings(sock);
    power_on_driver();

    vTaskDelay(100 / portTICK_PERIOD_MS);

    for (int i = 0; i < draw_black_on_startup; i++)
    {
 #if USING_TEMP_FUNCTIONS == 1
     // epd_push_pixels_i2s_(&render_context_, epd_full_screen_(), 3, 0);
  #else
      epd_push_pixels(epd_full_screen(), 3, 0);
  #endif
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);

    // dma_buffer = epd_get_current_buffer();
    //     print_free_internal_ram("before download_and_extract");

#if FT245MODE == 0
    download_and_extract(sock);
#else
    signal_245_fifo(sock);
#endif
  }

CLEAN_UP:
  ESP_LOGI(TAG, "Restarting the board in 2 seconds..");
  close(listen_sock);
  vTaskDelay(2000 / portTICK_PERIOD_MS);
  esp_restart();
  // vTaskDelete(NULL);
  // wifi_task(NULL);
}

void set_id_label(char* label, nvs_handle_t flash_handle){
   esp_err_t err = nvs_set_str(flash_handle, "id_label", label);
  if (err != ESP_OK) {
      printf("Error (%s) writing to NVS\n", esp_err_to_name(err));
  } else {
      printf("String value written to NVS\n");
  }

  err = nvs_commit(flash_handle);
  if (err != ESP_OK) {
      printf("Error (%s) committing NVS\n", esp_err_to_name(err));
  }
}

bool get_id_label(char* buffer, nvs_handle_t flash_handle ){
  //char value[200]; // Assuming the maximum length of the string is 20 characters
  size_t required_size;
  bool id_label_found_in_flash = false;
  esp_err_t err = nvs_get_str(flash_handle, "id_label", NULL, &required_size);
  if (err == ESP_OK)
  {
    if (required_size > 200)
    { // sizeof(buffer)
      sprintf(buffer, "String value too large for buffer"); // printf("String value too large for buffer\n");
    }
    else
    {
      id_label_found_in_flash = true;
      err = nvs_get_str(flash_handle, "id_label", buffer, &required_size);
      if (err != ESP_OK)
        sprintf(buffer, "Error (%s) reading from NVS", esp_err_to_name(err));
      // printf("Retrieved value from NVS: %s\n", value);
      //  else
      // printf("Error (%s) reading from NVS\n", esp_err_to_name(err));
    }
  }
  else
    sprintf(buffer, "Error (%s) reading from NVS", esp_err_to_name(err)); // printf("Error (%s) reading from NVS\n", esp_err_to_name(err));
  return id_label_found_in_flash;
}

void app_main()
{
  ////epd_set_board(&BOARD);
    ////epd_renderer_init(EPD_LUT_1K);
    print_free_internal_ram("before epd_init");
    
    epd_set_board(&BOARD);
    epd_set_display(&DISPLAY);
    // // epd_board_ = &BOARD;
    // // display_ =& DISPLAY;
    epd_renderer_init_basic(&BOARD);
    
    // epd_control_reg_init();

  //// epd_init(&BOARD, &ED097TC2, EPD_LUT_1K); //uses a lot of sram

  //// vTaskDelay(10000 / portTICK_PERIOD_MS);

  // frame_counter = 0;
  width_resolution = DISPLAY.width;
  height_resolution = DISPLAY.height;
  //current_buffer = 0;
  memset(clear, 0, 2);

  printf("w %d %d, h %d %d, \n", width_resolution, height_resolution, DISPLAY.width, DISPLAY.height);

  esp_log_level_set("wifi", ESP_LOG_NONE);

  heap_caps_print_heap_info(MALLOC_CAP_INTERNAL);
  heap_caps_print_heap_info(MALLOC_CAP_SPIRAM);
  // initialize NVS
  esp_err_t ret2 = nvs_flash_init();
  if (ret2 == ESP_ERR_NVS_NO_FREE_PAGES || ret2 == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    // NVS partition was truncated, reformat it
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret2 = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret2);
  // create the event group to handle wifi events
  wifi_event_group = xEventGroupCreate();

  // initialize the tcp stack
  tcpip_adapter_init();

  if (heap_caps_check_integrity_all(true) == 1)
    ESP_LOGI(TAG, "Checking heap integrity: OK ");
  else
    ESP_LOGI(TAG, "Heap is corrupted");

  // initialize the wifi event handler
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

  // initialize the wifi stack in STAtion mode with config in RAM
  wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();

  ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    print_free_internal_ram("after esp_wifi_init");

  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

  // configure the wifi connection and start the interface
  wifi_config_t wifi_config = {
      .sta = {
          .ssid = WIFI_SSID,
          .password = WIFI_PASS,
      },
  };

  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

  ESP_ERROR_CHECK(esp_wifi_start());

  wifi_task(NULL);

  array_with_zeros = (uint8_t *)heap_caps_malloc(129, MALLOC_CAP_SPIRAM);
  draw_black_bytes = (uint8_t *)heap_caps_malloc(129, MALLOC_CAP_SPIRAM);
  draw_white_bytes = (uint8_t *)heap_caps_malloc(129, MALLOC_CAP_SPIRAM);

  memset(array_with_zeros, 0, 129);
  memset(draw_black_bytes, 85, 129);
  memset(draw_white_bytes, 170, 129);
   
  queue = xQueueCreate(2, ITEM_SIZE);
  // buffer_queue[0] = xQueueCreate(QUEUE_LENGTH, ITEM_SIZE);
  // buffer_queue[1] = xQueueCreate(QUEUE_LENGTH, ITEM_SIZE);

  switcher = 0;
  // epd_base_init(DISPLAY.width);
  for (int i = 0; i < REQUIRED_BUFFERS_N; i++){
    buffer_queue[i] = xQueueCreate(QUEUE_LENGTH, ITEM_SIZE);

    memset(per_frame_settings_arr[i].rmt_high_times, -1, 99);
    //memset(per_frame_settings_arr[i].rmt_high_times_aux, -1, 99);
    per_frame_settings_arr[i].line_changed  = (uint8_t *)malloc(sizeof(uint8_t) * height_resolution); 
    if (per_frame_settings_arr[i].line_changed == NULL)
      printf("Memory allocation line_changed failed!\n");
    else
      printf("Memory allocation line_changed  successful!\n");

    per_frame_settings_arr[i].mouse_moved = 'm';
    per_frame_settings_arr[i].mode = 0;
    per_frame_settings_arr[i].do_full_refresh = 0;
    per_frame_settings_arr[i].rmt_high_times[0] = 34;
    per_frame_settings_arr[i].rmt_high_times_n = 1;
    per_frame_settings_arr[i].notes ="somenote";
    per_frame_settings_arr[i].wifi_transfer_size = 247500;
    per_frame_settings_arr[i].framebuffer_data_pos = 11;
    per_frame_settings_arr[i].framebuffer_data_size = 247500;
    per_frame_settings_arr[i].line_changed_pos = 12;
    per_frame_settings_arr[i].draw_count = 1;
    per_frame_settings_arr[i].total_lines_changed = 825;
    per_frame_settings_arr[i].need_to_extract = 1;
    per_frame_settings_arr[i].line_changed[0] = 99;
    per_frame_settings_arr[i].frame_buffer = NULL;
  }
  
  xTaskCreatePinnedToCore(&tcp_server_task, "tcp_server_task",  1 << 14, NULL, 5, NULL, 1);//tskNO_AFFINITY 1

  nvs_handle_t flash_handle;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &flash_handle);
  if (err != ESP_OK)
  {
    printf("Error (%s) opening NVS handle\n", esp_err_to_name(err));
    return;
  }

  //optional: uncomment this line to set an id_label, start the board, then uncomment and reflash the board, and the value of id_label will remain in flash for ever
  //set_id_label("your_id_label2", flash_handle); 

  // char * id_label_buffer = (char *)heap_caps_malloc(200, MALLOC_CAP_SPIRAM);
  // bool id_label_found_in_flash = get_id_label(id_label_buffer, flash_handle);

  // char * id_label_to_print = id_label_found_in_flash ? id_label_buffer : id_label;

  while (true)
  {
    if (!connectedToPc)
    {
      printf("{\"ip_address\": \"%s\", \"id_label\": \"%s\"}\n", ip_address, id_label);
      // print_free_internal_ram("after printing ip address");
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    if (stop)break;
  }
  nvs_close(flash_handle);
 // heap_caps_free(id_label_buffer);
}
