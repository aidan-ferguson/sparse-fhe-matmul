import torch
import torch.nn as nn
from torchvision import datasets, transforms
import numpy as np
import cv2
from scipy import ndimage
import math
from halo import Halo

BATCH_SIZE = 64

class MNISTNetwork(nn.Module):
    def __init__(self):
        super(MNISTNetwork, self).__init__()
        self.fc1 = nn.Linear(28*28, 128)
        self.fc2 = nn.Linear(128, 10)

    def forward(self, x):
        x = self.fc1(x)
        x = x * x
        x = self.fc2(x)
        x = x * x
        output = torch.nn.functional.log_softmax(x, dim=1)
        return output
    

def get_shift(img):
    """
    Calculate the best shift needed to center the mass of the image.
    Parameters:
        img (numpy.ndarray): The input image as a 2D numpy array.
    Returns:
        tuple: A tuple (shift_x, shift_y) where shift_x is the number of pixels to shift along the x-axis,
            and shift_y is the number of pixels to shift along the y-axis to center the mass of the image.
    """
    cy, cx = ndimage.center_of_mass(img)

    rows,cols = img.shape
    shift_x = np.round(cols / 2.0 - cx).astype(int)
    shift_y = np.round(rows / 2.0 - cy).astype(int)

    return shift_x, shift_y


def shift_image(image, sx, sy):
    """
    Shifts the input image by the specified number of pixels along the x and y axes.
    Parameters:
        img (array-like): The input image to be shifted.
        sx (int or float): The number of pixels to shift the image along the x-axis.
        sy (int or float): The number of pixels to shift the image along the y-axis.
    Returns:
        numpy.ndarray: The shifted image.
    """
    image = np.array(image)
    rows, cols = image.shape
    M = np.float32([[1 ,0, sx],[0, 1, sy]])
    image = cv2.warpAffine(image , M, (cols, rows))
    return image


def preprocess_handwritten(image):
    """
    Preprocesses a handwritten image for model input.
    1. Applies a blur filter to the image.
    2. Trims empty rows and columns from the edges of the image.
    3. Resizes the image to fit within a 20x20 box while maintaining aspect ratio.
    4. Pads the image to fit within a 28x28 box.
    5. Shifts the image to center it based on the best shift values.
    6. Normalizes the pixel values
    Args:
        image (numpy.ndarray): The input handwritten image as a 2D numpy array.
    Returns:
        numpy.ndarray: The preprocessed image as a 1D numpy array with shape (1, 784).
    """

    # Thanks to https://medium.com/@o.kroeger/
    image = cv2.blur(image, (2,2))  
    while np.sum(image[0]) == 0:
        image = image[1:]

    while np.sum(image[:,0]) == 0:
        image = np.delete(image,0,1)

    while np.sum(image[-1]) == 0:
        image = image[:-1]

    while np.sum(image[:,-1]) == 0:
        image = np.delete(image,-1,1)

    rows, cols = image.shape
    if rows > cols:
        factor = 20.0 / rows
        rows = 20
        cols = int(round(cols * factor))
        image = cv2.resize(image, (cols, rows))
    else:
        factor = 20.0 / cols
        cols = 20
        rows = int(round(rows*factor))
        image = cv2.resize(image, (cols, rows))

    colsPadding = (int(math.ceil((28 - cols) / 2.0)),int(math.floor((28 - cols)/2.0)))
    rowsPadding = (int(math.ceil((28 - rows) / 2.0)),int(math.floor((28 - rows)/2.0)))
    image = np.lib.pad(image,(rowsPadding,colsPadding),'constant')

    shift_x, shift_y = get_shift(image)
    image = shift_image(image, shift_x, shift_y)

    image = np.asarray(image, dtype=np.float64) / 255.0
    image = (image - 0.1307) / 0.3081
    return image.reshape(1, 28*28)


