# Amiga Boing Ball Screensaver (Cross-Platform)

A screensaver for **Windows 10/11** and **macOS** that recreates the iconic Amiga Boing Ball demo with modern rendering and configurable options.

**Cross-platform support: Windows (OpenGL) and macOS (OpenGL)**

---

## ✨ Features

- **Bouncing Boing Ball** with realistic gravity, restitution, and spin physics
- **Cross-platform**: Windows and macOS
- **Configurable options**:
  - Floor shadow toggle
  - Wall shadow toggle
  - Grid overlay toggle
  - Sound effects toggle
  - Background color picker
  - Geometry mode (Classic 16×8 vs Smooth 64×32 tessellation)
- **Authentic Amiga aesthetics** with red-and-white checkered pattern
- **Smooth 60 FPS rendering**
- **Nostalgic boing sound effects**

---

## 📂 Project Structure

```
src/
├── core/                      # Platform-independent modules
│   ├── BoingPhysics.h/cpp     # Physics engine
│   ├── BoingRenderer.h/cpp    # OpenGL renderer
│   ├── BoingConfig.h          # Configuration data
│   └── Platform.h             # Platform abstraction
├── platform/
│   ├── windows/               # Windows implementation
│   │   ├── WindowsPlatform.h/cpp
│   │   └── BoingBallSaver.cpp
│   └── macos/                 # macOS implementation
│       ├── MacPlatform.h/mm
│       ├── MacBoingBallView.h/mm  # OpenGL screensaver
│       └── Info.plist
├── BoingBallSaver.rc          # Windows resources
└── resource.h                 # Resource IDs

sounds/                         # Audio files (WAV format)
docs/                           # Documentation
CMakeLists.txt                  # Cross-platform build
```

---

## 🚀 Building

### Prerequisites

- **Windows**: Visual Studio 2019+ with C++ support
- **macOS**: Xcode Command Line Tools (`xcode-select --install`)

### Build Instructions

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

**Output:**
- **Windows**: `build/Release/BoingBallSaver.scr`
- **macOS**: `build/BoingBallSaver.saver`

### Installation

**Windows:**
```bash
copy build\Release\BoingBallSaver.scr C:\Windows\System32\
```

**macOS:**
```bash
cp -r build/BoingBallSaver.saver ~/Library/Screen\ Savers/
```

---

## 🎮 Usage

### Windows
1. Right-click desktop → Personalize → Screen Saver
2. Select "Boing Ball" from the dropdown
3. Click "Settings" to configure options
4. Click "Preview" to test

### macOS
1. System Settings → Screen Saver
2. Select "Boing Ball" from the list
3. Click "Screen Saver Options" to configure
4. Preview appears automatically

---

## 🏗️ Architecture

This project uses a clean separation between platform-independent core code and platform-specific implementations:

- **Core**: Physics engine, OpenGL rendering, configuration (shared C++)
- **Platform Layer**: Audio, timing, configuration persistence (platform-specific)
- **Entry Points**: Windows `.scr` executable, macOS `.saver` bundle

See `docs/ARCHITECTURE.md` for detailed technical information.

---

## 📝 License

See LICENSE file for details.

---

## 🙏 Credits

Original Amiga Boing Ball demo by R. J. Mical  
Windows screensaver by Sinphaltimus Exmortus  
macOS port by Adam Bell

---

## 📚 Documentation

- `docs/BUILD_GUIDE.md` - Detailed build instructions
- `docs/ARCHITECTURE.md` - Technical architecture overview
