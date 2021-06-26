
from utils import *
import mss
import os, sys
import pyautogui
from PIL import Image, ImageEnhance, ImageOps

import time
import subprocess
import io, struct
from random import randint
import numpy as np
import threading, cv2
from draw_cursor import generate_cursor, draw_cursor, draw_cursor_1bpp, paste_cursor, did_mouse_move
from multiprocessing import shared_memory, resource_tracker, Value
from collections import namedtuple


work_dir = f'{working_dir}'
if os.path.isdir(work_dir):
    os.chdir(work_dir)

PID_list = []
pid0 = os.getpid()
PID_list.append(pid0)


for u in range(nb_displays-1):
    if ctx.a.disable_logging:         
        P = subprocess.Popen([f'python',  'screen_capture.py',  f'{sys.argv[u+2]}', '-silent', 'child'])
    else:
        P = subprocess.Popen([f'python3',  'screen_capture.py',  sys.argv[u+2], 'child'])
    PID_list.append(P.pid)
    time.sleep(0.5)

    
if ctx.a.child_process == 0:

    if pipe_output and ctx.a.start_cpp_process :
        ctx.shared_buffer[0:100] = bytearray(100)
        if windows:
            binary = "process_capture.exe"
        elif linux:
            binary = "process_capture"
        for x in range(nb_displays):  
            time.sleep(0.5)

            R = subprocess.Popen([f'{working_dir}/{binary}', 
            f'{display_list[x].ip_address}',
            f'{display_list[x].id}',
            f'{display_list[x].width}',
            f'{display_list[x].height}',
            f'{display_list[x].refresh_every_x_frames}',
            f'{display_list[x].framebuffer_cycles}', 
            f'{display_list[x].rmt_high_time}',
            f'{display_list[x].enable_skipping}',
            f'{display_list[x].epd_skip_threshold}',
            f'{display_list[x].esp32_multithread}', 
          #  f'{display_list[x].epd_skip_mouse_only}',
            f'{display_list[x].framebuffer_cycles_2}',
            f'{display_list[x].framebuffer_cycles_2_threshold}',
            f'{modes[display_list[x].mode]}',
            f'{display_list[x].selective_compression}',
            f'{display_list[x].nb_chunks}', 
            f'{display_list[x].nb_draws}', 
            f'{display_list[x].draw_white_first}', 
            f'{modes[display_list[x].mode]}', 

            f'{display_list[x].do_full_refresh}', 

            f'{display_list[x].a.disable_logging}',
            f'{ctx.wifi_on}'])

            ctx.has_childs = 1

            PID_list.append(R.pid)
    else:   
        pid1 = None


if ctx.a.child_process == 0:
    thread1 = threading.Thread(target=check_key_presses, args=(PID_list, ctx.offsets))
    thread1.start()

