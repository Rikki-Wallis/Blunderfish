import argparse
import struct

parser = argparse.ArgumentParser(description="Embed NNUE model into a C++ header")
parser.add_argument("model_bin_path", help="Path to the model binary file")
parser.add_argument("header_path", help="Path to the output header file")
args = parser.parse_args()

model_bin_path = args.model_bin_path
header_path = args.header_path

mb = open(model_bin_path, "rb")

def read_floats(n):
    global mb
    return list(struct.unpack(f"{n}f", mb.read(n*4)))

def read_float_matrix(rows, cols):
    return [read_floats(cols) for _ in range(rows)]

ninputs = 768
n1 = 256
n2 = 32

# load the weights

w0 = read_float_matrix(n1, ninputs)
b0 = read_floats(n1)

w1 = read_float_matrix(n2, n1)
b1 = read_floats(n2)

w2 = read_float_matrix(1, n2);
b2 = read_floats(1)

out = open(header_path, "w")

out.write("#pragma once\n\n")
out.write(f"constexpr size_t NNUE_INPUT_FEATURES = {ninputs};\n")
out.write(f"constexpr size_t NNUE_ACCUMULATOR_SIZE = {n1};\n\n")

def write_matrix(m, name):
    global out

    rows = len(m)
    cols = len(m[0])

    out.write(f"static const float nnue_{name}[{rows}][{cols}] = {{\n")
    for i in range(rows):
        out.write("    { ")
        for j in range(cols):
            if j > 0:
                out.write(", ")
            out.write(f"{m[i][j]}f")
        out.write(" },\n")
    out.write("};\n\n");

def write_vector(v, name):
    global out

    n = len(v)

    out.write(f"static const float nnue_{name}[{n}] = {{\n")
    out.write("  ")

    for i in range(n):
        if i > 0:
            out.write(", ")
        out.write(f"{v[i]}f")

    out.write("\n")
    out.write("};\n\n");

def transpose(m):
    rows = len(m)
    cols = len(m[0])

    result = [[0]*rows for _ in range(cols)]

    for i in range(rows):
        for j in range(cols):
            result[j][i] = m[i][j]
    
    return result

w0_t = transpose(w0)

write_matrix(w0_t, "w0")
write_vector(b0, "b0")
write_matrix(w1, "w1")
write_vector(b1, "b1")
write_matrix(w2, "w2")
write_vector(b2, "b2")