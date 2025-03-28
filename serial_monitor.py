import serial
from datetime import datetime

# Configure the serial port
SERIAL_PORT = "COM3"  # Change to your port (e.g., "/dev/ttyUSB0" on Linux)
BAUD_RATE = 115200

def dump_serial_to_file():
    with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
        print(f"Listening on {SERIAL_PORT} at {BAUD_RATE} baud...")

        fileIncoming = False
        outfile = None

        try:
            while True:
                line = ser.readline()
                if line:
                    
                    if (b"FILE CONTENT END" in line):
                        fileIncoming = False
                        outfile.close()
                        continue

                    if fileIncoming:
                        outfile.write(line.strip() + b'\n')  # Write to file and add a newline

                    if (b"FILE CONTENT START" in line):
                        fileIncoming = True
                        current_time = datetime.now().strftime("%Y%m%d_%H%M%S")
                        output_filename = f"compact_hex_{current_time}.bin"
                        outfile = open(output_filename, "wb")

                        continue

                    print(line.decode('utf-8').strip())
        except KeyboardInterrupt:
            print("\nStopped by user.")
        finally:
            print(f"Exiting.")

if __name__ == "__main__":
    dump_serial_to_file()
