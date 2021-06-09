import os, sys, time

l = []

cdef int y = 0, x = 0, z = 0, f  = 0, a = 0, b = 0, c = 0
cdef int nb_draws = 2
nb_draws = int(sys.argv[1])
print("Number of framebuffers to rebuild:", nb_draws)

from PIL import Image

x = 0
y = 0
dir = "/home/amadeok/epdiy-working/examples/pc_monitor"
n0 = 0
while x < nb_draws:
    inp = open(f"/home/amadeok/epdiy-working/examples/pc_monitor/pc_host_app/eink_framebuffer{n0}", "rb" )
    l.append(inp.read())
    x+=1
    n0+=1

cdef int size = 1200 * 825 * 3
cdef int size2 = size/4
bytearr = []


x = 0
while x < nb_draws:
    bytearr.append(bytearray([0] * size))
    x+=1

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


def rebuild_fb(y , x , z , f, a, b, c ):

    while y < nb_draws:
        z = 0
        while z < size2:
            oribyte = int.from_bytes(l[y][z:z+1], 'big')
            current_byte_string = list(format(oribyte, "b").zfill(8))
            current_string =  ''.join(current_byte_string)
            inverted_out = ''
            # x = 8
            # while x > 0:
            #     pixel_pos = (x)
            #     inverted_out += current_string[pixel_pos-2:pixel_pos]
            #     x-=2
            bytearr[y][(z*4*3):(z*4*3)+4*3] = d2[current_string]
            z+=1
        f = 0
        y+=1
print("rebuild_fb")
rebuild_fb(y , x , z , f, a, b, c )

x = 0
for q in range(nb_draws):
    img = Image.frombytes("RGB", (1200, 825), bytes(bytearr[q]))
    img   = img.rotate(180,  expand=True)

    img.save(f"test{q}.bmp")
print("q in ran")

joined = (bytearray([0] * size))
y = 0; z = 0
print("joined")
print(size)

f = 0
while f < size:
    z = 0
    while z < nb_draws:
        joined[f] += bytearr[z][f]
        z+=1
    f+=1
    
img = Image.frombytes("RGB", (1200, 825), bytes(joined))
print(" Image.fromb")
#img = img.transpose(Image.FLIP_TOP_BOTTOM) #flip the image so that the first bytes contain the pixel data of the first lines
img   = img.rotate(180,  expand=True)
img.save("testj.bmp")

print(" img.save()")


