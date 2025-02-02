import serial
import json, time

def read_serial_ports(ports):
    serial_connections = []

    # Open serial connections for each port
    for port in ports:
        try:
            ser = serial.Serial(port, baudrate=115200, timeout=1)  # Adjust baudrate and timeout as needed
            serial_connections.append(ser)
            print(f"Opened serial port {port}")
        except serial.SerialException as e:
            print(f"Failed to open serial port {port}: {e}")
        time.sleep(0.1)

    data_received = {key: None for key in ports}

    while True:
        for ser in serial_connections:
            try:
                if ser.in_waiting > 0:
                    data = ser.readline().decode().strip()
                    o = json.loads(data)
                    data_received[ser.port] = o
                    print(f"Data from {ser.port}: {o}")
            except serial.SerialException as e:
                print(f"Error reading from {ser.port}: {e}")
        for key, value in data_received.items():
            if not value: break
        else:
            for ser in serial_connections:
                ser.close()
            return data_received

# Example usage
if __name__ == "__main__":
    ports = ["COM16", "COM17", "COM18", "COM19"]  # Add your list of COM ports here
    recv = read_serial_ports(ports)
    print(recv)
