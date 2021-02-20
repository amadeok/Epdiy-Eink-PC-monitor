import mss
import os, sys
import pyautogui
from PIL import Image
import time
import subprocess
import termios, tty
import io
from random import randint
import numpy as np
import threading
######### Settings #########
x_offset = 70                      # The screen will be captured x_offset number of pixels to the right from the top left corner
y_offset = 140                     # The screen will be captured y_offset number of pixels from the top from the top left corner
sleep_time = 150                    # number of miliseconds to sleep after capturing and piping the screen. lower value means higher framerate
grey_to_monochrome_threshold = 200 # when converting the 8bpp greyscale image to monochrome this threshold establishes 
                                   # which values will be pure black and which will be pure white. ie: with threshold 200
                                   # all bytes with value more than 200 the pixel will become white, otherwise will become black
############################        
width_res = 1200
height_res = 825
width_res2 = width_res + x_offset
height_res2 = height_res + y_offset
tot_nb_pixels = height_res * width_res

chunk_size = int(width_res*height_res/8/8)
monitor = {"top": y_offset, "left": x_offset,
           "width": width_res, "height": height_res} 

working_dir = os.getcwd()

first_white_arr_array = np.full((tot_nb_pixels), 255, dtype=np.uint8)
np_image_file = np.full((tot_nb_pixels), 0, dtype=np.uint8)

eight_bpp_bytearr_list = [first_white_arr_array, np_image_file]

byte_string_list = [bytearray([1] * 1*1), bytearray(b'\x00')]

eight_bpp_frame_list2 = [
    list([1] * width_res*height_res), list([1] * width_res*height_res)]

dif_list = bytearray(height_res)
dif_list_sum = 0
switcher = 0
fifo_path = "/tmp/epdiy_pc_monitor0"
fifo_path1 = "/tmp/epdiy_pc_monitor1"
input_file = ""
tty.setcbreak(sys.stdin)

try:
    os.mkfifo(fifo_path)
except OSError as oe:
    pass
try:
    os.mkfifo(fifo_path1)
except OSError as oe:
    pass


def get_nb_bytes_pad():
    global width_res
    tmp_width = width_res
    for x in range(4):
        rem = tmp_width % 32
        tmp_width += 8
        if rem == 0:
            return x


pad_bytes = get_nb_bytes_pad()
line_with_pad = int((width_res / 8) + pad_bytes)


def get_raw_pixels(image_file, file_path, save_raw_file, switcher):
    output = io.BytesIO()
    image_file.save(output, format='BMP')
    byte_string_raw = output.getvalue()
    byte_string_list[switcher] = bytearray(byte_string_raw)

    end = len(byte_string_list[switcher])
    start = end - pad_bytes

    for x in range(height_res):
        byte_string_list[switcher][start:end] = b''
        start -= line_with_pad  
        end -= line_with_pad
    byte_string_list[switcher][0:62] = b''
    
    if check_for_difference_esp:
        check_for_difference_esp_fun(byte_string_list)
    byte_frag = byte_string_list[switcher]

    if save_raw_file:
        with open(f"{working_dir}fragraw", "wb") as out:
            out.write(byte_frag)
        with open(file_path, "wb") as outfile:
            outfile.write(byte_string_list[switcher])

    return [byte_string_list[switcher], byte_string_raw, byte_string_list]

def check_for_difference_esp_fun(array_list):
    chunk_size = width_res//8
    startt = 0
    endd = chunk_size
    global dif_list_sum
    dif_list_sum = 0
    for t in range(height_res):
        if array_list[0][startt:endd] != array_list[1][startt:endd]:
            dif_list[t] = 1
            dif_list_sum += 1
            #print(f"row {t} is different")
        else:
            dif_list[t] = 0
        startt += chunk_size
        endd += chunk_size

