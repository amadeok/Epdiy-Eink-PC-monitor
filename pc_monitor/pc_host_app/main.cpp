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

char *line_changed; //array containing 1 or 0 depending on whether the corresponding line on the screen has changed

SOCKET socket_desc;
#if defined(_WIN32)
#define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
#else
#define ISVALIDSOCKET(s) ((s) >= 0)
#endif

int compressed_chunk_lengths[16];

int wifi_on = 1;
char *ready2;

char *decompressed;                   // for testing or debugging
unsigned char *decompressed_received; // for testing or debugging
unsigned char *compressed_received;   // for testing or debugging
unsigned char *received;              // for testing or debugging
char *tmp_array;

char *compressed_eink_framebuffer_ptrs[16]; //array of pointers pointing to chunks of framebuffer
int id, refresh_every_x_frames = 0, refresh_every_x_frames_, selective_compression;

int total_nb_pixels, eink_framebuffer_size, chunk_size, nb_chunks, nb_draws, nb_rmt_times;
int source_image_bit_depth = 1, mode = -1, draw_white_first, esp32_multithread;
int with_cv2 = 0;
bool disable_logging;
int mouse_moved = 0;
int do_full_refresh = 1;
unsigned char full_refresh_delay = 30;
int per_frame_wifi_settings_size;
char ready0[6];
char ready1[6];
char *per_frame_wifi_settings;
uint16_t *draw_rmt_times;
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

