
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
                # "nb_chunks": display_list[x].nb_chunks,
                "nb_draws": display_list[x].nb_draws,
                #"draw_white_first": display_list[x].draw_white_first,
                "with_cv2": display_list[x].with_cv2,
                "do_full_refresh": display_list[x].do_full_refresh,
                "disable_logging": display_list[x].a.disable_logging,
                "wifi_on": ctx.wifi_on,
                "refresh_on_startup": display_list[x].refresh_on_startup, 
                "pipe_bit_depth": display_list[x].pipe_bit_depth,    
                "draw_black_on_startup": display_list[x].draw_black_on_startup       
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


            # Read the image
            # img = opencv_image#cv2.imread('input_image.jpg', cv2.IMREAD_GRAYSCALE)

            # contours, hierarchy = cv2.findContours(img, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
            # drawing = []
            # for i,c in enumerate(contours):
            #     # Add contours which don't have any children, value will be -1 for these
            #     if hierarchy[0,i,1] < 0:
            #         drawing.append(c)
            # img = cv2.cvtColor(img,cv2.COLOR_GRAY2BGR)
            # # Draw filled contours
            # cv2.drawContours(img, drawing, -1, (255,255,255), thickness=cv2.FILLED)
            # # Draw contours around filled areas with red just to indicate where these happened
            # cv2.drawContours(img, drawing, -1, (0,0,255), 1)
            # cv2.imshow('filled',img)
            # cv2.imshow('output_image', opencv_image)
            
            
            


            cv2.waitKey(1)

            t = time.time()

            opencv_image_ori = cv2.cvtColor(np.array(sct_img), cv2.COLOR_BGRA2GRAY)

            inv_thres = r_shm(ctx.offsets.invert_threshold, 'i')

            n = np.mean(opencv_image_ori)

            opencv_image = None
            if n <  inv_thres:   
                opencv_image = 255-opencv_image_ori.copy()
            else: opencv_image = opencv_image_ori.copy()
            
            if r_shm(ctx.offsets.selective_invert,'i'):
                np_arr = np.ravel(opencv_image)
                dith.selective_invert_v2_(np_arr, 5, 5, 50, 60, 5)
                opencv_image = np_arr.reshape(opencv_image.shape)

            kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (3, 3))
            grad = cv2.morphologyEx(opencv_image, cv2.MORPH_GRADIENT, kernel)

            _, bw = cv2.threshold(grad, 0.0, 255.0, cv2.THRESH_BINARY | cv2.THRESH_OTSU)

            kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (9, 1))
            connected = cv2.morphologyEx(bw, cv2.MORPH_CLOSE, kernel)
            # using RETR_EXTERNAL instead of RETR_CCOMP
            contours, hierarchy = cv2.findContours(connected.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)
            #For opencv 3+ comment the previous line and uncomment the following line
            #_, contours, hierarchy = cv2.findContours(connected.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)

            mask = np.zeros(bw.shape, dtype=np.uint8)

            # for idx in range(len(contours)):
            #     continue
            #     x, y, w, h = cv2.boundingRect(contours[idx])
            #     mask[y:y+h, x:x+w] = 0
            #     cv2.drawContours(mask, contours, idx, (255, 255, 255), -1)
            #     r = float(cv2.countNonZero(mask[y:y+h, x:x+w])) / (w * h)

            #     if r > 0.45 and w > 8 and h > 8:
            #         cv2.rectangle(opencv_image, (x, y), (x+w-1, y+h-1), (255, 255, 255), 2)
            #opencv_image = (opencv_image[:,:]>=np.mean(opencv_image,axis=(0,1)))*255
            #opencv_image = cv2.cvtColor(opencv_image, cv2.COLOR_GRAY2BGR)
            #opencv_image = opencv_image.astype(np.uint8)

            def calculate_distance(cnt1, cnt2):
                cx1, cy1, w, h = cv2.boundingRect(cnt1)
                cx2, cy2, w, h = cv2.boundingRect(cnt2)
                # M1 = cv2.moments(cnt1)
                # M2 = cv2.moments(cnt2)
                # cx1 = int(M1['m10'] / M1['m00'])
                # cy1 = int(M1['m01'] / M1['m00'])
                # cx2 = int(M2['m10'] / M2['m00'])
                # cy2 = int(M2['m01'] / M2['m00'])
                return np.sqrt((cx2 - cx1)**2 + (cy2 - cy1)**2)


            # merged_contours = []
            # contour_threshold_distance = 200  # Adjust this value based on your requirement
            # for i, cnt1 in enumerate(contours):
            #     merged = False
            #     x, y, w, h = cv2.boundingRect(cnt1)
            #     if w*h < 100: continue
            #     for j, cnt2 in enumerate(contours):
            #         if i != j and calculate_distance(cnt1, cnt2) < contour_threshold_distance:
            #             merged_contours.append(np.vstack((cnt1, cnt2)))
            #             merged = True
            #             break
            #     if not merged:
            #         merged_contours.append(cnt1)


            thres = 100
            thres_max = 10000
            mult = 1.2
            for contour in contours:
                
                x, y, w, h = cv2.boundingRect(contour)
                centerx = x+w//2; centery =  y+h//2
                w= int(w*mult)
                h= int(h*mult)
                a = w*h
                if a < thres or a > thres_max:continue
                xx = centerx - w//2
                yy = centery - h//2
                enhanced_roi = opencv_image[yy:yy+h, xx:xx+w]
                alpha = 3# Contrast control (1.0-3.0)
                beta = 1    # Brightness control (0-100)
                #enhanced_roi = cv2.convertScaleAbs(enhanced_roi, alpha=alpha, beta=beta)
                #enhanced_roi = cv2.equalizeHist(enhanced_roi)
                #if n >  inv_thres:   
                # enhanced_roi = 255-enhanced_roi
                enhanced_roi = (enhanced_roi[:,:]>=np.mean(enhanced_roi,axis=(0,1)))*255
                # min_val = np.min(roi)
                # max_val = np.max(roi)
                # enhanced_roi = cv2.convertScaleAbs(roi, alpha=255.0/(max_val-min_val), beta=-min_val*(255.0/(max_val-min_val)))
                opencv_image[yy:yy+h, xx:xx+w] = enhanced_roi                    

            print("--->", time.time() -t)

            cv2.imshow('opencv_image', opencv_image)
            cv2.moveWindow('opencv_image', 1200, 0)

            #opencv_image = cv2.cvtColor(opencv_image, cv2.COLOR_GRAY2BGR)

            cv2.imshow(f"opencv_image_ori", opencv_image_ori)
            cv2.moveWindow('opencv_image_ori', 0, 825)
            
            
            #im_in = np.array(opencv_image)


            
            #im_floodfill_inv = cv2.bitwise_not(im_floodfill)            
            th = ctx.grey_monochrome_threshold+r_shm(ctx.offsets.grey_to_monochrome_threshold, 'i')

            _, im_in = cv2.threshold(opencv_image, th, 255, cv2.THRESH_BINARY)
            
            im_floodfill = im_in.copy()
            h, w = im_floodfill.shape[:2]
            mask = np.zeros((h+2, w+2), np.uint8)+ 0
            cv2.floodFill(im_floodfill, mask, (0,0), 255)

            #thresh, im_in = cv2.threshold(opencv_image, 128, 255, cv2.THRESH_BINARY | cv2.THRESH_OTSU)

            #im_in = 255 - im_in
            im_out = im_in.copy()

            #im_out = cv2.cvtColor(np.array(sct_img), cv2.COLOR_BGRA2GRAY)
                        
            kernel = np.ones((3,3), np.uint8)
            inverted_image = cv2.bitwise_not(im_in)
            eroded_image = cv2.erode(inverted_image, kernel, iterations=2)
            result_image = cv2.bitwise_not(eroded_image)

            mask = eroded_image != 0
            im_out[mask] = eroded_image[mask]
            
            cv2.imshow('im_in', im_in)
            cv2.moveWindow('im_in', 0, 0)

            cv2.imshow('im_out', im_out)
            cv2.moveWindow('im_out', 1200, 1000)

            cv2.imshow('array_thresholded', result_image)
            cv2.moveWindow('array_thresholded', 1200, 1000)


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
