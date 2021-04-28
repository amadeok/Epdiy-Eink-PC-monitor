import platform

windows= None; linux = None
if platform.system() == 'Linux': linux = True; 
elif platform.system() == 'Windows': windows = True;
else: print("Unknown platform")
import os, sys
if linux:
    import termios, tty
    tty.setcbreak(sys.stdin)
elif windows:
    #import keyboard
    import msvcrt
    import win32pipe, win32file, pywintypes, win32api
    import ctypes
   #ctypes.windll.shcore.SetProcessDpiAwareness((1))    
    ret = ctypes.windll.shcore.SetProcessDpiAwareness(2)
    if ret == 0: print("Dpi awareness set correctly")
    else: print("Error settings Dpi awareness")

import mss
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



width_res = 0
height_res = 0
width_res2 = 0
height_res2 = 0
tot_nb_pixels = 0

chunk_size = 0 
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
pipe_settings = bytearray(b'\x00\x00\x00')
print_settings = 0

input_file = ""

display_list = []
display_conf = []



def create_pipes(output_pipe, input_pipe, id):
    pipe_a = "epdiy_pc_monitor_a_"
    pipe_b  = "epdiy_pc_monitor_b_"
    if linux:
        output_pipe = '/tmp/' + output_pipe
        input_pipe = '/tmp/' + input_pipe

        if os.path.exists(output_pipe) == False:    
            os.mkfifo(output_pipe)
        
        if os.path.exists(input_pipe) == False:    
            os.mkfifo(input_pipe)
        fd1 = os.open(input_pipe, os.O_RDONLY)
        fd0 = os.open(output_pipe, os.O_WRONLY)
        return fd1, fd0
    elif windows:
        quit = 0
        output_pipe =  r'\\.\pipe\epdiy_pc_monitor_a_' + str(id)
        input_pipe =  r'\\.\pipe\epdiy_pc_monitor_b_' + str(id)


        while not quit:
            try:
                fd1 = win32file.CreateFile( input_pipe, win32file.GENERIC_READ | win32file.GENERIC_WRITE,  0,  None, win32file.CREATE_NEW,  0,  None)
                #res = win32pipe.SetNamedPipeHandleState(fd1, win32pipe.PIPE_READMODE_MESSAGE, None, None)
                
            except pywintypes.error as e:
                if e.args[0] == 2:
                    print("No Input pipe, trying again in a sec")
                    time.sleep(1)
                continue
            quit = 1
        print(f"Python capture ID {id}: Input pipe opened")

        mode = win32pipe.PIPE_TYPE_MESSAGE | win32pipe.PIPE_READMODE_MESSAGE | win32pipe.PIPE_WAIT
        fd0 = win32pipe.CreateNamedPipe( output_pipe, win32pipe.PIPE_ACCESS_DUPLEX, mode, 1, 65536*16, 65536*16, 0, None)
        ret = win32pipe.ConnectNamedPipe(fd0, None)
        if ret != 0:
            print("error fd0", win32api.GetLastError())
        print(f'Python capture ID {id}: Output pipe opened')
        return fd1, fd0


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
    
    if check_for_difference_esp:# and pseudo_greyscale_mode == 0:
        check_for_difference_esp_fun(byte_string_list, 1)
    byte_frag = byte_string_list[switcher]

    if save_raw_file:
        with open(f"{working_dir}fragraw", "wb") as out:
            out.write(byte_frag)
        with open(file_path, "wb") as outfile:
            outfile.write(byte_string_list[switcher])

    return [byte_string_list[switcher], byte_string_raw, byte_string_list]


def check_for_difference_esp_fun(array_list, bit_depth):
    if bit_depth == 8:
        divider = 1
    elif bit_depth == 1: divider = 8
    
    chunk_size = width_res//divider
    startt = 0
    endd = chunk_size

    global dif_list_sum
    dif_list_sum = 0
    #t0 = time.time()

    for t in range(height_res):
        if bit_depth == 8:
            arr = array_list[0][t]
            arr2 = array_list[1][t]
            ret = np.array_equal(arr, arr2)
            if ret == False:
                dif_list[t+1] = 1
                dif_list_sum += 1
                #print(f"row {t} is different")
            else:
                dif_list[t+1] = 0
                #print(f"not different")
        elif bit_depth == 1:
            if array_list[0][startt:endd] != array_list[1][startt:endd]:
                try:   
                    dif_list[t+1] = 1; 
                except: pass
                dif_list_sum += 1
                #print(f"row {t} is different")
            else:
                try: 
                    dif_list[t+1] = 0; 
                except: pass
        startt += chunk_size
        endd += chunk_size
    #print("check dif took", time.time() - t0)

