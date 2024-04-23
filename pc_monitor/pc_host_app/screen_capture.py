
from utils import *
import mss
import os, sys
import pyautogui
from PIL import Image, ImageEnhance, ImageOps
import tempfile

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
proc_list = []


for u in range(nb_displays-1):
    if ctx.a.disable_logging:         
        P = subprocess.Popen([f'python',  'screen_capture.py',  f'{sys.argv[u+2]}', 'silent', 'child'])
    else:
        P = subprocess.Popen([f'python',  'screen_capture.py',  sys.argv[u+2], 'child'])
    PID_list.append(P.pid)
    time.sleep(0.5)


def get_json_file(x, file_name=None):
    data = {
                "esp32_ip_address": display_list[x].ip_address,
                "id": display_list[x].id,
                "width_resolution": display_list[x].width,
                "height_resolution": display_list[x].height,
                "refresh_every_x_frames": display_list[x].refresh_every_x_frames,
                #"framebuffer_cycles": display_list[x].framebuffer_cycles,
                "draws_conf": display_list[x].draws_conf,
                "enable_skipping": display_list[x].enable_skipping,
                "epd_skip_threshold": display_list[x].epd_skip_threshold,
                "esp32_multithread": display_list[x].esp32_multithread,
                #"framebuffer_cycles_2": display_list[x].framebuffer_cycles_2,
                #"framebuffer_cycles_2_threshold": display_list[x].framebuffer_cycles_2_threshold,
                "mode": modes[display_list[x].mode],
                "selective_compression": display_list[x].selective_compression,
                "nb_chunks": display_list[x].nb_chunks,
                "nb_draws": display_list[x].nb_draws,
                #"draw_white_first": display_list[x].draw_white_first,
                "with_cv2": display_list[x].with_cv2,
                "do_full_refresh": display_list[x].do_full_refresh,
                "disable_logging": display_list[x].a.disable_logging,
                "wifi_on": ctx.wifi_on,
                "refresh_on_startup": display_list[x].refresh_on_startup, 
                "pipe_bit_depth": display_list[x].pipe_bit_depth,           
            }
    temp_file_path = ""
    
    if file_name:
        with open(file_name, "w") as temp_file:
            json.dump(data, temp_file, indent=4)
            print(temp_file.name)
            temp_file_path = temp_file.name
    else:
        with tempfile.NamedTemporaryFile(delete=False, mode="w") as temp_file:
            json.dump(data, temp_file, indent=4)
            print(temp_file.name)
            temp_file_path = temp_file.name
            
    return temp_file_path

if ctx.a.child_process == 0:

    if pipe_output and ctx.a.start_cpp_process :
        ctx.shared_buffer[0:100] = bytearray(100)
        if windows:
            binary = "process_capture.exe"
            binary = "proc_cap.bat" #this seems to fix the bullshit random errors
        elif linux:
            binary = "process_capture"
        for x in range(nb_displays):  
            time.sleep(0.5)
            
            temp_file_path = get_json_file(x)
                
            R = subprocess.Popen([f'{working_dir}/{binary}', temp_file_path],  creationflags=subprocess.CREATE_NEW_CONSOLE)
            ctx.has_childs = 1
            print(f"---------proc cap id {id} {R.pid}")
            PID_list.append(R.pid)
            proc_list.append(R)

            print()

    else: 
        for x in range(nb_displays):  
            
            print("conf file",os.path.basename(get_json_file(x, "args.json")))
        pid1 = None


if ctx.a.child_process == 0 :
    print("STARTING KEY PRESS CHECKER")
    thread1 = threading.Thread(target=check_key_presses, args=(PID_list, ctx.offsets))
    thread1.start()

def main_task(ctx):
    global quit_all
    if ctx.a.child_process == 0:
        w_shm(ctx.offsets.mode, modes.get(ctx.mode), 'a')
        w_shm(ctx.offsets.selective_invert, ctx.selective_invert, 'a')
        w_shm(ctx.offsets.settings_changed, 2, 'a')

        if ctx.invert > 1:
            w_shm(ctx.offsets.invert_threshold, ctx.invert, 'a')
            w_shm(ctx.offsets.invert, ctx.invert, 'a')

    if pipe_output:
        fd1, fd0 = open_pipes(ctx)

    dith.alloc_memory_()

    n = 0
    
    capture_list = []
    with mss.mss() as sct:
        capture_list = [sct.grab(ctx.monitor).raw, sct.grab(ctx.monitor).raw]
        
    while 1:
        t0 = time.time()

        ctx.switcher = 1- ctx.switcher
        
        with mss.mss() as sct: # to avoid crashes
            sct_img = sct.grab(ctx.monitor) #capture screen
        
        capture_list[ctx.switcher] = sct_img.raw

        screen_changed = capture_list[0] != capture_list[1]#check_for_difference_esp_fun(capture_list, True)
        
        #print(f"s.. {screen_changed} {(time.time() - t0):4.3f} ")
        mouse_moved = did_mouse_move(ctx)


        if mouse_moved:
            pass
        elif screen_changed == 0 and r_shm(ctx.offsets.settings_changed, 'i') == 0:
            time.sleep(ctx.sleep_time/1000)
            check_and_exit(fd0,fd1)
            #print_settings()
            if ctx.with_cv2 == withCv2Enum.PYTHON.value or  ctx.with_cv2 == withCv2Enum.BOTH.value:
                cv2.waitKey(1)
            if quit_all: break
            continue
        if quit_all: break
        
        if pipe_output:
            if linux: ready = os.read(fd1, 1)
            elif windows: ret2 = win32file.ReadFile(fd1, 1)

        #image_file =    Image.frombytes('RGB', (ctx.width, ctx.height), sct_img.rgb)
        image_file = Image.frombytes("RGB", sct_img.size, sct_img.bgra, "raw", "BGRX")  # Image.frombytes('RGB', (ctx.width, ctx.height), rgb)

        if ctx.pipe_bit_depth == 8:
            ctx.mouse_moved = paste_cursor(ctx, image_file)
            # if ctx.mouse_moved:
            #     print("8 moved")
            # else: print("not")

        mode = r_shm(ctx.offsets.mode, 'i')    
        ctx.mode_code = mode

        # if mode ==  10 or ctx.nb_draws > 1: ctx.pipe_bit_depth = 8 #to do support switching modes duing runtime
        # else: ctx.pipe_bit_depth = 1
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
        if mode != 10 and not ctx.nb_draws > 1 and 0:
            image_file = image_file.transpose(Image.FLIP_TOP_BOTTOM) #flip the image so that the first bytes contain the pixel data of the first lines
        if ctx.rotation != 0:
            image_file   = image_file.rotate(ctx.rotation,  expand=True)
        if ctx.with_cv2 == withCv2Enum.BOTH.value or ctx.with_cv2 == withCv2Enum.PYTHON.value:
            output_image = image_file.convert("L")

            opencv_image = np.array(output_image)

            opencv_image = cv2.cvtColor(opencv_image, cv2.COLOR_GRAY2BGR)

            cv2.imshow(f"python Image {ctx.id}", opencv_image)
            cv2.waitKey(1)
            
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

        #time.sleep(ctx.sleep_time/1000) #no need for this here because the python already waits for the ack from the board
        

main_task(ctx)