def pipe_output_f(raw_files):
    byte_frag = raw_files

    os.write(fd0, b'\xf6')

    if check_for_difference_esp == 1:
        os.write(fd0, dif_list)
    os.write(fd0, byte_frag)
    #print(f"data sent")
    return byte_frag


def generate_cursor():
    wp = b'\xff\xff\xff\xff'
    bp = b'\x00\x00\x00\xff'
    current_line = []
    current_line_bytes = []
    global byte_array_cursor
    byte_array_cursor = []
    cursor = [[bp]]
    for y in range(12):
        current_line.append(bp)
        current_line_bytes = bp
        for l in range(y):
            current_line.append(wp)
            current_line_bytes += wp
        current_line.append(bp)
        current_line_bytes += bp
        cursor.append(current_line[:])
        current_line.clear()

    current_line = [bp, wp, wp, wp, wp, wp, wp, bp, bp, bp, bp, bp, bp]
    cursor.append(current_line[:])
    current_line.clear()
    current_line = [bp, wp, wp, wp, bp, wp, wp, bp]
    cursor.append(current_line[:])
    current_line.clear()
    current_line = [bp, wp, wp, bp, wp, bp, wp, wp, bp]
    cursor.append(current_line[:])
    current_line.clear()
    current_line = [bp, wp, bp, wp, wp, bp, wp, wp, bp]
    cursor.append(current_line[:])
    current_line.clear()
    current_line = [bp, bp, wp, wp, wp, wp, bp, wp, wp, bp]
    cursor.append(current_line[:])
    current_line.clear()
    current_line = [bp, wp, wp, bp]  # 6 spaces before , list number 18
    cursor.append(current_line[:])
    current_line.clear()
    current_line = [bp, bp]  # 7 spaces before, list number 19
    cursor.append(current_line[:])
    current_line.clear()
    for k in range(20):
        current_line2 = b"".join(cursor[k])
        byte_array_cursor.append(current_line2[:])


def draw_cursor():
    #t0 = time.time()
    global x_offset
    global y_offset
    global width_res2
    global height_res2
    pos2 = pyautogui.position()
    if pos2.x >= x_offset and pos2.x <= width_res2 and pos2.y >= y_offset and pos2.y <= height_res2:
        x_cursor = pos2.x - x_offset
        y_cursor = pos2.y - y_offset
        linear_coor = (y_cursor*width_res*4) + x_cursor*4
        for h in range(15):
            sct_img.raw[linear_coor+(h*width_res*4):linear_coor+(
                h*width_res*4)+len(byte_array_cursor[h])] = byte_array_cursor[h]
        # print(sct_img.raw[linear_coor+(h*width_res*4):linear_coor+(h*width_res*4)+len(byte_array_cursor[h])])
        h += 1
        sct_img.raw[linear_coor+(h*width_res*4):linear_coor +
                    (h*width_res*4)+16] = byte_array_cursor[h][0:16]
        offset = 16 + 4
        sct_img.raw[linear_coor+(h*width_res*4)+offset:linear_coor +
                    (h*width_res*4)+offset+16] = byte_array_cursor[h][20:36]
        h += 1
        offset = 12 + 8
        sct_img.raw[linear_coor+(h*width_res*4):linear_coor +
                    (h*width_res*4)+3*4] = byte_array_cursor[h][0:3*4]
        sct_img.raw[linear_coor+(h*width_res*4)+offset:linear_coor +
                    (h*width_res*4)+offset+16] = byte_array_cursor[h][20:36]
        h += 1
        offset = 8+16
        sct_img.raw[linear_coor+(h*width_res*4):linear_coor +
                    (h*width_res*4)+2*4] = byte_array_cursor[h][0:2*4]
        sct_img.raw[linear_coor+(h*width_res*4)+offset:linear_coor +
                    (h*width_res*4)+offset+16] = byte_array_cursor[h][24:40]
        # h+=1
        sct_img.raw[linear_coor+(h*width_res*4)+offset:linear_coor +
                    (h*width_res*4)+offset+16] = byte_array_cursor[18]
        offset += 4
        h += 1
        sct_img.raw[linear_coor+(h*width_res*4)+offset:linear_coor +
                    (h*width_res*4)+offset+8] = byte_array_cursor[19]
        # print("inside")
