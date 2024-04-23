#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <fstream>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <vector>

#ifdef __linux__
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <netinet/tcp.h>
#elif _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib") //Winsock Library
#endif

#include <64bitLUTc.h>

#include <utils.h>
#include <rle_compression.h>
#include <generate_eink_framebuffer.h>
 #include <cJSON.h>

#define FT245MODE 0

#ifdef WITHOPENCV
#include <opencv2/opencv.hpp>
cv::Mat* whiteImage = nullptr;
#endif
enum withCv2Enum {    NONE,    PYTHON,    CPP ,    BOTH};

int width_resolution, height_resolution;

char compressed_chunk_lengths_in_bytes[64];

char working_dir[256];
unsigned char *array_with_zeros, *draw_white_bytes, *draw_black_bytes;

char *line_changed[16]; //array containing 1 or 0 depending on whether the corresponding line on the screen has changed

SOCKET socket_desc;
#if defined(_WIN32)
#define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
#else
#define ISVALIDSOCKET(s) ((s) >= 0)
#endif

int compressed_chunk_lengths[16];

const char* transfer_uuid1 = "8fPMGCramH2aqRY2v5CGqY";
const char* transfer_uuid2 = "gizUD6hB2kxJEewtbB4MvU";
#define UUID_SIZE 22
#define TRANSFER_MESSAGE_SIZE (UUID_SIZE*2)+4

char transfer_message[TRANSFER_MESSAGE_SIZE];

int wifi_on = 1;
char *ready2;
int start_nb_draws;

char *decompressed;                   // for testing or debugging
unsigned char *decompressed_received; // for testing or debugging
unsigned char *compressed_received;   // for testing or debugging
unsigned char *received;              // for testing or debugging
char *tmp_array;
// char * wifi_transfer_buffer;

char *compressed_eink_framebuffer_ptrs[16]; //array of pointers pointing to chunks of framebuffer
int id, refresh_every_x_frames = 0, refresh_every_x_frames_, selective_compression;
const int nb_chunks = 1; //to do: remove 
int total_nb_pixels, eink_framebuffer_size, chunk_size,  nb_rmt_times;
int source_image_bit_depth = 1, mode = -1, esp32_multithread;
int with_cv2 = 0;
bool disable_logging;
int mouse_moved = 0;
int do_full_refresh = 1;
// unsigned char full_refresh_delay = 30;
//int per_frame_wifi_settings_size;
char ready0[6];
char ready1[6];
//char *per_frame_wifi_settings;
// uint16_t *draw_rmt_times;
uint32_t loop_counter[1] = {0};

char input_pipe[200];
char output_pipe[200];


#ifdef __linux__
int fd0, fd1;
#elif _WIN32
HANDLE fd0, fd1;
DWORD dwRead;
DWORD dwBytesWritten;
#endif



