import platform, ctypes

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


working_dir = os.getcwd()

exiting = 0
print_settings = 0

display_list = []
display_conf = []

save_raw_file = 0
open_from_disk = 0
start_process = 1
save_chunck_files = 0
pipe_bit_depth = 1
pipe_output = 1
enable_raw_output = 1
save_bmp = 1
check_for_difference_esp = 1
working_dir = os.getcwd()
raw_output_file = f"{working_dir}/image_mode_raw"

def t():
    return time.time()

modes =  {
    "monochrome" : 0,    "Bayer16" : 1,    "Bayer8" : 2,    "Bayer4" : 3,    "Bayer3" : 4,
    "Bayer2" : 5,     "FS" : 6,     "SierraLite" : 7, 	     "Sierra" : 8,    "PIL_dither" : 9
}
def get_mode(mode_code):
    return [k for k,v in modes.items() if v == mode_code][0]

def eval_args(self):
    args = str(sys.argv)
    if "-silent" in sys.argv:
        self.disable_logging = 1; sys.argv.remove("-silent")
    else: self.disable_logging = 0
    if "child" in sys.argv:
        self.child_process = 1; sys.argv.remove("child")
    else: self.child_process = 0
    if "-disable_wifi" in sys.argv:
        self.disable_wifi = 1; sys.argv.remove("-disable_wifi")
    else: self.disable_wifi = 0
    if "-common" in sys.argv: 
        self.common = 1; sys.argv.remove("-common")
    else: self.common = 0
    nb_arg = len(sys.argv)
    nb_displays = nb_arg -1
    return nb_displays

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

    self.offset_variables = shared_var()

class args_eval:
    def __init__(self):
        pass
class display_settings(object):
    def get_dith(self):
        pass
    def __init__(self, names, args, configuration_file):
        def get_val(val):
            n = 0
            for c in val:
                if c == '.': n+=1
            if n> 1:
                return val
            elif n == 1: return float(val)
            elif val.isdigit():
                 return int(val)
            else: return val
        try:
            # Support space-separated name strings
            names = names.split()
        except AttributeError:
            pass
        for name in names:
            setattr(self, name[0][:-1], get_val(name[1]))
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
        self.pipe_settings = bytearray(b'\x00\x00\x00')

        self.dif_list_sum = 0
        self.switcher = 0

        self.dif_list = bytearray(self.height)
        self.configuration_file = configuration_file
        self.log = f"Python ID {self.id}: " 
        self.complete_output_file = f'{working_dir}/image_id_{self.id}.bmp'
        self.settings_dither = 0
        if self.a.disable_wifi == 1: self.wifi_on = 0;
        else: self.wifi_on = 1
        setup_shared_memory(self)
        self.mode = read_dither_method(self)

        
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

    new_conf = read_file(ctx.configuration_file)
    prev = ctx.mode
    selected = prev
    for elem in new_conf:
        if elem[0] == 'mode:':
            if elem[1] in modes.keys():
                #print(f"{ctx.log} Mode is: ", elem[1])
                return  elem[1]

            elif prev in modes.keys():
                print(f"{ctx.log} Invalid dither method selected, setting: ", prev)
                return prev

            else:
                print("Invalid dither method selected, setting monochrome") 
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
    
    if check_for_difference_esp:# and pseudo_greyscale_mode == 0:
        check_for_difference_esp_fun(ctx.byte_string_list, 1)
    byte_frag = ctx.byte_string_list[switcher]

    if save_raw_file:
        with open(f"{working_dir}fragraw", "wb") as out:
            out.write(byte_frag)
        with open(file_path, "wb") as outfile:
            outfile.write(ctx.byte_string_list[switcher])

    return [ctx.byte_string_list[switcher], byte_string_raw, ctx.byte_string_list]


def check_for_difference_esp_fun(array_list, bit_depth):
    if bit_depth == 8:
        divider = 1
    elif bit_depth == 1: divider = 8
    
    chunk_size = ctx.width//divider
    startt = 0
    endd = chunk_size
    dif_list = ctx.dif_list
    dif_list_sum = ctx.dif_list_sum
    #t0 = time.time()

    for t in range(ctx.height):
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

