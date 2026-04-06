import os
import glob
import numpy as np

struct_size = 12*8+4+1+1

with open("aggregate.bin", "wb") as out:
    for path in glob.glob(os.path.join("dataset", "*.bin")):
        data = np.fromfile(path, dtype=np.uint8).reshape(-1, struct_size)
        np.random.shuffle(data)
        out.write(data.tobytes())
        print(f"Wrote {path} ({len(data)} records)")