void wifi_transfer(char *eink_framebuffer_swapped, char* line_changed, int eink_framebuffer_size, cJSON* per_frame_settings_json)
{
    //cJSON *frameJsonMetadata = cJSON_CreateObject();

    DWORD ret2;
    char *framebuffer_to_send[16];
    int framebuffer_to_send_size;

    int ret = 0, buf_size = 4096, tot = 0, tot2 = 0, len = 0;

  //  per_frame_wifi_settings[0] = mouse_moved == 1 ? 'm' : 0;
  //  per_frame_wifi_settings[1] = mode;

    if (refresh_every_x_frames_ && loop_counter[0] == refresh_every_x_frames_ && do_full_refresh != 0)
    {
        loop_counter[0] = 0;
        cJSON_ReplaceItemInObject(per_frame_settings_json, "do_full_refresh", cJSON_CreateNumber(do_full_refresh));
        //per_frame_wifi_settings[2] = full_refresh_delay;
    }
    else
        cJSON_ReplaceItemInObject(per_frame_settings_json, "do_full_refresh", cJSON_CreateNumber(0));
      //  per_frame_wifi_settings[2] = 0;

    if (refresh_every_x_frames_&& loop_counter[0] == refresh_every_x_frames_ + 1)
        loop_counter[0] = 0;



    //memcpy(per_frame_wifi_settings + 6, draw_rmt_times, nb_rmt_times * 2);
    // for (int x = 0; x < nb_rmt_times; x++)
    //     printf(" %d ", draw_rmt_times[x]);
    // printf("\n wifi ");
    //  send(socket_desc, per_frame_wifi_settings, per_frame_wifi_settings_size, 0);
    // int ret0 = recv(socket_desc, ready2, per_frame_wifi_settings_size, 0);
    // for (int k = 0; k < per_frame_wifi_settings_size; k++)
    // {
    //     if (ready2[k] != per_frame_wifi_settings[k])
    //     {
    //         printf("%d \n", ready2[k]);
    //         printf("warning ready2 per_frame_wifi_settings dif \n");
    //         //sleep(100000);
    //     }
    // }

        //wifi_transfer_buffer
  /// ret2 = send(socket_desc, compressed_chunk_lengths_in_bytes, nb_chunks * 4 * sizeof(unsigned char), 0);
   
  // ret2 = send(socket_desc, line_changed, (height_resolution + 2) * sizeof(unsigned char), 0);

    //   array_to_file(eink_framebuffer_swapped, eink_framebuffer_size, working_dir, "eink_framebuffer_swapped", 0);
   // unsigned char ready[5];
    // int g, a_ = 0;
    // for (int h = 0; h < nb_chunks; h++)
    // {
    //     if (compressed_chunk_lengths[h] > chunk_size / 100 * selective_compression)
    //         a_++;
    // }
    //  if (a == nb_chunks)
    //     printf("a = nb_chunms\n");
    // for (int g = 0; g < nb_chunks; g++)
    // {
    
    if (compressed_chunk_lengths[0] > chunk_size / 100 * selective_compression && selective_compression != 0)
    {
        framebuffer_to_send[0] = eink_framebuffer_swapped + (chunk_size * 0); //*g
        framebuffer_to_send_size = chunk_size;
        cJSON_ReplaceItemInObject(per_frame_settings_json, "need_to_extract", cJSON_CreateNumber(0));
    }
    else
    {
        framebuffer_to_send[0] = compressed_eink_framebuffer_ptrs[0];
        framebuffer_to_send_size = compressed_chunk_lengths[0];
        cJSON_ReplaceItemInObject(per_frame_settings_json, "need_to_extract", cJSON_CreateNumber(1));
    }
    cJSON_ReplaceItemInObject(per_frame_settings_json, "framebuffer_data_size", cJSON_CreateNumber(framebuffer_to_send_size));

    char *json_serialized = cJSON_PrintUnformatted(per_frame_settings_json);
    int32_t per_frame_settings_size = snprintf(NULL, 0, "%s", json_serialized);
    int32_t framebuffer_data_pos = per_frame_settings_size;
    int32_t line_changed_pos =  framebuffer_data_pos + framebuffer_to_send_size;

    int32_t wifi_transfer_size = per_frame_settings_size + framebuffer_to_send_size + height_resolution;

    // cJSON_ReplaceItemInObject(per_frame_settings_json, "framebuffer_data_pos", cJSON_CreateNumber(framebuffer_data_pos));
    // cJSON_ReplaceItemInObject(per_frame_settings_json, "line_changed_pos", cJSON_CreateNumber(line_changed_pos));
    
    memcpy(transfer_message+UUID_SIZE, &per_frame_settings_size, 4);
    
    ret2 = send(socket_desc, transfer_message, TRANSFER_MESSAGE_SIZE, 0);
   // int ret0 = recv(socket_desc, ready2, per_frame_settings_size, 0);

   //std::cout  << std::string(cJSON_Print(per_frame_settings_json)) << "\n";

    ret2 = send(socket_desc, json_serialized, per_frame_settings_size, 0);
    ret2 = send(socket_desc, line_changed, (height_resolution) * sizeof(unsigned char), 0);
    // char line_changed2[height_resolution];
    //  int ret0 = recv(socket_desc, line_changed2, height_resolution, 0);
    // for (int n = 0; n < height_resolution; n++){
    //     if (line_changed[n] != line_changed2[n]){
    //         printf("WARNING LINE CHANGED MISMATCH\n");
    //     }
    // }

    buf_size = 4096 * 5;
    int len2;
    tot = 0;
    ret = 0;
    int converted_number = 0;
    if (framebuffer_to_send_size < buf_size)
        buf_size = framebuffer_to_send_size;
    do
    {
        ret = send(socket_desc, framebuffer_to_send[0] + tot, buf_size, 0);
        tot += ret;
        // printf("send %d \n", ret);
        if (ret == -1)
        {
            printf("c++ id %d wifi transfer returned -1, exiting\n", id);
            exit(EXIT_FAILURE);
        }
        if (framebuffer_to_send_size - tot < buf_size + 5000)
            buf_size = framebuffer_to_send_size - tot;
    } while (tot < framebuffer_to_send_size);
    //     printf("tot %d\n", tot);
    if (tot != framebuffer_to_send_size)
    {
        printf("warning tot != compress size\n");
    }
    // }

    // puts("Data Send to esp\n");
    //    array_to_file(received, eink_framebuffer_size, working_dir, "received", 0);
}
#include <cassert>

void send_refresh_framebuffers(char *padded_2bpp_framebuffer_current, char *compressed_eink_framebuffer)
{
    assert(0); //TO DO
    printf("send_refresh_framebuffers\n");
    memset(line_changed, 1, height_resolution);

    memset(padded_2bpp_framebuffer_current, 85, eink_framebuffer_size);
    rle_compress(padded_2bpp_framebuffer_current, tmp_array, nb_chunks, compressed_eink_framebuffer, eink_framebuffer_size, chunk_size);
    for (int g = 0; g < nb_chunks * 4; g += 4)
    {
        unsigned int number2 = htonl(compressed_chunk_lengths[g / 4]);
        memcpy(compressed_chunk_lengths_in_bytes + g, &compressed_chunk_lengths[g / 4], 4);
    }
    if (wifi_on)
    {
      //  wifi_transfer(compressed_eink_framebuffer, 0);
        recv(socket_desc, ready0, 6, 0);
    }
    memset(padded_2bpp_framebuffer_current, 170, eink_framebuffer_size);
    rle_compress(padded_2bpp_framebuffer_current, tmp_array, nb_chunks, compressed_eink_framebuffer, eink_framebuffer_size, chunk_size);
    if (wifi_on)
    {
      //  wifi_transfer(compressed_eink_framebuffer, 0);
        recv(socket_desc, ready0, 6, 0);
    }
}

void print_chunk_sizes()
{
    for (int h = 0; h < nb_chunks; h++) // for debugging
        printf(" %5d ", compressed_chunk_lengths[h]);
    printf("\n");
}

