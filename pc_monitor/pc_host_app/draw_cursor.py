import pyautogui, time, io, platform
from PIL import Image

windows= None; linux = None
if platform.system() == 'Linux': linux = True; 
elif platform.system() == 'Windows': windows = True;

class cursor_coor:
    def __init__(self):
        self.x = 0
        self.y = 0
curr_coor = cursor_coor()
prev_coor = cursor_coor()

if windows:
    import win32gui
    pos = win32gui.GetCursorPos()
    prev_coor.x = pos[0]; prev_coor.y = pos[1]

elif linux:
    prev_coor = pyautogui.position() 

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

    global curr_coor; global prev_coor
    
    if linux:
        prev_coor = curr_coor
        curr_coor = pyautogui.position()
    elif windows: 
        prev_coor.x = curr_coor.x
        prev_coor.y = curr_coor.y
        pos = win32gui.GetCursorPos()
        curr_coor.x = pos[0]; curr_coor.y = pos[1]
    
    if curr_coor.x >= x_offset and curr_coor.x <= width_res2 and curr_coor.y >= y_offset and curr_coor.y <= height_res2-22:
        t0 = time.time()
        x = int((curr_coor.x - x_offset) / 8)
        y = int((curr_coor.y - y_offset +1))
        #print(f"inside : curr_coor.x  {curr_coor.x } curr_coor.y  {curr_coor.y }, y {y}, x {x},  ")


        if conf.rotation == 180:
            x = width_res-x
            y = height_res-y
            xrem = 7- (curr_coor.x % 8)
            y-=8
            x-=4

        else:
            xrem = curr_coor.x % 8
            y+=16

        yrem = (curr_coor.y - y_offset) % 8
        
        line_coor = (y*width_res//8) + x #+ 62
        #print(f"inv x {x}, line_coor {line_coor}")
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

        if prev_coor.x == curr_coor.x and prev_coor.y == curr_coor.y:
            #print(f" not moved px {prev_coor.x}, cx {curr_coor.x}, py {prev_coor.y}, cy {curr_coor.y}")
            return 0
        else:
            #print(f" moved ::  px {prev_coor.x}, cx {curr_coor.x}, py {prev_coor.y}, cy {curr_coor.y}")
            return 1
    else:
        #print(f"outside : curr_coor.x  {curr_coor.x } curr_coor.y  {curr_coor.y }")
        return 0

def did_mouse_move(ctx):
    global curr_coor; global prev_coor

    if linux:
        prev_coor = curr_coor
        curr_coor = pyautogui.position()
    elif windows: 
        prev_coor.x = curr_coor.x
        prev_coor.y = curr_coor.y
        pos = win32gui.GetCursorPos()
        curr_coor.x = pos[0]; curr_coor.y = pos[1]

    if curr_coor.x >= ctx.x_offset and curr_coor.x <= ctx.width_res2 and curr_coor.y >= ctx.y_offset and curr_coor.y <= ctx.height_res2-22:
        if prev_coor.x == curr_coor.x and prev_coor.y == curr_coor.y:
          #  print(f" not moved px {prev_coor.x}, cx {curr_coor.x}, py {prev_coor.y}, cy {curr_coor.y}")
            return 0
        else:
         #   print(f" moved ::  px {prev_coor.x}, cx {curr_coor.x}, py {prev_coor.y}, cy {curr_coor.y}")
            return 1
    else:
       # print(f"outside : curr_coor.x  {curr_coor.x } curr_coor.y  {curr_coor.y }")
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


    x_offset = conf.x_offset
    y_offset = conf.y_offset

    width_res = conf.width
    height_res = conf.height
    width_res2 = conf.width_res2
    height_res2 = conf.height_res2

    global curr_coor; global prev_coor

    if linux:
        prev_coor = curr_coor
        curr_coor = pyautogui.position()
    elif windows: 
        prev_coor.x = curr_coor.x
        prev_coor.y = curr_coor.y
        pos = win32gui.GetCursorPos()
        curr_coor.x = pos[0]; curr_coor.y = pos[1]

    if curr_coor.x >= x_offset and curr_coor.x <= width_res2 and curr_coor.y >= y_offset and curr_coor.y <= height_res2:
        x_cursor = curr_coor.x - x_offset
        y_cursor = curr_coor.y - y_offset
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
        if prev_coor.x == curr_coor.x and prev_coor.y == curr_coor.y:
            #print(f" not moved px {prev_coor.x}, cx {curr_coor.x}, py {prev_coor.y}, cy {curr_coor.y}")
            return 0
        else:
            #print(f" moved ::  px {prev_coor.x}, cx {curr_coor.x}, py {prev_coor.y}, cy {curr_coor.y}")
            return 1
    else:
        #print(f"outside : curr_coor.x  {curr_coor.x } curr_coor.y  {curr_coor.y }")
        return 0
def paste_cursor(ctx, image_file):


    global curr_coor; global prev_coor; global cursor
    if linux:
        prev_coor = curr_coor
        curr_coor = pyautogui.position()
    elif windows: 
        prev_coor.x = curr_coor.x
        prev_coor.y = curr_coor.y
        pos = win32gui.GetCursorPos()
        curr_coor.x = pos[0]; curr_coor.y = pos[1]
    
    if curr_coor.x >= ctx.x_offset and curr_coor.x <= ctx.width_res2 and curr_coor.y >= ctx.y_offset and curr_coor.y <= ctx.height_res2-22:
        image_file.paste(ctx.cursor,box=(curr_coor.x-ctx.x_offset,curr_coor.y-ctx.y_offset),mask=ctx.cursor)
        if prev_coor.x == curr_coor.x and prev_coor.y == curr_coor.y:
            #print(f" not moved px {prev_coor.x}, cx {curr_coor.x}, py {prev_coor.y}, cy {curr_coor.y}")
            return 0
        else:
            #print(f" moved ::  px {prev_coor.x}, cx {curr_coor.x}, py {prev_coor.y}, cy {curr_coor.y}")
            return 1
    else:
        #print(f"outside : curr_coor.x  {curr_coor.x } curr_coor.y  {curr_coor.y }")
        return 0
   
