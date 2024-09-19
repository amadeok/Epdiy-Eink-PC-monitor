import platform, ctypes
import configparser
import json, cv2

from numpy.lib.type_check import imag

windows= None; linux = None
if platform.system() == 'Linux': linux = True; 
elif platform.system() == 'Windows': windows = True;
else: print("Unknown platform")
if linux:
    import termios, tty,sys
    tty.setcbreak(sys.stdin)
elif windows:
    #import keyboard
    import msvcrt
    import win32pipe, win32file, pywintypes, win32api
   #ctypes.windll.shcore.SetProcessDpiAwareness((1))    
    ret = ctypes.windll.shcore.SetProcessDpiAwareness(2)
    if ret == 0: print("Dpi awareness set correctly")
    else: print("Error settings Dpi awareness")

import os, numpy as np, time, io, struct, sys
from collections import namedtuple
from PIL import Image, ImageChops, ImageEnhance, ImageOps
from multiprocessing import shared_memory, resource_tracker, Value

from enum import Enum

class withCv2Enum(Enum):
    NONE = 0
    PYTHON = 1
    CPP = 2
    BOTH = 3
    
working_dir = os.getcwd()

exiting = 0
settings_changed = 0

display_list = []
display_conf = []
quit_all  = False
save_raw_file = 0
open_from_disk = 0
save_chunck_files = 0
pipe_output = 1
enable_raw_output = 1
save_bmp = 0
check_for_difference_esp = 1
working_dir = os.getcwd()
raw_output_file = f"{working_dir}/image_mode_raw"
t_counter = 0;t0 = 0

def t(text=None):
    global t_counter;global t0
    t_counter+=1
    if t_counter == 1:
        t0 = time.time()
    elif t_counter == 2:
        t_counter = 0
        print(f"{text} {time.time()-t0}")
    return time.time()

modes =  {
    "monochrome" : 0,    "Bayer16" : 1,    "Bayer8" : 2,    "Bayer4" : 3,    "Bayer3" : 4,
    "Bayer2" : 5,     "FS" : 6,     "SierraLite" : 7, 	     "Sierra" : 8, 
    # "PIL_dither" : 9,
    #"4grayscale": 10
}

polarize_modes =  {
    "off" : 0,    "rgb" : 1,    "L" : 2
}

def get_mode(dict, mode_code):
    return [k for k,v in dict.items() if v == mode_code][0]

def eval_args(self):
    args = str(sys.argv)
    if "silent" in sys.argv:
        self.disable_logging = 1; sys.argv.remove("silent")
    else: self.disable_logging = 0
    if "child" in sys.argv:
        self.child_process = 1; sys.argv.remove("child")
    else: self.child_process = 0
    if "nokeychecker" in sys.argv:
        self.nokeychecker = 1; sys.argv.remove("nokeychecker")
    else: self.nokeychecker = 0
    if "dw" in sys.argv:
        self.disable_wifi = 1; sys.argv.remove("dw")
    else: self.disable_wifi = 0
    if "common" in sys.argv: 
        self.common = 1; sys.argv.remove("common")
    else: self.common = 0
    if "dp" in sys.argv: 
        self.start_cpp_process = 0; sys.argv.remove("dp")
    else: self.start_cpp_process  = 1
    for i, a in enumerate(sys.argv):
        if "override_ip" in a:
            self.overr_ip_address = a.split(":")[1]
            del sys.argv[i]
            break

    nb_arg = len(sys.argv)
    nb_displays = nb_arg -1
    return nb_displays


def apply_pole_method_grayscale(ctx, opencv_image):
    ctx.np_arr = np.ravel(opencv_image)
    dith.polarize_(ctx.np_arr, ctx.pole_factor,ctx.pole_pivot, ctx.tot_nb_pixels)
    opencv_image = ctx.np_arr.reshape(opencv_image.shape)
    return opencv_image

def apply_pole_menthod_rgb(ctx, opencv_image):
    opencv_image = cv2.cvtColor(opencv_image, cv2.COLOR_RGBA2BGR)
    ctx.np_arr = np.ravel(opencv_image)
    dith.polarize_(ctx.np_arr, ctx.pole_factor, ctx.pole_pivot, ctx.tot_nb_pixels*3)
    opencv_image = ctx.np_arr.reshape(opencv_image.shape)
    return opencv_image

def setup_shared_memory(self):
    from random import randint
    nb_displays = len(sys.argv) -1

    if self.a.common == 1 or self.a.child_process or nb_displays >1:
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
    self.shm_a = shm_a
    self.shared_buffer = shm_a.buf

    self.offsets = shared_var()

def parse_value(value:str):
    try:
        return int(value)
    except ValueError:
        try:
            return float(value)
        except ValueError:
            try:
                value= value.replace("\n", "")
                value= value.replace("\r", "")
                return  json.loads(value)
            except:
                return value
        
class args_eval:
    def __init__(self):
        self.overr_ip_address = None
        pass
