# Platform tespiti
ifeq ($(OS),Windows_NT)
    # Windows ayarları (MinGW / GCC)
    TARGET = mahjong.exe
    CC = gcc
    CFLAGS = -Wall -Wextra -O2
    LDFLAGS = -lraylib -lopengl32 -lgdi32 -lwinmm -mwindows
else
    # macOS ayarları (Clang / Homebrew)
    TARGET = mahjong
    CC = clang
    CFLAGS = -Wall -Wextra -O2 -I/opt/homebrew/include
    LDFLAGS = -L/opt/homebrew/lib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
endif

all: $(TARGET)

$(TARGET): mahjong.c
	$(CC) $(CFLAGS) mahjong.c -o $(TARGET) $(LDFLAGS)

clean:
	rm -f mahjong mahjong.exe
