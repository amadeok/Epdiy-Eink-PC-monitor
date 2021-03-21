import pyautogui, time, io
from PIL import Image
#from bitstring import BitStream, BitArray




name = "cursor_test_image.bmp"
ext = ".bmp"
input_file = f"{name}{ext}"
list_xy = []
path = "/home/amadeok/epdiy-working/examples/pc_monitor/pc_host_app/"
string = ""
remove_eol = 0
new_byte_string = b''
header_path = "header1200x825" # header936x704 , 936x88header
t0 = time.time()
pos2 = pyautogui.position() 
print(time.time()-t0)

class position():
    x = 0
    y = 0
pos = position()

width_res = 1200
height_res = 825

with open(f"{path}{header_path}.bmp", "rb") as h:
    header = h.read()
#with open(f"{path}{name}", "rb") as file:
#    byte_string_raw = bytearray(file.read())

def get_nb_bytes_pad():
    global width_res
    tmp_width = width_res
    for x in range(4):
        rem = tmp_width % 32
        tmp_width += 8
        if rem == 0:
            return x

global line_with_pad
pad_bytes = get_nb_bytes_pad()
line_with_pad = int((width_res / 8) + pad_bytes)

image_file = Image.open(f"{path}{name}") # for testing
image_file = image_file.convert('1')

output = io.BytesIO()
image_file.save(output, format='BMP')
byte_string_raw = output.getvalue()
byte_string_raw = bytearray(byte_string_raw)

end = len(byte_string_raw)
start = end - pad_bytes

for x in range(height_res):
    byte_string_raw[start:end] = b''
    start -= line_with_pad  
    end -= line_with_pad
#byte_string_raw[0:62] = b''

def process_string(current_byte_string):
    inverted_output0= ''
    inverted_output1= ''  
    current_byte_string_0= ''.join(current_byte_string[0:16])
    current_byte_string_1= ''.join(current_byte_string[16:32])

    current_byte_string = ''.join(current_byte_string)
    # for x in range(16, 0, -1):
    #     pixel_pos = (x-1)
    #     inverted_output0 += current_byte_string_0[pixel_pos:pixel_pos+1]
    #     inverted_output1 += current_byte_string_1[pixel_pos:pixel_pos+1]

    integer_rep0 = int(current_byte_string[0:16], 2)
    integer_rep1 = int(current_byte_string[16:32], 2)
    #integer_rep0 = int(inverted_output0, 2)
    #integer_rep1 = int(inverted_output1, 2)
    target_byte0 = integer_rep0.to_bytes(2, 'big')
    target_byte1 = integer_rep1.to_bytes(2, 'big')
    fin = target_byte0 + target_byte1
    return fin

#def init_cursor(conf):


