import subprocess
import sys, os
import threading
import time
#

import msvcrt

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
                break
        time.sleep(0.05)

                
#os.system(r"""C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Bypass -NoExit -File "C:\Espressif/Initialize-Idf.ps1" -IdfId esp-idf-7a538bcf490d7cccefdf2fcb09abae4f""")
exiting = False
#os.system("esptool.py")
def flash_program(serial_port, chip_type, binary_file, baud_rate):
    flash_command = [
        r"C:\Users\amade\AppData\Local\Programs\Python\Python310\Scripts\esptool.py.exe",
        "--chip",
        chip_type,
        "--port",
        serial_port,
        "--baud",
        str(baud_rate),  # Convert baud rate to string
        "write_flash",
        "0x10000",  # Address where the program will be flashed
        binary_file
    ]
    global exiting
    for x in range(100):
        try:
            # Run esptool.py command to flash the program
            subprocess.run(flash_command, check=True)
            print("Flash successful!")
            break

        # except KeyboardInterrupt:
        #     exiting = True
        #     # Additional cleanup code can be added here if n 
        except subprocess.CalledProcessError as e:
            print(f"Error flashing program: {e}, try {x}")
            if exiting: 
                print("Ctrl+C pressed. Exiting gracefully.")
                break
            time.sleep(1)

threads = []
def flash_in_thread(serial_port, chip_type, binary_file, baud_rate):
    thread = threading.Thread(target=flash_program, args=(serial_port, chip_type, binary_file, baud_rate))
    threads.append(thread)
    thread.start()
    
def main():
    # Serial ports where the ESP devices are connected
    serial_ports = ["COM16", "COM17", "COM18", "COM19"]  # Adjust this according to your system
    serial_ports = serial_ports
    # Type of ESP chip, either "esp32" or "esp8266"
    chip_type = "esp32"  # Adjust this according to your ESP chip
    baud_rate = 691200# 460800  # Adjust this according to your preference

    # Path to the binary file of your program
    binary_file = r"C:\epdiy\examples\pc_monitor\build\firmware.bin"  # Adjust this with the path to your binary file


    escape_thread = threading.Thread(target=esc_listener)
    escape_thread.start()
    
    for port in serial_ports:
        flash_in_thread(port, chip_type, binary_file, baud_rate)
    for thread in threads:
        thread.join()
    global exiting
    exiting = True

if __name__ == "__main__":
    main()
