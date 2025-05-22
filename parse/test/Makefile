CFLAGS = -Wall  		\
	-Werror                 \
	-Wextra                 \
	-Wconversion            \
	-Wsign-conversion       \
	-Wshadow                \
	-Wstrict-prototypes     \
	-Wpointer-arith         \
	-Wcast-align            \
	-Wuninitialized         \
	-Winit-self             \
	-Wundef                 \
	-Wredundant-decls       \
	-Wwrite-strings         \
	-Wformat=2              \
	-Wswitch-enum           \
	-Wstrict-overflow=5     \
	-Wno-unused-parameter   \
	-pedantic
FFLAGS  = $(CFLAGS) -O3
DFLAGS  = $(CFLAGS) -DDBUG -fsanitize=address,undefined
SRC     = main.c ../../lib/src/util.c ../src/iobuf.c ../src/lex.c ../src/req.c
CC      = gcc

safe:
	./mkreq
	$(CC) $(DFLAGS) $(SRC)

fast:
	./mkreq
	$(CC) $(FFLAGS) $(SRC)
