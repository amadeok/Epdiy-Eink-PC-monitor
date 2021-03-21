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
#include "epd_driver.h"
#include "ed097oc4.h"

#define WIFI_SSID "wifi_ssid"
#define WIFI_PASS "wifi_password"

// #define WIFI_SSID "Androide"
// #define WIFI_PASS ""
#define MULTITASK 0
#define FT245MODE 0

#define DEBUG_MSGs 0
int sock = 0;
#define PORT 3333
int buf_size;
volatile int is_connected = 0;
extern volatile int renderer_chunk_counter;
extern volatile int downloader_chunk_counter;
volatile int mirroring_active = 0;
extern volatile int current_buffer;
uint8_t *compressed_chunk;
int multitask = 0;
extern uint8_t **framebuffer_chunks;
extern uint8_t *second_framebuffer;
uint8_t *fc0, *fc1 = NULL, *fc2 = NULL, *fc3 = NULL, *fc4 = NULL, *fc5 = NULL, *fc6 = NULL, *fc7 = NULL, *fc8 = NULL, *fc9 = NULL;

uint8_t *array_with_zeros;
uint8_t *draw_white_bytes;
uint8_t *draw_black_bytes;
uint8_t *line_changed;
int16_t total_lines_changed[1] = {0};
int16_t prev_total_lines_changed = 0;
volatile uint8_t mouse_moved = 0;
uint8_t *where_to_download;
int download_size;

uint16_t *settings;
//bool already_got_settings = false;

int width_resolution = EPD_WIDTH, height_resolution = EPD_HEIGHT;

int total_nb_pixels, eink_framebuffer_size, chunk_size, nb_chunks = 5, nb_rows_per_chunk;

int framebuffer_cycles; // sets the number of times to write the current framebuffer to the screen
int rmt_high_time;      // defined in rmt_pulse.h, a higher value makes blacks blacker and whites whiter
int framebuffer_cycles_2, framebuffer_cycles_2_threshold;
int enable_skipping, epd_skip_threshold;
int pseudo_greyscale_mode;
int selective_compression;

uint8_t *chunk_lenghts;
int32_t *chunk_lenghts_int;
uint8_t ready0[6];
uint8_t ready1[6];
static int frame_counter = 0, space = 4;
bool stop = false;

// Event group
static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;
static const char *TAG = "pc_monitor";

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

// Main task
tcpip_adapter_ip_info_t wifi_task(void *pvParameter)
{
  // wait for connection
  printf("Main task: waiting for connection to the wifi network... \n");
  xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, 50000000);
  printf("connected!\n");

  // print the local IP address
  tcpip_adapter_ip_info_t ip_info;
  ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info));
  printf("IP Address:  %s\n", ip4addr_ntoa(&ip_info.ip));
  printf("Subnet mask: %s\n", ip4addr_ntoa(&ip_info.netmask));
  printf("Gateway:     %s\n", ip4addr_ntoa(&ip_info.gw));
  return ip_info;
  // while (1)
  //  vTaskDelay(4000 / portTICK_PERIOD_MS);
}
void rle_extract1(int compressed_size, uint8_t *decompressed_ptr, uint8_t *compressed)
{
  if (compressed == NULL)
    printf("compress NULL\n");
  if (decompressed_ptr == NULL)
    printf("decompressed_ptr NULL\n");

  int counter, counter2, id = 0, j = 0;

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
  epd_clear();
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
    if (chunk_size - tot < 4096 * 6)
    {
      buf_size2 = chunk_size - tot;
    }
  } while (tot < chunk_size);
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

int set_download_pointer(int chunk_number)
{

  int need_to_extract;
  if (chunk_lenghts_int[chunk_number] > chunk_size / 100 * selective_compression && selective_compression != 0)
  {
    where_to_download = get_current_chunk_ptr(chunk_number);
    download_size = chunk_size;
#if DEBUG_MSGs == 1
    printf("receving uncompressed framebuffer %d\n", chunk_lenghts_int[chunk_number]);
#endif
    need_to_extract = 0;
  }
  else
  {
    where_to_download = compressed_chunk;
    download_size = chunk_lenghts_int[chunk_number];
#if DEBUG_MSGs == 1
    printf("receving compressed framebuffer %d\n", download_size);
#endif
    need_to_extract = 1;
  }
  return need_to_extract;
  //  printf("where_to_download %p, download_size %d\n", where_to_download, download_size);
}