def train(model, device, train_loader, optimizer):
    """
    Train the given model using the provided data loader and optimizer.
    Args:
        model (torch.nn.Module): The neural network model to be trained.
        device (torch.device): The device (CPU or GPU) to perform training on.
        train_loader (torch.utils.data.DataLoader): DataLoader for the training data.
        optimizer (torch.optim.Optimizer): Optimizer for updating the model parameters.
    """

    model.train()
    for data, target in train_loader:
        data, target = data.to(device), target.to(device)
        data = data.reshape(-1, 28*28)
        
        optimizer.zero_grad()
        output = model(data)
        loss = torch.nn.functional.nll_loss(output, target)
        loss.backward()
        optimizer.step()


def test(model, device, test_loader):
    """
    Evaluate the model on the test dataset.
    Args:
        model (torch.nn.Module): The neural network model to be evaluated.
        device (torch.device): The device (CPU or GPU) on which the model and data are located.
        test_loader (torch.utils.data.DataLoader): DataLoader for the test dataset.
    Returns:
        tuple: A tuple containing:
            - test_loss (float): The average loss over the test dataset.
            - correct (int): The number of correct predictions over the test dataset.
    """

    model.eval()
    test_loss = correct = n_guess = 0
    with torch.no_grad():
        for data, target in test_loader:
            data, target = data.to(device), target.to(device)
            data = data.reshape(-1, 28*28)
            output = model(data)
            test_loss += torch.nn.functional.nll_loss(output, target, reduction='sum').item()
            pred = output.argmax(dim=1, keepdim=True)
            correct += pred.eq(target.view_as(pred)).sum().item()
            n_guess += pred.shape[0]

    test_loss /= len(test_loader.dataset)
    accuracy = correct / n_guess
    return test_loss, accuracy


def prune_layer(layer, sparsity):
    """
    Prunes the given layer by setting elements below a certain threshold to zero.
    Parameters:
        layer (numpy.ndarray): The layer to be pruned, represented as a NumPy array.
        sparsity (float): The fraction of elements to be pruned, between 0 and 1.
    Returns:
        numpy.ndarray: The pruned layer with elements below the pruning threshold set to zero.
    """

    sorted = np.sort(layer.flatten())
    pruning_threshold = sorted[int(sparsity*len(sorted))]
    layer[layer < pruning_threshold] = 0

    return layer


def load_network(save_path):
    device = torch.device("cuda") if torch.cuda.is_available() else torch.device("cpu")
    
    # Attempt to load network from path if it has already been trained
    if save_path.exists():
        with Halo(text='Loading neural network from disk', spinner='dots'):
            model = MNISTNetwork().to(device=device)
            model.load_state_dict(torch.load(save_path))
        model.eval()
        return model
    
    # Model not yet trained, we need to train
    train_kwargs = {'batch_size': BATCH_SIZE}
    test_kwargs = {'batch_size': BATCH_SIZE}
    if torch.cuda.is_available():
        cuda_kwargs = {'num_workers': 1, 'pin_memory': True, 'shuffle': True}
        train_kwargs.update(cuda_kwargs)
        test_kwargs.update(cuda_kwargs)

    # MNIST transforms
    transform=transforms.Compose([
        transforms.ToTensor(),
        transforms.Normalize((0.1307,), (0.3081,))
    ])
    train_dataset = datasets.MNIST('../data', train=True, download=True, transform=transform)
    test_dataset = datasets.MNIST('../data', train=False, transform=transform)
    train_loader = torch.utils.data.DataLoader(train_dataset,**train_kwargs)
    test_loader = torch.utils.data.DataLoader(test_dataset, **test_kwargs)

    model = MNISTNetwork().to(device)
    optimizer = torch.optim.Adadelta(model.parameters(), lr=1e-3)
    scheduler = torch.optim.lr_scheduler.StepLR(optimizer, step_size=1, gamma=0.7)

    # Run for a static 5 epochs, this seems to yield a decent accuracy for our simple network
    with Halo(text='Training neural network as checkpoint not found on disk', spinner='dots'):
        for epoch in range(0, 5):
            train(model, device, train_loader, optimizer, epoch)
            scheduler.step()
        loss, accuracy = test(model, device, test_loader)
    print(f"- Model trained with accuracy {accuracy} and loss {loss}")

    torch.save(model.state_dict(), save_path)
    model.eval()
    return model