void wifi_transfer(char *eink_framebuffer_swapped, int eink_framebuffer_size)
{
    DWORD ret2;
    char *framebuffer_to_send[16];
    int framebuffer_to_send_size;

    int ret = 0, buf_size = 4096, tot = 0, tot2 = 0, len = 0;

    if (mouse_moved == 1)
        per_frame_wifi_settings[0] = 'm';
    else
        per_frame_wifi_settings[0] = 0;

    per_frame_wifi_settings[1] = mode;

    if (refresh_every_x_frames_ && loop_counter[0] == refresh_every_x_frames_ && do_full_refresh != 0)
    {
        loop_counter[0] = 0;
        per_frame_wifi_settings[2] = full_refresh_delay;
    }
    else
        per_frame_wifi_settings[2] = 0;

    memcpy(per_frame_wifi_settings + 6, draw_rmt_times, nb_rmt_times * 2);
    // for (int x = 0; x < nb_rmt_times; x++)
    //     printf(" %d ", draw_rmt_times[x]);
    // printf("\n wifi ");

    send(socket_desc, per_frame_wifi_settings, per_frame_wifi_settings_size, 0);
    int ret0 = recv(socket_desc, ready2, per_frame_wifi_settings_size, 0);

    for (int k = 0; k < per_frame_wifi_settings_size; k++)
    {
        if (ready2[k] != per_frame_wifi_settings[k])
        {
            printf("%d \n", ready2[k]);
            printf("warning ready2 per_frame_wifi_settings dif \n");
            //sleep(100000);
        }
    }

    if (refresh_every_x_frames_&& loop_counter[0] == refresh_every_x_frames_ + 1)
        loop_counter[0] = 0;
    ret2 = send(socket_desc, compressed_chunk_lengths_in_bytes, nb_chunks * 4 * sizeof(unsigned char), 0);

    ret2 = send(socket_desc, line_changed, (height_resolution + 2) * sizeof(unsigned char), 0);

    //   array_to_file(eink_framebuffer_swapped, eink_framebuffer_size, working_dir, "eink_framebuffer_swapped", 0);
    unsigned char ready[5];
    int g, a = 0;
    for (int h = 0; h < nb_chunks; h++)
    {
        if (compressed_chunk_lengths[h] > chunk_size / 100 * selective_compression)
            a++;
    }
    //  if (a == nb_chunks)
    //     printf("a = nb_chunms\n");
    for (g = 0; g < nb_chunks; g++)
    {
        if (compressed_chunk_lengths[g] > chunk_size / 100 * selective_compression && selective_compression != 0)
        {
            framebuffer_to_send[g] = eink_framebuffer_swapped + (chunk_size * g);
            framebuffer_to_send_size = chunk_size;
        }
        else
        {
            framebuffer_to_send[g] = compressed_eink_framebuffer_ptrs[g];
            framebuffer_to_send_size = compressed_chunk_lengths[g];
        }
        buf_size = 4096 * 5;
        int len2;
        tot = 0;
        ret = 0;
        int converted_number = 0;
        if (framebuffer_to_send_size < buf_size)
            buf_size = framebuffer_to_send_size;
        do
        {
            ret = send(socket_desc, framebuffer_to_send[g] + tot, buf_size, 0);
            tot += ret;
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
    }

    //puts("Data Send to esp\n");
    //   array_to_file(received, eink_framebuffer_size, working_dir, "received", 0);
}

void send_refresh_framebuffers(char *padded_2bpp_framebuffer_current, char *compressed_eink_framebuffer)
{
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
        wifi_transfer(compressed_eink_framebuffer, 0);
        recv(socket_desc, ready0, 6, 0);
    }
    memset(padded_2bpp_framebuffer_current, 170, eink_framebuffer_size);
    rle_compress(padded_2bpp_framebuffer_current, tmp_array, nb_chunks, compressed_eink_framebuffer, eink_framebuffer_size, chunk_size);
    if (wifi_on)
    {
        wifi_transfer(compressed_eink_framebuffer, 0);
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
    per_frame_wifi_settings = (char *)calloc(per_frame_wifi_settings_size, sizeof(char));

    line_changed = (char *)calloc(height_resolution + 2, sizeof(char));
    source_1bpp = (unsigned char *)calloc(total_nb_pixels, sizeof(unsigned char));
    tmp_array = (char *)calloc(eink_framebuffer_size + 50000, sizeof(unsigned char));

    ready2 = (char *)calloc(per_frame_wifi_settings_size, sizeof(char));
    ack2 = (unsigned char *)calloc(3 + (nb_rmt_times * 2), sizeof(char));
    memset(ready2, 0, per_frame_wifi_settings_size);

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
    for (int h = 0; h < nb_draws; h++)
        eink_framebuffer[h] = (char *)calloc(eink_framebuffer_size, sizeof(char));

    if (source_image_bit_depth == 1)
    { //assume the first screen capture to be completely white
        memset(padded_2bpp_framebuffer_previous, 85, eink_framebuffer_size * sizeof(unsigned char));
        memset(padded_2bpp_framebuffer_current, 85, eink_framebuffer_size * sizeof(unsigned char));
    }
    //else if (source_image_bit_depth == 8)
    //{ //assume the first screen capture to be completely white

    //  }
    else
    {
        printf("unsupported bit depth\n");
        return -1;
    }
    if (draw_white_first && mode == 10)
        white_pixel = 255;
    else
        white_pixel = 1;
    memset(source_8bpp_current, white_pixel, total_nb_pixels * sizeof(unsigned char));
    memset(source_8bpp_modified_current, white_pixel, total_nb_pixels * sizeof(unsigned char));
    memset(source_8bpp_previous, white_pixel, total_nb_pixels * sizeof(unsigned char));

    int total[] = {1}, repeat_counter = 0, next = 0;
    char *eight_bpp_ptr;

    printf("C++ ID %d mirroring started \n", id);

    while (1)
    {
        if (total[0] != 0 && wifi_on == 1) // if screen didn't change don't wait for ack from board
            recv(socket_desc, ready0, 6, 0);

        ret2 = pipe_write(fd1, ack, 1, ret2);
        int r_size = 3 + nb_rmt_times * sizeof(uint16_t); 
        ret2 = pipe_read(fd0, ack2, r_size, ret2);
        if (ret2 != r_size){
            printf("c++ id %d warning ret2 nb_rmt_times \n", id);
            return -1;
        }
        memcpy(draw_rmt_times, ack2 + 3, nb_rmt_times * sizeof(uint16_t));
        mode = ack2[2];
        if (mode == 10 || draw_white_first)
            refresh_every_x_frames_ = refresh_every_x_frames * nb_draws;
        else
            refresh_every_x_frames_ = refresh_every_x_frames;
        if (ack2[0] == 101)
        {
            printf("C++ app ID %d exiting \n", id);
            close(socket_desc);
            exit(EXIT_SUCCESS);
        }
        if (ack2[1] == 1)
        {
            mouse_moved = 1;
            // printf("moved\n");
        }
        else
        {
            mouse_moved = 0;
            //   printf("not moved\n");
        }

        if (ack2[2] == 10 || draw_white_first == 1)
        {
            if (mode == 10)
                eight_bpp_ptr = source_8bpp_modified_current;
            else
                eight_bpp_ptr = source_8bpp_current;

            if (draw_white_first && mode == 10)
                white_pixel = 255;
            else
                white_pixel = 1;
            source_image_bit_depth = 8;
        }
        else
        {
            source_image_bit_depth = 1;
            nb_draws = 1;
        }
        // for (int a = 0; a < nb_rmt_times; a++)
        //     printf("%d ", draw_rmt_times[a]);
        // printf("%d \n", loop_counter[0]);

        long t0 = getTick();
        ret2 = pipe_read(fd0, line_changed, height_resolution, ret2);
        if (ret2 != height_resolution){
            printf("c++ id %d warning ret2 line_changed \n", id);
            return -1;
        }

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
            case 1:
                generate_eink_framebuffer_v2(source_8bpp_current, source_8bpp_previous, source_8bpp_modified_previous, eink_framebuffer, ack2[2]);
            default:
                //  int t0 = getTick();
                if (mode == 10)
                    quantize(source_8bpp_current, source_8bpp_modified_current, total_nb_pixels);

                //array_to_file(source_8bpp_previous, total_nb_pixels, working_dir, "source_8bpp_previous", 0);
                // system("python3 /home/amadeok/epdiy-working/examples/pc_monitor/pc_host_app/img_test.py source_8bpp_previous0");
                // array_to_file(eight_bpp_ptr, total_nb_pixels, working_dir, "eight_bpp_ptr", 0);
                // system("python3 /home/amadeok/epdiy-working/examples/pc_monitor/pc_host_app/img_test.py eight_bpp_ptr0");

                // printf("%d \n", getTick() - t0);
                generate_eink_framebuffer_v2(eight_bpp_ptr, source_8bpp_previous, source_8bpp_modified_previous, eink_framebuffer, ack2[2]);

                //  array_to_file(source_8bpp_modified_current, total_nb_pixels, working_dir, "source_8bpp_modified_current", 0);
            }
        }
        for (int g = 0; g < nb_draws; g++)
        {
            if (g != 0)
            {
                if (total[0] != 0 && wifi_on == 1) // if screen didn't change don't wait for ack from board
                    recv(socket_desc, ready0, 6, 0);
            }

            swap_bytes(eink_framebuffer[g], eink_framebuffer_swapped, eink_framebuffer_size, source_image_bit_depth);

            //array_to_file(eink_framebuffer_swapped, eink_framebuffer_size, working_dir, "eink_fb_sw", 0);

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

            total[0] = 0;
            uint16_t total2[1];

            if (refresh_every_x_frames_ && loop_counter[0] == refresh_every_x_frames_ || loop_counter[0] == refresh_every_x_frames_ + 1)
            {
                memset(line_changed, 1, height_resolution);
                total[0] = height_resolution;
            }
            //  if (loop_counter[0] != refresh_every_x_frames_)
            else
            {
                for (int h = 0; h < height_resolution; h++)
                    total[0] += line_changed[h];
                memcpy(line_changed + height_resolution, total, 2);
                memcpy(total2, line_changed + height_resolution, 2);
            }

            if (total[0] != 0 && wifi_on == 1 && FT245MODE == 0) //send framebuffer only if current capture is different than previous
            {
                wifi_transfer(eink_framebuffer_swapped, eink_framebuffer_size);
                repeat_counter = 0;
                // printf("loop_counter %d\n", loop_counter[0]);
                if (total[0] > 85) // don't increase the counter to clear te display if only 85 lines have changed
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
                    char c_ = eink_framebuffer[0][n];
                    unsigned char c = eink_framebuffer[0][n];
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
    }

    return 0;
}
 #include <cJSON.h>


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
    int framebuffer_cycles;
    char *rmt_high_time_s;
    int enable_skipping;
    int epd_skip_threshold;
    int esp32_multithread;
    int framebuffer_cycles_2;
    int framebuffer_cycles_2_threshold;

    std::string jsonStr;
    cJSON *root = NULL;
    char* argsfile = argv[1];
    printf("argsfile: %s\n", argsfile);
    if (nb_args == 2)
    {
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
            {
                std::cerr << "Error before: " << error_ptr << std::endl;
            }
            cJSON_Delete(root);
            return 1;
        }
        esp32_ip_address = cJSON_GetObjectItemCaseSensitive(root, "esp32_ip_address")->valuestring;
        id = cJSON_GetObjectItemCaseSensitive(root, "id")->valueint;
        width_resolution = cJSON_GetObjectItemCaseSensitive(root, "width_resolution")->valueint;
        height_resolution = cJSON_GetObjectItemCaseSensitive(root, "height_resolution")->valueint;
        refresh_every_x_frames = cJSON_GetObjectItemCaseSensitive(root, "refresh_every_x_frames")->valueint;
        framebuffer_cycles = cJSON_GetObjectItemCaseSensitive(root, "framebuffer_cycles")->valueint;
        rmt_high_time_s = cJSON_GetObjectItemCaseSensitive(root, "rmt_high_time")->valuestring;
        enable_skipping = cJSON_GetObjectItemCaseSensitive(root, "enable_skipping")->valueint;
        epd_skip_threshold = cJSON_GetObjectItemCaseSensitive(root, "epd_skip_threshold")->valueint;
        esp32_multithread = cJSON_GetObjectItemCaseSensitive(root, "esp32_multithread")->valueint;
        framebuffer_cycles_2 = cJSON_GetObjectItemCaseSensitive(root, "framebuffer_cycles_2")->valueint;
        framebuffer_cycles_2_threshold = cJSON_GetObjectItemCaseSensitive(root, "framebuffer_cycles_2_threshold")->valueint;
        mode = cJSON_GetObjectItemCaseSensitive(root, "mode")->valueint;
        selective_compression = cJSON_GetObjectItemCaseSensitive(root, "selective_compression")->valueint;
        nb_chunks = cJSON_GetObjectItemCaseSensitive(root, "nb_chunks")->valueint;
        nb_draws = cJSON_GetObjectItemCaseSensitive(root, "nb_draws")->valueint;
        draw_white_first = cJSON_GetObjectItemCaseSensitive(root, "draw_white_first")->valueint;
        with_cv2 = cJSON_GetObjectItemCaseSensitive(root, "with_cv2")->valueint;
        do_full_refresh = cJSON_GetObjectItemCaseSensitive(root, "do_full_refresh")->valueint;
        disable_logging = cJSON_GetObjectItemCaseSensitive(root, "disable_logging")->valueint;
        wifi_on = cJSON_GetObjectItemCaseSensitive(root, "wifi_on")->valueint;

    }else printf("json settings file not specified\n");
    int16_t settings_size[1];
    settings_size[0] = jsonStr.size(); //(nb_args - 7 - 1) * 2;
    int16_t *esp32_settings = (int16_t *)calloc(settings_size[0], sizeof(uint8_t));

    //  printf("settings size %d \n", settings_size[0]);
    //   for (int i = 1; i < argc; i++)
    //   {
    //     char * cur = argv[i];
    //       cJSON *root = cJSON_Parse(argv[i]);
    //       cJSON *name = cJSON_GetObjectItem(root, "esp32_ip_address");
    //     //printf("Name: %s\n", name->valuestring);

    //       iterateJson(root);
    //       cJSON_Delete(root);

