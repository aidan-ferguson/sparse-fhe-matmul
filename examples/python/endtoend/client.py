import argparse
import numpy as np
import tkinter as tk
from PIL import Image, ImageDraw
import socket
from halo import Halo

from pyfhe_sparse_matmul import SealCKKSContext, SparseCSRFHE
from common import recv_bytes, send_bytes
from model import preprocess_handwritten

class DigitWindow:
    """
    Class for getting a drawing of a digit from the user 
    """
    CANVAS_SIZE = 512

    def __init__(self, root):
        self.root = root
        self.root.title("Draw a Digit")
        
        self.canvas = tk.Canvas(root, width=DigitWindow.CANVAS_SIZE, height=DigitWindow.CANVAS_SIZE, bg='black')
        self.canvas.pack()

        self.image = Image.new("L", (DigitWindow.CANVAS_SIZE, DigitWindow.CANVAS_SIZE), color="black")
        self.draw = ImageDraw.Draw(self.image)

        self.canvas.bind("<B1-Motion>", self.paint)
        self.canvas.bind("<ButtonRelease-1>", self.reset)

        tk.Button(root, text="Submit", command=self.submit).pack()
        tk.Button(root, text="Clear", command=self.clear).pack()

        self.last_x, self.last_y = None, None

    def clear(self):
        self.image = None
        self.root.destroy()

    def paint(self, event):
        x, y = event.x, event.y
        if self.last_x and self.last_y:
            self.canvas.create_line((self.last_x, self.last_y, x, y), width=DigitWindow.CANVAS_SIZE//17, fill='white', capstyle=tk.ROUND, smooth=True)
            self.draw.line((self.last_x, self.last_y, x, y), fill='white', width=DigitWindow.CANVAS_SIZE//17)
        self.last_x, self.last_y = x, y

    def reset(self, _):
        self.last_x, self.last_y = None, None

    def submit(self):
        self.root.destroy()


def get_image():
    """
    Prompts the user to draw a digit and returns it in MNIST network input format.

    Returns:
        numpy.ndarray: The preprocessed image ready for network input
    """
    # Get an image from the user using digit window, preprocessed into mnist format for network input
    image = None
    while image is None:
        root = tk.Tk()
        app = DigitWindow(root)
        root.mainloop()
        image = app.image
    image = np.asarray(image.resize((28, 28)))
    image = preprocess_handwritten(image)
    return image


def connect_server(ip, port):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect((ip, port))
        print("- Connected to server")

        # Create FHE context and send the non-private runtime context to the server
        fhe_context = SealCKKSContext(8192)
        send_bytes(sock, fhe_context.runtime.serialize())

        while True:
            # Get handwritten digit from user, encrypt and send to server for inference
            image = get_image()
            with Halo(text='Encrypting and sending image', spinner='dots'):
                enc_image = SparseCSRFHE(image, fhe_context.runtime, 5)
                send_bytes(sock, enc_image.serialize())

            # We get the matrix output here (pre activation) as microsoft seal does not support
            # bootstrapping yet. This means we run out of noise budget and have to 'refresh'
            # Note, all computation is still performed in FHE space, we just emulate bootstrapping here
            with Halo(text='Waiting on FC-1 activation', spinner='dots'):
                enc_fc1_activation = recv_bytes(sock)
            fc1_activation = SparseCSRFHE(enc_fc1_activation, fhe_context.runtime).decrypt(fhe_context.secret)
            enc_fc1_activation = SparseCSRFHE(fc1_activation, fhe_context.runtime, 1)
            send_bytes(sock, enc_fc1_activation.serialize())

            with Halo(text='Waiting on FC-2 activation', spinner='dots'):
                enc_fc2_activation = SparseCSRFHE(recv_bytes(sock), fhe_context.runtime)
            fc2_activation = enc_fc2_activation.decrypt(fhe_context.secret)

            print(fc2_activation)
            print(f"- Server securely predicted '{np.argmax(fc2_activation)}' using homomorphic encryption sparse matmul.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Client for connecting to the server.")
    parser.add_argument("--ip", type=str, required=True, help="IP address of the server")
    parser.add_argument("--port", type=int, required=True, help="Port number of the server")
    args = parser.parse_args()

    print(f"Connecting to server at {args.ip}:{args.port}")
    connect_server(args.ip, args.port)