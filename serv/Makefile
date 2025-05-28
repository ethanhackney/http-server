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
SRC     = main.c 		\
	  ../lib/src/util.c	\
	  ../io/src/iobuf.c 	\
	  ../parse/src/lex.c	\
	  ../http/src/req.c	\
	  ../http/src/res.c
CC      = gcc

safe:
	./perf/perf
	$(CC) $(DFLAGS) $(SRC)

fast:
	./perf/perf
	$(CC) $(FFLAGS) $(SRC)