def pipe_output_f(raw_files, np_image_file, mouse_moved):
    byte_frag = raw_files[0]
    if child_process ==  0:
        end_val = exiting
    else: end_val = shared_buffer[0]

    if end_val == 101:

        if linux: os.write(fd0, shared_buffer[0:2])
        elif windows: win32file.WriteFile(fd0, shared_buffer[0:2])

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

    pipe_settings[1] = mouse_moved

    pipe_settings[2] = pseudo_greyscale_mode

    if linux: os.write(fd0, pipe_settings)
    elif windows: win32file.WriteFile(fd0, pipe_settings)

    if check_for_difference_esp == 1:
        check_for_difference_esp_fun(raw_files[2], 1)

        if linux: os.write(fd0, dif_list)
        elif windows: win32file.WriteFile(fd0, dif_list)

    if linux: os.write(fd0, byte_frag)
    elif windows: win32file.WriteFile(fd0, byte_frag)
    # if display_list[0].improve_dither:
    #     ready = os.read(fd1, 1)
    #     os.write(fd0, pipe_settings)
    #     os.write(fd0, np_image_file)

    #print(f"data sent")
    return byte_frag

def write_to_shared_mem(obj, increase, type):
    obj.value += increase
    if type == 'f':
        shared_buffer[obj.pos:obj.pos+4] = float_to_bytearray(obj.value)    
    elif type == 'i':
        shared_buffer[obj.pos:obj.pos+4] = obj.value.to_bytes(4,  byteorder = 'big', signed=True)
    elif type == 'a':
        shared_buffer[obj.pos:obj.pos+4] = increase.to_bytes(4,  byteorder = 'big', signed=True)

def get_val_from_shm(obj, type):
    if type == 'f':
        return round((bytearray_to_float(shared_buffer[obj.pos:obj.pos+4]))[0], 1)
    elif type == 'i':
        return int.from_bytes(shared_buffer[obj.pos:obj.pos+4], byteorder='big', signed=True)



def check_key_presses(PID_list, conf):
    x = 0
    if linux:
        orig_settings = termios.tcgetattr(sys.stdin)

    global exiting; global print_settings
    while 1 and exiting == 0:  # ESC
        if linux:
            x = sys.stdin.read(1)[0]
        elif windows:
            x = msvcrt.getch().decode('UTF-8')
        if x == 'q' or x == 'Q':
            print("Exiting")
            if has_childs == 1:
                shared_buffer[0] = 101
            exiting = 101
            time.sleep(5)
            try:
                for v in range(len(PID_list)-1, 0, -1):
                    if PID_list[v] != None:
                        os.kill(PID_list[v], 9)
                os.kill(PID_list[0], 9)
            except:
                pass
        elif x == 'm' or x == 'M':
            if get_val_from_shm(conf.pseudo_greyscale_mode, 'i') == 1:
                write_to_shared_mem(conf.pseudo_greyscale_mode, -1, 'i')
                print("Pseudo greyscale mode is off ", get_val_from_shm(conf.pseudo_greyscale_mode, 'i'))
            else:
                write_to_shared_mem(conf.pseudo_greyscale_mode, +1, 'i')
                print("Pseudo greyscale mode is on ", get_val_from_shm(conf.pseudo_greyscale_mode, 'i'))
        elif x == 'i' or x == 'I':
            if get_val_from_shm(conf.invert, 'i') == 1:
                write_to_shared_mem(conf.invert, -1, 'i')
                print("Invert is off ", get_val_from_shm(conf.invert, 'i'))
            else:
                write_to_shared_mem(conf.invert, +1, 'i')
                print("Invert is on ", get_val_from_shm(conf.invert, 'i'))
        elif x == '1':
            write_to_shared_mem(conf.color, -0.1, 'f')
        elif x == '2':
            write_to_shared_mem(conf.color, +0.1, 'f')
        elif x == '3':
            write_to_shared_mem(conf.contrast, -0.1, 'f')
        elif x == '4':
            write_to_shared_mem(conf.contrast, +0.1, 'f')
        elif x == '5':
            write_to_shared_mem(conf.brightness, -0.1, 'f')
        elif x == '6':
            write_to_shared_mem(conf.brightness, +0.1, 'f')
        elif x == '7':
            write_to_shared_mem(conf.sharpness, -0.1, 'f')
        elif x == '8':
            write_to_shared_mem(conf.sharpness, +0.1, 'f')
        elif x == '9':
            write_to_shared_mem(conf.grey_to_monochrome_threshold, -10, 'i')
        elif x == '0':
            write_to_shared_mem(conf.grey_to_monochrome_threshold, +10, 'i')
        elif x == 'b' or x == 'B':
            if get_val_from_shm(conf.enhance_before_greyscale, 'i') == 1:
                write_to_shared_mem(conf.enhance_before_greyscale, -1, 'i')
                print("enhance_before_greyscale is off ", get_val_from_shm(conf.enhance_before_greyscale, 'i'))
            else:
                write_to_shared_mem(conf.enhance_before_greyscale, +1, 'i')
                print("enhance_before_greyscale is on ", get_val_from_shm(conf.enhance_before_greyscale, 'i'))
        
        else: print("No keybind for", x)
        
        try: 
            n = int(x); 
            if n >= 0 and n <= 9:
                print_settings = 1
                #print(f"color {get_val_from_shm(conf.color, 'f')}, contrast  {get_val_from_shm(conf.contrast, 'f')}  brightness {get_val_from_shm(conf.brightness, 'f')}, sharpness {get_val_from_shm(conf.sharpness, 'f')},  grey_to_monochrome_threshold {get_val_from_shm(conf.grey_to_monochrome_threshold, 'i')}")
        except: pass
        
    if linux:
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, orig_settings)

