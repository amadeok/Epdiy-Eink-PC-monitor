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
from multiprocessing import shared_memory, resource_tracker


width_res = 0
height_res = 0
width_res2 = 0
height_res2 = 0
tot_nb_pixels = 0

chunk_size = 0
# monitor = {"top": y_offset, "left": x_offset,
#            "width": width_res, "height": height_res} 

working_dir = os.getcwd()

first_white_arr_array = np.full((tot_nb_pixels), 255, dtype=np.uint8)
np_image_file = np.full((tot_nb_pixels), 0, dtype=np.uint8)
eight_bpp_bytearr_list = [first_white_arr_array, np_image_file]
byte_string_list = [bytearray([1] * 1*1), bytearray(b'\x00')]

dif_list = bytearray(height_res)
dif_list_sum = 0
switcher = 0
pad_bytes = 0
exiting = 0

input_file = ""
tty.setcbreak(sys.stdin)

def create_pipes(output_pipe, input_pipe):
    try:
        os.mkfifo(output_pipe)
    except OSError as oe:
        pass
    try:
        os.mkfifo(input_pipe)
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
    if child_process ==  0:
        end_val = exiting
    else: end_val = shared_buffer[0]

    if end_val == 101:
        os.write(fd0, shared_buffer[0:2])
        time.sleep(0.5)
        if child_process == 0:
            time.sleep(1.5)

            try: shm_a.unlink()
            except: pass
        else:
            try: resource_tracker.unregister(shm_a._name, 'shared_memory')
            except: pass
            shm_a.close()
        sys.exit(f'Python capture ID {display_id} terminated')
    else:
        os.write(fd0, b'\xf6\x00')


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
        current_line.append(bp)

        current_line_bytes = bp
        current_line_bytes += bp

        for l in range(y-2):
            current_line.append(wp)
            current_line_bytes += wp
        current_line.append(bp)
        current_line.append(bp)

        current_line_bytes += bp
        current_line_bytes += bp

        cursor.append(current_line[:])
        current_line.clear()

    current_line = [bp, bp, wp, wp, wp, wp, wp, bp, bp, bp, bp, bp, bp]
    cursor.append(current_line[:])
    current_line.clear()
    current_line = [bp, bp, wp, wp, bp, wp, bp, bp]
    cursor.append(current_line[:])
    current_line.clear()
    current_line = [bp, bp, wp, bp, wp, bp, wp, bp, bp]
    cursor.append(current_line[:])
    current_line.clear()
    current_line = [bp, bp, bp, wp, wp, bp, wp, bp, bp]
    cursor.append(current_line[:])
    current_line.clear()
    current_line = [bp, bp, wp, wp, wp, wp, bp, wp, bp, bp]
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
def check_key_presses(PID_list):
    x = 0
    orig_settings = termios.tcgetattr(sys.stdin)
    global exiting
    
    while 1 and exiting == 0:  # ESC
        x = sys.stdin.read(1)[0]
        if x == 'q':
            print("exiting")
            if has_childs == 1:
                shared_buffer[0] = 101
            exiting = 101
            # for v in range(len(PID_list)-1, 0, -1):
            #     if PID_list[v] != None:
            #         os.kill(PID_list[v], 9)
            # os.kill(PID_list[0], 9)

        print("You pressed", x)

    termios.tcsetattr(sys.stdin, termios.TCSADRAIN, orig_settings)

class display_settings:
    def __init__(self, settings_list):
        self.ip_address = settings_list[0][1]
        self.display_id = int(settings_list[1][1])
        self.width = int(settings_list[2][1])
        self.height = int(settings_list[3][1])
        self.x_offset = int(settings_list[4][1])
        self.y_offset = int(settings_list[5][1])
        self.rotation = int(settings_list[6][1])
        self.grey_to_monochrome_threshold =  int(settings_list[7][1]) 
        self.sleep_time= int(settings_list[8][1]) 
        self.refresh_every_x_frames = int(settings_list[9][1])
        self.framebuffer_cycles = int(settings_list[10][1])
        self.rmt_high_time= int(settings_list[11][1])
        self.enable_skipping =  int(settings_list[12][1])
        self.epd_skip_threshold =  int(settings_list[13][1])
        self.framebuffer_cycles_2 =  int(settings_list[14][1])
        self.framebuffer_cycles_2_threshold =  int(settings_list[15][1])
        self.nb_chunks =  int(settings_list[16][1])

        self.disable_logging = settings_list[len(settings_list)-1]

# display_index = 0
display_list = []
display_conf = []
def generate_display_class(display_conf):
    display_list.append(display_settings(display_conf))

def get_display_settings(conf_file, disable_logging):
    #global display_index
    display_conf.clear()
    ins = open(f'{working_dir}/{conf_file}', "r")
    for line in ins:
        number_strings = line.split()  # Split the line on runs of whitespace
        #numbers = [int(n) for n in number_strings]  # Convert to integers
        if line != '' and line != '\n':
            display_conf.append(number_strings)  # Add the "row" to your list.
    display_conf.append(disable_logging)
    generate_display_class(display_conf)
    