<<<<<<< Updated upstream
    esp32_settings[0] = std::stoi(argv[6]);
    char *rmt_high_time_s = argv[7];
    esp32_settings[2] = std::stoi(argv[8]);
    esp32_settings[3] = std::stoi(argv[9]);
    esp32_settings[4] = std::stoi(argv[10]);
    esp32_settings[5] = std::stoi(argv[11]);
    esp32_settings[6] = std::stoi(argv[12]);
    esp32_settings[7] = std::stoi(argv[13]);
    esp32_settings[8] = std::stoi(argv[14]);
    esp32_settings[9] = std::stoi(argv[15]);
    esp32_settings[10] = std::stoi(argv[16]);
    draw_white_first = std::stoi(argv[17]);
    mode = std::stoi(argv[18]);
=======
    //       printf(" %d args %s |", i, argv[i]);
    //   }
    // printf("\n");
    // //   char *esp32_ip_address = argv[1];
    // char *display_id = argv[2];
    // id = std::stoi(argv[2]);
    // width_resolution = std::stoi(argv[3]);
    // height_resolution = std::stoi(argv[4]);
    // refresh_every_x_frames = std::stoi(argv[5]);
    // esp32_settings[0] = std::stoi(argv[6]); // framebuffer_cycles
    // // char *rmt_high_time_s = argv[7];
    // esp32_settings[2] = std::stoi(argv[8]);  // enable_skipping
    // esp32_settings[3] = std::stoi(argv[9]);  // epd_skip_threshold
    // esp32_settings[4] = std::stoi(argv[10]); // esp32_multithread
    // esp32_settings[5] = std::stoi(argv[11]); // framebuffer_cycles_2
