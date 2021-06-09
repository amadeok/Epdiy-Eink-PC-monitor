import os, time, sys
from PIL import Image
t_counter = 0
def t():
    global t_counter;global t0
    t_counter+=1
    if t_counter == 1:
        t0 = time.time()
    elif t_counter == 2:
        t_counter = 0
        print(time.time()-t0)
    return time.time()

l = []
t()
nb_draws = 2
dir = "C:\\Users\\amade\\Documents\\pc_host_app\\"
for x in range(nb_draws):
    inp = open(f"{dir}\\eink_framebuffer{x}", "rb" )
    l.append(inp.read())

size = 1200 * 825 * 3
bytearr = []
print(len(l[0]))
for x in range(nb_draws):
    bytearr.append(bytearray([0] * size))

strl = []
bytel = []
i = 255//nb_draws

red = bytearray((i, 0, 0))
blu = bytearray((0, 0, i))
green = bytearray((0, i, 0))
black = bytearray((0, 0, 0))
white = bytearray((i, i, i))
d = {
    '00': black,
    '01': red,
    '10': blu,
    '11': white,

}

for x in range(256):
    current_byte_string = list(format(x, "b").zfill(8))
    current_string =  ''.join(current_byte_string)

    pixel_pack = d[current_string[6:8]] + d[current_string[4:6]] + d[current_string[2:4]] + d[current_string[0:2]]
    strl.append(current_string)
    bytel.append(pixel_pack)

zipbObj = zip(strl, bytel)
d2 = dict(zipbObj)

def rebuild_fb():


    for y in range (nb_draws):
        for z in range(len(l[0])):
            oribyte = int.from_bytes(l[y][z:z+1], 'big')
            current_byte_string = list(format(oribyte, "b").zfill(8))
            current_string =  ''.join(current_byte_string)
            inverted_out = ''
            x = 8
            while x > 0:
                pixel_pos = (x)
                inverted_out += current_string[pixel_pos-2:pixel_pos]
                x-=2
            bytearr[y][(z*4*3):(z*4*3)+4*3] = d2[inverted_out]
        f = 0

rebuild_fb()
for x in range(nb_draws):
    img = Image.frombytes("RGB", (1200, 825), bytes(bytearr[x]))
    img.save(f"test{x}.bmp")

joined = (bytearray([0] * size))

for y in range (1):
    for f in range(size):
        for z in range(nb_draws):
            joined[f] += bytearr[z][f]
            # a = bytearr[0][f]#int.from_bytes(bytearr[0][f], 'big')
            # b = bytearr[1][f]#int.from_bytes(bytearr[1][f], 'big')
            # c = bytearr[2][f]#int.from_bytes(bytearr[2][f], 'big')

        #joined[f] = a+b+c
img = Image.frombytes("RGB", (1200, 825), bytes(joined))
img = img.transpose(Image.FLIP_TOP_BOTTOM) #flip the image so that the first bytes contain the pixel data of the first lines
img   = img.rotate(180,  expand=True)
img.save("testj.bmp")

t()