class display_settings(object):
    def get_dith(self):
        pass
    def check_resize(self):

        if isinstance(self.resize_w, int) and isinstance(self.resize_h, int):

            if self.width / self.height != self.resize_w / self.resize_h:
                print(self.log, "Invalid resize setting")
                sys.exit()
        else: print(self.log, "Not resizing"); return -1
        print(self.log, "Resize resolution: ", self.resize_w, self.resize_h)

    def __init__(self, names, args, configuration_file):
        
        # with open(configuration_file, 'r') as file:
        #     data = json.load(file)
        #     for element in data:
        #         # Print each element
        #         setattr(self, name, parse_value(value))

        config = configparser.ConfigParser()
        config.read(configuration_file)
        for section_name in config.sections():
            for name, value in config.items(section_name):
                print( '  %s = %s' % (name, value))
                setattr(self, name, parse_value(value))
                
        try:  getattr(self, "with_cv2")
        except:  self.with_cv2 = 0
        
        def generate_sequence(start, end, num_elements):
            if num_elements == 1:  return [start]
            step = (end - start) / (num_elements - 1)
            sequence = [int(start + i * step) for i in range(num_elements)]
            return sequence

        def generate_sequence2(start, end, num_elements):
            s1 = generate_sequence(start, end, num_elements)
            s2 = generate_sequence(end, start, num_elements)
            return s1 + s2[1:-1] + [start]
        
        
        self.a = args
        self.width_res2 = self.width + self.x_offset
        self.height_res2 = self.height + self.y_offset
        self.conf_type = 'read_from_file'

        self.has_childs = 0

        self.pad_bytes = get_nb_bytes_pad(self)
        self.line_with_pad = int((self.width / 8) + self.pad_bytes)

        self.tot_nb_pixels = self.height * self.width
        self.chunk_size = int(self.width*self.height/8/8)
        self.monitor = {"top": self.y_offset, "left": self.x_offset,  "width": self.width, "height": self.height} 

        self.byte_string_list = [bytearray([1] * 1*1), bytearray(b'\x00')]

        self.switcher = 0

        self.configuration_file = configuration_file
        self.log = f"Python ID {self.id}: " 
        self.complete_output_file = f'{working_dir}/image_id_{self.id}.bmp'
        self.settings_dither = 0
        self.np_arr = None

        if not type(self.draws_conf) == dict:
            raise Exception("draws_conf is not a valid json string")
        self.nb_draws = len(self.draws_conf["draw_list"])
        if 1: #self.mode == "4grayscale" or self.nb_draws > 1 or 1: # or draw_white_first
            self.pipe_bit_depth = 8
            self.eight_bpp = np.full((self.width, self.height), 255, dtype=np.uint8)
            self.byte_string_list = [self.eight_bpp, self.eight_bpp]
        else: 
            self.pipe_bit_depth = 1
            self.eight_bpp = None

        #self.cursor = Image.open('imgs\cursor.png')
        self.cursor = cv2.imread('imgs\cursor_thick_alpha_big.png', cv2.IMREAD_UNCHANGED) 
        if self.cursor.shape[2] == 3:
            self.cursor = cv2.cvtColor(self.cursor, cv2.COLOR_BGR2RGBA)

        self.setup_settings_bytearray()

        self.mouse_moved = 0
        self.settings_changed = 0
        #self.check_resize()

        if self.a.disable_wifi == 1: self.wifi_on = 0;
        else: self.wifi_on = 1
        setup_shared_memory(self)
        self.mode = read_dither_method(self)
        # self.pole_factor = float(self.polarize.split(',')[0])
        # self.pole_pivot = int(self.polarize.split(',')[1])
        # self.pole_mode = self.polarize.split(',')[2]


    def setup_settings_bytearray(self):

        self.pipe_settings = {"signal": -1,
                              "mouse_moved": 0,
                              "mode": self.mode, 
                              "do_full_refresh": self.do_full_refresh,
                              "draws_conf": self.draws_conf,
                              #"rmt_high_times": self.rmt_high_times,
                              "notes": "",
                              "wifi_transfer_size": -1,
                            #   "framebuffer_data_pos": -1,
                              "framebuffer_data_size": -1,
                             # "line_changed_pos":  -1 ,
                              "draw_count": -1,
                              "total_lines_changed": -1,
                              "need_to_extract": -1,
                              "rotation": self.rotation,
                              "pipe_bit_depth": self.pipe_bit_depth
                              }# bytearray(b'\x00\x00\x00')




def read_file(conf_file):
    display_conf = []
    conf_file_path = f'{working_dir}/{conf_file}'
    with open(conf_file, "r") as ins:
        for line in ins:
            number_strings = line.split() 
            if line != '' and line != '\n':
                display_conf.append(number_strings)  #
    return display_conf

def read_dither_method(ctx):
    config = configparser.ConfigParser()
    config.read(ctx.configuration_file)
    #new_conf = read_file(ctx.configuration_file)
    prev = ctx.mode
    selected = prev
    #for elem in new_conf:
    #if elem[0] == 'mode:':
    new_mode = config["main"]["mode"]
    if new_mode in modes.keys():
        #print(f"{ctx.log} Mode is: ", elem[1])
        return  new_mode

    elif prev in modes.keys():
        print(f"{ctx.log} Invalid dither method selected, setting: ", prev)
        return prev

    else:
        print("Invalid dither method selected") 
        raise Exception("Invalid dither method selected, options are:", modes)
        return "monochrome"

