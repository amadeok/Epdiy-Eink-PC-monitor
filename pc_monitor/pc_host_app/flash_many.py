import ctypes
import subprocess
import msvcrt, time, threading
exiting = False

            
def esc_listener():
    global exiting
    while True:
        if exiting: break
        # Check if a key is pressed
        if msvcrt.kbhit():
            # Get the pressed key
            k = msvcrt.getch()
            key = ord(msvcrt.getch())
            print("-->key", k)
            # Check if it's the Escape key
            if key == 99:
                exiting = True
                for p in procs:
                    p.kill()
                break

                
        time.sleep(0.05)
        
import psutil, pygetwindow as gw
     
def get_window_handle_from_pid(pid):
    try:
        process = psutil.Process(pid)
        process_name = process.name()
        windows = gw.getWindowsWithTitle(process_name)
        if windows:
            return windows[0]._hWnd
        else:
            return None
    except psutil.NoSuchProcess:
        return None

def move_window(window_handle, x, y):
    w = gw.Window(window_handle)
    w.moveTo(x, y)

def main():
    serial_ports = ["COM19", "COM18", "COM16", "COM17"]  # Adjust this according to your system
    serial_ports = serial_ports[0:1]
    chip_type = "esp32"  # Adjust this according to your ESP chip
    baud_rate = 691200# 460800  # Adjust this according to your preference

    binary_file = r"C:\epdiy\examples\pc_monitor\build\firmware.bin"  


    escape_thread = threading.Thread(target=esc_listener)
    escape_thread.start()
    global procs    
    procs = []
    for i, port in enumerate(serial_ports):

        p = subprocess.Popen(["python", "flasher2.py", port, binary_file], creationflags=subprocess.CREATE_NEW_CONSOLE)
        time.sleep(1)
        h = get_window_handle_from_pid(p.pid)
        move_window(h, 0, i*600)
        procs.append(p)
        
    for thread in procs:
        thread.wait()
    global exiting
    
    time.sleep(5)
    exiting = True

if __name__ == "__main__":
    main()
