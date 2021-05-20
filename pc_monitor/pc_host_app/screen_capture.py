
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
import threading
from draw_cursor import generate_cursor, draw_cursor, draw_cursor_1bpp
from multiprocessing import shared_memory, resource_tracker, Value
from collections import namedtuple


#generate_cursor()
work_dir = f'{working_dir}/pc_host_app'
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

    if pipe_output and start_process:
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
            f'{display_list[x].epd_skip_mouse_only}',
            f'{display_list[x].framebuffer_cycles_2}',
            f'{display_list[x].framebuffer_cycles_2_threshold}',
            f'{modes[display_list[x].mode]}',
            f'{display_list[x].selective_compression}',
            f'{display_list[x].nb_chunks}', 
            f'{display_list[x].do_full_refresh}', 
            f'{display_list[x].a.disable_logging}',
            f'{ctx.wifi_on}'])

            ctx.has_childs = 1

            PID_list.append(R.pid)
    else:   
        pid1 = None


if ctx.a.child_process == 0:
    thread1 = threading.Thread(target=check_key_presses, args=(PID_list, ctx.offset_variables))
    thread1.start()

def main_task(ctx):
    if ctx.a.child_process == 0:
        write_to_shared_mem(ctx.offset_variables.mode, modes.get(ctx.mode), 'a')

    if pipe_output:
        fd1, fd0 = open_pipes(ctx)

    with mss.mss() as sct:

        while 1:
            t0 = time.time()
            if ctx.switcher == 0:
                ctx.switcher = 1;
            else:
                ctx.switcher = 0;
            t0 = time.time()

            if linux: ready = os.read(fd1, 1)
            elif windows: ret2 = win32file.ReadFile(fd1, 1)

            sct_img = sct.grab(ctx.monitor) #capture screen

            #mouse_moved = draw_cursor(display_list[0], sct_img)
            image_file = Image.frombytes(
                "RGB", sct_img.size, sct_img.bgra, "raw", "RGBX")

            mode = get_val_from_shm(ctx.offset_variables.mode, 'i')    

            if mode == 9: #PIL dithering
                
                image_file = convert_to_greyscale_and_enhance(image_file, ctx, ctx.offset_variables)

                image_file = image_file.convert('1')

            elif mode == 0: #Monochrome

                image_file = convert_to_greyscale_and_enhance(image_file, ctx, ctx.offset_variables)
                th = ctx.grey_monochrome_threshold+get_val_from_shm(ctx.offset_variables.grey_to_monochrome_threshold, 'i')

                def fn(x): return 255 if x > th else 0

                image_file = image_file.point(fn, mode='1')

            elif mode > 0 and mode < 9: #other dithering
                image_file = convert_to_greyscale_and_enhance(image_file, ctx, ctx.offset_variables)

                np_arr = np.asarray(image_file)

                np_arr = np.ravel(np_arr)
                mode = get_mode(get_val_from_shm(ctx.offset_variables.mode, 'i'))
                if dith.apply(np_arr, mode) == -1:
                    print("error dither type")
                    byte_frag = pipe_output_f(raw_files, None, mouse_moved, fd1, fd0)  # 1bpp->raw_files[0]
                    continue

                image_file = Image.frombytes('RGB', sct_img.size, np_arr)
                #t0 = t()

                image_file = image_file.convert('L')
                def fn(x): return x

                image_file = image_file.point(fn, mode='1')
                #print(t()-t0)
            else:
                print("error?")
            image_file = image_file.transpose(Image.FLIP_TOP_BOTTOM) #flip the image so that the first bytes contain the pixel data of the first lines
            if ctx.rotation != 0:
                image_file   = image_file.rotate(ctx.rotation,  expand=True)

            if enable_raw_output: 
                raw_files = get_raw_pixels(
                    image_file, raw_output_file, save_raw_file, ctx.switcher) #remove bitmap pad bytes
            if save_bmp:
                image_file.save(ctx.complete_output_file)
            if pipe_output:  # and dif_list_sum
                if pipe_bit_depth == 1:
                    mouse_moved = draw_cursor_1bpp(display_list[0], raw_files[0])
                    
                    byte_frag = pipe_output_f(raw_files, None, mouse_moved, fd1, fd0)  # 1bpp->raw_files[0]
                #elif pipe_bit_depth == 8:
                    #byte_frag = pipe_output_f(np_image_file, np_image_file, mouse_moved)  # 1bpp->raw_files[0]
            if ctx.a.disable_logging == 0:
                took = int(((time.time() - t0)*1000))
                if took > 1000:
                    print("mroe than")
                print(f"Display ID: {ctx.id}, capture took {took}ms")
            time.sleep(ctx.sleep_time/1000)

main_task(ctx)