def get_display_settings(conf_file, args):
    display_conf = read_file(conf_file)
                
    display_list.append(display_settings(display_conf, args, conf_file))
    


def create_pipes(output_pipe, input_pipe, id):
    pipe_a = "epdiy_pc_monitor_a_"
    pipe_b  = "epdiy_pc_monitor_b_"
    if linux:
        output_pipe = output_pipe
        input_pipe = input_pipe

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

def open_pipes(ctx):       
    display_id = ctx.id
    output_pipe = f"epdiy_pc_monitor_a_{display_id}"
    input_pipe = f"epdiy_pc_monitor_b_{display_id}"
    if linux:
        output_pipe = f"/tmp/{output_pipe}"
        input_pipe = f"/tmp/{input_pipe}"
        create_pipes(output_pipe, input_pipe, display_id)
        print("Opening pipes...")
        fd1 = os.open(input_pipe, os.O_RDONLY)
        fd0 = os.open(output_pipe, os.O_WRONLY)
        print("Pipes opened")
    elif windows:
        fd1, fd0 = create_pipes(output_pipe, input_pipe, display_id)
    return fd1, fd0
def get_nb_bytes_pad(self):
    tmp_width = self.width
    for x in range(4):
        rem = tmp_width % 32
        tmp_width += 8
        if rem == 0:
            return x

def get_raw_pixels(image_file, file_path, save_raw_file, switcher):
    if ctx.pipe_bit_depth == 1:
        output = io.BytesIO()
        image_file.save(output, format='BMP')
        byte_string_raw = output.getvalue()
        ctx.byte_string_list[switcher] = bytearray(byte_string_raw)

        end = len(ctx.byte_string_list[switcher])
        start = end - ctx.pad_bytes

        for x in range(ctx.height):
            ctx.byte_string_list[switcher][start:end] = b''
            start -= ctx.line_with_pad  
            end -= ctx.line_with_pad
        ctx.byte_string_list[switcher][0:62] = b''
        
        #if check_for_difference_esp:# and pseudo_greyscale_mode == 0:
            #check_for_difference_esp_fun(ctx.byte_string_list)
        byte_frag = ctx.byte_string_list[switcher]

        if save_raw_file:
            with open(f"{working_dir}fragraw", "wb") as out:
                out.write(byte_frag)
            with open(file_path, "wb") as outfile:
                outfile.write(ctx.byte_string_list[switcher])

    elif ctx.pipe_bit_depth == 8:
        ctx.byte_string_list[switcher] = np.asarray(image_file, dtype=np.uint8)
        byte_string_raw = None

    # if check_for_difference_esp == 1:
    #     check_for_difference_esp_fun(ctx.byte_string_list)

    return [ctx.byte_string_list[switcher], byte_string_raw, ctx.byte_string_list]


def check_and_exit(fd0, fd1):
    global quit_all
    if ctx.a.child_process ==  0:
        end_val = exiting
    else: end_val = ctx.shared_buffer[0]

    if end_val == 101:
        try:
            if pipe_output:
                if linux: os.write(fd0, ctx.shared_buffer[0:2])
                elif windows: win32file.WriteFile(fd0, ctx.shared_buffer[0:2])
        except Exception as e:
            print(f"check and exit e {e}")
            quit_all = True
            return -1

        time.sleep(0.5)
        if ctx.a.child_process == 0:
            time.sleep(1.5)

            try: ctx.shm_a.unlink()
            except: pass
        else:
            try: resource_tracker.unregister(ctx.shm_a._name, 'shared_memory')
            except: pass
            ctx.shm_a.close()
        os._exit(0)
        #sys.exit(f'Python capture ID {ctx.id} terminated')
        
def pipe_output_f(raw_files, np_image_file, mouse_moved, fd1, fd0):
    global quit_all
    byte_frag = raw_files#raw_files[0]

    if check_and_exit(fd0,fd1) == -1:
        quit_all = True
        return -1

    ctx.pipe_settings["mouse_moved"] = ord('m') if  mouse_moved else 0
    
    ctx.pipe_settings["mode"] =  r_shm(ctx.offsets.mode, 'i')
    
    ctx.pipe_settings["do_full_refresh"] =  ctx.do_full_refresh

    ser_pipe_settings = json.dumps(ctx.pipe_settings).encode('utf-8')

    ser_pipe_settings_s = struct.pack('<H', len(ser_pipe_settings))

    if linux: os.write(fd0, ser_pipe_settings_s)
    elif windows: win32file.WriteFile(fd0, ser_pipe_settings_s)
    
    if linux: os.write(fd0, ctx.pipe_settings)
    elif windows: win32file.WriteFile(fd0, bytes(ser_pipe_settings))

    # if check_for_difference_esp == 1:
    #     #check_for_difference_esp_fun(raw_files[2])
    #     if linux: os.write(fd0, ctx.dif_list[0:ctx.height])
    #     elif windows: win32file.WriteFile(fd0, ctx.dif_list[0:ctx.height])
    # cv2.imshow('output_image_byte', byte_frag)
    # cv2.waitKey(1)

    if linux: os.write(fd0, byte_frag)
    elif windows: win32file.WriteFile(fd0, np.ravel(byte_frag))
    return byte_frag