def check_key_presses(pid0, pid1):
    x = 0
    orig_settings = termios.tcgetattr(sys.stdin)

    while x != chr(27): # ESC
        x=sys.stdin.read(1)[0]
        if x == 'q':
            os.kill(pid1, 9)

            os.kill(pid0, 9)

            print("exiting")
        print("You pressed", x)

    termios.tcsetattr(sys.stdin, termios.TCSADRAIN, orig_settings) 


with mss.mss() as sct:
    use_bitmapf_dec = 0
    save_raw_file = 0
    open_from_disk = 0
    start_process = 1
    check_compressor = 0
    test_extractor = 0
    save_chunck_files = 0
    randomize_dots = 0
    check_for_difference = 0
    set_image_size_to_source_res = 0
    ###################
    pipe_bit_depth = 8
    pipe_output = 1
    enable_raw_output = 1
    mode = 2
    save_bmp = 0
    check_for_difference_esp = 1
    ###################
    generate_cursor()


    if pipe_output and start_process:
        R = subprocess.Popen([f'{working_dir}/process_capture'])
    pid0 = os.getpid()
    pid1 = R.pid
    thread1 = threading.Thread(target=check_key_presses, args=(pid0, pid1))
    thread1.start()

    if pipe_output:
        print("Opening pipes...")
        fd0 = os.open(fifo_path, os.O_WRONLY)
        fd1 = os.open(fifo_path1, os.O_RDONLY)
        print("Pipes opened")

    global sct_image
    complete_output_file = f'{working_dir}/image_mode_{mode}.bmp'
    raw_output_file = f"{working_dir}/image_mode_{mode}raw"
    while 1:
        t0 = time.time()
        if switcher == 0:
            switcher = 1;
        else:
            switcher = 0;
        t0 = time.time()

        ready = os.read(fd1, 1)

        sct_img = sct.grab(monitor) #capture screen

        draw_cursor()

        image_file = Image.frombytes(
            "RGB", sct_img.size, sct_img.bgra, "raw", "RGBX")

        if open_from_disk == 1:
            image_file = Image.open(f"{working_dir}{input_file}") # for testing


        if mode == 1:
            # convert image to black and white
            image_file = image_file.convert('1')
            #image_file   = image_file.rotate(90,  expand=True)
            image_file = image_file.transpose(Image.FLIP_TOP_BOTTOM)

        elif mode == 2:
            grey_to_monochrome_threshold = 200 

            def fn(x): return 255 if x > grey_to_monochrome_threshold else 0

            image_file = image_file.convert('L')

            image_file = image_file.point(fn, mode='1')

            np_image_file = np.asarray(image_file, dtype=np.uint8)

            eight_bpp_bytearr_list[switcher] = np_image_file

            image_file = image_file.transpose(Image.FLIP_TOP_BOTTOM) #flip the image so that the first bytes contain the pixel data of the first lines
            #image_file   = image_file.rotate(90,  expand=True)

        if enable_raw_output: 
            raw_files = get_raw_pixels(
                image_file, raw_output_file, save_raw_file, switcher) #remove bitmap pad bytes
        if save_bmp:
            image_file.save(complete_output_file)
        if pipe_output:  # and dif_list_sum
            if pipe_bit_depth == 1:
                byte_frag = pipe_output_f(raw_files[0])  # 1bpp->raw_files[0]
            elif pipe_bit_depth == 8:
                byte_frag = pipe_output_f(np_image_file)  # 1bpp->raw_files[0]
        print(f"capturing screen took {((time.time() - t0)*1000):0>5f}ms")
        time.sleep(sleep_time/1000)

