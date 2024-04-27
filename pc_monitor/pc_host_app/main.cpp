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

char *compressed_eink_framebuffer_ptrs[16]; //array of pointers pointing to chunks of framebuffer
int id, refresh_every_x_frames = 0, refresh_every_x_frames_, selective_compression;
int total_nb_pixels, eink_framebuffer_size,  nb_rmt_times;
int source_image_bit_depth = 1, mode = -1, esp32_multithread;
int with_cv2 = 0;
bool disable_logging;
int mouse_moved = 0;
int do_full_refresh = 1;

char ready0[6];
char ready1[6];

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


    
    if (compressed_chunk_lengths[0] > eink_framebuffer_size / 100 * selective_compression && selective_compression != 0)
    {
        framebuffer_to_send[0] = eink_framebuffer_swapped + (eink_framebuffer_size * 0); //*g
        framebuffer_to_send_size = eink_framebuffer_size;
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
    rle_compress(padded_2bpp_framebuffer_current, tmp_array, 1, compressed_eink_framebuffer, eink_framebuffer_size, eink_framebuffer_size);
    for (int g = 0; g < 1 * 4; g += 4)
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
    rle_compress(padded_2bpp_framebuffer_current, tmp_array, 1, compressed_eink_framebuffer, eink_framebuffer_size, eink_framebuffer_size);
    if (wifi_on)
    {
      //  wifi_transfer(compressed_eink_framebuffer, 0);
        recv(socket_desc, ready0, 6, 0);
    }
}