def w_shm(obj, increase, type):
    shared_buffer = ctx.shared_buffer 
    obj.value += increase
    if type == 'f':
        shared_buffer[obj.pos:obj.pos+4] = float_to_bytearray(obj.value)    
    elif type == 'i':
        shared_buffer[obj.pos:obj.pos+4] = obj.value.to_bytes(4,  byteorder = 'big', signed=True)
    elif type == 'a':
        shared_buffer[obj.pos:obj.pos+4] = increase.to_bytes(4,  byteorder = 'big', signed=True)

def r_shm(obj, type):
    if type == 'f':
        return round((bytearray_to_float(ctx.shared_buffer[obj.pos:obj.pos+4]))[0], 1)
    elif type == 'i':
        return int.from_bytes(ctx.shared_buffer[obj.pos:obj.pos+4], byteorder='big', signed=True)

def check_arr(array):
        if isinstance(array, np.ndarray) and array.dtype == np.uint8 and len(array.shape)==1:
            ptr0 = array.ctypes.data
            ptr0= ctypes.c_uint64(ptr0)
        else: 
            print("pixel data must be 1d for dither")
            return -1
        return ptr0

class dither_setup:
    path = os.path.dirname(__file__)
    cdll = ctypes.CDLL(os.path.join(path, "dither_.dll" if sys.platform.startswith("win") else "dither_.so"))
    def indirect(self,i):
        method_name= str(i)
        method=getattr(self.cdll,method_name,lambda :'Invalid')
        return method

    def __init__(self):
        self.pixel_invert = np.full((ctx.width, ctx.height), 0, dtype=np.bool8)

        
    def apply(self, pixel_data, dither_method):

        if dither_method == 'monochrome' or dither_method == 'PIL_dither':
            return -1
        if isinstance(pixel_data, np.ndarray) and pixel_data.dtype == np.uint8 and len(pixel_data.shape)==1:
            v = pixel_data.ctypes.data
            v1= ctypes.c_uint64(v)
        else: 
            print("pixel data must be 1d for dither")
            return -1
        f_meth = "makeDither" + dither_method
        method = self.indirect(f_meth)
       # method = self.cdll.makeDitherSierraLite #self.indirect(dither_method)
        method(v1, ctx.width, ctx.height)

        return 1 
    def selective_invert_(self, pixels, chunk_w, chunk_h, thres):
        if isinstance(pixels, np.ndarray) and pixels.dtype == np.uint8 and len(pixels.shape)==1:
            ptr0 = pixels.ctypes.data
            ptr0= ctypes.c_uint64(ptr0)
            ptr1 = self.pixel_invert.ctypes.data
            ptr1= ctypes.c_uint64(ptr1)
        else: 
            print("pixel data must be 1d for dither")
            return -1
        self.cdll.selective_invert(ptr0, ptr1, ctx.width,ctx.height, chunk_w, chunk_h, thres)
    def quantize_(self, pixels, pixels2, size):
        if isinstance(pixels, np.ndarray) and pixels.dtype == np.uint8 and len(pixels.shape)==1:
            ptr0 = pixels.ctypes.data
            ptr0= ctypes.c_uint64(ptr0)
            ptr1 = pixels2.ctypes.data
            ptr1= ctypes.c_uint64(ptr1)
        else: 
            print("pixel data must be 1d for dither")
            return -1
        self.cdll.quantize(ptr0, ptr1, size)

    def polarize_(self, pixels, factor, pivot, size):
        #size = ctx.width * ctx.height
        factor_offset = r_shm(ctx.offsets.polarize_factor, 'f')
        factor_ = ctypes.c_float(factor + factor_offset)
        size_ = ctypes.c_uint64(size)
        self.cdll.polarize(check_arr(pixels), factor_, pivot, size_)
        
    def polarize_24bit_(self, pixels, pixels24, factor, pivot):
        v0 = check_arr(pixels)
        v1 = check_arr(pixels24)
        factor_ = ctypes.c_float(factor)
        size = ctypes.c_uint64(ctx.tot_nb_pixels)
        self.cdll.polarize_24bit(v0, v1, factor_, pivot, size)

    def alloc_memory_(self):
        self.cdll.alloc_memory(ctx.tot_nb_pixels)

    def selective_invert_v2_(self, pixels,  chunk_w,  chunk_h,  thres_perc,  b_thres,  fill_thres):
        self.cdll.selective_invert_v2(check_arr(pixels), ctx.width,  ctx.height,  chunk_w,  chunk_h,  thres_perc,  b_thres,  fill_thres)

    def invert_task_(self, pixels,  chunk_w,  chunk_h,  thres_perc,  b_thres,  fill_thres, pole_factor,  pivot):
        self.cdll.invert_task(check_arr(pixels), ctx.width,  ctx.height,  chunk_w,  chunk_h,  thres_perc,  b_thres,  fill_thres)

