import socket
import struct
import time

def parse_received_data(data):
    floats = struct.unpack('3f', data)
    return floats

def run_client():
    host = '192.168.203.1'
    port = 32001

    # Create a TCP socket
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    try:
        # Connect to the server
        client_socket.connect((host, port))
        print(f"Connected to {host}:{port}")
        cmd = b'A\n'
        while True:
            # Send 'A' to the server
            print(cmd)
            client_socket.send(cmd)

            # Receive 12 bytes of data from the server
            data = client_socket.recv(14)
            print(', '.join(hex(byte) for byte in data))  # 将字节数组以16进制形式拼接为一行并打印

            # Parse the received data into three float numbers
            floats = parse_received_data(data[:-2])

            # Print the float numbers
            print("Received float numbers:", floats)

            # Wait for 1 second before sending the next 'A'
            time.sleep(1)

    except ConnectionRefusedError:
        print(f"Connection to {host}:{port} refused.")

    finally:
        # Close the client socket
        client_socket.close()

if __name__ == "__main__":
    run_client()