def draw_cursor_1bpp(conf, byte_string_raw):

    x_offset = conf.x_offset
    y_offset = conf.y_offset

    width_res = conf.width
    height_res = conf.height
    width_res2 = conf.width_res2
    height_res2 = conf.height_res2

    global pos2
    previous_pos = pos2
    pos2 = pyautogui.position()
    offset = 0
    if pos2.x >= x_offset and pos2.x <= width_res2 and pos2.y >= y_offset and pos2.y <= height_res2-22:
        t0 = time.time()
        x = int((pos2.x - offset) / 8)
        y = int((pos2.y - offset +1))



        if conf.rotation == 180:
            x = width_res-x
            y = height_res-y
            xrem = 7- (pos2.x % 8)
            y-=8
            x-=4

        else:
            xrem = pos2.x % 8
            y+=16

        yrem = (pos2.y - offset) % 8
        
        line_coor = (y*width_res//8) + x #+ 62

        if conf.rotation ==  0:
            for j in range(16):
                oribyte = int.from_bytes(byte_string_raw[line_coor+(j*-width_res//8):line_coor+4+(j*-width_res//8)], 'big')
                current_byte_string = list(format(oribyte, "b").zfill(32))

                current_byte_string[xrem] = '0'
                try: current_byte_string[xrem+1] = '0'
                except: pass
                try:
                    current_byte_string[xrem+(16-j)] = '0'
                    current_byte_string[xrem+(16-j+1)] = '0'
                except: pass
                #if j > 2:
                for x in range(16-j-2):
                    try:current_byte_string[xrem+2+x] = '1'
                    except: pass

                fin = process_string(current_byte_string)
                #current_byte_string = ''.join(current_byte_string)
                #integer_rep1 = int(current_byte_string, 2)
                #integer_rep0 = int(inverted_output0, 2)
                #integer_rep1 = int(inverted_output1, 2)
                #fin = integer_rep1.to_bytes(2, 'big')
                byte_string_raw[line_coor+(j*-width_res//8):line_coor+4+(j*-width_res//8)] =  fin
                
            j-=17
            oribyte = int.from_bytes(byte_string_raw[line_coor+(j*-width_res//8):line_coor+4+(j*-width_res//8)], 'big')
            current_byte_string = list(format(oribyte, "b").zfill(32)) 
            for x in range(16):
                try: current_byte_string[xrem+x] = '0'
                except:pass

            fin = process_string(current_byte_string)
            byte_string_raw[line_coor+(j*-width_res//8):line_coor+4+(j*-width_res//8)] =  fin
            j+=1
            byte_string_raw[line_coor+(j*-width_res//8):line_coor+4+(j*-width_res//8)] =  fin


        else:
            for j in range(16):
                oribyte = int.from_bytes(byte_string_raw[line_coor+(j*-width_res//8):line_coor+4+(j*-width_res//8)], 'big')
                current_byte_string = list(format(oribyte, "b").zfill(32))
                
                current_byte_string[xrem-8] = '0'
                if xrem+1 < 8:
                    try: current_byte_string[xrem+1-8] = '0'
                    except: pass



                if j > 2:
                    for x in range(j-2):
                        try:current_byte_string[-7+xrem-2-x] = '1'
                        except: pass

                try:
                    current_byte_string[xrem+(-8-j)] = '0'
                    current_byte_string[xrem+(-8-j+1)] = '0'
                except:  current_byte_string[xrem+(-8-j-1)] = '0'        
                fin = process_string(current_byte_string)

                #current_byte_string = ''.join(current_byte_string)
                #integer_rep1 = int(current_byte_string, 2)

                #fin = integer_rep1.to_bytes(2, 'big')
                byte_string_raw[line_coor+(j*-width_res//8):line_coor+4+(j*-width_res//8)] =  fin

            j-=1
            oribyte = int.from_bytes(byte_string_raw[line_coor+(j*-width_res//8):line_coor+4+(j*-width_res//8)], 'big')
            current_byte_string = list(format(oribyte, "b").zfill(32)) 
            for x in range(12):
                try: current_byte_string[xrem+x+12] = '0'
                except:pass

            fin = process_string(current_byte_string)
            byte_string_raw[line_coor+(j*-width_res//8):line_coor+4+(j*-width_res//8)] =  fin
            j+=1
            byte_string_raw[line_coor+(j*-width_res//8):line_coor+4+(j*-width_res//8)] =  fin


        # start = width_res//8+62
        # end = width_res//8+62
        # path = "/home/amadeok/epdiy-working/examples/pc_monitor/pc_host_app/"
        # header_path = "header1200x825" # header936x704 , 936x88header
        # with open(f"{path}{header_path}.bmp", "rb") as h:
        #     header = h.read()
        # temp_byte_arr = b''
        # for x in range(pad_bytes):
        #     temp_byte_arr+= b'\x00'
        # b2 = byte_string_raw[:]
        # for x in range(height_res): 
        #     # 936x702 -> 84299 to 84302;  
        #     b2[start:end] = temp_byte_arr
        #     #print(byte_string[0:end+30])
        #     start+=line_with_pad  # (936 + 24) / 8
        #     end+=line_with_pad
        #     #if byte_string[start:end] != b'\x00\x00\x00':
        #     #   print("warning")
        # b2[0:0] = header
        # with open(f"{path}cursortest.bmp", "wb") as out2:
        #     out2.write(b2)
        #     output = io.BytesIO()
        # image_file = Image.open(f"{path}cursortest.bmp") # for testing
        # if conf.rotation != 0:
        #     image_file   = image_file.rotate(180,  expand=True)
        # image_file = image_file.transpose(Image.FLIP_TOP_BOTTOM) 
        # image_file.save(f"{path}cursortest.bmp")
        # byte_string_raw2 = output.getvalue()
        if previous_pos.x != pos2.x and previous_pos.y != pos2.y:
            return 1
        else:
            return 0

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


def draw_cursor(conf, sct_img):
    global pos2
    previous_pos = pos2
    pos2 = pyautogui.position()
    x_offset = conf.x_offset
    width_res2 = conf.width_res2
    y_offset = conf.y_offset
    height_res2 = conf.height_res2

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
        if previous_pos.x != pos2.x and previous_pos.y != pos2.y:
            return 1
        else:
            return 0