def save_bmp_fun(image_file, mode):
    image_file2 = image_file.copy()
    if mode != 10 and not ctx.draw_white_first:
        image_file2 = image_file2.transpose(Image.FLIP_TOP_BOTTOM) #flip the image so that the first bytes contain the pixel data of the first lines
    if ctx.rotation != 0 or ctx.draw_white_first:
        image_file2   = image_file2.rotate(ctx.rotation,  expand=True)
    image_file2.save(ctx.complete_output_file)

def check_key_presses(PID_list, conf):
    x = 0
    if linux:
        orig_settings = termios.tcgetattr(sys.stdin)
    class Switcher():
        sl = 0.0
            
        def indirect(self,i):
            method_name='fun_'+str(i)
            method=getattr(self,method_name,lambda :'Invalid')
            return method()
        def fun_q(self):
            print("Exiting")
            if ctx.has_childs == 1:
                ctx.shared_buffer[0] = 101
            exiting = 101
            time.sleep(5)
            try:
                for v in range(len(PID_list)-1, 0, -1):
                    if PID_list[v] != None:
                        os.kill(PID_list[v], 9)
                os.kill(PID_list[0], 9)
            except: pass    
            
        # def fun_m(self):
        #     ctx.settings_dither = 1

        #     ctx.mode = "monochrome"
        #     w_shm(conf.mode, 0, 'a');  print("Monochrome mode is on ", r_shm(conf.mode, 'i'))
        #     time.sleep(self.sl)
        #     ctx.settings_dither = 0

        # def fun_p(self):
        #     ctx.settings_dither = 1
        #     ctx.mode = "PIL_dither"
        #     w_shm(conf.mode, 9, 'a'); print("Pil dithering mode is on ", r_shm(conf.mode, 'i'))

        #     time.sleep(self.sl)
        #     ctx.settings_dither = 0

        # def fun_d(self):
        #     ctx.settings_dither = 1
        #     ctx.mode = read_dither_method(ctx)
        #     w_shm(conf.mode, modes.get(ctx.mode), 'a')
        #     print(f"Dithering mode {ctx.mode} is on {r_shm(conf.mode, 'i')}" )    
        #     time.sleep(self.sl)
        #     ctx.settings_dither = 0

        # def fun_i(self):
        #     inv = r_shm(conf.invert, 'i')
        #     if inv != 0:
        #         w_shm(conf.invert, 0, 'a');  print("Invert is off ", r_shm(conf.invert, 'i'))
        #     elif inv != 1:
        #         w_shm(conf.invert, 1, 'a'),  print("Invert is on ", r_shm(conf.invert, 'i'))
     
        def fun_s(self):
            selective_invert = r_shm(conf.selective_invert, 'i')
            if selective_invert != 0:
                w_shm(conf.selective_invert, 0, 'a');  print("selective_invert is off ", r_shm(conf.selective_invert, 'i'))
                ctx.selective_invert = 0
            elif selective_invert != 1:
                w_shm(conf.selective_invert, 1, 'a'),  print("selective_invert is on ", r_shm(conf.selective_invert, 'i'))
                ctx.selective_invert = 1

       # def fun_g(self):
       #     w_shm(conf.mode, 10, 'a');  print("greyscale mode is on ", r_shm(conf.mode, 'i'))
        # def fun_w(self): w_shm(conf.polarize_factor, -0.1, 'f');  print("polarize_factor is  ", r_shm(conf.polarize_factor, 'f') + ctx.pole_factor)
        # def fun_e(self): w_shm(conf.polarize_factor, +0.1, 'f');  print("polarize_factor is  ", r_shm(conf.polarize_factor, 'f')+ ctx.pole_factor)
        # def fun_k(self):
        #     pole_mode = r_shm(conf.pole_mode, 'i')
        #     if pole_mode == 2:
        #         pole_mode = -1
        #     w_shm(conf.pole_mode, pole_mode+1, 'a')
        #     ctx.pole_mode = get_mode(polarize_modes, (r_shm(conf.pole_mode, 'i')))
        #     print("pole_mode ", ctx.pole_mode)
        def fun_o(self):
            val = r_shm(conf.polarize_text, "i")
            self.polarize_text = 1-val
            w_shm(conf.polarize_text,  self.polarize_text, 'a')
            print("polarize_text is  ",  self.polarize_text)
        def fun_h(self):
            val = r_shm(conf.fill_blacks, "i")
            self.fill_blacks = 1-val
            w_shm(conf.fill_blacks, self.fill_blacks, 'a')
            print("fill_blacks is  ", self.fill_blacks)
        def fun_1(self): w_shm(conf.color, -0.1, 'f')
        def fun_2(self): w_shm(conf.color, +0.1, 'f')
        def fun_3(self): w_shm(conf.contrast, -0.1, 'f')
        def fun_4(self): w_shm(conf.contrast, +0.1, 'f')
        def fun_5(self): w_shm(conf.brightness, -0.1, 'f')
        def fun_6(self): w_shm(conf.brightness, +0.1, 'f')
        def fun_7(self): w_shm(conf.sharpness, -0.1, 'f')
        def fun_8(self): w_shm(conf.sharpness, +0.1, 'f')
        def fun_9(self): w_shm(conf.grey_to_monochrome_threshold, -10, 'i')
        def fun_0(self): w_shm(conf.grey_to_monochrome_threshold, +10, 'i')
        # def fun_y(self):
        #     w_shm(conf.invert, 2, 'a'); 
        #     ctx.invert = 2
        #     w_shm(conf.invert_threshold, -10, 'i'); 
        #     print("Smart invert is on with threshold ", r_shm(conf.invert_threshold, 'i') )
        # def fun_u(self):  
        #     w_shm(conf.invert, 2, 'a'); 
        #     ctx.invert = 2
        #     w_shm(conf.invert_threshold, +10, 'i');
        #     print("Smart invert is on with threshold ", r_shm(conf.invert_threshold, 'i'))
        def fun_b(self):
            if r_shm(conf.enhance_before_greyscale, 'i') == 1:
                ctx.enhance_before_greyscale = 0
                w_shm(conf.enhance_before_greyscale, 0, 'a'); print("enhance_before_greyscale is off ", r_shm(conf.enhance_before_greyscale, 'i'))
            else:
                w_shm(conf.enhance_before_greyscale, 1, 'a');  print("enhance_before_greyscale is on ", r_shm(conf.enhance_before_greyscale, 'i'))
                ctx.enhance_before_greyscale = 1


    s=Switcher()
    global exiting;
    while 1 and exiting == 0:  # ESC
        if linux:
            x = sys.stdin.read(1)[0]
        elif windows:
            x = msvcrt.getch().decode('UTF-8')
            
        if x == 'q' or x == 'Q':
            print("Exiting")
            if linux: termios.tcsetattr(sys.stdin, termios.TCSADRAIN, orig_settings)
            if ctx.has_childs == 1:
                ctx.shared_buffer[0] = 101
            exiting = 101
            time.sleep(5)
            try:
                for v in range(len(PID_list)-1, 0, -1):
                    if PID_list[v] != None:
                        os.kill(PID_list[v], 9)
                os.kill(PID_list[0], 9)
            except: pass


        elif not ctx.a.nokeychecker:
            ret0 = s.indirect(x.lower()) 

        if not ctx.a.nokeychecker:
            try: 
                n = int(x); 
                if n >= 0 and n <= 9:
                    w_shm(conf.settings_changed, 2, 'a')
                    #print(f"color {r_shm(conf.color, 'f')}, contrast  {r_shm(conf.contrast, 'f')}  brightness {r_shm(conf.brightness, 'f')}, sharpness {r_shm(conf.sharpness, 'f')},  grey_to_monochrome_threshold {r_shm(conf.grey_to_monochrome_threshold, 'i')}")
                
            except:  w_shm(conf.settings_changed, 1, 'a')