static int mirroring_task()
{
#if FT245MODE == 1
    FT_HANDLE ft_handle = init_ft245_mode();
#endif
#ifdef __linux__

    fd1 = open(output_pipe, O_WRONLY);

    fd0 = open(input_pipe, O_RDONLY);
    int change_pipe_size = true;
    if (change_pipe_size == true)
    {
        long pipe_size = (long)fcntl(fd0, F_GETPIPE_SZ);
        int ret = fcntl(fd0, F_SETPIPE_SZ, 1048576);
        std::string the_ret = "";
        pipe_size = (long)fcntl(fd0, F_GETPIPE_SZ);
        std::string string_buf = "";
        printf("new pipe size: %ld\n", pipe_size);
    }
#endif

    //nb_chunks = 5; // number of pieces into which divide the framebuffer (for multiprocessing)
    int white_pixel;
    int first_time = 1;
    total_nb_pixels = width_resolution * height_resolution;
    eink_framebuffer_size = total_nb_pixels / 4;
    chunk_size = (eink_framebuffer_size / nb_chunks);

    unsigned char ack[1] = {246};
    unsigned char *ack2;
    char ready0[6];
    char ready1[6];

    int tot = 0, pos = 0;
    DWORD ret2 = 0;

    int compressed_framebuffer_size = 0;

    char *source_8bpp_current;           // array containing the current monochrome 8bpp screen capture
    char *source_8bpp_previous;          //array containing the previous monochrome 8bpp screen capture
    char *source_8bpp_modified_current;  //same as 'source_8bpp_current' but has been modified
    char *source_8bpp_modified_previous; //same as 'source_8bpp_previous' but has been modified

    char *eink_framebuffer[16]; // the 2bpp array that will be fed directly to the display

    char *filter_framebuffer;        // a filter framebuffer
    char *eink_framebuffer_modified; // a filter framebuffer that has been modified

    char *eink_framebuffer_swapped; // an 'eink_framebuffer' with bytes swapped

    char *compressed_eink_framebuffer; // an 'eink_framebuffer' that has been compressed with RLE compression

    char *padded_2bpp_framebuffer_current;  //array containing the current monochrome 2bpp screen capture
    char *padded_2bpp_framebuffer_previous; //array containing the previous monochrome 2bpp screen capture

    unsigned char *source_1bpp; //array containing the current monochrome 1bpp screen capture
    uint16_t *added_compression_arr[8];
    //per_frame_wifi_settings = (char *)calloc(per_frame_wifi_settings_size, sizeof(char));

    //line_changed = (char *)calloc(height_resolution, sizeof(char));
    source_1bpp = (unsigned char *)calloc(total_nb_pixels, sizeof(unsigned char));
    tmp_array = (char *)calloc(eink_framebuffer_size + 50000, sizeof(unsigned char));
   // wifi_transfer_buffer = (char *)calloc(eink_framebuffer_size *2, sizeof(unsigned char));

    //ready2 = (char *)calloc(per_frame_wifi_settings_size, sizeof(char));
    ack2 = (unsigned char *)calloc(256*256, sizeof(char));
    //memset(ready2, 0, per_frame_wifi_settings_size);

    padded_2bpp_framebuffer_current = (char *)calloc(eink_framebuffer_size, sizeof(unsigned char));
    padded_2bpp_framebuffer_previous = (char *)calloc(eink_framebuffer_size, sizeof(unsigned char));

    source_8bpp_modified_current = (char *)calloc(total_nb_pixels, sizeof(unsigned char));
    source_8bpp_modified_previous = (char *)calloc(total_nb_pixels, sizeof(unsigned char));
    source_8bpp_current = (char *)calloc(total_nb_pixels, sizeof(unsigned char));
    source_8bpp_previous = (char *)calloc(total_nb_pixels, sizeof(unsigned char));
    compressed_eink_framebuffer = (char *)calloc(eink_framebuffer_size, sizeof(char));
    filter_framebuffer = (char *)calloc(eink_framebuffer_size, sizeof(char));
    eink_framebuffer_modified = (char *)calloc(eink_framebuffer_size, sizeof(char));
    eink_framebuffer_swapped = (char *)calloc(eink_framebuffer_size, sizeof(char));
    decompressed = (char *)calloc(eink_framebuffer_size + 50000, sizeof(char));
    for (int h = 0; h < nb_chunks; h++)
    {
        compressed_eink_framebuffer_ptrs[h] = (char *)calloc(chunk_size * 2, sizeof(char));
        //added_compression_arr[h] = (uint16_t *)calloc(chunk_size * 2, sizeof(char));
    }

    const int preallocated_eink_framebuffer_n = 2;
    for (int h = 0; h < 16; h++)
        eink_framebuffer[h] = nullptr;

    for (int h = 0; h < preallocated_eink_framebuffer_n; h++)
        eink_framebuffer[h] = (char *)calloc(eink_framebuffer_size, sizeof(char));

    for (int h = 0; h < 16; h++)
        line_changed[h] = (char *)calloc(height_resolution, sizeof(char));

    if (source_image_bit_depth == 1)
    { //assume the first screen capture to be completely white
        memset(padded_2bpp_framebuffer_previous, 85, eink_framebuffer_size * sizeof(unsigned char));
        memset(padded_2bpp_framebuffer_current, 85, eink_framebuffer_size * sizeof(unsigned char));
    }
    else if (source_image_bit_depth == 8)
    { //assume the first screen capture to be completely white

     }
    else
    {
        printf("unsupported bit depth\n");
        return -1;
    }

     white_pixel =   source_image_bit_depth == 8 > 1 && mode == FourShadesGrayscale ? 255 : 1; //start_nb_draws 
    
    memset(source_8bpp_current, white_pixel, total_nb_pixels * sizeof(unsigned char));
    memset(source_8bpp_modified_current, white_pixel, total_nb_pixels * sizeof(unsigned char));
    memset(source_8bpp_previous, white_pixel, total_nb_pixels * sizeof(unsigned char));

    int tot_lines_changed[] = {1}, repeat_counter = 0, next = 0;
    

    printf("C++ ID %d mirroring started \n", id);

    while (1)
    {

        if (tot_lines_changed[0] != 0 && wifi_on == 1) // if screen didn't change don't wait for ack from board
            recv(socket_desc, ready0, 6, 0);

        ret2 = pipe_write(fd1, ack, 1, ret2);

        int16_t r_size = 0;
        ret2 = pipe_read(fd0, &r_size, 2, ret2);
       // int r_size = 3 + nb_rmt_times * sizeof(uint16_t); 
        ret2 = pipe_read(fd0, ack2, r_size, ret2);
        if (ret2 != r_size){
            printf("c++ id %d warning ret2 pipe_settings \n", id);
            return -1;
        }

        cJSON *per_frame_settings_json_root = cJSON_Parse((const char*)ack2);

        if (per_frame_settings_json_root == nullptr)
        {
            const char *error_ptr = cJSON_GetErrorPtr();
            if (error_ptr != nullptr)
                std::cerr << "Error before: " << error_ptr << std::endl;
            cJSON_Delete(per_frame_settings_json_root);
            return -1;
        }
        auto root_ = per_frame_settings_json_root;

        int signal = cJSON_GetObjectItem(root_, "signal")->valueint;
        mouse_moved = cJSON_GetObjectItem(root_, "mouse_moved")->valueint;
        mode = cJSON_GetObjectItem(root_, "mode")->valueint;
        do_full_refresh = cJSON_GetObjectItem(root_, "do_full_refresh")->valueint;
        std::string notes(cJSON_GetObjectItem(root_, "notes")->valuestring);
        int rotation = cJSON_GetObjectItem(root_, "rotation")->valueint;
        source_image_bit_depth = cJSON_GetObjectItem(root_, "pipe_bit_depth")->valueint;

        cJSON *draws_conf = cJSON_GetObjectItem(root_, "draws_conf");
        cJSON *draw_list = cJSON_GetObjectItem(draws_conf, "draw_list");
        int nb_draws = cJSON_GetArraySize(draw_list);
        cJSON_AddNumberToObject(draws_conf, "nb_draws", nb_draws);

        if (nb_draws > preallocated_eink_framebuffer_n)
            for (int i = preallocated_eink_framebuffer_n; i < nb_draws; i++)
                eink_framebuffer[i] = (char *)calloc(eink_framebuffer_size, sizeof(char));

        // cJSON *numbers_array = cJSON_GetObjectItem(rmt_high_times, "main");
        //if (numbers_array != NULL && cJSON_IsArray(numbers_array)) {
          //  int array_size = cJSON_GetArraySize(draw_list);
        // printf("nb_draws: %d\n", nb_draws);
        std::vector< draw_conf>  draws_conf_array;
        for (int i = 0; i < nb_draws; i++)
        {
            draw_conf conf;

            cJSON *element = cJSON_GetArrayItem(draw_list, i);
            conf.json_element = element;
            conf.type = cJSON_GetObjectItem(element, "type")->valuestring;
            cJSON *rmt_high_times = cJSON_GetObjectItem(element, "rmt_high_times");
            conf.rmt_high_times_n = cJSON_GetArraySize(rmt_high_times);
         //   printf("draw %d  | type %s | rmt_high_times_n %d || ", i, conf.type, conf.rmt_high_times_n);
            cJSON_AddNumberToObject(element, "rmt_high_times_n", conf.rmt_high_times_n);
            for (int i = 0; i < conf.rmt_high_times_n; i++)
            {
                conf.rmt_high_times[i] = cJSON_GetArrayItem(rmt_high_times, i)->valueint;
            //    printf(" %d ", conf.rmt_high_times[i]);
            }
            conf.typeID =  draw_type_map.find(std::string(conf.type))->second;
            
            draws_conf_array.push_back(conf);
          //  printf("\n");
        }
       //std::cout << "JSON Object:\n"   << std::string(cJSON_Print(root_)) << std::endl;

        // }

        // memcpy(draw_rmt_times, ack2 + 3, nb_rmt_times * sizeof(uint16_t));
        // mode = ack2[2];

        //        refresh_every_x_frames_ = mode == FourShadesGrayscale || draw_white_first ? refresh_every_x_frames * nb_draws : refresh_every_x_frames;

        if (signal == 101) //ack2[0]
        {
            printf("C++ app ID %d exiting \n", id);
            close(socket_desc);
            exit(EXIT_SUCCESS);
        }

        char *eight_bpp_ptr = mode == FourShadesGrayscale ? source_8bpp_modified_current : source_8bpp_current;
        
        white_pixel = source_image_bit_depth == 8 && mode == FourShadesGrayscale ? 255 : 1;// draw_white_first && mode == FourShadesGrayscale ? 255 : 1;

        if (mode == FourShadesGrayscale || source_image_bit_depth == 8 )  // || nb_draws > 1
        {
          //  source_image_bit_depth = 8;
            refresh_every_x_frames_ = refresh_every_x_frames * nb_draws;
        }
        else
        {
            refresh_every_x_frames_ = refresh_every_x_frames;
         //   source_image_bit_depth = 1;
          //  nb_draws = 1;
        }
        // for (int a = 0; a < nb_rmt_times; a++)
        //     printf("%d ", draw_rmt_times[a]);
        // printf("%d \n", loop_counter[0]);

        long t0 = getTick();
        // ret2 = pipe_read(fd0, line_changed, height_resolution, ret2);
        // if (ret2 != height_resolution){
        //     printf("c++ id %d warning ret2 line_changed \n", id);
        //     return -1;
        // }

        if (source_image_bit_depth == 1)
        {
            ret2 = pipe_read(fd0, source_1bpp, total_nb_pixels / 8, ret2);
            if (ret2 != total_nb_pixels / 8)
            {
                printf("c++ id %d warning ret2 1bpp \n", id);
                return -1;
            }
        }
        if (refresh_every_x_frames_ && loop_counter[0] == refresh_every_x_frames_)
        {
            if (do_full_refresh == 0)
                send_refresh_framebuffers(padded_2bpp_framebuffer_current, compressed_eink_framebuffer);

            if (source_image_bit_depth == 1)
                memset(padded_2bpp_framebuffer_previous, 85, eink_framebuffer_size * sizeof(unsigned char));
            else
                memset(source_8bpp_previous, white_pixel, total_nb_pixels * sizeof(unsigned char));
            #ifdef WITHOPENCV
            if (with_cv2 == withCv2Enum::BOTH || with_cv2 == withCv2Enum::CPP){
                 *whiteImage = cv::Mat(height_resolution,width_resolution, CV_8UC1, cv::Scalar(0));
            }
            #endif
        }
        else
        {
            if (esp32_multithread == 1 && first_time == 1 && loop_counter != 0)
            {
                memset(padded_2bpp_framebuffer_current, 85, eink_framebuffer_size * sizeof(unsigned char));
                memset(eight_bpp_ptr, white_pixel, total_nb_pixels * sizeof(unsigned char));
                first_time = 0;
            }
            if (source_image_bit_depth == 1)
                memcpy(padded_2bpp_framebuffer_previous, padded_2bpp_framebuffer_current, eink_framebuffer_size);
            else
                memcpy(source_8bpp_previous, eight_bpp_ptr, total_nb_pixels * sizeof(unsigned char));
        }

        if (source_image_bit_depth == 8)
        {

            ret2 = pipe_read(fd0, eight_bpp_ptr, total_nb_pixels, ret2);
            //  array_to_file(eight_bpp_ptr, total_nb_pixels, working_dir, "eight_bpp_ptr", 0);

            if (ret2 != total_nb_pixels)
            {
                printf("c++ id %d warning ret2 8bpp \n", id);
                return -1;
            }
        }

        if (source_image_bit_depth == 1)
        {
            generate_eink_framebuffer_v1(source_1bpp, padded_2bpp_framebuffer_current, padded_2bpp_framebuffer_previous, eink_framebuffer[0]);

            optimize_rle(eink_framebuffer[0]);

        }
        else if (source_image_bit_depth == 8)
        {

            switch (nb_draws)
            {
            case 99: //to do
                generate_eink_framebuffer_v2(source_8bpp_current, source_8bpp_previous, source_8bpp_modified_previous, eink_framebuffer, mode, draws_conf_array);
                break;
            default:
                //  int t0 = getTick();
                if (mode == FourShadesGrayscale)
                    quantize(source_8bpp_current, source_8bpp_modified_current, total_nb_pixels);
                //array_to_file(source_8bpp_previous, total_nb_pixels, working_dir, "source_8bpp_previous", 0);
                // system("python3 /home/amadeok/epdiy-working/examples/pc_monitor/pc_host_app/img_test.py source_8bpp_previous0");
                // array_to_file(eight_bpp_ptr, total_nb_pixels, working_dir, "eight_bpp_ptr", 0);
                // system("python3 /home/amadeok/epdiy-working/examples/pc_monitor/pc_host_app/img_test.py eight_bpp_ptr0");
                // printf("%d \n", getTick() - t0);
                generate_eink_framebuffer_v2(eight_bpp_ptr, source_8bpp_previous, source_8bpp_modified_previous, eink_framebuffer, mode, draws_conf_array);
                break;

                //  array_to_file(source_8bpp_modified_current, total_nb_pixels, working_dir, "source_8bpp_modified_current", 0);
            }
        }
        tot_lines_changed[0] = 0;
        for (int g = 0; g < nb_draws; g++)
            tot_lines_changed[0] += get_n_lines_changed_1bpp(eink_framebuffer[g], line_changed[g], rotation);
        // char line_changed_combined[height_resolution];
        // int tot_debug = 0;
        // for (int i = 0; i < height_resolution; i++){
        //     line_changed_combined[i] = 0;
        //     for (int g = 0; g < nb_draws; g++)
        //     {
        //         if (line_changed[g][i])
        //         {
        //             line_changed_combined[i] = 1;
        //             break;
        //         }
        //     }
        // } 
        // for (int y = 0; y < height_resolution; y++)
        //     tot_debug+= line_changed_combined[y];
      //  assert(tot_debug == tot_lines_changed[0]);
        for (int g = 0; g < nb_draws; g++)
        {

            //    std::string js2 = std::string(cJSON_Print(root_));
            //     printf("%s\n", js2.c_str());
            if (g != 0)
            {
                cJSON_ReplaceItemInObject(root_, "current_draw_conf", cJSON_Duplicate(draws_conf_array[g].json_element, 1));
                if (tot_lines_changed[0] != 0 && wifi_on == 1) // if screen didn't change don't wait for ack from board
                    recv(socket_desc, ready0, 6, 0);
            }
            else
                cJSON_AddItemToObject(root_, "current_draw_conf", cJSON_Duplicate(draws_conf_array[g].json_element, 1));

            swap_bytes(eink_framebuffer[g], eink_framebuffer_swapped, eink_framebuffer_size, source_image_bit_depth);

            // array_to_file(eink_framebuffer_swapped, eink_framebuffer_size, working_dir, "eink_fb_sw", 0);

            rle_compress(eink_framebuffer_swapped, tmp_array, nb_chunks, compressed_eink_framebuffer, eink_framebuffer_size, chunk_size);

            // rle_compress_v2(eink_framebuffer_swapped, tmp_array, nb_chunks, added_compression_arr, chunk_size);

            for (int g = 0; g < nb_chunks * 4; g += 4)
            {
                unsigned int number2 = htonl(compressed_chunk_lengths[g / 4]);
                memcpy(compressed_chunk_lengths_in_bytes + g, &compressed_chunk_lengths[g / 4], 4);
                // tot += foo;
            }
            // rle_extract1(decompressed, nb_chunks, eink_framebuffer_swapped, eink_framebuffer_size, compressed_chunk_lengths[0]); //for testing

            // print_chunk_sizes();


            if (refresh_every_x_frames_ && loop_counter[0] == refresh_every_x_frames_ || loop_counter[0] == refresh_every_x_frames_ + 1)
            {
                memset(line_changed[g], 1, height_resolution);
                tot_lines_changed[0] = height_resolution;
            }
            //  if (loop_counter[0] != refresh_every_x_frames_)

            printf("total line changed %d\n", tot_lines_changed[0]);

            cJSON_ReplaceItemInObject(per_frame_settings_json_root, "total_lines_changed", cJSON_CreateNumber(tot_lines_changed[0]));
            cJSON_ReplaceItemInObject(per_frame_settings_json_root, "draw_count", cJSON_CreateNumber(g));

            if (tot_lines_changed[0] != 0 && wifi_on == 1 && FT245MODE == 0) // send framebuffer only if current capture is different than previous
            {
                wifi_transfer(eink_framebuffer_swapped, line_changed[g], eink_framebuffer_size, per_frame_settings_json_root);
                repeat_counter = 0;
                // printf("loop_counter %d\n", loop_counter[0]);
                if (tot_lines_changed[0] > 85) // don't increase the counter to clear te display if only 85 lines have changed
                    loop_counter[0]++;
            }

// #define WITHOPENCV 1
#ifdef WITHOPENCV
            if (with_cv2 == withCv2Enum::BOTH || with_cv2 == withCv2Enum::CPP)
            {
                unsigned char pixels[4];
                unsigned char *imageData = whiteImage->data;

                for (int n = 0; n < chunk_size; n++)
                {
                    char c_ = eink_framebuffer[g][n];
                    unsigned char c = eink_framebuffer[g][n];
                    unpackByte(c, pixels);
                    for (int i = 0; i < 4; i++)
                    {
                        if (pixels[i] == 2)
                        {
                            int y = ((n*4)+i) / width_resolution; // Integer division gives the row number
                            int x = ((n*4)+i) % width_resolution; // Modulus gives the column number within the row
                            uchar &pixelValue = whiteImage->at<uchar>(y, x);
                             pixelValue = 0; // Subtract 50 from each pixel value

                            // imageData[((n*4)+i)] = 0;
                            // imageData[((n*4*3)+i*3)] = 0;
                            // imageData[((n*4*3)+i*3)+1] = 0;
                            // imageData[((n*4*3)+i*3)+2] = 0;
                        }
                        else if (pixels[i] == 1)
                        {
                            // imageData[(((n*4)) + i)] = 255;
                            int y = ((n*4)+i) / width_resolution; // Integer division gives the row number
                            int x = ((n*4)+i) % width_resolution; // Modulus gives the column number within the row
                            uchar &pixelValue = whiteImage->at<uchar>(y, x);
                              pixelValue = 255; // Subtract 50 from each pixel value
                            // imageData[((n*4*3)+i*3)] = 255;
                            // imageData[((n*4*3)+i*3)+1] = 255;
                            // imageData[((n*4*3)+i*3)+2] = 255;
                        }
                    }

                }
                    cv::Mat horizontal_flip;
                    cv::flip(*whiteImage, horizontal_flip, 1);

                    // Flip the horizontally flipped image vertically to get both horizontal and vertical flip
                    cv::Mat both_flips;
                    cv::flip(horizontal_flip, both_flips, 0);
                    cv::imshow("White Image with Changed Pixels", both_flips);
                    cv::waitKey(1);
            }
#endif
        }
        if (disable_logging == 0)
            printf("Processing time %dms\n", getTick() - t0);
#ifdef WITHOPENCV
        if (with_cv2 == withCv2Enum::BOTH || with_cv2 == withCv2Enum::CPP)
            cv::waitKey(1);
#endif
    cJSON_Delete(per_frame_settings_json_root);

    }
    return 0;
}


