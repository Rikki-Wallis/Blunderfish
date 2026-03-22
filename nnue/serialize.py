from nn import *

import numpy as np

model = NNUE()
state_dict = torch.load("model_epoch10_val0.005863.pt", map_location="cpu")
state_dict = {k.replace("_orig_mod.", ""): v for k, v in state_dict.items()}
model.load_state_dict(state_dict)
model.eval()

layers = [model.net[0], model.net[2], model.net[4]]

with open("model.bin", "wb") as f:
    for layer in layers:
        w = layer.weight.detach().numpy().astype(np.float32)
        b = layer.bias.detach().numpy().astype(np.float32)
        w.tofile(f)
        b.tofile(f)