class volatile_settings:
    def __init__():
        pass

class display_settings:
    def __init__(self, settings_list):

        d = {}
        for y in range(len(settings_list)):
            d[settings_list[y][0]] = settings_list[y][1]

        def get_val(string, type):
            if type == 'i':
                return int(d[string+':'])
            elif type == 'f':
                return float(d[string+':'])
                
        self.ip_address = d['ip_address:']
        self.display_id = get_val('id', 'i')
        self.width = get_val('width', 'i')
        self.height = get_val('height', 'i')
        self.x_offset = get_val('x_offset', 'i')
        self.y_offset = get_val('y_offset', 'i')
        self.rotation = get_val('rotation', 'i')
        self.grey_to_monochrome_threshold = get_val('grey_monochrome_threshold', 'i')

        self.sleep_time= get_val('sleep_time', 'i')
        self.refresh_every_x_frames =get_val('refresh_every_x_frames', 'i')
        self.framebuffer_cycles = get_val('framebuffer_cycles', 'i')
        self.rmt_high_time= get_val('rmt_high_time', 'i')
        self.enable_skipping =  get_val('enable_skipping', 'i')
        self.epd_skip_threshold =  get_val('epd_skip_threshold', 'i')
        self.epd_skip_mouse_only = get_val('epd_skip_mouse_only', 'i')
        self.framebuffer_cycles_2 =  get_val('framebuffer_cycles_2', 'i')
        self.framebuffer_cycles_2_threshold =  get_val('framebuffer_cycles_2_threshold', 'i')
        self.pseudo_greyscale_mode =  get_val('pseudo_greyscale_mode', 'i')
        self.color =  get_val('color', 'f')
        self.contrast =  get_val('contrast', 'f')
        self.brightness = get_val('brightness', 'f')
        self.sharpness =get_val('sharpness', 'f')
        self.invert = get_val('invert', 'i')
        self.enhance_before_greyscale =  get_val('enhance_before_greyscale', 'i')

        self.selective_compression =get_val('selective_compression', 'i')
        self.nb_chunks =  get_val('nb_chunks', 'i')
        self.do_full_refresh = get_val('do_full_refresh', 'i')

        self.disable_logging = get_val('disable_logging', 'i')

        self.width_res2 = self.width + self.x_offset
        self.height_res2 = self.height + self.y_offset
        self.conf_type = 'read_from_file'


def float_to_bytearray(float):
    return bytearray(struct.pack("f", float))
def bytearray_to_float(bytearr):
    return struct.unpack('f', bytearr)   

class offset_object:
    def __init__(self, byte_position, value):
        self.pos = byte_position
        self.value = value
    def round(self):
        self.value = round(self.value, 1)
class shared_var:
    def __init__(self):
        self.pseudo_greyscale_mode = offset_object(10, 0)
        self.color =  offset_object(14, 0)
        self.contrast =  offset_object(18, 0)
        self.brightness =  offset_object(22, 0)
        self.sharpness =  offset_object(26, 0)
        self.enhance_before_greyscale = offset_object(30, 0)
        self.grey_to_monochrome_threshold =   offset_object(34, 0)
        self.invert =   offset_object(38, 0)



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
    display_conf.append(['disable_logging:', disable_logging])
    generate_display_class(display_conf)
    
