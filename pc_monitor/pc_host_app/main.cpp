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

int width_resolution, height_resolution;

char compressed_chunk_lengths_in_bytes[64];

char working_dir[256];
unsigned char *array_with_zeros, *draw_white_bytes, *draw_black_bytes;

char *line_changed; //array containing 1 or 0 depending on whether the corresponding line on the screen has changed

#if !defined(_WIN32)
#define SOCKET int
#define HANDLE int
#endif
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
int id, refresh_every_x_frames = 0, selective_compression;
int total_nb_pixels, eink_framebuffer_size, chunk_size, nb_chunks;
int source_image_bit_depth = 1;
bool disable_logging;
uint32_t loop_counter[1] = {0};
int mouse_moved = 0;
int pseudo_greyscale_mode = 0, improve_dither;
int do_full_refresh = 1;
unsigned char full_refresh_delay = 30;

char ready0[6];
char ready1[6];
char per_frame_wifi_settings[6] = {0, 0, 0, 0, 0, 0};

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

    if (pseudo_greyscale_mode == 1)
        per_frame_wifi_settings[1] = 'p';
    else
        per_frame_wifi_settings[1] = 0;
    if (loop_counter[0] == refresh_every_x_frames && do_full_refresh != 0)
    {
        loop_counter[0] = 0;
        per_frame_wifi_settings[2] = full_refresh_delay;
    }
    else
        per_frame_wifi_settings[2] = '0';

    send(socket_desc, per_frame_wifi_settings, 6, 0);
    int ret0 = recv(socket_desc, ready2, 6, 0);
    for (int k = 0; k < 6; k++)
    {
        if (ready2[k] != per_frame_wifi_settings[k])
        {
            printf("warning ready2 per_frame_wifi_settings dif \n");
            sleep(100000);
        }
    }

    if (loop_counter[0] == refresh_every_x_frames + 1)
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
                printf("error transfer wifi returneed -1\n");
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
    memset(line_changed, 1, height_resolution);

    memset(padded_2bpp_framebuffer_current, 85, eink_framebuffer_size);
    rle_compress(padded_2bpp_framebuffer_current, tmp_array, nb_chunks, compressed_eink_framebuffer, eink_framebuffer_size, chunk_size);
    for (int g = 0; g < nb_chunks * 4; g += 4)
    {
        unsigned int number2 = htonl(compressed_chunk_lengths[g / 4]);
        memcpy(compressed_chunk_lengths_in_bytes + g, &compressed_chunk_lengths[g / 4], 4);
    }
    wifi_transfer(compressed_eink_framebuffer, 0);
    recv(socket_desc, ready0, 6, 0);
    memset(padded_2bpp_framebuffer_current, 170, eink_framebuffer_size);
    rle_compress(padded_2bpp_framebuffer_current, tmp_array, nb_chunks, compressed_eink_framebuffer, eink_framebuffer_size, chunk_size);
    wifi_transfer(compressed_eink_framebuffer, 0);
    recv(socket_desc, ready0, 6, 0);
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

    total_nb_pixels = width_resolution * height_resolution;
    eink_framebuffer_size = total_nb_pixels / 4;
    chunk_size = (eink_framebuffer_size / nb_chunks);

    unsigned char ack[1] = {246};
    unsigned char ack2[3];
    char ready0[6];
    char ready1[6];

    int tot = 0, pos = 0;
    DWORD ret2 = 0;

    int compressed_framebuffer_size = 0;

    unsigned char *source_8bpp_current;           // array containing the current monochrome 8bpp screen capture
    unsigned char *source_8bpp_previous;          //array containing the previous monochrome 8bpp screen capture
    unsigned char *source_8bpp_modified_current;  //same as 'source_8bpp_current' but has been modified
    unsigned char *source_8bpp_modified_previous; //same as 'source_8bpp_previous' but has been modified

    char *eink_framebuffer; // the 2bpp array that will be fed directly to the display

    char *filter_framebuffer;        // a filter framebuffer
    char *eink_framebuffer_modified; // a filter framebuffer that has been modified

    char *eink_framebuffer_swapped; // an 'eink_framebuffer' with bytes swapped

    char *compressed_eink_framebuffer; // an 'eink_framebuffer' that has been compressed with RLE compression

    char *padded_2bpp_framebuffer_current;  //array containing the current monochrome 2bpp screen capture
    char *padded_2bpp_framebuffer_previous; //array containing the previous monochrome 2bpp screen capture

    unsigned char *source_1bpp; //array containing the current monochrome 1bpp screen capture
    uint16_t *added_compression_arr[8];
    line_changed = (char *)calloc(height_resolution + 2, sizeof(char));
    source_1bpp = (unsigned char *)calloc(total_nb_pixels, sizeof(unsigned char));
    tmp_array = (char *)calloc(eink_framebuffer_size + 50000, sizeof(unsigned char));
    ready2 = (char *)calloc(6, sizeof(char));
    memset(ready2, 0, 6);
    padded_2bpp_framebuffer_current = (char *)calloc(eink_framebuffer_size, sizeof(unsigned char));
    padded_2bpp_framebuffer_previous = (char *)calloc(eink_framebuffer_size, sizeof(unsigned char));

    source_8bpp_modified_current = (unsigned char *)calloc(total_nb_pixels, sizeof(unsigned char));
    source_8bpp_modified_previous = (unsigned char *)calloc(total_nb_pixels, sizeof(unsigned char));
    source_8bpp_current = (unsigned char *)calloc(total_nb_pixels, sizeof(unsigned char));
    source_8bpp_previous = (unsigned char *)calloc(total_nb_pixels, sizeof(unsigned char));

    compressed_eink_framebuffer = (char *)calloc(eink_framebuffer_size, sizeof(char));
    eink_framebuffer = (char *)calloc(eink_framebuffer_size, sizeof(char));
    filter_framebuffer = (char *)calloc(eink_framebuffer_size, sizeof(char));
    eink_framebuffer_modified = (char *)calloc(eink_framebuffer_size, sizeof(char));
    eink_framebuffer_swapped = (char *)calloc(eink_framebuffer_size, sizeof(char));
    decompressed = (char *)calloc(eink_framebuffer_size + 50000, sizeof(char));

    for (int h = 0; h < nb_chunks; h++)
    {
        compressed_eink_framebuffer_ptrs[h] = (char *)calloc(chunk_size * 2, sizeof(char));
        added_compression_arr[h] = (uint16_t *)calloc(chunk_size * 2, sizeof(char));
    }
    if (source_image_bit_depth == 1)
    { //assume the first screen capture to be completely white
        memset(padded_2bpp_framebuffer_previous, 85, eink_framebuffer_size * sizeof(unsigned char));
        memset(padded_2bpp_framebuffer_current, 85, eink_framebuffer_size * sizeof(unsigned char));
    }
    else if (source_image_bit_depth == 8)
    { //assume the first screen capture to be completely white
        memset(source_8bpp_current, 255, total_nb_pixels * sizeof(unsigned char));
        memset(source_8bpp_previous, 255, total_nb_pixels * sizeof(unsigned char));
    }
    else
    {
        printf("unsupported bit depth\n");
        return -1;
    }
    int total[] = {1}, repeat_counter = 0, next = 0;
    ;
    while (1)
    {
        if (total[0] != 0 && wifi_on == 1) // if screen didn't change don't wait for ack from board
            recv(socket_desc, ready0, 6, 0);
        ret2 = pipe_write(fd1, ack, 1, ret2);
        ret2 = pipe_read(fd0, ack2, 3, ret2);

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
        if (ack2[2] == 1)
            pseudo_greyscale_mode = 1;
        else
            pseudo_greyscale_mode = 0;

        long t0 = getTick();
        ret2 = pipe_read(fd0, line_changed, height_resolution, ret2);

        if (source_image_bit_depth == 1)
        {
            ret2 = pipe_read(fd0, source_1bpp, total_nb_pixels / 8, ret2);
            if (ret2 != total_nb_pixels / 8)
            {
                printf("warning ret2 1bpp \n");
                return -1;
            }
        }

        if (source_image_bit_depth == 8 || improve_dither == 1)
        {
            ret2 = pipe_write(fd1, ack, 1, ret2);
            ret2 = pipe_read(fd0, ack2, 3, ret2);

            memcpy(source_8bpp_previous, source_8bpp_current, total_nb_pixels * sizeof(unsigned char));

            ret2 = pipe_read(fd0, source_8bpp_current, total_nb_pixels, ret2);
            if (ret2 != total_nb_pixels)
                printf("warning ret2 total_nb_pixels\n");
        }

        if (source_image_bit_depth == 1)
        {
            if (loop_counter[0] == refresh_every_x_frames)
            {
                if (do_full_refresh == 0)
                    send_refresh_framebuffers(padded_2bpp_framebuffer_current, compressed_eink_framebuffer);
                memset(padded_2bpp_framebuffer_previous, 85, eink_framebuffer_size * sizeof(unsigned char));
                //  for (int h = 0; h < eink_framebuffer_size; h++)
                //     padded_2bpp_framebuffer_previous[h] = 170;
                // array_to_file(padded_2bpp_framebuffer_previous, 100000, working_dir, "padded_2bpp_framebuffer_previous", 0);
            }
            else
                memcpy(padded_2bpp_framebuffer_previous, padded_2bpp_framebuffer_current, eink_framebuffer_size);

            generate_eink_framebuffer_v1(source_1bpp, padded_2bpp_framebuffer_current, padded_2bpp_framebuffer_previous, eink_framebuffer);

            optimize_rle(eink_framebuffer);
            // if (improve_dither == 1)
            // {
            //     generate_filter_framebuffer(source_8bpp_current, source_8bpp_previous, filter_framebuffer);
            //     array_to_file(filter_framebuffer, total_nb_pixels / 4, working_dir, "filter_framebuffer", 0);

            //     filter_unwanted_dither(eink_framebuffer, filter_framebuffer, eink_framebuffer_modified, eink_framebuffer_size);
            //     array_to_file(eink_framebuffer, total_nb_pixels / 4, working_dir, "eink_framebuffer", 0);
            //     array_to_file(eink_framebuffer_modified, total_nb_pixels / 4, working_dir, "eink_framebuffer_modified", 0);
            // }
        }
        else if (source_image_bit_depth == 8)
        {
            generate_eink_framebuffer_v2(source_8bpp_current, source_8bpp_previous, source_8bpp_modified_previous, eink_framebuffer);
        }
        // if (pseudo_greyscale_mode == 1)
        //     improve_dither_compression(eink_framebuffer, eink_framebuffer_size, line_changed, width_resolution, height_resolution);
        swap_bytes(eink_framebuffer, eink_framebuffer_swapped, eink_framebuffer_size, source_image_bit_depth);

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

        if (loop_counter[0] == refresh_every_x_frames || loop_counter[0] == refresh_every_x_frames + 1)
        {
            memset(line_changed, 1, height_resolution);
            total[0] = height_resolution;
        }
        //  if (loop_counter[0] != refresh_every_x_frames)
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
            if (total[0] > 85) //don't increase the counter to clear te display if only 85 lines have changed
                loop_counter[0]++;
        }
        if (disable_logging == 0)
            printf("Processing time %dms\n", getTick() - t0);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    char nb_args = argc;
    char settings_size[1];
    settings_size[0] = (nb_args - 7) * 2;
    int16_t *esp32_settings = (int16_t *)calloc(settings_size[0], sizeof(uint8_t));

    //  printf("settings size %d \n", settings_size[0]);

    char *esp32_ip_address = argv[1];
    char *display_id = argv[2];
    id = std::stoi(argv[2]);
    width_resolution = std::stoi(argv[3]);
    height_resolution = std::stoi(argv[4]);
    refresh_every_x_frames = std::stoi(argv[5]);

    esp32_settings[0] = std::stoi(argv[6]);
    esp32_settings[1] = std::stoi(argv[7]);
    esp32_settings[2] = std::stoi(argv[8]);
    esp32_settings[3] = std::stoi(argv[9]);
    esp32_settings[4] = std::stoi(argv[10]);
    esp32_settings[5] = std::stoi(argv[11]);
    esp32_settings[6] = std::stoi(argv[12]);
    esp32_settings[7] = std::stoi(argv[13]);
    esp32_settings[8] = std::stoi(argv[14]);
    esp32_settings[9] = std::stoi(argv[15]);
    do_full_refresh = std::stoi(argv[16]);

    disable_logging = std::stoi(argv[nb_args - 2]);
    wifi_on = std::stoi(argv[nb_args - 1]);

    printf("esp32_ip_address: %s\n", esp32_ip_address);
    printf("display id: %s\n", display_id);
    printf("refresh_every_x_frames: %d\n", refresh_every_x_frames);
    printf("do_full_refresh: %d\n", do_full_refresh);

    printf("framebuffer_cycles: %d\n", esp32_settings[0]);
    printf("rmt_high_time: %d\n", esp32_settings[1]);
    printf("enable_skipping: %d\n", esp32_settings[2]);
    printf("epd_skip_threshold: %d\n", esp32_settings[3]);
    printf("epd_skip_mouse_only: %d\n", esp32_settings[4]);

    printf("framebuffer_cycles_2: %d\n", esp32_settings[5]);
    printf("framebuffer_cycles_2_threshold: %d\n", esp32_settings[6]);
    printf("pseudo_greyscale_mode: %d\n", esp32_settings[7]);
    printf("selective_compression: %d\n", selective_compression = esp32_settings[8]);
    printf("nb_chunks: %d\n", nb_chunks = esp32_settings[9]);
    //  printf("improve_dither: %d\n", improve_dither = std::stoi(argv[15]));

    if (disable_logging == 1)
        printf("logging disabled \n");
    if (wifi_on == 1)
        printf("wifi enabled \n");
    else
        printf("wifi disabled \n");
