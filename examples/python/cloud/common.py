def recv_bytes(socket):
    # Get data payload size
    size_data = socket.recv(4)
    if size_data is None:
        raise ConnectionError
    n = int.from_bytes(size_data, byteorder='big')
    
    # Read n  bytes from socket
    data = b''
    while len(data) < n:
        packet = socket.recv(n - len(data))
        if not packet:
            raise ConnectionError
        data += packet
    if len(data) == 0:
        raise ConnectionError
    return data


def send_bytes(socket, bytes): 
    # Send the size of the payload followed by the data itself
    socket.sendall(len(bytes).to_bytes(4, byteorder='big'))
    socket.sendall(bytes)