def select_inv(image_file, chunk_w, chunk_h, thres_perc, b_thres, fill_thres):
    np_arr = np.ravel(image_file)
    dith.selective_invert_v2_(np_arr, chunk_w, chunk_h, thres_perc, b_thres, fill_thres)
    return image_file

def smart_invert(image_file):
    inv_thres = r_shm(ctx.offsets.invert_threshold, 'i')
    selective_invert = r_shm(ctx.offsets.selective_invert,'i')

    n = np.mean(image_file)
    inv = 0; inv2 = 0

    if n < inv_thres:
        image_file =  cv2.bitwise_not(image_file)# ImageOps.invert(image_file)
        inv = 1

    if selective_invert == 1:
        image_file = select_inv(image_file, 15, 15, 50, 60, 5)#, 2.0, 130);  # good 15, 15, 80 

    #print("###", int(n), inv, inv2)
    return image_file

def check_and_invert(image_file):
    invert =  r_shm(ctx.offsets.invert, 'i')
    if invert > 0:
        if invert == 1:
            image_file = cv2.bitwise_not(image_file)#ImageOps.invert(image_file)
        else:
            image_file = smart_invert(image_file)
    return image_file


def float_to_bytearray(float):
    return bytearray(struct.pack("f", float))
def bytearray_to_float(bytearr):
    return struct.unpack('f', bytearr)   

class offset_object:
    def __init__(self, byte_position, value, type):
        self.pos = byte_position
        self.value = value
        self.type = type
    def round(self):
        self.value = round(self.value, 1)
        
class shared_var:
    def __init__(self):
        self.mode = offset_object(10, 0, 'a')
        self.color =  offset_object(14, 0, 'f')
        self.contrast =  offset_object(18, 0, 'f')
        self.brightness =  offset_object(22, 0, 'f')
        self.sharpness =  offset_object(26, 0, 'f')
        self.enhance_before_greyscale = offset_object(30, 0, 'i')
        self.grey_to_monochrome_threshold =   offset_object(34, 0,'i')
        self.invert =   offset_object(38, 0,'i')
        self.invert_threshold =   offset_object(42, 0,'i')
        self.selective_invert =   offset_object(46, 0,'i')
        self.polarize_factor =   offset_object(50, 0,'f')
        self.pole_mode =   offset_object(54, 0,'i')
        self.settings_changed =   offset_object(58, 0,'i')
        self.fill_blacks = offset_object(62, 0,'i')
        self.polarize_text = offset_object(66, 0,'i')