def pipe_output_f(raw_files, np_image_file, mouse_moved, fd1, fd0):
    byte_frag = raw_files[0]
    if ctx.a.child_process ==  0:
        end_val = exiting
    else: end_val = ctx.shared_buffer[0]

    if end_val == 101:

        if linux: os.write(fd0, ctx.shared_buffer[0:2])
        elif windows: win32file.WriteFile(fd0, ctx.shared_buffer[0:2])

        time.sleep(0.5)
        if ctx.a.child_process == 0:
            time.sleep(1.5)

            try: ctx.shm_a.unlink()
            except: pass
        else:
            try: resource_tracker.unregister(ctx.shm_a._name, 'shared_memory')
            except: pass
            ctx.shm_a.close()
        sys.exit(f'Python capture ID {ctx.id} terminated')

    ctx.pipe_settings[1] = mouse_moved

    ctx.pipe_settings[2] = modes[ctx.mode]

    if linux: os.write(fd0, ctx.pipe_settings)
    elif windows: win32file.WriteFile(fd0, ctx.pipe_settings)

    if check_for_difference_esp == 1:
        check_for_difference_esp_fun(raw_files[2], 1)

        if linux: os.write(fd0, ctx.dif_list)
        elif windows: win32file.WriteFile(fd0, ctx.dif_list)

    if linux: os.write(fd0, byte_frag)
    elif windows: win32file.WriteFile(fd0, byte_frag)
    # if display_list[0].improve_dither:
    #     ready = os.read(fd1, 1)
    #     os.write(fd0, pipe_settings)
    #     os.write(fd0, np_image_file)

    #print(f"data sent")
    return byte_frag

def write_to_shared_mem(obj, increase, type):
    shared_buffer = ctx.shared_buffer 
    obj.value += increase
    if type == 'f':
        shared_buffer[obj.pos:obj.pos+4] = float_to_bytearray(obj.value)    
    elif type == 'i':
        shared_buffer[obj.pos:obj.pos+4] = obj.value.to_bytes(4,  byteorder = 'big', signed=True)
    elif type == 'a':
        shared_buffer[obj.pos:obj.pos+4] = increase.to_bytes(4,  byteorder = 'big', signed=True)

def get_val_from_shm(obj, type):
    if type == 'f':
        return round((bytearray_to_float(ctx.shared_buffer[obj.pos:obj.pos+4]))[0], 1)
    elif type == 'i':
        return int.from_bytes(ctx.shared_buffer[obj.pos:obj.pos+4], byteorder='big', signed=True)


