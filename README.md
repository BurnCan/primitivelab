# Dependencies

PrimitiveLab requires:

- A C++17-compatible compiler (GCC 9+, Clang 10+, or MSVC 2022+ recommended)
- CMake 3.16+
- Git

> **Note:** PrimitiveLab uses CMake's `FetchContent` module to automatically download and build third-party libraries during configuration. You **do not** need to manually install libraries such as GLFW, GLAD, or GLM.

---

# OS-Specific Setup

## 🪟 Windows (MSYS2 UCRT64)

1. Download and install the latest 64-bit version of MSYS2.
2. Open the **MSYS2 UCRT64** terminal.
3. Update MSYS2:

```bash
pacman -Syu
```

If prompted, restart the terminal and run:

```bash
pacman -Su
```

4. Install the required development tools:

```bash
pacman -S \
    mingw-w64-ucrt-x86_64-toolchain \
    mingw-w64-ucrt-x86_64-cmake \
    git
```

That's it! PrimitiveLab downloads all required third-party libraries automatically during the CMake configuration step.

---

## 🍏 macOS

Install Homebrew:

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

Follow the post-install instructions displayed by Homebrew to add it to your shell environment.

Then install the required tools:

```bash
brew install cmake git
```

PrimitiveLab downloads all required third-party libraries automatically during the CMake configuration step.

---

## 🐧 Ubuntu / Debian

Update your package list and install the required development tools:

```bash
sudo apt update

sudo apt install \
    build-essential \
    cmake \
    git \
    libgl1-mesa-dev \
    libx11-dev \
    libxi-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev
```

PrimitiveLab downloads all required third-party libraries automatically during the CMake configuration step.

---

# Building PrimitiveLab

Clone the repository:

```bash
git clone https://github.com/burncan/primitivelab.git
cd primitivelab
```

Alternatively:

💡 To clone a specific branch (e.g., development):
```bash
git clone --branch development --single-branch https://github.com/BurnCan/primitivelab
```

Configure the project:

```bash
cmake -S . -B build
```

During this step, CMake automatically downloads and builds any required third-party dependencies using `FetchContent`.

Build the project:

```bash
cmake --build build
```

---

# Running

## FPS controls

- **W/A/S/D:** Move along the ground
- **Mouse:** Look around
- **Space:** Jump
- **Left or right Ctrl:** Crouch

## Game

From the project root:

```bash
./build/Game
```

Or from inside the `build` directory:

```bash
./Game
```

## Editor

From the project root:

```bash
./build/Editor
```

Or from inside the `build` directory:

```bash
./Editor
```

## Shader assets and render modes

Renderable scene objects can independently use the texture pipeline or a discovered material. Texture shaders live in `shaders/texture`, internal renderer shaders and the shared material vertex shader live in `shaders/engine`, and selectable fragment shaders live directly in `shaders/material`. Adding a `.frag` file to `shaders/material` makes it available at the next application start without C++ registration. Material IDs are lowercase filename stems.