def apply_enhancements(opencv_image, conf):
    #t0 = time.time()
    settings = [conf.color, conf.contrast, conf.brightness, conf.sharpness]
    val0, val1, val2, val3 = [ val + settings[i] for i, val in enumerate(print_settings())]
    if val0 != 0 or val1 != 0 or val2 != 0 or val3 != 0:
        pil_image = Image.fromarray(opencv_image)
        if val0 != 1.0:
            enhancer = ImageEnhance.Color(pil_image)
            pil_image = enhancer.enhance(val0)     

        if  val1  != 1.0:
            enhancer = ImageEnhance.Contrast(pil_image)
            pil_image = enhancer.enhance( val1)

        if val2 != 1.0:
            enhancer = ImageEnhance.Brightness(pil_image)
            pil_image = enhancer.enhance(val2)

        if  val3 != 1.0:
            enhancer = ImageEnhance.Sharpness(pil_image)
            pil_image = enhancer.enhance( val3)

        #print("enhance took", time.time() - t0 )
        arr = np.array(pil_image)
        if pil_image.mode == "L":
            opencv_image = arr.reshape(opencv_image.shape)
            pass
            #opencv_image = cv2.cvtColor(arr, cv2.COLOR_RGBA2GRAY)
        else:
            opencv_image = cv2.cvtColor(arr, cv2.COLOR_RGB2BGRA)


    return opencv_image

def print_settings(check_only=None):
    ctx.settings_changed = r_shm(ctx.offsets.settings_changed, 'i')
    val0 = r_shm(ctx.offsets.color, 'f')
    val1 = r_shm(ctx.offsets.contrast, 'f')
    val2 = r_shm(ctx.offsets.brightness, 'f')
    val3 = r_shm(ctx.offsets.sharpness, 'f')
    if  ctx.a.child_process == 0:
        if ctx.settings_changed == 2:
            print(f"color {ctx.color + val0}, contrast  {ctx.contrast + val1}  brightness {ctx.brightness + val2}, sharpness {ctx.sharpness + val3},  grey_to_monochrome_threshold {ctx.grey_monochrome_threshold + r_shm(ctx.offsets.grey_to_monochrome_threshold, 'i')}")

        w_shm(ctx.offsets.settings_changed, 0, 'a')

    return val0, val1, val2, val3

def find_contours(bgra_image): #for testing
    gray = cv2.cvtColor(bgra_image, cv2.COLOR_BGRA2GRAY)
    ret, thresh_gray = cv2.threshold(gray, 200, 255, cv2.THRESH_BINARY)
    contours, hier = cv2.findContours(thresh_gray, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)

    # Erase small contours, and contours which small aspect ratio (close to a square)
    for c in contours:
        area = cv2.contourArea(c)

        # Fill very small contours with zero (erase small contours).
        if area < 10:
            cv2.fillPoly(thresh_gray, pts=[c], color=0)
            continue

        # https://stackoverflow.com/questions/52247821/find-width-and-height-of-rotatedrect
        rect = cv2.minAreaRect(c)
        (x, y), (w, h), angle = rect
        aspect_ratio = max(w, h) / min(w, h)

        # Assume zebra line must be long and narrow (long part must be at lease 1.5 times the narrow part).
        if (aspect_ratio < 1.5):
            cv2.fillPoly(thresh_gray, pts=[c], color=0)
            continue

    # Use "close" morphological operation to close the gaps between contours
    # https://stackoverflow.com/questions/18339988/implementing-imcloseim-se-in-opencv
    thresh_gray = cv2.morphologyEx(thresh_gray, cv2.MORPH_CLOSE, cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (51,51)));

    # Find contours in thresh_gray after closing the gaps
    contours, hier = cv2.findContours(thresh_gray, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)

    for c in contours:
        area = cv2.contourArea(c)

        # Small contours are ignored.
        if area < 500:
            cv2.fillPoly(thresh_gray, pts=[c], color=0)
            #continue

        rect = cv2.minAreaRect(c)
        box = cv2.boxPoints(rect)
        # convert all coordinates floating point values to int
        box = np.int0(box)
        cv2.drawContours(bgra_image, [box], 0, (0, 255, 0),1)

    cv2.imshow('paper', bgra_image)
    #cv2.imwrite('paper.jpg', paper)
    cv2.waitKey(1)

