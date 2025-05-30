# Getting Started

This guide helps you build and install Flox on your machine.

## Prerequisites

- C++20 or later (e.g., GCC 13)
- CMake 3.22+
- Git
- Linux (recommended)
- GoogleTest and Google Benchmark

## Clone and Build

```bash
git clone https://github.com/eeiaao/flox.git
cd flox
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Install Dependencies

If `gtest` and `benchmark` are not already available system-wide, you can install them manually:

```bash
# GoogleTest
git clone --depth=1 https://github.com/google/googletest.git
cmake -B gtest-build -S googletest
cmake --build gtest-build --target gtest gtest_main gmock gmock_main
sudo cmake --install gtest-build

# Google Benchmark
git clone --depth=1 https://github.com/google/benchmark.git
cmake -B benchmark-build -S benchmark -DCMAKE_BUILD_TYPE=Release -DBENCHMARK_DOWNLOAD_DEPENDENCIES=ON
cmake --build benchmark-build -j$(nproc)
sudo cmake --install benchmark-build
```

> If you already have them installed via your package manager, you can skip this step.

## Custom Build Options

Flox supports optional components controlled via CMake options:

- `ENABLE_TESTS` (default: ON) – build unit tests
- `ENABLE_BENCHMARK` (default: ON) – build performance benchmarks

You can disable them like so:

```bash
cmake .. -DENABLE_TESTS=OFF -DENABLE_BENCHMARK=OFF
```

## Run Tests

From the `build` directory:

```bash
ctest --output-on-failure
```

## Run Benchmarks

From the `build` directory:

```bash
./benchmarks/full_order_book_benchmark
```

You can also run any other benchmark binary in the `benchmarks/` directory.

## Install

To install the library system-wide:

```bash
sudo make install
```

## Use in Your Project

Flox is a collection of reusable low-latency building blocks.  
You can use it as a library for developing your own HFT engine or trading infrastructure.