static void download_and_extract(const int sock)
{
  mirroring_active = true;
  while (1)
  {
    // printf("download_and_extract \n");
    int len = 0, tot = 0, compressed_size, buf_size = 4096 * 5;
    int delta = 0;
    downloader_chunk_counter = 0;
    send(sock, "ready0", 6, 0);

    recv(sock, ready1, 6, 0);
    send(sock, ready1, 6, 0);

    if (ready1[0] == 'm')
      mouse_moved = 1;
    else
      mouse_moved = 0;
    if (ready1[1] == 'p')
      pseudo_greyscale_mode = 1;
    else
      pseudo_greyscale_mode = 0;

    recv(sock, chunk_lenghts, nb_chunks * 4, 0);

    for (int a = 0; a < nb_chunks; a++)
    {
      memcpy(chunk_lenghts_int + (a * 1), chunk_lenghts + a * 4, 4 * sizeof(uint8_t));
      // chunk_lenghts_int[a] = chunk_lenghts[(a * 2)] | chunk_lenghts[(a * 2) + 1] << 8;
      // printf(" %d %d ", chunk_lenghts[(a*2)], chunk_lenghts[(a*2)+1]);
#if DEBUG_MSGs == 1
      printf(" %d ", chunk_lenghts_int[a]);
      if (a == nb_chunks)
        printf("\n");
#endif
      printf(" %d ", chunk_lenghts_int[a]);
    }
    printf("\n");
    long time1 = xTaskGetTickCount();
    recv(sock, line_changed, height_resolution + 2, 0);
    int total = 0;
    //  long time2 = xTaskGetTickCount();

    // for (int h = 0; h < height_resolution; h++)
    // total += line_changed[h];
    int prev_total_lines_changed_2 = prev_total_lines_changed;
    prev_total_lines_changed = total_lines_changed[0];
    memcpy(total_lines_changed, line_changed + height_resolution, 2);
    // printf("line changed %d, prev_total_lines_changed %d, 3rd %d \n", total_lines_changed[0], prev_total_lines_changed, prev_total_lines_changed_2); //, xTaskGetTickCount() - time2 );

    uint8_t need_to_extract = set_download_pointer(0);

    //  compressed_size = chunk_lenghts_int[0];
    // printf("cs0 %d \n", compressed_size);
    if (download_size < buf_size)
      buf_size = download_size;
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

      if (download_size - tot < 4096 * 6)
      {
        buf_size = download_size - tot;
      }
    } while (tot < download_size);
    if (len < 0)
    {
      printf("Powering off Epdiy board \n ");
      mirroring_active = false;
      epd_poweroff();
      break;
    }
#if DEBUG_MSGs == 1
    printf("tot %d \n", tot);
#endif

#if MULTITASK == 0
    if (need_to_extract == 1)
      rle_extract1(download_size, get_current_chunk_ptr(0), where_to_download);
#else
    printf("current_buffer %d \n", current_buffer);
    if (chunk_lenghts_int[0] < chunk_size / 10 * 3)
    {
      if (need_to_extract == 1)
        rle_extract1(download_size, get_current_chunk_ptr(0), where_to_download);
      else if (current_buffer == 1)
        rle_extract1(download_size, second_framebuffer, where_to_download);
    }

#endif
    //   delta = xTaskGetTickCount() - time2;
    //  printf("extracting took : %d ", delta);
    downloader_chunk_counter++;
    //printf("down cc %d \n", downloader_chunk_counter);

    for (int h = 0; h < nb_chunks - 1; h++)
    {
      // printf("D renderer %d downloader %d\n", renderer_chunk_counter, downloader_chunk_counter);
      tot = 0;
      len = 0;
      buf_size = 4096 * 5;
      need_to_extract = set_download_pointer(h + 1);

      // compressed_size = chunk_lenghts_int[h + 1];
      //   printf("cs%d %d \n", h + 1, compressed_size);

      if (download_size < buf_size)
        buf_size = download_size;

      do
      {
        len = recv(sock, where_to_download + tot, buf_size, 0);
#if DEBUG_MSGs == 1
        printf("len %d, tot %d\n", len, tot);
#endif
        //   print_values(tot);
        tot += len;
        if (len < 0)
          break;

        if (download_size - tot < 4096 * 6)
        {
          buf_size = download_size - tot;
        }
      } while (tot < download_size);
#if DEBUG_MSGs == 1
      printf("tot %d \n", tot);
#endif
      if (len < 0)
      {
        mirroring_active = false;
        printf("Powering off Epdiy board \n ");
        epd_poweroff();
        break;
      }
      if (need_to_extract == 1)
        rle_extract1(download_size, get_current_chunk_ptr(h + 1), where_to_download);

      downloader_chunk_counter++;
      //    printf("down cc %d \n", downloader_chunk_counter);
    }

    printf("Download and extract took : %lu\n", xTaskGetTickCount() - time1);
#if MULTITASK == 0
    //   if (enable_skipping == 1)
    pc_monitor_feed_display_with_skip(total_lines_changed[0], prev_total_lines_changed, prev_total_lines_changed_2);
    //  else
    //    pc_monitor_feed_display(total_lines_changed[0], prev_total_lines_changed, prev_total_lines_changed_2);
#endif
    frame_counter++;
  }
}
void receive_settings(const int sock)
{
  printf("Receiving settings.. \n");
  int8_t settings_size[1];
  recv(sock, settings_size, 1, 0);
  printf("settings_size %d \n", settings_size[0]);
  settings = (uint16_t *)calloc(settings_size[0], sizeof(uint16_t));

  int ret = recv(sock, settings, settings_size[0], 0);
  printf("### Settings ### %d \n", ret);
  printf("framebuffer_cycles %d \n", framebuffer_cycles = settings[0]);
  printf("rmt_high_time %d \n", rmt_high_time = settings[1]);
  printf("enable_skipping %d \n", enable_skipping = settings[2]);
  printf("epd_skip_threshold %d \n", epd_skip_threshold = settings[3]);
  printf("framebuffer_cycles_2 %d \n", framebuffer_cycles_2 = settings[4]);
  printf("framebuffer_cycles_2_threshold %d \n", framebuffer_cycles_2_threshold = settings[5]);
  printf("pseudo_greyscale_mode %d \n", pseudo_greyscale_mode = settings[6]);
  printf("selective_compression %d \n", selective_compression = settings[7]);
  printf("################# \n");
  // already_got_settings = true;
}

