<p align="center"><img src="resources/icons/hicolor/256x256/apps/iv.png"></p>
<p align="center">Minimal image viewer</p>

> [!WARNING]
> This project is still in early development. Expect bugs and missing features.
> Current alpha version: 0.2.0

![iv-demo](images/iv.gif)
![minimap](images/minimap-demo.gif)

# Features

+ Tabbed interface to open multiple images in one window
+ Auto reload images when they change on disk (optional)
+ Minimap to preview the entire image and navigate easily (with click to move support)
+ Support for animated images
+ View EXIF metadata \[OPTIONAL\](Enabled if `libexiv2` is found during compilation)
+ Support for AVIF images \[OPTIONAL\](Enabled if `libavif` is found during compilation)
+ Configuration using `TOML` (with auto reload support)


## Supported Image Formats

+ All the well-known image formats.
+ Includes support for animated images.

[Supported Image Formats](https://imagemagick.org/script/formats.php#supported)

+ AVIF support \[OPTIONAL\](Enabled if `libavif` is found during compilation)
+ EXIF metadata viewing \[OPTIONAL\](Enabled if `libexiv2` is found during compilation)

## Installation

1. Install the following dependencies:

    - Qt6
    - ImageMagick
    - cmake & ninja (for building)
    - libavif [OPTIONAL] (for avif image support)
    - libexiv2 [OPTIONAL] (for EXIF metadata support)

> [!NOTE]
> libavif is most probably installed on most systems already.
> If found during compilation, AVIF support will automatically be enabled.
> Similarly, if libexiv2 is found, EXIF metadata support will be enabled.

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

## Command Line Options

```bash
iv [options] [files...]
```
Options:
```
-h, --help            Show this help message and exit
-v, --version         Show application version and exit
-c, --config          Use the specified configuration (TOML) file
--commands            Show list of available commands and exit
--not-tabbed          Open images in separate windows instead of tabs
```

## Configuration

Iv is configured using `TOML`. The configuration file is read from `~/.config/iv/config.toml` by default. You can specify a custom configuration file path using the `-c` or `--config` command line option.

**NOTE: A sample configuration file is included in this repository.**

# CHANGELOG

Check the [CHANGELOG](CHANGELOG.md) for more details on changes and updates.
