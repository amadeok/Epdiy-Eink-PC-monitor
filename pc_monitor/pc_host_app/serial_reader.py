import serial
import sys

def read_serial(serial_port):
    # Define the serial port settings
    baud_rate = 115200  # Change this to match your device's baud rate

    # Open the serial port
    ser = serial.Serial(serial_port, baud_rate)

    try:
        # Continuously read data from the serial port and print it
        while True:
            try:
                data = ser.readline().decode().strip()  # Read a line of data from the serial port
                print(data)
            except Exception as e:
                print("re", e) 
    except KeyboardInterrupt:
        # If Ctrl+C is pressed, close the serial port
        ser.close()

if __name__ == "__main__":
    # Check if the user provided the serial port as an argument
    if len(sys.argv) != 2:
        print("Usage: python script.py <serial_port>")
        sys.exit(1)

    serial_port = sys.argv[1]
    read_serial(serial_port)