static void tcp_server_task(void *pvParameters)
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
  int yes = 0;
  int result = setsockopt(listen_sock,
                          IPPROTO_TCP,
                          TCP_NODELAY,
                          (char *)&yes,
                          sizeof(int)); // 1 - on, 0 - off
  if (result < 0)
    printf("error setting tcp socket options\n");
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

    ESP_LOGI(TAG, "Socket listening");
    struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
    uint addr_len = sizeof(source_addr);
    sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
    result = setsockopt(sock,
                        IPPROTO_TCP,
                        TCP_NODELAY,
                        (char *)&yes,
                        sizeof(int)); // 1 - on, 0 - off
    if (result < 0)
      printf("error setting tcp socket options\n");
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
    //  if (already_got_settings == false)

    power_on_driver();
    receive_settings(sock);

// dma_buffer = epd_get_current_buffer();
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
  //vTaskDelete(NULL);
  //wifi_task(NULL);
 // xTaskCreatePinnedToCore(&tcp_server_task, "tcp_server_task", 10000, NULL, 5, NULL, 1);
}

void init_memory(int extra_bytes)
{
  if (nb_chunks > 0)
    fc0 = (uint8_t *)heap_caps_malloc((chunk_size + extra_bytes) * sizeof(uint8_t), MALLOC_CAP_SPIRAM);
  if (nb_chunks > 1)
    fc1 = (uint8_t *)heap_caps_malloc((chunk_size + extra_bytes) * sizeof(uint8_t), MALLOC_CAP_SPIRAM);
  if (nb_chunks > 2)
    fc2 = (uint8_t *)heap_caps_malloc((chunk_size + extra_bytes) * sizeof(uint8_t), MALLOC_CAP_SPIRAM);
  if (nb_chunks > 3)
    fc3 = (uint8_t *)heap_caps_malloc((chunk_size + extra_bytes) * sizeof(uint8_t), MALLOC_CAP_SPIRAM);
  if (nb_chunks > 4)
    fc4 = (uint8_t *)heap_caps_malloc((chunk_size + extra_bytes) * sizeof(uint8_t), MALLOC_CAP_SPIRAM);
  if (nb_chunks > 5)
    fc5 = (uint8_t *)heap_caps_malloc((chunk_size + extra_bytes) * sizeof(uint8_t), MALLOC_CAP_SPIRAM);
  if (nb_chunks > 6)
    fc6 = (uint8_t *)heap_caps_malloc((chunk_size + extra_bytes) * sizeof(uint8_t), MALLOC_CAP_SPIRAM);
  if (nb_chunks > 7)
    fc7 = (uint8_t *)heap_caps_malloc((chunk_size + extra_bytes) * sizeof(uint8_t), MALLOC_CAP_SPIRAM);
  if (nb_chunks > 8)
    fc8 = (uint8_t *)heap_caps_malloc((chunk_size + extra_bytes) * sizeof(uint8_t), MALLOC_CAP_SPIRAM);
  if (nb_chunks > 9)
    fc9 = (uint8_t *)heap_caps_malloc((chunk_size + extra_bytes) * sizeof(uint8_t), MALLOC_CAP_SPIRAM);
}
void app_main()
{
  esp_log_level_set("wifi", ESP_LOG_NONE);

  heap_caps_print_heap_info(MALLOC_CAP_INTERNAL);
  heap_caps_print_heap_info(MALLOC_CAP_SPIRAM);
  // initialize NVS
  ESP_ERROR_CHECK(nvs_flash_init());

  // create the event group to handle wifi events
  wifi_event_group = xEventGroupCreate();

  // initialize the tcp stack
  tcpip_adapter_init();

  // initialize the wifi event handler
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

  // initialize the wifi stack in STAtion mode with config in RAM
  wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();

  ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));

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
  printf("Connecting to %s\n", WIFI_SSID);
  wifi_task(NULL);

  total_nb_pixels = width_resolution * height_resolution;
  eink_framebuffer_size = total_nb_pixels / 4;
  chunk_size = (eink_framebuffer_size / nb_chunks);
  nb_rows_per_chunk = height_resolution / nb_chunks;
  int extra_bytes = 200000 / nb_chunks;
  if (nb_chunks == 2)
    nb_rows_per_chunk = 412;
  //  int free_mem = esp_get_free_heap_size();
  // ESP_LOGI(TAG, "free memory %d ", free_mem);

  compressed_chunk = (uint8_t *)heap_caps_malloc(chunk_size, MALLOC_CAP_SPIRAM);
  chunk_lenghts = (uint8_t *)heap_caps_malloc(64, MALLOC_CAP_SPIRAM);
  chunk_lenghts_int = (int32_t *)heap_caps_malloc(nb_chunks * 64, MALLOC_CAP_SPIRAM);
  line_changed = (uint8_t *)heap_caps_malloc(height_resolution + 2, MALLOC_CAP_SPIRAM);

  // framebuffer_chunks = (uint8_t **)calloc(nb_chunks, sizeof(uint8_t));
  init_memory(extra_bytes);
  for (int g = 0; g < nb_chunks; g++)
  {
    // framebuffer_chunks[g] = (uint8_t *)heap_caps_malloc(chunk_size + 10000, MALLOC_CAP_SPIRAM);
    //   memset(framebuffer_chunks[g], 0, chunk_size + 10000);
    if (get_current_chunk_ptr(g) == NULL)
      ESP_LOGI(TAG, "ptr %d is null ", g);
    //else
    // ESP_LOGI(TAG, "ptr %p: ", get_current_chunk_ptr(g));
  }
  //free_mem = esp_get_free_heap_size();
  //ESP_LOGI(TAG, "free memory %d ", free_mem);

  array_with_zeros = (uint8_t *)heap_caps_malloc(129, MALLOC_CAP_SPIRAM);

  draw_black_bytes = (uint8_t *)heap_caps_malloc(129, MALLOC_CAP_SPIRAM);

  draw_white_bytes = (uint8_t *)heap_caps_malloc(129, MALLOC_CAP_SPIRAM);
  memset(array_with_zeros, 0, 129);
  memset(draw_black_bytes, 85, 129);
  memset(draw_white_bytes, 170, 129);
  memset(line_changed, 0, height_resolution + 2);

  epd_base_init(EPD_WIDTH);
  if (heap_caps_check_integrity_all(true) == 1)
    ESP_LOGI(TAG, "Checking heap integrity: OK ");
  else
    ESP_LOGI(TAG, "Heap is corrupted");

  ESP_LOGI(TAG, "nb_chunks %d, nb_rows_chunks %d, chunk_size %d, eink_framebuffer_size %d, chunk_size+extra_bytes %d", nb_chunks, nb_rows_per_chunk, chunk_size, eink_framebuffer_size, chunk_size + extra_bytes);
#if MULTITASK == 1
  multitask = 1;
  xTaskCreatePinnedToCore(&pc_monitor_feed_display_multithreaded_v1_one_chunk, "feed_display_task", 10000, NULL, 5, NULL, 0);
  second_framebuffer = (uint8_t *)heap_caps_malloc(eink_framebuffer_size, MALLOC_CAP_SPIRAM);
#endif
  xTaskCreatePinnedToCore(&tcp_server_task, "tcp_server_task", 10000, NULL, 5, NULL, 1);
}