def main_task(ctx):
    if ctx.a.child_process == 0:
        w_shm(ctx.offsets.mode, modes.get(ctx.mode), 'a')
        w_shm(ctx.offsets.selective_invert, ctx.selective_invert, 'a')
        w_shm(ctx.offsets.settings_changed, 2, 'a')

        if ctx.invert > 1:
            w_shm(ctx.offsets.invert_threshold, ctx.invert, 'a')
            w_shm(ctx.offsets.invert, ctx.invert, 'a')

        w_shm(ctx.offsets.fb1_rmt, ctx.draw_rmt_times[0], 'a')
        w_shm(ctx.offsets.fb2_rmt, ctx.draw_rmt_times[1], 'a')
        w_shm(ctx.offsets.invert_draw_times, ctx.invert_draw_times, 'a')

    if pipe_output:
        fd1, fd0 = open_pipes(ctx)

    dith.alloc_memory_()

    n = 0
    with mss.mss() as sct:
        capture_list = [sct.grab(ctx.monitor).raw, sct.grab(ctx.monitor).raw]
        while 1:
            t0 = time.time()

            if ctx.switcher == 0:
                ctx.switcher = 1;
            else:
                ctx.switcher = 0;

            sct_img = sct.grab(ctx.monitor) #capture screen
            
            capture_list[ctx.switcher] = sct_img.raw

            screen_changed = check_for_difference_esp_fun(capture_list, True)
            mouse_moved = did_mouse_move(ctx)

            if mouse_moved:
                pass
            elif screen_changed == 0 and r_shm(ctx.offsets.settings_changed, 'i') == 0:
                time.sleep(ctx.sleep_time/1000)
                check_and_exit(fd0,fd1)
                #print_settings()
                continue

            if pipe_output:
                if linux: ready = os.read(fd1, 1)
                elif windows: ret2 = win32file.ReadFile(fd1, 1)

            image_file =    Image.frombytes('RGB', (ctx.width, ctx.height), sct_img.rgb)
            # image_file.save("correct_colors.png")

            # image_file = Image.frombytes("RGB", sct_img.size, sct_img.bgra, "raw", "BGRX")  # Image.frombytes('RGB', (ctx.width, ctx.height), rgb)
            # image_file.save("wrong_colors.png")

            if ctx.pipe_bit_depth == 8:
                ctx.mouse_moved = paste_cursor(ctx, image_file)
                # if ctx.mouse_moved:
                #     print("8 moved")
                # else: print("not")

            mode = r_shm(ctx.offsets.mode, 'i')    
            ctx.mode_code = mode

            if mode ==  10 or ctx.draw_white_first: ctx.pipe_bit_depth = 8
            else: ctx.pipe_bit_depth = 1
            if mode == 9: #PIL dithering
                
                image_file = convert_to_greyscale_and_enhance(image_file, ctx)

                image_file = image_file.convert('1')

            elif mode == 0: #Monochrome


                image_file = convert_to_greyscale_and_enhance(image_file, ctx)
                th = ctx.grey_monochrome_threshold+r_shm(ctx.offsets.grey_to_monochrome_threshold, 'i')
                
                def fn(x): return 255 if x > th else 0

                image_file = image_file.point(fn, mode='1')

            elif mode > 0 and mode < 9: #other dithering
                image_file = convert_to_greyscale_and_enhance(image_file, ctx)

                mode = get_mode(modes, r_shm(ctx.offsets.mode, 'i'))


                if dith.apply(ctx.np_arr, mode) == -1:
                   # print("error dither type")
                    byte_frag = pipe_output_f(raw_data, None, ctx.mouse_moved, fd1, fd0)  # 1bpp->raw_files[0]
                    continue

                image_file = Image.frombytes('RGB', sct_img.size, ctx.np_arr)
                #t0 = t()
                
                image_file = image_file.convert('L')

                invert =  r_shm(ctx.offsets.invert, 'i')

                if invert > 0:
                    if invert == 1: image_file = ImageOps.invert(image_file)
                    else:   image_file = smart_invert(image_file) 


                def fn(x): return x

                image_file = image_file.point(fn, mode='1')

                #print(t()-t0)
                
            elif mode == 10: # 4 shades grayscale mode

                image_file = convert_to_greyscale_and_enhance(image_file, ctx)

                # dith.quantize_(np_arr,  np_arr, ctx.width*ctx.height)
                #image_file = Image.frombytes('L', sct_img.size, np_arr)

            else:
                print("error?")

            update_rmt_times(ctx, image_file)

            if mode != 10 and not ctx.draw_white_first:
                image_file = image_file.transpose(Image.FLIP_TOP_BOTTOM) #flip the image so that the first bytes contain the pixel data of the first lines
            if ctx.rotation != 0:
                image_file   = image_file.rotate(ctx.rotation,  expand=True)

            if enable_raw_output: 
                raw_data = get_raw_pixels(
                    image_file, raw_output_file, save_raw_file, ctx.switcher) #remove bitmap pad bytes
            if save_bmp:
                save_bmp_fun(image_file, mode)

            if pipe_output: # and dif_list_sum
                
                if ctx.pipe_bit_depth == 1:
                    ctx.mouse_moved = draw_cursor_1bpp(display_list[0], raw_data[0])
                    # if ctx.mouse_moved:
                    #     print("1 moved")
                    # else: print("not")
                    pipe_output_f(raw_data, ctx.eight_bpp, ctx.mouse_moved, fd1, fd0)  # 1bpp->raw_data[0]
                elif ctx.pipe_bit_depth == 8:
                    pipe_output_f(raw_data, None, ctx.mouse_moved, fd1, fd0) 
                    
            if ctx.a.disable_logging == 0:
                took = int(((time.time() - t0)*1000))

                print(f"Display ID: {ctx.id}, capture took {took}ms")

            time.sleep(ctx.sleep_time/1000)

main_task(ctx)