void iterateJson(cJSON *json) {
    cJSON *current = NULL;
    cJSON_ArrayForEach(current, json) {
        if (current->type == cJSON_Object) {
            // If the element is an object, iterate its members
            printf("Object:\n");
            iterateJson(current);
        } else if (current->type == cJSON_Array) {
            // If the element is an array, iterate its elements
            printf("Array:\n");
            iterateJson(current);
        } else {
            // Handle other types like strings, numbers, etc.
            printf("Type: %d, Value: %s\n", current->type, cJSON_Print(current));
        }
    }
}

int main(int argc, char *argv[])
{
    int nb_args = argc;
    char *esp32_ip_address = NULL;
    // int framebuffer_cycles;
    // char *rmt_high_time_s;
    int enable_skipping;
    int epd_skip_threshold;
    int esp32_multithread;
    // int framebuffer_cycles_2;
    // int framebuffer_cycles_2_threshold;

    std::string jsonStr;
    cJSON *root = NULL;

    const char *argsfile = nb_args > 1 ? argv[1] : "args.json";
    printf("argsfile: %s\n", argsfile);
    if (nb_args < 2)
        printf("json settings file not specified\n");

    std::ifstream file(argsfile);

    if (!file.is_open())
    {
        std::cerr << "Error opening file!" << std::endl;
        return 1;
    }
    jsonStr = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    root = cJSON_Parse(jsonStr.c_str());

    if (root == nullptr)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != nullptr)
            std::cerr << "Error before: " << error_ptr << std::endl;

        cJSON_Delete(root);
        return 1;
    }

    esp32_ip_address = cJSON_GetObjectItem(root, "esp32_ip_address")->valuestring;
    id = cJSON_GetObjectItem(root, "id")->valueint;
    width_resolution = cJSON_GetObjectItem(root, "width_resolution")->valueint;
    height_resolution = cJSON_GetObjectItem(root, "height_resolution")->valueint;
    refresh_every_x_frames = cJSON_GetObjectItem(root, "refresh_every_x_frames")->valueint;
    // framebuffer_cycles = cJSON_GetObjectItem(root, "framebuffer_cycles")->valueint;
    // rmt_high_time_s = cJSON_GetObjectItem(root, "rmt_high_time")->valuestring;
    enable_skipping = cJSON_GetObjectItem(root, "enable_skipping")->valueint;
    epd_skip_threshold = cJSON_GetObjectItem(root, "epd_skip_threshold")->valueint;
    esp32_multithread = cJSON_GetObjectItem(root, "esp32_multithread")->valueint;
    // framebuffer_cycles_2 = cJSON_GetObjectItem(root, "framebuffer_cycles_2")->valueint;
    // framebuffer_cycles_2_threshold = cJSON_GetObjectItem(root, "framebuffer_cycles_2_threshold")->valueint;
    mode = cJSON_GetObjectItem(root, "mode")->valueint;
    selective_compression = cJSON_GetObjectItem(root, "selective_compression")->valueint;
   // nb_chunks = cJSON_GetObjectItem(root, "nb_chunks")->valueint;
    start_nb_draws = cJSON_GetObjectItem(root, "nb_draws")->valueint;
    // draw_white_first = cJSON_GetObjectItem(root, "draw_white_first")->valueint;
    with_cv2 = cJSON_GetObjectItem(root, "with_cv2")->valueint;
    do_full_refresh = cJSON_GetObjectItem(root, "do_full_refresh")->valueint;
    disable_logging = cJSON_GetObjectItem(root, "disable_logging")->valueint;
    wifi_on = cJSON_GetObjectItem(root, "wifi_on")->valueint;
    source_image_bit_depth = cJSON_GetObjectItem(root, "pipe_bit_depth")->valueint;

    jsonStr = std::string(cJSON_PrintUnformatted(root));
    std::cout << "JSON Object:\n"
              << jsonStr << std::endl;

    int16_t settings_size[1];
    settings_size[0] = jsonStr.size(); //(nb_args - 7 - 1) * 2;

    //  int16_t *esp32_settings = (int16_t *)calloc(settings_size[0], sizeof(uint8_t));

    //  printf("settings size %d \n", settings_size[0]);
    //   for (int i = 1; i < argc; i++)
    //   {
    //     char * cur = argv[i];
    //       cJSON *root = cJSON_Parse(argv[i]);
    //       cJSON *name = cJSON_GetObjectItem(root, "esp32_ip_address");
    //     //printf("Name: %s\n", name->valuestring);
    //       iterateJson(root);
    //       cJSON_Delete(root);
    // esp32_settings[6] = std::stoi(argv[12]);  // framebuffer_cycles_2_threshold
    // esp32_settings[7] = std::stoi(argv[17]);  // mode
    // esp32_settings[8] = std::stoi(argv[14]);  // selective_compression
    // esp32_settings[9] = std::stoi(argv[15]);  // nb_chunks
    // esp32_settings[10] = std::stoi(argv[16]); // nb_draws
    // draw_white_first = esp32_settings[7];
    // mode = std::stoi(argv[18]);
    // with_cv2 = std::stoi(argv[19]);
    // do_full_refresh = std::stoi(argv[20]);
    // disable_logging = std::stoi(argv[nb_args - 2]);
    // wifi_on = std::stoi(argv[nb_args - 1]);

    printf("esp32_ip_address: %s\n", esp32_ip_address);
    printf("display id: %d\n", id);
    printf("refresh_every_x_frames: %d\n", refresh_every_x_frames);
    printf("do_full_refresh: %d\n", do_full_refresh);

    // printf("framebuffer_cycles: %d\n", framebuffer_cycles);
    // printf("rmt_high_time: %d\n", esp32_settings[1]);
    printf("enable_skipping: %d\n", enable_skipping);
    printf("epd_skip_threshold: %d\n", epd_skip_threshold);
    printf("esp32_multithread: %d\n", esp32_multithread);

    // printf("framebuffer_cycles_2: %d\n", framebuffer_cycles_2);
    // printf("framebuffer_cycles_2_threshold: %d\n", framebuffer_cycles_2_threshold);
    //  printf("pseudo_greyscale_mode: %d\n", esp32_settings[7]);
    printf("selective_compression: %d\n", selective_compression);
   // printf("nb_chunks: %d\n", nb_chunks);
    printf("start_nb_draws: %d\n", start_nb_draws);
    //printf("draw_white_first: %d\n",draw_white_first);
    printf("mode: %d\n", mode);
    printf("with_cv2: %d\n", with_cv2);
    printf("wifi_on: %d\n", wifi_on);
    printf("source_image_bit_depth: %d\n", source_image_bit_depth);


