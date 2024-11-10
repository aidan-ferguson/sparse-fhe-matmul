## End-to-End MNIST DNN example

This demonstrates using the library Python bindings in a real world client/server configuration for inference on a DNN trained on the MNIST dataset.
Speed can be increased by increasing the pruning level (sparsity) on the server

### Running

Please ensure the python bindings have been build and installed according to the [README.md](../../../README.md) file in the root of the repository.


#### Server
Start the server using the following command

```bash
python3 server.py --ip <default: 0.0.0.0> --port <default: 5555> --sparsity <default 0.5>
```
The server will attempt to bind to the `ip` and `port` parameters. DNN weights will be pruned to the level defined by `sparsity`

On first run, the server will download the MNIST dataset and train a simple [dense network](model.py), it will be cached to disk after the first run.

#### Client
Start the client with the following command

```bash
python3 client.py --ip --port <default: 5555>
```

This will attempt to connect to the server, exchange information about the FHE runtime then prompt the user for a hand-written digit to predict. In my experience it helps to use a touch screen here as it is a bit more representative.

After some (quite a bit) of time, you will see the server processing the data and the client will decrypt the result and display it on the command line.

### Notes

1. __This example does not perform bootstrapping__. Microsoft SEAL does not yet support bootstrapping for CKKS, as such we must refresh the encrypted activations by sending it back to the client for decryption and re-encryption. This does expose information about the weights to the client. However, the input data is _still encrypted_ from the viewpoint of the server. Additionally, all computation is performed in the encrypted domain.

2. __Model Complexity__. In order to keep runtime low, this model is relatively small with only ~80% accuracy on MNIST, quite low compared to modern approaches. As such, pruning the weights does drop off accuracy quite quickly.

3. __Non-linearity__. As we cannot use ReLu or other common activation functions (can't perform comparisons in FHE). We introduce non-linearity into the model by squaring the layer outputs. We also don't introduce bias into the model. Note, while it is possible to approximate ReLu, you need quite a large multiplicative depth, without bootstrapping this would become quite annoying. 