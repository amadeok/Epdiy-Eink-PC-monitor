
import mss
from PIL import Image
import time, subprocess
path = "/home/amadeo/epdiy/encoder/python/Pillow/"
file = "google.png"
input_file = f"{path}{file}"

#set capture frame size
monitor = {"top": 0, "left": 0, "width": 1200, "height": 825}
#set which mode to use, see examples
mode = 2
z = 0
while z < 2: ##loop for testing speed
    with mss.mss() as sct:
        # Get raw pixels from the screen
        sct_img = sct.grab(monitor)
    #make an image object for Pillow
    image_file = Image.open(input_file)
    if mode == 1:
        output_file = f'{path}result{mode}.bmp'
        image_file = image_file.convert('1') # convert image to black and white
        image_file.save(output_file)
    
    elif mode == 2:
        output_file = f'{path}result{mode}.bmp'
        threshold = 225
        fn = lambda x : 255 if x > threshold else 0
        r = image_file.convert('L').point(fn, mode='1')
        r.save(output_file)

    output_rle_c = f"{path}result{mode}.rle"
    output_rle_x = f"{path}result{mode}_x.bmp"

    ##compress with rle
    st = subprocess.run(["rle", "-c", f"{output_file}", f"{output_rle_c}"]) 
    t0 = time.time()
    ##extract the rle file
    st = subprocess.run(["rle", "-x", f"{output_rle_c}", f"{output_rle_x}"]) 
    print(time.time() - t0)
    time.sleep(0.03)
    z+= 0