#ifdef __linux__

    sprintf(input_pipe, "%s%s", "/tmp/epdiy_pc_monitor_a_", display_id);
    sprintf(output_pipe, "%s%s", "/tmp/epdiy_pc_monitor_b_", display_id);
    // printf("input pipe : %s, output pipe: %s\n", input_pipe, output_pipe);

    mkfifo(input_pipe, 0666);
    mkfifo(output_pipe, 0666);

#elif _WIN32
    sprintf(input_pipe, "%s%s", "\\\\.\\pipe\\epdiy_pc_monitor_a_", display_id);
    sprintf(output_pipe, "%s%s", "\\\\.\\pipe\\epdiy_pc_monitor_b_", display_id);

    char buffer[1024];

    fd1 = CreateNamedPipe(TEXT(output_pipe),
                          PIPE_ACCESS_DUPLEX,
                          PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, // FILE_FLAG_FIRST_PIPE_INSTANCE is not needed but forces CreateNamedPipe(..) to fail if the pipe already exists...
                          1, 1024 * 64, 1024 * 64, NMPWAIT_USE_DEFAULT_WAIT, NULL);

    if (bool ret = ConnectNamedPipe(fd1, NULL) != FALSE)
        printf("pipe fd1 is ok\n");
    else
        printf("Pipe  fd1 not ok \n");
    fd0 = CreateFile(TEXT(input_pipe), PIPE_ACCESS_DUPLEX, 0, NULL, OPEN_EXISTING, 0, NULL);

    if (fd0 == INVALID_HANDLE_VALUE)
        printf("invalid handle fd0, error %d \n", GetLastError());
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
    int err = WSAGetLastError();
    if (!ISVALIDSOCKET(socket_desc))
    {
        fprintf(stderr, "socket() failed! %d\n", WSAGetLastError());
    }
