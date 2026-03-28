import os
import glob

out = open("aggregate.bin", "wb")

for path in glob.glob(os.path.join("dataset", "*.bin")):
    with open(path, "rb") as f:
        out.write(f.read())