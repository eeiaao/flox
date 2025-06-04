# Getting Started

This guide helps you build and install Flox on your machine.

## Prerequisites

- C++20 or later (e.g., GCC 13)
- CMake 3.22+
- Git
- Linux (recommended)
- GoogleTest and Google Benchmark
- clang-format 18.1.8 (for development and contribution)

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

CI uses clang-format 18 to ensure formatting. For clang-format version matters (!). To install it locally:
```bash
sudo apt install -y wget gnupg lsb-release software-properties-common
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 18
sudo apt install -y clang-format-18
sudo update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-18 100
```

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

## Contribution

All contributions are welcome via pull requests.

Please follow the existing project structure and naming conventions.  
Add tests, benchmarks, and documentation where appropriate.  
We use `clang-format` for code style, the `.clang-format` file is provided in the project root.

A pre-commit hook that auto-formats changed `.cpp` and `.h` files is automatically installed when you run `cmake ..` (enabled by default).  
It will copy `scripts/pre-commit` into `.git/hooks/pre-commit`.

If needed, you can also install it manually:

```bash
cp scripts/pre-commit .git/hooks/pre-commit
chmod +x .git/hooks/pre-commit
```

## Use in Your Project

Flox is a collection of reusable low-latency building blocks.  
You can use it as a library for developing your own HFT engine or trading infrastructure.