#ifdef __linux__ 
    int yes = 0; // 1 - on, 0 - off
    int result = setsockopt(socket_desc,
                            IPPROTO_TCP,
                            TCP_NODELAY,
                            (char *)&yes,
                            sizeof(int));
    if (result < 0)
        printf("error setting socket options\n");
#endif
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

    getcwd(working_dir, sizeof(working_dir));
    printf("current working directory is: %s\n", working_dir);
    if (wifi_on == 1)
    {
        if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
        {
            puts("PC-host application Error: \n Error connecting to esp, wrong ESP32 ip address or wifi network?\n");
            return 1;
        }
        puts("Connected to esp wifi\n");
        send(socket_desc, settings_size, 1, 0);
        char esp32_settings_char[settings_size[0]];
        memcpy(esp32_settings_char, esp32_settings, settings_size[0]);
        send(socket_desc, esp32_settings_char, settings_size[0], 0);
    }

    array_with_zeros = (unsigned char *)calloc(129, sizeof(unsigned char));
    draw_black_bytes = (unsigned char *)calloc(129, sizeof(unsigned char));
    draw_white_bytes = (unsigned char *)calloc(129, sizeof(unsigned char));
    memset(draw_black_bytes, 85, 129);
    memset(draw_white_bytes, 170, 129);
    mirroring_task(); //Start the mirroring process
}
