from flask import Flask, render_template, request, jsonify
import socket
import threading
import time
import struct

app = Flask(__name__)

# Configuration for the C++ TCP client connection.
CPP_CLIENT_HOST = '192.168.2.63' # IP of your PS5 running the nihonium payload
CPP_CLIENT_PORT = 60002  # Port of your nihonium payload waiting for commands

# Global persistent connection and lock for synchronizing access.
cpp_client_socket = None

def init_cpp_client_socket():
    """Initialize and return a new socket connection to the C++ TCP client."""
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(20)  # 20-second timeout for responsiveness
    s.connect((CPP_CLIENT_HOST, CPP_CLIENT_PORT))
    return s

def get_cpp_client_socket():
    """Return the persistent socket connection, reinitializing it if needed."""
    global cpp_client_socket
    if cpp_client_socket is None:
        cpp_client_socket = init_cpp_client_socket()
    return cpp_client_socket

def recv_all( num_of_bytes ):
    global cpp_client_socket

    data = b''
    while len( data ) < num_of_bytes:
        packet = cpp_client_socket.recv( num_of_bytes - len( data ) )

        if not packet:
            print("Invalid packet received in recv_all")
            return None

        data += packet
    
    return data

def recv_message_from_cpp_client():
    global cpp_client_socket

    # First I need to read the first message, which tells me the length of the next message
    # As the cpp client is x64 and sents an std::size_t the length of this message is always 8
    incoming_msg_length = recv_all( 8 )
    if incoming_msg_length is None:
        print("Error while receiving message from cpp client, message is None")
        return None
    
    msg_length = struct.unpack("<Q", incoming_msg_length)[ 0 ]
    print("The incoming message has", msg_length, "bytes")
    msg = recv_all( msg_length )

    return msg.decode( 'utf-8' )

def send_command_to_cpp_client(command):
    """Send a command to the C++ TCP client using the persistent connection and return the response."""
    global cpp_client_socket
    print("Sending command to ps5:", repr(command))
    try:
        sock = get_cpp_client_socket()
        sock.sendall(command.encode('utf-8'))
    except Exception as e:
        # On error, close the socket and reset the global connection.
        if cpp_client_socket:
            try:
                cpp_client_socket.close()
            except Exception:
                pass
        cpp_client_socket = None
        print("Error sending command:", e)

# Stub functions for each memory-read command.
def kread8(address):
    command = f"kread8 {address}"
    send_command_to_cpp_client(command)
    return recv_message_from_cpp_client()

def kread16(address):
    command = f"kread16 {address}"
    send_command_to_cpp_client(command)
    return recv_message_from_cpp_client()

def kread32(address):
    command = f"kread32 {address}"
    send_command_to_cpp_client(command)
    return recv_message_from_cpp_client()

def kread64(address):
    command = f"kread64 {address}"
    send_command_to_cpp_client(command)
    return recv_message_from_cpp_client()

def kreadblock(address, length):
    command = f"kreadblock {address} {length}"
    send_command_to_cpp_client(command)
    return recv_message_from_cpp_client()

# API endpoint for retreiving the list of processes
@app.route('/get_procs', methods=['GET'])
def get_procs():
    command = f"get_procs"

    send_command_to_cpp_client(command)
    
    response = recv_message_from_cpp_client()
    return jsonify({"processes": response})

# API endpoint to read a block of memory (default 1024 bytes)
@app.route('/read_block', methods=['GET'])
def read_block():
    address = request.args.get('address')
    length = request.args.get('length', default='1024')
    if not address:
        return jsonify({"error": "Missing address parameter"}), 400
    try:
        length = int(length)
    except ValueError:
        return jsonify({"error": "Invalid length parameter"}), 400
    data = kreadblock(address, length)
    return jsonify({"address": address, "length": length, "data": data})

# The original single-value read endpoint is still available.
@app.route('/read_memory', methods=['GET'])
def read_memory():
    address = request.args.get('address')
    data_type = request.args.get('type')
    
    if not address or not data_type:
        return jsonify({"error": "Missing address or type parameter"}), 400

    if data_type == 'uint8_t':
        data = kread8(address)
    elif data_type == 'uint16_t':
        data = kread16(address)
    elif data_type == 'uint32_t':
        data = kread32(address)
    elif data_type == 'uint64_t':
        data = kread64(address)
    else:
        return jsonify({"error": "Unsupported data type"}), 400

    return jsonify({"address": address, "type": data_type, "data": data})

# New endpoint to terminate ps5 payload.
@app.route('/terminate_payload')
def terminate_payload():
    command = f"quit"
    send_command_to_cpp_client(command)
    return "<h1>OrbisView payload terminate command sent!</h1>"

@app.route('/')
def index():
    return render_template('orbis_view.html')

@app.before_request
def initialize_cpp_connection():
    get_cpp_client_socket()

if __name__ == '__main__':
    app.run(debug=True)

