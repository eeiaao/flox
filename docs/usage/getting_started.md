# Getting Started

Step-by-step guide to **build, test, and install** Flox on your machine.

---

## Prerequisites

| Tool / Library | Recommended Version | Notes |
|----------------|---------------------|-------|
| C++ compiler   | **C++20** (GCC ≥ 13) | Clang 15+ also works |
| CMake          | **3.22 or newer**   | Ninja generator optional |
| Git            | Latest stable       | — |
| Linux          | x86-64              | Other POSIX may work |
| GoogleTest     | latest              | For unit tests |
| Google Benchmark| latest             | For micro-benchmarks |
| clang-format   | **18.1.8**          | Ensures CI style compliance |

---

## Clone & Build

```bash
git clone https://github.com/eeiaao/flox.git
cd flox
mkdir build && cd build
cmake ..
make -j$(nproc)
````

---

## Installing Dependencies

If `gtest` and `benchmark` are **not** available system-wide:

```bash
# -------- GoogleTest --------
git clone --depth=1 https://github.com/google/googletest.git
cmake -B gtest-build -S googletest
cmake --build gtest-build --target gtest gtest_main gmock gmock_main
sudo cmake --install gtest-build

# -------- Google Benchmark --------
git clone --depth=1 https://github.com/google/benchmark.git
cmake -B benchmark-build -S benchmark \
      -DCMAKE_BUILD_TYPE=Release \
      -DBENCHMARK_DOWNLOAD_DEPENDENCIES=ON
cmake --build benchmark-build -j$(nproc)
sudo cmake --install benchmark-build
```

> If your package manager already provides these libraries, skip the manual steps.

### clang-format 18

CI enforces code style with clang-format 18:

```bash
sudo apt install -y wget gnupg lsb-release software-properties-common
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 18
sudo apt install -y clang-format-18
sudo update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-18 100
```

---

## Custom CMake Options

| Option             | Default | Effect                       |
| ------------------ | ------- | ---------------------------- |
| `ENABLE_TESTS`     | **ON**  | Build unit tests             |
| `ENABLE_BENCHMARK` | **ON**  | Build performance benchmarks |

Disable options:

```bash
cmake .. -DENABLE_TESTS=OFF -DENABLE_BENCHMARK=OFF
```

---

## Running Tests

```bash
cd build
ctest --output-on-failure
```

---

## Running Benchmarks

```bash
cd build
./benchmarks/full_order_book_benchmark
```

Any other binary in `benchmarks/` can be executed the same way.

---

## Install System-Wide

```bash
sudo make install
```

---

## Contributing

1. Follow existing directory layout and naming conventions.
2. **Add tests, benchmarks, and docs** for new features.
3. Code must pass `clang-format` 18; a `.clang-format` file is provided.

A *pre-commit* hook that auto-formats changed files is installed by default when
you run `cmake ..`. To install manually:

```bash
cp scripts/pre-commit .git/hooks/pre-commit
chmod +x .git/hooks/pre-commit
```

---

## Using Flox in Your Project

Flox is a set of reusable **low-latency building blocks**.
Link the library, include the headers you need, and compose subsystems to build
your own HFT engine or market-data infrastructure.