>>>>>>> Stashed changes

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

    printf("framebuffer_cycles: %d\n", framebuffer_cycles);
    // printf("rmt_high_time: %d\n", esp32_settings[1]);
    printf("enable_skipping: %d\n", enable_skipping);
    printf("epd_skip_threshold: %d\n", epd_skip_threshold);
    printf("esp32_multithread: %d\n", esp32_multithread);

    printf("framebuffer_cycles_2: %d\n", framebuffer_cycles_2);
    printf("framebuffer_cycles_2_threshold: %d\n", framebuffer_cycles_2_threshold);
    //  printf("pseudo_greyscale_mode: %d\n", esp32_settings[7]);
<<<<<<< Updated upstream
    printf("selective_compression: %d\n", selective_compression = esp32_settings[8]);
    printf("nb_chunks: %d\n", nb_chunks = esp32_settings[9]);
    printf("nb_draws: %d\n", nb_draws = esp32_settings[10]);
    printf("draw_white_first: %d\n", draw_white_first);
=======
    printf("selective_compression: %d\n", selective_compression);
    printf("nb_chunks: %d\n", nb_chunks);
    printf("nb_draws: %d\n", nb_draws);
    printf("draw_white_first: %d\n",draw_white_first);
>>>>>>> Stashed changes
    printf("mode: %d\n", mode);
    printf("with_cv2: %d\n", with_cv2);
    printf("wifi_on: %d\n", wifi_on);


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

    if (nb_draws > esp32_settings[0])
        nb_rmt_times = nb_draws;
    else
        nb_rmt_times = esp32_settings[0];
    nb_rmt_times = nb_rmt_times < 2 ? 2: nb_rmt_times; // we need at least 2 rmt times, one for framebuffer_cycles_1 and framebuffer_cycles_2

    per_frame_wifi_settings_size = 6 + nb_rmt_times * 2;
    // esp32_settings[11] = per_frame_wifi_settings_size;
    cJSON_AddNumberToObject(root, "per_frame_wifi_settings_size", per_frame_wifi_settings_size);
    jsonStr = std::string(cJSON_Print(root));
    std::cout << "JSON Object:\n" << jsonStr << std::endl;
    draw_rmt_times = (uint16_t *)calloc(nb_rmt_times, sizeof(uint16_t));

    rmt_high_time_s = strtok(rmt_high_time_s, ":");
    int n = 0;

    while (rmt_high_time_s != NULL)
    {
        draw_rmt_times[n] = std::stoi(rmt_high_time_s);
        rmt_high_time_s = strtok(NULL, ":");
        n++;
    }
    while (n < nb_rmt_times)
    {
        draw_rmt_times[n] = draw_rmt_times[0];
        n++;
    }
    printf("draw_rmt_times: ");
    for (int x = 0; x < nb_rmt_times; x++)
        printf(" %d ", draw_rmt_times[x]);
    printf(" \n");

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

    int yes = 0; // 1 - on, 0 - off
    int result = setsockopt(socket_desc,
                            IPPROTO_TCP,
                            TCP_NODELAY,
                            (char *)&yes,
                            sizeof(int));
    if (result < 0)
        printf("error setting socket options\n");
    server.sin_addr.s_addr = inet_addr(esp32_ip_address);
    server.sin_family = AF_INET;
    server.sin_port = htons(3333);
    //Connect to remote server

    if (wifi_on == 1)qqq
    {
            int timeout = 20000; // Timeout in milliseconds
        setsockopt(socket_desc, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
        setsockopt(socket_desc, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));

        if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
        {
            puts("PC-host application Error: \n Error connecting to esp, wrong ESP32 ip address or wifi network?\n");
            return 1;
        }
        puts("Connected to esp wifi\n");
        send(socket_desc, (char*) settings_size, 2, 0);
        char esp32_settings_char[settings_size[0]];
        memcpy(esp32_settings_char, jsonStr.c_str(), settings_size[0]);
        send(socket_desc, esp32_settings_char, settings_size[0], 0);
    }

    array_with_zeros = (unsigned char *)calloc(129, sizeof(unsigned char));
    draw_black_bytes = (unsigned char *)calloc(129, sizeof(unsigned char));
    draw_white_bytes = (unsigned char *)calloc(129, sizeof(unsigned char));
    memset(draw_black_bytes, 85, 129);
    memset(draw_white_bytes, 170, 129);
    if (mirroring_task() == -1){ //Start the mirroring process
        printf("mirroring_task id %d returned -1 \n", id);
        closesocket(socket_desc);
        }
    cJSON_Delete(root);
}