#ifdef WITHOPENCV
    if (with_cv2 == withCv2Enum::BOTH || with_cv2 == withCv2Enum::CPP){

        whiteImage = new cv::Mat(height_resolution,width_resolution, CV_8UC1, cv::Scalar(0));
        // CV_8UC3, cv::Scalar(255, 255, 255));
        // cv::Mat image = cv::Mat::zeros(400, 400, CV_8UC3);
        // cv::Point topLeft(100, 100);
        // cv::Point bottomRight(300, 300);
        // cv::rectangle(image, topLeft, bottomRight, cv::Scalar(0, 255, 0), -1); // -1 fills the rectangle
        // cv::imshow("Green Square", image);
        // cv::waitKey(0);
    }
#endif

    // if (nb_draws > framebuffer_cycles)
    //     nb_rmt_times = nb_draws;
    // else
    //     nb_rmt_times = framebuffer_cycles;
    // nb_rmt_times = nb_rmt_times < 2 ? 2: nb_rmt_times; // we need at least 2 rmt times, one for framebuffer_cycles_1 and framebuffer_cycles_2
    //_size = 6 + nb_rmt_times * 2;
    // esp32_settings[11] = per_frame_wifi_settings_size;
    //cJSON_AddNumberToObject(root, "per_frame_wifi_settings_size", per_frame_wifi_settings_size);

    // draw_rmt_times = (uint16_t *)calloc(nb_rmt_times, sizeof(uint16_t));
    // rmt_high_time_s = strtok(rmt_high_time_s, ":");
    // int n = 0;
    // while (rmt_high_time_s != NULL)
    // {
    //     draw_rmt_times[n] = std::stoi(rmt_high_time_s);
    //     rmt_high_time_s = strtok(NULL, ":");
    //     n++;
    // }
    // while (n < nb_rmt_times)
    // {
    //     draw_rmt_times[n] = draw_rmt_times[0];
    //     n++;
    // }
    // printf("draw_rmt_times: ");
    // for (int x = 0; x < nb_rmt_times; x++)
    //     printf(" %d ", draw_rmt_times[x]);
    // printf(" \n");

    if (disable_logging == 1)
        printf("logging disabled \n");
    if (wifi_on == 1)
        printf("wifi enabled \n");
    else
        printf("wifi disabled \n");

