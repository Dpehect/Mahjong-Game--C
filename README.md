# Mahjong Solitaire (Shanghai) - C Desktop Game (Raylib)

This project is a clean, fully-featured, and highly interactive **Mahjong Solitaire** (Shanghai) desktop application written in pure C using the **Raylib** graphics library.

It opens a graphical window directly and supports native mouse inputs. It compiles and runs seamlessly on both **macOS** and **Windows**.

## Features
- **Direct GUI Window**: Runs in a standalone 1020x720 window, bypassing the terminal.
- **Mouse Controls**: Select, match, and clear cards directly using left-clicks.
- **3D Layer Layout (Pyramid)**: Features a symmetric 144-tile layout stacked across 4 vertical layers, rendered with a realistic 3D isometric offset.
- **Simplified Card Shapes**: Replaces complex traditional characters with universally recognized shapes:
  - **Circles (Blue)**: Renders Dots (D1-D9)
  - **Squares (Green)**: Renders Bamboos (B1-B9)
  - **Triangles (Red)**: Renders Characters (C1-C9)
  - **Stars (Gold)**: Renders Winds (WE, WS, WW, WN)
  - **Diamonds (Gray)**: Renders Dragons (DR, DG, DW)
  - **Hearts (Red)**: Renders Seasons (S1-S4) *(Any Season matches any other Season!)*
  - **Crosses (Purple)**: Renders Flowers (F1-F4) *(Any Flower matches any other Flower!)*
- **Dual Identification**: Each card face shows the shape grid in the center, the level layer (0-3) in the top-left, and the code indicator (e.g. `D5`, `WE`, `DR`, etc.) in the top-right corner.
- **Assistance Tools**: Clickable on-screen buttons for:
  - **HINT**: Highlights a valid matching pair on the board (-50 points).
  - **UNDO**: Rolls back the last match (-20 points).
  - **SHUFFLE**: Re-shuffles the remaining tiles when no moves are left.
  - **NEW GAME**: Restarts the game.
  - **EXIT**: Closes the game window.

## Requirements
You must have the **Raylib** library installed on your compiler path.

### 1. macOS Setup
Install via Homebrew:
```bash
brew install raylib
```

### 2. Windows Setup
Ensure a GCC environment (like w64devkit or MSYS2 MinGW) is set up, and place Raylib binaries in your library path.

---

## Compiling the Game

### 1. On macOS
Open a terminal in the project directory and run:
```bash
make
```
This generates a standalone executable named `mahjong`.

### 2. On Windows
Open a terminal (Command Prompt, PowerShell, or MSYS2) and run:
```bash
make
```
or compile directly with `gcc`:
```bash
gcc mahjong.c -o mahjong.exe -lraylib -lopengl32 -lgdi32 -lwinmm -mwindows
```
*Note: The `-mwindows` flag compiles the program as a Win32 GUI app, preventing a black terminal window from launching behind the game.*

---

## Running the Game

### macOS:
```bash
./mahjong
```

### Windows:
Double-click the generated `mahjong.exe` file to play!