class dither_setup:
    path = os.path.dirname(__file__)
    cdll = ctypes.CDLL(os.path.join(path, "dither_.dll" if sys.platform.startswith("win") else "dither_.so"))
    def indirect(self,i):
        method_name= str(i)
        method=getattr(self.cdll,method_name,lambda :'Invalid')
        return method

    #def __init__(self, ctx):
        
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
        def fun_m(self):
            ctx.settings_dither = 1

            ctx.mode = "monochrome"
            write_to_shared_mem(conf.mode, 0, 'a');  print("Monochrome mode is on ", get_val_from_shm(conf.mode, 'i'))
            time.sleep(self.sl)
            ctx.settings_dither = 0

        def fun_p(self):
            ctx.settings_dither = 1
            ctx.mode = "PIL_dither"
            write_to_shared_mem(conf.mode, 9, 'a'); print("Pil dithering mode is on ", get_val_from_shm(conf.mode, 'i'))

            time.sleep(self.sl)
            ctx.settings_dither = 0

        def fun_d(self):
            ctx.settings_dither = 1
            ctx.mode = read_dither_method(ctx)
            write_to_shared_mem(conf.mode, modes.get(ctx.mode), 'a')
            print(f"Dithering mode {ctx.mode} is on {get_val_from_shm(conf.mode, 'i')}" )    
            time.sleep(self.sl)
            ctx.settings_dither = 0

        def fun_i(self):
            if get_val_from_shm(conf.invert, 'i') == 1:
                write_to_shared_mem(conf.invert, -1, 'i');  print("Invert is off ", get_val_from_shm(conf.invert, 'i'))
            else:
                write_to_shared_mem(conf.invert, +1, 'i'),  print("Invert is on ", get_val_from_shm(conf.invert, 'i'))
     
        def fun_1(self): write_to_shared_mem(conf.color, -0.1, 'f')
        def fun_2(self): write_to_shared_mem(conf.color, +0.1, 'f')
        def fun_3(self): write_to_shared_mem(conf.contrast, -0.1, 'f')
        def fun_4(self): write_to_shared_mem(conf.contrast, +0.1, 'f')
        def fun_5(self): write_to_shared_mem(conf.brightness, -0.1, 'f')
        def fun_6(self): write_to_shared_mem(conf.brightness, +0.1, 'f')
        def fun_7(self): write_to_shared_mem(conf.sharpness, -0.1, 'f')
        def fun_8(self): write_to_shared_mem(conf.sharpness, +0.1, 'f')
        def fun_9(self): write_to_shared_mem(conf.grey_to_monochrome_threshold, -10, 'i')
        def fun_0(self): write_to_shared_mem(conf.grey_to_monochrome_threshold, +10, 'i')
        def fun_b(self):
            if get_val_from_shm(conf.enhance_before_greyscale, 'i') == 1:
                write_to_shared_mem(conf.enhance_before_greyscale, -1, 'i'); print("enhance_before_greyscale is off ", get_val_from_shm(conf.enhance_before_greyscale, 'i'))
            else:
                write_to_shared_mem(conf.enhance_before_greyscale, +1, 'i');  print("enhance_before_greyscale is on ", get_val_from_shm(conf.enhance_before_greyscale, 'i'))
    
    s=Switcher()
    global exiting; global print_settings
    while 1 and exiting == 0:  # ESC
        if linux:
            x = sys.stdin.read(1)[0]
        elif windows:
            x = msvcrt.getch().decode('UTF-8')
            
        if x == 'q' or x == 'Q':
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


        else:
            ret0 = s.indirect(x) 

        
        try: 
            n = int(x); 
            if n >= 0 and n <= 9:
                print_settings = 1
                #print(f"color {get_val_from_shm(conf.color, 'f')}, contrast  {get_val_from_shm(conf.contrast, 'f')}  brightness {get_val_from_shm(conf.brightness, 'f')}, sharpness {get_val_from_shm(conf.sharpness, 'f')},  grey_to_monochrome_threshold {get_val_from_shm(conf.grey_to_monochrome_threshold, 'i')}")
        except: pass
        
    if linux:
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, orig_settings)





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
    
    if print_settings and ctx.a.child_process == 0:
        print(f"color {conf.color + val0}, contrast  {conf.contrast + val1}  brightness {conf.brightness + val2}, sharpness {conf.sharpness + val3},  grey_to_monochrome_threshold {conf.grey_monochrome_threshold + get_val_from_shm(offsets.grey_to_monochrome_threshold, 'i')}")
        print_settings = 0

    #print("enhance took", time.time() - t0 )

    return image_file


def convert_to_greyscale_and_enhance(image_file, conf, offset_variables):
    if get_val_from_shm(offset_variables.enhance_before_greyscale, 'i'):
        image_file = apply_enhancements(image_file, conf, offset_variables)
        
    val = get_val_from_shm(offset_variables.mode, 'i')
    if val == 0 or val == 9:
        image_file = image_file.convert('L')

    if get_val_from_shm(offset_variables.invert, 'i')  == 1:
        image_file = ImageOps.invert(image_file)
    
    if get_val_from_shm(offset_variables.enhance_before_greyscale, 'i')  == 0:
        image_file = apply_enhancements(image_file, conf, offset_variables)
    return image_file

args = args_eval() 
nb_displays = eval_args(args)

for x in range(nb_displays):
    get_display_settings(sys.argv[x+1], args)

ctx = display_list[0]

dith = dither_setup()

g = 0
""" path = os.path.dirname(__file__)
cdll = ctypes.CDLL(os.path.join(path, "dither_.dll" if sys.platform.startswith("win") else "dither_.so"))
makeDitherBayer16 = cdll.makeDitherSierraLite


def sum_it(l, w, h):
    if isinstance(l, np.ndarray) and l.dtype == np.uint8 and len(l.shape)==1 or 1:
        v = l.ctypes.data
        v1= ctypes.c_uint64(v)
    makeDitherBayer16(v1, w, h)
    return l """

""" monitor = {"top": 0, "left": 0, "width": 1200, "height": 825}
output = "sct-{top}x{left}_{width}x{height}.png".format(**monitor)
import mss
def t():
    return time.time()
# Grab the data
with mss.mss() as sct:
    sct_img = sct.grab(monitor)
    img = bytearray(sct_img.rgb)
    s0 = sct_img.size

im = Image.frombytes('RGB', sct_img.size, sct_img.rgb)

test = np.asarray(im)
test = np.ravel(test)
t0 =t() 

a = dith.apply(test, 'makeDitherSierraLite')
print(t() - t0)
pili = Image.frombytes('RGB', sct_img.size, test)
pili = pili.convert('1')
pili.save("pili.bmp")
 """