void print_eink_framebuffer_sizes()
{
    for (int h = 0; h < 1; h++) // for debugging
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

    int white_pixel;
    int first_time = 1;
    total_nb_pixels = width_resolution * height_resolution;
    eink_framebuffer_size = total_nb_pixels / 4;

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

    compressed_eink_framebuffer_ptrs[0] = (char *)calloc(eink_framebuffer_size * 2, sizeof(char));
    //added_compression_arr[h] = (uint16_t *)calloc(eink_framebuffer_size * 2, sizeof(char));

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

     white_pixel =  255;// source_image_bit_depth == 8  && mode == FourShadesGrayscale ? 255 : 1; //start_nb_draws 
    
    memset(source_8bpp_current, white_pixel, total_nb_pixels * sizeof(unsigned char));
    memset(source_8bpp_modified_current, white_pixel, total_nb_pixels * sizeof(unsigned char));
    memset(source_8bpp_previous, white_pixel, total_nb_pixels * sizeof(unsigned char));

    int tot_lines_changed[] = {1}, repeat_counter = 0, next = 0;
    

    printf("C++ ID %d mirroring started \n", id);

    while (1)
    {
        ret2 = pipe_write(fd1, ack, 1, ret2); //before or after rect?


        if (tot_lines_changed[0] != 0 && wifi_on == 1) // if screen didn't change don't wait for ack from board
            recv(socket_desc, ready0, 6, 0);


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

        int signal = getIntValueFromObject(root_, "signal");
        mouse_moved = getIntValueFromObject(root_, "mouse_moved");
        mode = getIntValueFromObject(root_, "mode");
        do_full_refresh = getIntValueFromObject(root_, "do_full_refresh");
        std::string notes(getStringValueFromObject(root_, "notes"));
        int rotation = getIntValueFromObject(root_, "rotation");
        source_image_bit_depth = getIntValueFromObject(root_, "pipe_bit_depth");

        cJSON *draws_conf = checkGetJsonOject(root_, "draws_conf");
        cJSON *draw_list = checkGetJsonOject(draws_conf, "draw_list");
        
        int nb_draws = cJSON_GetArraySize(draw_list);
        cJSON_AddNumberToObject(draws_conf, "nb_draws", nb_draws);

        if (nb_draws > preallocated_eink_framebuffer_n)
            for (int i = preallocated_eink_framebuffer_n; i < nb_draws; i++)
                eink_framebuffer[i] = (char *)calloc(eink_framebuffer_size, sizeof(char));

        std::vector< draw_conf>  draws_conf_array;
        for (int i = 0; i < nb_draws; i++)
        {
            draw_conf conf;

            cJSON *element = cJSON_GetArrayItem(draw_list, i);
            assert(element != nullptr);
            conf.json_element = element;
            conf.type = getStringValueFromObject(element, "type");
            cJSON *rmt_high_times = checkGetJsonOject(element, "rmt_high_times");
            conf.rmt_high_times_n = cJSON_GetArraySize(rmt_high_times);
         //   printf("draw %d  | type %s | rmt_high_times_n %d || ", i, conf.type, conf.rmt_high_times_n);
            cJSON_AddNumberToObject(element, "rmt_high_times_n", conf.rmt_high_times_n);
            for (int i = 0; i < conf.rmt_high_times_n; i++)
            {
                conf.rmt_high_times[i] = getIntArrayItemFromObject(rmt_high_times, i);
            //    printf(" %d ", conf.rmt_high_times[i]);
            }
            conf.typeID =  draw_type_map.find(std::string(conf.type))->second;
            
            draws_conf_array.push_back(conf);
          //  printf("\n");
        }

        if (signal == 101) //ack2[0]
        {
            printf("C++ app ID %d exiting \n", id);
            close(socket_desc);
            exit(EXIT_SUCCESS);
        }

        char *eight_bpp_ptr = mode == FourShadesGrayscale ? source_8bpp_modified_current : source_8bpp_current;
        
        white_pixel = 255;//source_image_bit_depth == 8 && mode == FourShadesGrayscale ? 255 : 1;// draw_white_first && mode == FourShadesGrayscale ? 255 : 1;

        if (mode == FourShadesGrayscale || source_image_bit_depth == 8 )  // || nb_draws > 1
        {
          //  source_image_bit_depth = 8;
            refresh_every_x_frames_ = refresh_every_x_frames * nb_draws;
        }
        else
        {
            refresh_every_x_frames_ = refresh_every_x_frames;
         //   source_image_bit_depth = 1;
        }


        long t0 = getTick();


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

            if (ret2 != total_nb_pixels)
            {
                printf("c++ id %d warning ret2 8bpp \n", id);
                return -1;
            }
        }

        if (source_image_bit_depth == 1)
        {
            generate_eink_framebuffer_v1(source_1bpp, padded_2bpp_framebuffer_current, padded_2bpp_framebuffer_previous, eink_framebuffer[0]);

            optimize_rle(eink_framebuffer[0], eink_framebuffer_size);

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

        for (int g = 0; g < nb_draws; g++)
        {

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

            rle_compress(eink_framebuffer_swapped, tmp_array, 1, compressed_eink_framebuffer, eink_framebuffer_size, eink_framebuffer_size);

            // rle_compress_v2(eink_framebuffer_swapped, tmp_array, 1, added_compression_arr, eink_framebuffer_size);

            
                unsigned int number2 = htonl(compressed_chunk_lengths[0]);
                memcpy(compressed_chunk_lengths_in_bytes, &compressed_chunk_lengths[0], 4);
                // tot += foo;
            
            // rle_extract1(decompressed, 1, eink_framebuffer_swapped, eink_framebuffer_size, compressed_chunk_lengths[0]); //for testing

            if (refresh_every_x_frames_ && loop_counter[0] == refresh_every_x_frames_ || loop_counter[0] == refresh_every_x_frames_ + 1)
            {
                memset(line_changed[g], 1, height_resolution);
                tot_lines_changed[0] = height_resolution;
            }
            //  if (loop_counter[0] != refresh_every_x_frames_)

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


#ifdef WITHOPENCV  //for debuggin only
            if (with_cv2 == withCv2Enum::BOTH || with_cv2 == withCv2Enum::CPP)
            {
                unsigned char pixels[4];
                unsigned char *imageData = whiteImage->data;

                for (int n = 0; n < eink_framebuffer_size; n++)
                {
                    char c_ = eink_framebuffer[g][n];
                    unsigned char c = eink_framebuffer[g][n];
                    unpackByte(c, pixels);
                    for (int i = 0; i < 4; i++)
                    {
                        if (pixels[i] == 2)
                        {
                            int y = ((n*4)+i) / width_resolution;
                            int x = ((n*4)+i) % width_resolution; 
                            uchar &pixelValue = whiteImage->at<uchar>(y, x);
                             pixelValue = 0;
                            // imageData[((n*4)+i)] = 0;
                            // imageData[((n*4*3)+i*3)] = 0;
                            // imageData[((n*4*3)+i*3)+1] = 0;
                            // imageData[((n*4*3)+i*3)+2] = 0;
                        }
                        else if (pixels[i] == 1)
                        {
                            // imageData[(((n*4)) + i)] = 255;
                            int y = ((n*4)+i) / width_resolution;
                            int x = ((n*4)+i) % width_resolution; 
                            uchar &pixelValue = whiteImage->at<uchar>(y, x);
                              pixelValue = 255;
                            // imageData[((n*4*3)+i*3)] = 255;
                            // imageData[((n*4*3)+i*3)+1] = 255;
                            // imageData[((n*4*3)+i*3)+2] = 255;
                        }
                    }

                }
                    cv::Mat horizontal_flip;
                    cv::flip(*whiteImage, horizontal_flip, 1);
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



int main(int argc, char *argv[])
{
    int nb_args = argc;
    const char *esp32_ip_address = NULL;

    int enable_skipping;
    int epd_skip_threshold;
    int esp32_multithread;


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

    esp32_ip_address = getStringValueFromObject(root, "esp32_ip_address");
    id = getIntValueFromObject(root, "id");
    width_resolution = getIntValueFromObject(root, "width_resolution");
    height_resolution = getIntValueFromObject(root, "height_resolution");
    refresh_every_x_frames = getIntValueFromObject(root, "refresh_every_x_frames");
    enable_skipping = getIntValueFromObject(root, "enable_skipping");
    epd_skip_threshold = getIntValueFromObject(root, "epd_skip_threshold");
    esp32_multithread = getIntValueFromObject(root, "esp32_multithread");
    mode = getIntValueFromObject(root, "mode");
    selective_compression = getIntValueFromObject(root, "selective_compression");
    start_nb_draws = getIntValueFromObject(root, "nb_draws");
    with_cv2 = getIntValueFromObject(root, "with_cv2");
    do_full_refresh = getIntValueFromObject(root, "do_full_refresh");
    disable_logging = getIntValueFromObject(root, "disable_logging");
    wifi_on = getIntValueFromObject(root, "wifi_on");
    source_image_bit_depth = getIntValueFromObject(root, "pipe_bit_depth");

    jsonStr = std::string(cJSON_PrintUnformatted(root));
    std::cout << "JSON Object:\n"
              << jsonStr << std::endl;

    int16_t settings_size[1];
    settings_size[0] = jsonStr.size();

    printf("esp32_ip_address: %s\n", esp32_ip_address);
    printf("display id: %d\n", id);
    printf("refresh_every_x_frames: %d\n", refresh_every_x_frames);
    printf("do_full_refresh: %d\n", do_full_refresh);
    printf("enable_skipping: %d\n", enable_skipping);
    printf("epd_skip_threshold: %d\n", epd_skip_threshold);
    printf("esp32_multithread: %d\n", esp32_multithread);

    printf("selective_compression: %d\n", selective_compression);
    printf("start_nb_draws: %d\n", start_nb_draws);
    printf("mode: %d\n", mode);
    printf("with_cv2: %d\n", with_cv2);
    printf("wifi_on: %d\n", wifi_on);
    printf("source_image_bit_depth: %d\n", source_image_bit_depth);


#ifdef WITHOPENCV //for debugging only
    if (with_cv2 == withCv2Enum::BOTH || with_cv2 == withCv2Enum::CPP){
        whiteImage = new cv::Mat(height_resolution,width_resolution, CV_8UC1, cv::Scalar(0));
    }
#endif


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
    const char *dir2 = "\\pc_host_app\\";
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
