# Boing Ball Screensaver for macOS

A macOS port of the [Amiga Boing Ball Screensaver for Windows](https://github.com/Sinphaltimus/Amiga-Boing-Ball-Screensaver-for-Windows-10-and-11), featuring smooth 120 FPS animation and authentic sound effects.

> **Note:** This is a macOS-only port. For the original Windows version, see the [original repository](https://github.com/Sinphaltimus/Amiga-Boing-Ball-Screensaver-for-Windows-10-and-11).

## Features

- **Smooth 120 FPS animation** - Optimized rendering for buttery-smooth performance
- **Authentic sound effects** - Original bounce and wall hit sounds
- **Customisable appearance** - Configure shadows, grid, ball lighting, and background colour
- **FPS counter** - Optional on-screen frame rate display for performance monitoring
- **High-quality rendering** - Smooth or low-poly geometry options

## Requirements

- macOS 10.13 (High Sierra) or later
- Xcode Command Line Tools (for building from source)

## Installation

### From Source

1. Clone this repository:
```bash
git clone https://github.com/adamb3ll/Amiga-Boing-Ball-Screensaver.git
cd Amiga-Boing-Ball-Screensaver
```

2. Build the screensaver:
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

3. Install:
```bash
cmake --install .
```

The screensaver will be installed to `~/Library/Screen Savers/BoingBallSaver.saver`

4. Enable it:
   - Open System Settings → Desktop & Dock → Screen Saver
   - Select "Boing Ball" from the list
   - Click "Screen Saver Options" to configure

### Test App

A standalone test app is also built for performance testing:

```bash
cd build
./BoingBallTestApp.app
```

Press **Cmd+Q**, **ESC**, or **Q** to quit the test app.

## Configuration

Click "Screen Saver Options" in System Settings to configure:

- **Floor Shadow** - Show shadow on the floor
- **Wall Shadow** - Show shadow on the wall
- **Grid** - Display checkerboard grid
- **Sound** - Enable bounce sound effects
- **Smooth Geometry** - Use high-quality sphere rendering
- **Ball Lighting** - Enable dynamic lighting on the ball
- **Show FPS Counter** - Display frame rate in top-left corner
- **Background Colour** - Choose the background colour

## Performance

The screensaver is optimised for 120 FPS performance with:
- Direct OpenGL rendering (no AppKit overhead)
- Efficient physics calculations
- Cached OpenGL resources
- Optimised timer callbacks

## Building

### Prerequisites

- CMake 3.15 or later
- Xcode Command Line Tools

### Build Steps

```bash
mkdir build
cd build
cmake ..
cmake --build .
cmake --install .  # Installs to ~/Library/Screen Savers
```

### Targets

- `BoingBallSaver` - The screensaver bundle (.saver)
- `BoingBallTestApp` - Standalone test application (.app)

## License

Licensed under the MIT License. See LICENSE file for details.

## Credits

- **Original Amiga Boing Ball Demo**: Eric Graham (1986)
- **Windows Screensaver**: [Sinphaltimus Exmortus](https://github.com/Sinphaltimus/Amiga-Boing-Ball-Screensaver-for-Windows-10-and-11)
- **macOS Port**: adamb3ll

This macOS port maintains the authentic look and feel of the original Windows version while adding modern performance optimisations for macOS.

