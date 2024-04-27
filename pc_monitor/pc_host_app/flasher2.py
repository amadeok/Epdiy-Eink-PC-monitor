import subprocess
import sys, os
import threading
import time
#



                
exiting = False
def flash_program(serial_port, chip_type, binary_file, baud_rate):
    flash_command = [
        "py", "-m", "esptool",
        #r"C:\Users\amade\AppData\Local\Programs\Python\Python310\Scripts\esptool.py.exe",
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
            subprocess.run(flash_command, check=True)
            print("Flash successful!")
            break
        except subprocess.CalledProcessError as e:
            print(f"Error flashing program: {e}, try {x}")
            if exiting: 
                print("Ctrl+C pressed. Exiting gracefully.")
                break
            time.sleep(1)

    
def main():
    if len(sys.argv) != 3:
        print("Usage: python flash_program.py <serial_port> <binary_file>")
        sys.exit(1)
        
    serial_port = sys.argv[1]
    binary_file = sys.argv[2]
    
    chip_type = "esp32"  # Adjust this according to your ESP chip
    baud_rate = 691200# 460800  # Adjust this according to your preference

    # escape_thread = threading.Thread(target=esc_listener)
    # escape_thread.start()
    
    flash_program(serial_port, chip_type, binary_file, baud_rate)

    global exiting
    exiting = True

if __name__ == "__main__":
    main()
