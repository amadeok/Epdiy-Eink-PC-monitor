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

int sock = 0;
#define PORT 3333
int buf_size;

extern volatile int renderer_chunk_counter;
extern volatile int downloader_chunk_counter;

uint8_t *compressed_chunk;

extern uint8_t **framebuffer_chunks;

uint8_t *array_with_zeros;

int width_resolution = EPD_WIDTH, height_resolution = EPD_HEIGHT;

int total_nb_pixels, eink_framebuffer_size, chunk_size, nb_chunks, nb_rows_per_chunk;

uint8_t chunk_lenghts[16];
int chunk_lenghts_int[8];
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
    break;

  default:
    break;
  }

  return ESP_OK;
}

// Main task
void wifi_task(void *pvParameter)
{
  // wait for connection
  printf("Main task: waiting for connection to the wifi network... \n");
  xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
  printf("connected!\n");

  // print the local IP address
  tcpip_adapter_ip_info_t ip_info;
  ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info));
  printf("IP Address:  %s\n", ip4addr_ntoa(&ip_info.ip));
  printf("Subnet mask: %s\n", ip4addr_ntoa(&ip_info.netmask));
  printf("Gateway:     %s\n", ip4addr_ntoa(&ip_info.gw));
}
void rle_extract1(int compressed_size, uint8_t *decompressed_ptr, uint8_t *compressed)
{
  int counter, counter2, id = 0, j = 0;

  counter2 = 0, counter = 0, buf_size = 4096;
  int offset = 0;
  int8_t i = 0;
  while (counter < compressed_size)
  {
    // if (counter2 > 45000)
    //   vTaskDelay(200 / portTICK_PERIOD_MS);

    i = compressed[counter];
    //printf( "counter %d, CAC: %hhx \n", counter, compressed[counter]);
    counter++;
    j = compressed[counter] & 0xff;
    //printf( "counter %d, CAC: %hhx \n", counter, compressed[counter]);
    counter++;

    if (i < 0)
    {
      offset = (i + 130);
      // if (counter2 > 45000)
      //   printf("offset %*d \n\n", space, offset);
      switch (j)
      {
      case 0:
        memcpy(decompressed_ptr + counter2, array_with_zeros, offset);
        counter2 += offset;

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
    if (49500 - tot < 4096 * 6)
    {
      buf_size2 = 49500 - tot;
    }
  } while (tot < 49500);
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
static void download_and_extract(const int sock)
{
  while (1)
  {
    // printf("download_and_extract \n");
    int len = 0, tot = 0, compressed_size, buf_size = 4096 * 5;
    int delta = 0;
    downloader_chunk_counter = 0;
    send(sock, "ready0", 6, 0);

    recv(sock, ready1, 6, 0);

    recv(sock, chunk_lenghts, 16, 0);
    for (int a = 0; a < 8; a++)
    {
      chunk_lenghts_int[a] = chunk_lenghts[(a * 2)] | chunk_lenghts[(a * 2) + 1] << 8;
      // printf(" %d %d ", chunk_lenghts[(a*2)], chunk_lenghts[(a*2)+1]);
    }

    long time1 = xTaskGetTickCount();

    compressed_size = chunk_lenghts_int[0];
    //  printf("cs0 %d \n", compressed_size);
    if (compressed_size < buf_size)
      buf_size = compressed_size;
    do
    {
      len = recv(sock, compressed_chunk + tot, buf_size, 0);
      //   printf("len %d, tot %d\n", len, tot);
      //   print_values(tot);
      tot += len;
      if (len < 0)
          break;

      if (compressed_size - tot < 4096 * 6)
      {
        buf_size = compressed_size - tot;
      }
    } while (tot < compressed_size);
    if (len < 0)
      break;
    // printf("tot %d \n", tot);

    rle_extract1(compressed_size, framebuffer_chunks[0], compressed_chunk);

    downloader_chunk_counter++;

    for (int h = 0; h < nb_chunks - 1; h++)
    {
      // printf("D renderer %d downloader %d\n", renderer_chunk_counter, downloader_chunk_counter);
      tot = 0;
      len = 0;
      buf_size = 4096 * 5;

      compressed_size = chunk_lenghts_int[h + 1];
      //  printf("cs%d %d \n",h + 1, compressed_size);

      if (compressed_size < buf_size)
        buf_size = compressed_size;

      do
      {
        len = recv(sock, compressed_chunk + tot, buf_size, 0);
        //  printf("len %d, tot %d\n", len, tot);
        //   print_values(tot);
        tot += len;
        if (len < 0)
          break;

        if (compressed_size - tot < 4096 * 6)
        {
          buf_size = compressed_size - tot;
        }
      } while (tot < compressed_size);
      // printf("tot %d \n", tot);
      if (len < 0)
        break;
      rle_extract1(compressed_size, framebuffer_chunks[h + 1], compressed_chunk);

      downloader_chunk_counter++;
    }

    delta = xTaskGetTickCount() - time1;
    printf("downloading and extracting took : %d\n", delta);
    pc_monitor_feed_display();

    frame_counter++;
  }
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
    power_on_driver();
    // dma_buffer = epd_get_current_buffer();

    download_and_extract(sock);
  }

CLEAN_UP:
  close(listen_sock);
  vTaskDelete(NULL);
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
  //   ESP_ERROR_CHECK(nvs_flash_init());

  nb_chunks = 5; // number of pieces into which divide the framebuffer (for multiprocessing)
  total_nb_pixels = width_resolution * height_resolution;
  eink_framebuffer_size = total_nb_pixels / 4;
  chunk_size = (eink_framebuffer_size / nb_chunks);
  nb_rows_per_chunk = height_resolution / nb_chunks;
  int free_mem = esp_get_free_heap_size();
  ESP_LOGI(TAG, "free memory %d ", free_mem);

  compressed_chunk = (uint8_t *)heap_caps_malloc(chunk_size, MALLOC_CAP_SPIRAM);

  free_mem = esp_get_free_heap_size();
  ESP_LOGI(TAG, "free memory %d ", free_mem);

  framebuffer_chunks = (uint8_t **)calloc(nb_chunks, sizeof(uint8_t));

  for (int g = 0; g < nb_chunks; g++)
  {
    framebuffer_chunks[g] = (uint8_t *)heap_caps_malloc(chunk_size + 10000, MALLOC_CAP_SPIRAM);

    memset(framebuffer_chunks[g], 0, chunk_size + 10000);

    if (framebuffer_chunks[g] == NULL)
      ESP_LOGI(TAG, "framebuffer_chunks %d is null ", g);
  }

  array_with_zeros = (uint8_t *)calloc(129, sizeof(uint8_t));

  epd_base_init(EPD_WIDTH);

  xTaskCreatePinnedToCore(&tcp_server_task, "tcp_server_task", 10000, NULL, 5, NULL, 1);
}
