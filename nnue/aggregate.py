import os
import glob

out = open("aggregate.bin", "wb")

struct_size = 12*8+2*4

for path in glob.glob(os.path.join("dataset", "*.bin")):
    with open(path, "rb") as f:
        data = f.read()
        assert len(data)%struct_size == 0
        out.write(data)