#ifdef __linux__
    char *dir2 = "/pc_host_app/";

    sprintf(input_pipe, "%s%d", "/tmp/epdiy_pc_monitor_a_", id);
    sprintf(output_pipe, "%s%d", "/tmp/epdiy_pc_monitor_b_", id);
    // printf("input pipe : %s, output pipe: %s\n", input_pipe, output_pipe);

    mkfifo(input_pipe, 0666);
    mkfifo(output_pipe, 0666);

#elif _WIN32
    char *dir2 = "\\pc_host_app\\";
    sprintf(input_pipe, "%s%d", "\\\\.\\pipe\\epdiy_pc_monitor_a_", id);
    sprintf(output_pipe, "%s%d", "\\\\.\\pipe\\epdiy_pc_monitor_b_", id);

    char buffer[1024];

    fd1 = CreateNamedPipe(TEXT(output_pipe),
                          PIPE_ACCESS_DUPLEX,
                          PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, // FILE_FLAG_FIRST_PIPE_INSTANCE is not needed but forces CreateNamedPipe(..) to fail if the pipe already exists...
                          1, 1024 * 64, 1024 * 64, NMPWAIT_USE_DEFAULT_WAIT, NULL);

    if (bool ret = ConnectNamedPipe(fd1, NULL) != FALSE)
        printf("pipe fd1 is ok\n");
    else
        printf("Pipe  fd1 not ok \n");
    while (true)
    {
        fd0 = CreateFile(TEXT(input_pipe), PIPE_ACCESS_DUPLEX, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (fd0 == INVALID_HANDLE_VALUE)
        {
            printf("invalid handle fd0, error %d, trying again \n", GetLastError());
            sleep(1);
        }
        else
        {
            printf("pipe fd0 is ok now\n");
            break;
        }
    }

    /*     BOOL fSuccess = FALSE;

    DWORD cbRead, cbToWrite, cbWritten, dwMode;
    dwMode = PIPE_READMODE_MESSAGE;

    fSuccess = SetNamedPipeHandleState(fd0, &dwMode, NULL, NULL);
    if (!fSuccess)
        printf(TEXT("SetNamedPipeHandleState failed. GLE=%d\n"), GetLastError()); */
    WSADATA wsa;
    printf("\nInitialising Winsock...");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("Failed. Error Code : %d", WSAGetLastError());
        return 1;
    }
    printf("WSAD Initialised.\n");

