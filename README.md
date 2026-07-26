Dependencies
You will need:

C++17 compatible compiler
GCC 9+ / Clang 10+ recommended
CMake 3.16+
OpenGL 4.5
GLFW
GLAD
GLM
All third-party dependencies are fetched automatically via CMake where applicable.

OS Specific installation instructions for prerequisite tools:
🪟 Windows (MSYS2 + MinGW64)
1. Download and install the appropriate 64-bit version from the MSYS2 website. 
2. Open MSYS2 UCRT 64-bit terminal
Not MSYS, MinGW, or CLANG.

3. Update the package database and core system
pacman -Syu
# Restart terminal if prompted
pacman -Su
4. Install build tools
pacman -S mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-make git
📦 Optional: Install libraries manually Example:

pacman -S mingw-w64-x86_64-glfw mingw-w64-x86_64-glew mingw-w64-x86_64-glm mingw-w64-x86_64-imgui
If the libraries you want to install are included in FetchContent in CMake, this is not required.

🍏 macOS
1. Install Homebrew using the install script from the Homebrew website
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
💡 Important: Follow the post-install instructions to add Homebrew to your shell config.

Post-installation steps When you install Homebrew, it prints some directions for updating your shell’s config. If you don’t follow those directions, Homebrew will not work.

You need to update your shell’s config file (which file exactly depends on your shell, for example ~/.bashrc or ~/.zshrc) to include this Example:

echo 'eval "$(/opt/homebrew/bin/brew shellenv)"' >> ~/.zprofile
eval "$(/opt/homebrew/bin/brew shellenv)"
Replace with the directory where Homebrew is installed on your system. You can find Homebrew’s default install location in this FAQ entry.

2. Install tools
brew install cmake git
🐧 Ubuntu / Debian Linux
1. Install tools
sudo apt update
sudo apt install build-essential cmake git libgl1-mesa-dev libx11-dev libxi-dev libxrandr-dev libxinerama-dev libxcursor-dev


Build Instructions (Linux / macOS / Win)

bash

git clone https://github.com/burncan/primitivelab
cd primitivelab

mkdir build && cd build


cmake ..
cmake --build .

Running the Game
From the build directory:

bash

./Game

Running the Editor
From the build directory:

bash

./Editor
