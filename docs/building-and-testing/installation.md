# Installation Guide

To install FlashFIX you must compile it from source, as that will guarantee the best performance.
You can customize the maximum number of FIX fields per message by specifying the FIX_MAX_FIELDS variable at compile time, the default value is 256.

## Requirements

  - CMake 3.10 or later
  - gcc with c23 support
  - CPU with unaligned memory access support

## Building

  - Clone the repository: ```git clone https://github.com/Raimo33/FlashFIX.git``` or download the source code from the [release page](https://github.com/Raimo33/FlashFIX/releases)
  - Generate the build files: ```cmake .``` (add ```-DFIX_MAX_FIELDS=512``` to change the maximum number of fields)
  - Build the library: ```cmake --build .```
  - Install the library: ```cmake --install .``` (optional)

## Testing

For testing, the library must to be compiled with -DFIX_MAX_FIELDS=64.
  
  - Compile the tests: ```cmake --build . --target test```
  - Run the test executable: ```./test```

in case of failure, please open an issue on [GitHub](https://github.com/Raimo33/FlashFIX/labels/test-failed) if there isn't one already.