import argparse
import struct
import math
import random

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

def transpose(m):
    rows = len(m)
    cols = len(m[0])

    result = [[0]*rows for _ in range(cols)]

    for i in range(rows):
        for j in range(cols):
            result[j][i] = m[i][j]
    
    return result

w0_t = transpose(w0)

# quantize the weights

def quantize_matrix(m, q, bitwidth):
    qm = [[int(round(q*x)) for x in row] for row in m]
    assert all(abs(x) <= (1 << bitwidth)-2 for row in qm for x in row)
    return qm

def quantize_vector(v, q, bitwidth):
    qv = [int(round(q*x)) for x in v]
    assert all(abs(x) <= (1 << bitwidth)-2 for x in qv)
    return qv

w0_t = quantize_matrix(w0_t, 64, 16)
b0 = quantize_vector(b0, 64, 32)

w1 = quantize_matrix(w1, 64, 16)
b1 = quantize_vector(b1, 64*127, 32)

w2 = quantize_matrix(w2, 64, 16)
b2 = quantize_vector(b2, 64*127, 32)

# write the header

out = open(header_path, "w")

out.write("#pragma once\n\n")
out.write(f"constexpr size_t NNUE_INPUT_FEATURES = {ninputs};\n")
out.write(f"constexpr size_t NNUE_ACCUMULATOR_SIZE = {n1};\n\n")

def write_matrix(m, name, type):
    global out

    rows = len(m)
    cols = len(m[0])

    out.write(f"static const {type} nnue_{name}[{rows}][{cols}] = {{\n")
    for i in range(rows):
        out.write("    { ")
        for j in range(cols):
            if j > 0:
                out.write(", ")
            out.write(f"{m[i][j]}")
        out.write(" },\n")
    out.write("};\n\n");

def write_vector(v, name, type):
    global out

    n = len(v)

    out.write(f"static const {type} nnue_{name}[{n}] = {{\n")
    out.write("  ")

    for i in range(n):
        if i > 0:
            out.write(", ")
        out.write(f"{v[i]}")

    out.write("\n")
    out.write("};\n\n");

write_matrix(w0_t, "w0", "int16_t")
write_vector(b0,   "b0", "int32_t")
write_matrix(w1,   "w1", "int16_t")
write_vector(b1,   "b1", "int32_t")
write_matrix(w2,   "w2", "int16_t")
write_vector(b2,   "b2", "int32_t")