#endif

    //Create socket
    struct sockaddr_in server;
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket\n");
    }
#ifdef _WIN32
    int err = WSAGetLastError();
    if (!ISVALIDSOCKET(socket_desc))
    {
        fprintf(stderr, "socket() failed! %d\n", WSAGetLastError());
    }
#endif
    getcwd(working_dir, sizeof(working_dir));

    sprintf(working_dir, "%s%s", working_dir, dir2);

    DIR *dir = opendir(working_dir);
    if (dir)
    {
        cd(working_dir);
        closedir(dir);
    }
    getcwd(working_dir, sizeof(working_dir));
    printf("current working directory is: %s\n", working_dir);

    // int yes = 0; // 1 - on, 0 - off
    // int result = setsockopt(socket_desc,
    //                         IPPROTO_TCP,
    //                         TCP_NODELAY,
    //                         (char *)&yes,
    //                         sizeof(int));
    // if (result < 0)
    //     printf("error setting socket options\n");

    // Set TCP_NODELAY option
    BOOL flag = TRUE;
    if (setsockopt(socket_desc, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(BOOL)) == SOCKET_ERROR) {
        std::cerr << "Error setting TCP_NODELAY: " << WSAGetLastError() << "\n";
        closesocket(socket_desc);
        WSACleanup();
        return 1;
    }


    server.sin_addr.s_addr = inet_addr(esp32_ip_address);
    server.sin_family = AF_INET;
    server.sin_port = htons(3333);
    // Connect to remote server

    if (wifi_on == 1)
    {
        const char* transfer_uuid1 = "8fPMGCramH2aqRY2v5CGqY";
        const char* transfer_uuid2 = "gizUD6hB2kxJEewtbB4MvU";
        //char transfer_message[48];
        memset(transfer_message, 0, UUID_SIZE*2 );
        memcpy(transfer_message, transfer_uuid1, UUID_SIZE);
        memcpy(transfer_message+UUID_SIZE+4, transfer_uuid2, UUID_SIZE);

        int timeout = 20000; // Timeout in milliseconds
        setsockopt(socket_desc, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
        setsockopt(socket_desc, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));

        if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
        {
            puts("PC-host application Error: \n Error connecting to esp, wrong ESP32 ip address or wifi network?\n");
            return 1;
        }
        puts("Connected to esp wifi\n");
        send(socket_desc, (char *)settings_size, 2, 0);
        char esp32_settings_char[settings_size[0]];
        memcpy(esp32_settings_char, jsonStr.c_str(), settings_size[0]);
        send(socket_desc, esp32_settings_char, settings_size[0], 0);
    }

    array_with_zeros = (unsigned char *)calloc(129, sizeof(unsigned char));
    draw_black_bytes = (unsigned char *)calloc(129, sizeof(unsigned char));
    draw_white_bytes = (unsigned char *)calloc(129, sizeof(unsigned char));
    memset(draw_black_bytes, 85, 129);
    memset(draw_white_bytes, 170, 129);
    if (mirroring_task() == -1)
    { // Start the mirroring process
        printf("mirroring_task id %d returned -1 \n", id);
        closesocket(socket_desc);
    }
    cJSON_Delete(root);
}