def polarize_text(grayscale_image, final_kernel_size, apply_to=np.array([[0]])):
    #find_contours(grayscale_image)
    opencv_image_ori = grayscale_image#cv2.cvtColor(np.array(sct_img), cv2.COLOR_BGRA2GRAY)

    inv_thres = r_shm(ctx.offsets.invert_threshold, 'i')

    n = np.mean(opencv_image_ori)

    opencv_image = None
    if n <  inv_thres and 0:   
        opencv_image = 255-opencv_image_ori.copy()
    else: opencv_image = opencv_image_ori.copy()
    
    if r_shm(ctx.offsets.selective_invert,'i'):
        np_arr = np.ravel(opencv_image)
        dith.selective_invert_v2_(np_arr, 5, 5, 50, 60, 5)
        opencv_image = np_arr.reshape(opencv_image.shape)

    kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (3, 3))
    grad = cv2.morphologyEx(opencv_image, cv2.MORPH_GRADIENT, kernel)

    _, bw = cv2.threshold(grad, 0.0, 255.0, cv2.THRESH_BINARY | cv2.THRESH_OTSU)

    kernel = cv2.getStructuringElement(cv2.MORPH_RECT, final_kernel_size) #9,1
    connected = cv2.morphologyEx(bw, cv2.MORPH_CLOSE, kernel)
    
    # kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (30,1 ))
    # connected2 = cv2.morphologyEx(bw, cv2.MORPH_CLOSE, kernel)
    # merge = cv2.bitwise_or(connected, connected2)

    #cv2.imshow("connected", connected)
    #cv2.imshow("connected2", connected2)
    #cv2.imshow("merge", merge)
    contours, hierarchy = cv2.findContours(connected.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)

    perform_on = apply_to if apply_to.shape != (1, 1) else opencv_image

    thres = 100
    thres_max = 10000
    mult = 1.0
    for contour in contours:
        x, y, w, h = cv2.boundingRect(contour)
        # cv2.rectangle(perform_on, (x, y), (x + w, y + h), (0, 255, 0), 2)
        # continue
        centerx = x+w//2; centery =  y+h//2
        w= int(w*mult)
        h= int(h*mult)
        a = w*h
        #if a < thres or a > thres_max:continue
        xx = centerx - w//2
        yy = centery - h//2
        enhanced_roi = perform_on[yy:yy+h, xx:xx+w]
        #alpha = 3# Contrast control (1.0-3.0)
        #beta = 1    # Brightness control (0-100)
        #enhanced_roi = cv2.convertScaleAbs(enhanced_roi, alpha=alpha, beta=beta)
        #enhanced_roi = cv2.equalizeHist(enhanced_roi)
        #if n >  inv_thres:   
        mean = np.mean(enhanced_roi)
        if mean < 120:
            enhanced_roi = 255-enhanced_roi
        enhanced_roi = (enhanced_roi[:,:]>=np.mean(enhanced_roi,axis=(0,1)))*255
        # min_val = np.min(roi)
        # max_val = np.max(roi)
        # enhanced_roi = cv2.convertScaleAbs(roi, alpha=255.0/(max_val-min_val), beta=-min_val*(255.0/(max_val-min_val)))
        perform_on[yy:yy+h, xx:xx+w] = enhanced_roi                    

    #print("--->", time.time() -t)

    cv2.imshow('perform_on', perform_on)
    cv2.moveWindow('perform_on', 1200, 0)

    cv2.imshow(f"opencv_image_ori", opencv_image_ori)
    cv2.moveWindow('opencv_image_ori', 0, 825)
    

    return perform_on
    
def fill_blacks(opencv_image):
    
    #im_floodfill_inv = cv2.bitwise_not(im_floodfill)            
    th = ctx.grey_monochrome_threshold+r_shm(ctx.offsets.grey_to_monochrome_threshold, 'i')

    _, im_in = cv2.threshold(opencv_image, th, 255, cv2.THRESH_BINARY)
    
    im_floodfill = im_in.copy()
    h, w = im_floodfill.shape[:2]
    mask = np.zeros((h+2, w+2), np.uint8)#+ 0
    cv2.floodFill(im_floodfill, mask, (0,0), 255)

    #thresh, im_in = cv2.threshold(opencv_image, 128, 255, cv2.THRESH_BINARY | cv2.THRESH_OTSU)

    #im_in = 255 - im_in
    im_out = im_in.copy()

    #im_out = cv2.cvtColor(np.array(sct_img), cv2.COLOR_BGRA2GRAY)
                
    kernel = np.ones((3,3), np.uint8)
    inverted_image = cv2.bitwise_not(im_in)
    eroded_image = cv2.erode(inverted_image, kernel, iterations=3)
    result_image = cv2.bitwise_not(eroded_image)

    mask = eroded_image != 0
    im_out[mask] = eroded_image[mask]
    
    # cv2.imshow('im_in', im_in)
    # cv2.moveWindow('im_in', 0, 0)

    # cv2.imshow('im_out', im_out)
    # cv2.moveWindow('im_out', 1200, 1000)

    # cv2.imshow('array_thresholded', result_image)
    # cv2.moveWindow('array_thresholded', 1200, 1000)

    # cv2.waitKey(1)
    return im_out
    
def check_if_before_apply_enhancements(opencv_image, ctx, of):
    if of - r_shm(ctx.offsets.enhance_before_greyscale, 'i'):
        return apply_enhancements(opencv_image, ctx)
    return opencv_image

args = args_eval() 
nb_displays = eval_args(args)


for x in range(nb_displays):
    get_display_settings(sys.argv[x+1], args)

ctx = display_list[0]

dith = dither_setup()
