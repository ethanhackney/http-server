#!/usr/bin/python3
import ctypes
import sys
import os

if len(sys.argv) == 1:
    print("no arguments given")
    exit(1)

if sys.argv[1] != "--name":
    print("no name given")
    exit(1)

lib = ctypes.CDLL("./lib.so")
lib.str_hash.argtypes = [ctypes.c_char_p, ctypes.c_size_t]
lib.str_hash.restype  = ctypes.c_size_t
name = sys.argv[2]
print(f"#ifndef {name}_H")
print(f"#define {name}_H\n")
for arg in sys.argv[3:]:
    try:
        with open(arg, "r") as f:
            kvps = []
            for line in f:
                k, v = line.strip().split("=")
                kvps.append((k, v))

            perf = dict()
            cap = 1
            kvp = 0
            while kvp < len(kvps):
                k, v = kvps[kvp]
                bkt = lib.str_hash(k.encode(), cap)
                if bkt not in perf:
                    perf[bkt] = (k, v)
                    kvp += 1
                    continue
                cap += 1
                perf.clear()
                kvp = 0

            tabname = os.path.splitext(os.path.basename(arg))[0]
            print(f"static const struct __STRUCT__ {tabname}_hash[{cap}] = {{")
            for bkt in range(cap):
                if bkt in perf:
                    k, v = perf[bkt]
                    print(f"\t{{ \"{k}\", {v} }},")
                else:
                    print("\t{ NULL },")
            print("};")
            print(f"static const size_t {tabname}_hash_cap = {cap};\n")
    except Exception as e:
        print(f"perf: {e}")

print(f"#endif /* {name}_H */")