with mss.mss() as sct:
    args = str(sys.argv)
    
    disable_logging = 0
    child_process = 0
    has_childs = 0
    try:
        ret = sys.argv.remove("-silent")
        disable_logging = 1
    except: pass
    try:
        ret = sys.argv.remove("child")
        child_process = 1
    except: pass
    nb_arg = len(sys.argv)
    nb_displays = nb_arg -1

    # for x in range(nb_arg-1):
    #     if sys.argv[x] == "-silent":
    #         disable_logging = 1
    #         ret = sys.argv.remove("-silent")
    #     if sys.argv[x] == "child":
    #         child_process = 1
    #         ret = sys.argv.remove("child")
    for x in range(nb_displays):
        get_display_settings(sys.argv[x+1], disable_logging)
    
    ip_address = display_list[0].ip_address
    width_res = display_list[0].width
    height_res = display_list[0].height
    x_offset = display_list[0].x_offset                      
    y_offset = display_list[0].y_offset
    rotation =  display_list[0].rotation
    grey_to_monochrome_threshold =  display_list[0].grey_to_monochrome_threshold
    sleep_time = display_list[0].sleep_time
    display_id = display_list[0].display_id
    framebuffer_cycles = display_list[0].framebuffer_cycles
    rmt_high_time =  display_list[0].rmt_high_time
    epd_skip_threshold =  display_list[0].epd_skip_threshold
    framebuffer_cycles_2 = display_list[0].framebuffer_cycles_2
    framebuffer_cycles_2_threshold = display_list[0].framebuffer_cycles_2_threshold
    nb_chunks =  display_list[0].nb_chunks
    #disable_logging
    global line_with_pad
    pad_bytes = get_nb_bytes_pad()
    line_with_pad = int((width_res / 8) + pad_bytes)
    width_res2 = width_res + x_offset
    height_res2 = height_res + y_offset
    tot_nb_pixels = height_res * width_res
    chunk_size = int(width_res*height_res/8/8)
    monitor = {"top": y_offset, "left": x_offset,
            "width": width_res, "height": height_res} 
    first_white_arr_array = np.full((tot_nb_pixels), 255, dtype=np.uint8)
    np_image_file = np.full((tot_nb_pixels), 0, dtype=np.uint8)

    eight_bpp_bytearr_list = [first_white_arr_array, np_image_file]

    byte_string_list = [bytearray([1] * 1*1), bytearray(b'\x00')]

    dif_list = bytearray(height_res)
                  
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
    pipe_bit_depth = 1
    pipe_output = 1
    enable_raw_output = 1
    mode = 2
    save_bmp = 0
    check_for_difference_esp = 1
    ###################
    generate_cursor()
    try:
        os.chdir(f'{working_dir}/pc_host_app')
    except:pass
    PID_list = []
    working_dir = os.getcwd()
    pid0 = os.getpid()
    PID_list.append(pid0)

    # global shared_buffer
    try: shm_a = shared_memory.SharedMemory(create=True, size=10, name='screen_capture_shm')
    except: shm_a = shared_memory.SharedMemory(name='screen_capture_shm')
    shared_buffer = shm_a.buf

    if pipe_output and start_process and child_process == 0:
        shared_buffer[:10] = bytearray([0, 0, 0, 0, 0, 0, 0, 0, 0, 0]) 
        buffer2 = bytearray(shared_buffer)
        for x in range(nb_displays):  
            R = subprocess.Popen([f'{working_dir}/process_capture', 
             f'{display_list[x].ip_address}',
              f'{display_list[x].display_id}',
              f'{display_list[x].width}',
              f'{display_list[x].height}',
              f'{display_list[x].refresh_every_x_frames}',
              f'{display_list[x].framebuffer_cycles}', 
              f'{display_list[x].rmt_high_time}',
              f'{display_list[x].enable_skipping}',
              f'{display_list[x].epd_skip_threshold}',
              f'{display_list[x].framebuffer_cycles_2}',
              f'{display_list[x].framebuffer_cycles_2_threshold}',
              f'{display_list[x].nb_chunks}', 
              f'{display_list[x].disable_logging}'])
            time.sleep(0.5)
            has_childs = 1

            PID_list.append(R.pid)

    else:   
        pid1 = None


    nb_arg = len(sys.argv)
    for u in range(nb_displays-1):
        if disable_logging:         
            P = subprocess.Popen([f'python3',  'screen_capture.py',  f'{sys.argv[u+2]}', '-silent', 'child'])
        else:
            P = subprocess.Popen([f'python3',  'screen_capture.py',  sys.argv[u+2], 'child'])
        PID_list.append(P.pid)
        time.sleep(0.5)

        
    if child_process == 0:
        thread1 = threading.Thread(target=check_key_presses, args=(PID_list,))
        thread1.start()
    #for x in range 

    if pipe_output:
        output_pipe = f"/tmp/epdiy_pc_monitor_a_{display_id}"

        input_pipe = f"/tmp/epdiy_pc_monitor_b_{display_id}"
        create_pipes(output_pipe, input_pipe)

        print("Opening pipes...")
        fd1 = os.open(input_pipe, os.O_RDONLY)

        fd0 = os.open(output_pipe, os.O_WRONLY)
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
            if rotation != 0:
                image_file   = image_file.rotate(90,  expand=True)
            image_file = image_file.transpose(Image.FLIP_TOP_BOTTOM)

        elif mode == 2:

            def fn(x): return 255 if x > grey_to_monochrome_threshold else 0

            image_file = image_file.convert('L')

            image_file = image_file.point(fn, mode='1')

            np_image_file = np.asarray(image_file, dtype=np.uint8)

            eight_bpp_bytearr_list[switcher] = np_image_file

            image_file = image_file.transpose(Image.FLIP_TOP_BOTTOM) #flip the image so that the first bytes contain the pixel data of the first lines
            if rotation != 0:
                image_file   = image_file.rotate(rotation,  expand=True)

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
        if disable_logging == 0:
            print(f"Display ID: {display_id}, capture took {int(((time.time() - t0)*1000))}ms")
        time.sleep(sleep_time/1000)