def write_initial_settings(shared, conf):
    write_to_shared_mem(shared.pseudo_greyscale_mode, conf.pseudo_greyscale_mode, 'a')
    write_to_shared_mem(shared.color, conf.color, 'a')
    write_to_shared_mem(shared.contrast, conf.contrast, 'a')
    write_to_shared_mem(shared.brightness, conf.brightness, 'a')
    write_to_shared_mem(shared.sharpness, conf.sharpness, 'a')
    write_to_shared_mem(shared.enhance_before_greyscale, conf.enhance_before_greyscale, 'a')
    write_to_shared_mem(shared.grey_to_monochrome_threshold, conf.grey_to_monochrome_threshold, 'a')
    write_to_shared_mem(shared.invert, conf.invert, 'a')


def apply_enhancements(image_file, conf, offsets):
    #t0 = time.time()
    global print_settings
    val0 = get_val_from_shm(offsets.color, 'f')
    if conf.color + val0 != 1.0:
        enhancer = ImageEnhance.Color(image_file)
        image_file = enhancer.enhance(conf.color + val0)     

    val1 = get_val_from_shm(offsets.contrast, 'f')
    if conf.contrast + val1  != 1.0:
        enhancer = ImageEnhance.Contrast(image_file)
        image_file = enhancer.enhance(conf.contrast + val1)

    val2 = get_val_from_shm(offsets.brightness, 'f')
    if conf.brightness + val2 != 1.0:
        enhancer = ImageEnhance.Brightness(image_file)
        image_file = enhancer.enhance(conf.brightness + val2)

    val3 = get_val_from_shm(offsets.sharpness, 'f')
    if conf.sharpness + val3 != 1.0:
        enhancer = ImageEnhance.Sharpness(image_file)
        image_file = enhancer.enhance(conf.sharpness + val3)
    
    if print_settings and child_process == 0:
        print(f"color {conf.color + val0}, contrast  {conf.contrast + val1}  brightness {conf.brightness + val2}, sharpness {conf.sharpness + val3},  grey_to_monochrome_threshold {conf.grey_to_monochrome_threshold + get_val_from_shm(offsets.grey_to_monochrome_threshold, 'i')}")
        print_settings = 0

    #print("enhance took", time.time() - t0 )

    return image_file


def convert_to_greyscale_and_enhance(image_file, conf, offset_variables):
    if get_val_from_shm(offset_variables.enhance_before_greyscale, 'i'):
        image_file = apply_enhancements(image_file, conf, offset_variables)

    image_file = image_file.convert('L')
    if get_val_from_shm(offset_variables.invert, 'i')  == 1:
        image_file = ImageOps.invert(image_file)
    
    if get_val_from_shm(offset_variables.enhance_before_greyscale, 'i')  == 0:
        image_file = apply_enhancements(image_file, conf, offset_variables)
    return image_file

