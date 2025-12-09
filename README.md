<p align="center"><img src="resources/icons/hicolor/256x256/apps/iv.png"></p>
<p align="center">Minimal image viewer</p>

# Table of Contents

1. [Supported Image Formats](#supported-image-formats)
2. [Installation](#installation)
3. [Usage](#usage)
4. [Default Keybindings](#default-keybindings)
5. [Configuration](#configuration)

## Supported Image Formats

+ All the well-known image formats.
+ Includes support for animated images.

[Look here](https://imagemagick.org/script/formats.php#supported)

+ AVIF support [OPTIONAL]

## Installation

1. Install the following dependencies:

    - Qt6
    - ImageMagick
    - cmake & ninja (for building)
    - libavif [OPTIONAL] (for avif image support)

> [!NOTE]
> libavif is most probably installed on most systems already.
> If found during compilation, AVIF support will automatically be enabled.

2. Run the following commands to clone the repo and build iv

```bash
git clone github.com/dheerajshenoy/iv
cd iv
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release
ninja
sudo ninja install
```

## Usage

Open images

```bash
iv <path-to-files>
```

## Default Keybindings

| Key Combo | Action              |
|-----------|---------------------|
| Ctrl+W    | Close File          |
| o         | Open File           |
| q         | Open File Location  |
| =         | Zoom In             |
| -         | Zoom Out            |
| >         | Rotate Clockwise    |
| <         | Rotate Anticlockwise|
| 1         | Fit to Width        |
| 2         | Fit to Height       |
| h         | Scroll Left         |
| j         | Scroll Down         |
| k         | Scroll Up           |
| l         | Scroll Right        |
| t         | Toggle Tabs         |
| m         | Toggle Minimap      |

## Configuration

Iv is configured using `TOML`. The configuration file is expected at `~/.config/iv/config.toml`.
A sample configuration file is included in this repository.
