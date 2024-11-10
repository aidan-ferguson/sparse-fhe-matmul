import socket
import threading
import torch
import os
from pathlib import Path
import argparse

from pyfhe_sparse_matmul import SealCKKSRuntimeContext, SparseCSRFHE
from model import load_network, prune_layer
from common import recv_bytes, send_bytes


# TODO: common naming convention for sockets
def process_socket(sock, client_address, network, sparsity):
    print(f"- Client connected from {client_address}.")

    # Listen for CKKS runtime context
    rt_buff = recv_bytes(sock)
    fhe_rt = SealCKKSRuntimeContext(rt_buff)

    # Load and encrypt weights using runtime context
    fc1_T = network.fc1.weight.to(torch.float64).cpu().detach().numpy().T
    fc2_T = network.fc2.weight.to(torch.float64).cpu().detach().numpy().T
    fc1_T = prune_layer(fc1_T, sparsity=sparsity)
    fc2_T = prune_layer(fc2_T, sparsity=sparsity)

    print("- Encrypting neural network weights, this may take a while")
    enc_fc_1_T = SparseCSRFHE(fc1_T, fhe_rt, 5)
    enc_fc_2_T = SparseCSRFHE(fc2_T, fhe_rt, 5)

    try:
        while True:
            # Listen for encrypted image from client
            enc_image_buff = recv_bytes(sock)
            enc_image = SparseCSRFHE(enc_image_buff, fhe_rt)

            # Perform fc1 inference then send back to client for activation function and refreshing
            # Note, when Microsoft SEAL implements CKKS bootstrapping, this will not have to happen
            print("- Performing FC-1 sparse activation")
            enc_fc1_activation = enc_image.fhe_matmul(enc_fc_1_T, fhe_rt, os.cpu_count())
            enc_fc1_activation.square_inplace(fhe_rt)
            send_bytes(sock, enc_fc1_activation.serialize())

            enc_fc1_activation = SparseCSRFHE(recv_bytes(sock), fhe_rt)
            print("- Performing FC-2 sparse activation")
            enc_fc2_activation = enc_fc1_activation.fhe_matmul(enc_fc_2_T, fhe_rt, os.cpu_count())
            enc_fc2_activation.square_inplace(fhe_rt)
            send_bytes(sock, enc_fc2_activation.serialize())

    except KeyboardInterrupt:
        sock.close()
        raise KeyboardInterrupt


def start_server(ip, port, sparsity):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((ip, port))
    server_socket.listen(5)

    network = load_network(Path("mnist.pt"))
    
    print(f"- Server listening on {ip}:{port}")
    try:
        while True:
            sock, client_address = server_socket.accept()
            client_handler = threading.Thread(target=process_socket, args=(sock, client_address, network, sparsity))
            client_handler.start()
    except ConnectionError:
        print("Connection error occurred, client disconnected")
    except KeyboardInterrupt:
        server_socket.close()
            

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Client for connecting to the server.")
    parser.add_argument("--ip", type=str, default="0.0.0.0", help="IP to bind the server to")
    parser.add_argument("--port", type=int, default=5555, help="Port to bind the server to")
    parser.add_argument("--sparsity", type=float, default=0.5, help="Sparsity to prune the weights to")
    args = parser.parse_args()

    start_server(args.ip, args.port, args.sparsity)