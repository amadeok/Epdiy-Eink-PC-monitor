import os
from PIL import Image

l = []
nb_draws = 3
dir = "/home/amadeok/epdiy-working/examples/pc_monitor"
for x in range(nb_draws):
    inp = open(f"/home/amadeok/epdiy-working/examples/pc_monitor/eink_framebuffer{x}", "rb" )
    l.append(inp.read())

size = 1200 * 825
bytearr = []
print(len(l[0]))
for x in range(nb_draws):
    bytearr.append(bytearray([0] * size))

def rebuild_fb():
    
    d = {   '00000000': b'\x00'+b'\x00'+b'\x00'+b'\x00' ,
            '01010101': b'\x55'+b'\x55'+b'\x55'+b'\x55', 
            '00000001': b'\x00'+b'\x00'+b'\x00'+b'\x55',
            '00000100': b'\x00'+b'\x00'+b'\x55'+b'\x00',
            '00010000': b'\x00'+b'\x55'+b'\x00'+b'\x00',
            '01000000': b'\x55'+b'\x00'+b'\x00'+b'\x00',
            '01000001': b'\x55'+b'\x00'+b'\x00'+b'\x55',
            '00000101': b'\x00'+b'\x00'+b'\x55'+b'\x55',
            '00010100': b'\x00'+b'\x55'+b'\x55'+b'\x00' ,
            '01010000': b'\x55'+b'\x55'+b'\x00'+b'\x00', 
            '01010100': b'\x55'+b'\x55'+b'\x55'+b'\x00', 
            '00010101': b'\x00'+b'\x55'+b'\x55'+b'\x55',  
            '01000100': b'\x55'+b'\x00'+b'\x55'+b'\x00',
            '00010001': b'\x00'+b'\x55'+b'\x00'+b'\x55',
            '01010001': b'\x55'+b'\x55'+b'\x00'+b'\x55', 
            '01000101': b'\x55'+b'\x00'+b'\x55'+b'\x55',  

 }
    n = d["01000101"]
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
            bytearr[y][(z*4):(z*4)+4] = d[inverted_out]
        f = 0

rebuild_fb()
for x in range(nb_draws):
    img = Image.frombytes("L", (1200, 825), bytes(bytearr[x]))
    img.save(f"test{x}.bmp")

joined = (bytearray([0] * size))

for y in range (1):
    for f in range(size):
        a = bytearr[0][f]#int.from_bytes(bytearr[0][f], 'big')
        b = bytearr[1][f]#int.from_bytes(bytearr[1][f], 'big')
        c = bytearr[2][f]#int.from_bytes(bytearr[2][f], 'big')

        joined[f] = a+b+c
img = Image.frombytes("L", (1200, 825), bytes(joined))
img.save("testj.bmp")



