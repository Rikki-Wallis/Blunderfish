import sys

if len(sys.argv) == 1:
    print(f"Usage {sys.argv[0]} <path>")
else:
    path = sys.argv[1]

    try:
        file = open(path, "w")
    except:
        print(f"failed to write {path}")
        sys.exit(1)

    file.write("whats good bro\n")