child_process = 0
with mss.mss() as sct:
    args = str(sys.argv)
    
    disable_logging = '0'
    has_childs = 0
    disable_wifi = 0
    common = 0
    try:
        ret = sys.argv.remove("-silent")
        disable_logging = '1'
    except: pass
    try:
        ret = sys.argv.remove("child")
        child_process = 1
    except: pass
    try:
        ret = sys.argv.remove("-disable_wifi")
        disable_wifi = 1
    except: pass
    try:
        ret = sys.argv.remove("-common")
        common = 1
    except: pass
    nb_arg = len(sys.argv)
    nb_displays = nb_arg -1



    for x in range(nb_displays):
        get_display_settings(sys.argv[x+1], disable_logging)
    
    ip_address = display_list[0].ip_address
    width_res = display_list[0].width
    height_res = display_list[0].height
    x_offset = display_list[0].x_offset                      
    y_offset = display_list[0].y_offset
    rotation =  display_list[0].rotation
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
    pseudo_greyscale_mode = display_list[0].pseudo_greyscale_mode
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

    global shared_buffer
    if common == 1 or child_process or nb_displays >1:
        try: 
            shm_a = shared_memory.SharedMemory(create=True, size=100, name='screen_capture_shm')
        except: 
            shm_a = shared_memory.SharedMemory(name='screen_capture_shm')
    else:
        randi = randint(0, 99000)
        try: 
            shm_a = shared_memory.SharedMemory(create=True, size=100, name=f'screen_capture_shm{randi}')
        except: 
            shm_a = shared_memory.SharedMemory(name=f'screen_capture_shm{randi}')
    shared_buffer = shm_a.buf

    ba = float_to_bytearray(1.23)

    global offset_variables
    offset_variables = shared_var()

    #write_initial_settings(offset_variables, display_list[0])

    if disable_wifi:
        wifi_on = 0
    else: wifi_on = 1

    nb_arg = len(sys.argv)
    for u in range(nb_displays-1):
        if disable_logging:         
            P = subprocess.Popen([f'python',  'screen_capture.py',  f'{sys.argv[u+2]}', '-silent', 'child'])
        else:
            P = subprocess.Popen([f'python3',  'screen_capture.py',  sys.argv[u+2], 'child'])
        PID_list.append(P.pid)
        time.sleep(0.5)


    if child_process == 0:

        if pipe_output and start_process:
            shared_buffer[0:100] = bytearray(100)
            if windows:
                binary = "process_capture.exe"
            elif linux:
                binary = "process_capture"
            for x in range(nb_displays):  
                time.sleep(0.5)

                R = subprocess.Popen([f'{working_dir}/{binary}', 
                f'{display_list[x].ip_address}',
                f'{display_list[x].display_id}',
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
                f'{display_list[x].pseudo_greyscale_mode}',
                f'{display_list[x].selective_compression}',

                f'{display_list[x].nb_chunks}', 
                f'{display_list[x].do_full_refresh}', 

                f'{display_list[x].disable_logging}',
                f'{wifi_on}'])

                has_childs = 1

                PID_list.append(R.pid)
        else:   
            pid1 = None



    

    conf = display_list[0]

    if child_process == 0:
        thread1 = threading.Thread(target=check_key_presses, args=(PID_list, offset_variables))
        thread1.start()
    #for x in range 

    if pipe_output:   
        output_pipe = f"epdiy_pc_monitor_a_{display_id}"
        input_pipe = f"epdiy_pc_monitor_b_{display_id}"
        if linux:
            create_pipes(output_pipe, input_pipe, display_id)
            print("Opening pipes...")
            fd1 = os.open('/tmp/' + input_pipe, os.O_RDONLY)
            fd0 = os.open('/tmp/' + output_pipe, os.O_WRONLY)
            print("Pipes opened")
        elif windows:
            fd1, fd0 = create_pipes(output_pipe, input_pipe, display_id)
            output_pipe = r"\\.\pipe\epdiy_pc_monitor_a_0" 
            #output_pipe += str(display_id)


    global sct_image
    complete_output_file = f'{working_dir}/image_mode_.bmp'
    raw_output_file = f"{working_dir}/image_mode_raw"

    while 1:
        t0 = time.time()
        if switcher == 0:
            switcher = 1;
        else:
            switcher = 0;
        t0 = time.time()

        if linux: ready = os.read(fd1, 1)
        elif windows: ret2 = win32file.ReadFile(fd1, 1)

        sct_img = sct.grab(monitor) #capture screen

        #mouse_moved = draw_cursor(display_list[0], sct_img)
        image_file = Image.frombytes(
            "RGB", sct_img.size, sct_img.bgra, "raw", "RGBX")

        if open_from_disk == 1:
            image_file = Image.open(f"{working_dir}{input_file}") # for testing
        
        if get_val_from_shm(offset_variables.pseudo_greyscale_mode, 'i') == 1:
            
            if rotation != 0:
                image_file   = image_file.rotate(rotation,  expand=True)
            
            image_file = convert_to_greyscale_and_enhance(image_file, conf, offset_variables)

                
            image_file = image_file.transpose(Image.FLIP_TOP_BOTTOM)


            image_file = image_file.convert('1')


        else:

            image_file = convert_to_greyscale_and_enhance(image_file, conf, offset_variables)
            th = conf.grey_to_monochrome_threshold+get_val_from_shm(offset_variables.grey_to_monochrome_threshold, 'i')
            #print(f"id {display_id} th {th}")
            def fn(x): return 255 if x > th else 0

            image_file = image_file.point(fn, mode='1')

            #np_image_file = np.asarray(image_file, dtype=np.uint8)

            #eight_bpp_bytearr_list[switchernamed pipe windows error 2] = np_image_file

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
                mouse_moved = draw_cursor_1bpp(display_list[0], raw_files[0])
                
                byte_frag = pipe_output_f(raw_files, np_image_file, mouse_moved)  # 1bpp->raw_files[0]
            elif pipe_bit_depth == 8:
                byte_frag = pipe_output_f(np_image_file, np_image_file, mouse_moved)  # 1bpp->raw_files[0]
        if disable_logging == '0':
            took = int(((time.time() - t0)*1000))
            if took > 1000:
                print("mroe than")
            print(f"Display ID: {display_id}, capture took {took}ms")
        time.sleep(sleep_time/1000)

