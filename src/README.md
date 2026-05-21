# CommandsApp

A lightweight, modern, and high-performance desktop application for **macOS** and **Ubuntu** written in C++ using **wxWidgets**. 
It helps you manage a quick-access list of terminal commands, code snippets, or texts. **Clicking any command in the list copies it instantly to your clipboard** with visual feedback, and you can easily add, edit, delete, and search your commands in real-time.

---

## Features
- 📋 **Single-Click to Copy**: Simply click any command in the list to copy it to your system clipboard.
- ⚡ **Real-Time Filtering**: Easily search through your commands dynamically as you type in the search bar.
- 💾 **Automatic Persistence**: All commands are saved to `~/.config/commands_app/commands.json` (or fallback text file) and loaded automatically when the app starts.
- 🎨 **Native Theme Alignment**: Clean integration with native system windowing (Cocoa on macOS, GTK on Linux), fully respecting dark and light mode settings automatically.
- 🛠️ **Simple CRUD**: Create, read, update (edit), and delete commands from a user-friendly UI.

---

## Build Requirements

Ensure you have a C++ compiler, CMake, and the **wxWidgets** development library installed.

### macOS Configuration
You can install the dependencies via Homebrew:
```bash
brew install cmake wxwidgets
```

### Ubuntu Configuration
You can install the dependencies via `apt-get`:
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake libwxgtk3.2-dev
```

---

## Compiling & Running

Once the dependencies are installed, you can build the application using CMake:

```bash
# 1. Create a build directory
mkdir build && cd build

# 2. Configure the project with CMake
cmake ..

# 3. Build the application
cmake --build .
```

### Running the App:
- **macOS**: An app bundle named `CommandsApp.app` is generated in the build directory. Run it by double-clicking it or using the terminal:
  ```bash
  open CommandsApp.app
  ```
- **Ubuntu**: An executable binary named `CommandsApp` is generated. Run it in terminal:
  ```bash
  ./CommandsApp
  ```
