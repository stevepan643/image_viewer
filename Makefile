CC = gcc
CFLAGS = -Wall -Isrc -Isrc/glad/include
LIBS = -lGL -lglfw -lm

TARGET = iv

DEBUGFLAG = -g

SRCS = src/glad/src/glad.c \
	   src/main.c \
	   src/img.c \
	   src/img_png.c \
	   src/img_bmp.c \
	   src/img_ppm.c \
	   src/bit_stream.c

OBJS = $(addprefix obj/, $(notdir $(SRCS:.c=.o)))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LIBS) $(DEBUGFLAG)

vpath %.c src:src/glad/src

obj/%.o: %.c | obj
	$(CC) $(CFLAGS) -c $< -o $@ $(DEBUGFLAG)

obj:
	mkdir -p obj

clean:
	rm -rf obj $(TARGET)

.PHONYL: all clean
