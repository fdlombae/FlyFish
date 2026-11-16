# FlyFish

3D PPGA C++ Library used in Howest for the elective course Geometric Algebra.

Header-only, C++23. Everything lives in the `flyfish` namespace; the named
operator functions are in `flyfish::GA`.

## Requirements

A compiler with C++23 `if consteval`:

| Compiler      | Minimum version | Flag                                      |
|---------------|-----------------|-------------------------------------------|
| Visual Studio | 2022 17.14      | `/std:c++latest` (or `/std:c++23preview`) |
| GCC           | 12              | `-std=c++23`                              |
| Clang         | 14              | `-std=c++23`                              |

With CMake the flag is set for you (see below).

## Quick start

```cpp
#include <flyfish/FlyFish.h>
using namespace flyfish;

const TriVector p = TriVector::Point(1, 0, 0);
const BiVector zAxis = BiVector::LineFromPoints(TriVector::Point(0, 0, 0),
                                                TriVector::Point(0, 0, 1));

const Motor rotation = Motor::Rotation(90_deg, zAxis); // or Angle::FromDegrees(90)
const Motor translation = Motor::Translation(1, 2, 3);
const Motor motor = translation * rotation;

const TriVector moved = (motor * p * ~motor).Grade3(); // (1, 3, 3)
```

| Operator   | Meaning                   | Named function     |
|------------|---------------------------|--------------------|
| `a * b`    | geometric product         | `GA::Gep(a, b)`    |
| `a \| b`   | inner (fat dot) product   | `GA::Inner(a, b)`  |
| `a ^ b`    | outer product (meet)      | `GA::Outer(a, b)`  |
| `a & b`    | regressive product (join) | `GA::Join(a, b)`   |
| `!a`       | Hodge dual                | `GA::HodgeDual(a)` |
| `~a`       | inverse                   | `GA::Inverse(a)`   |
| `a.Gexp()` | exponential               | `GA::Gexp(a)`      |

C++ gives `^`, `&` and `|` a lower precedence than `+`, `-`, `*` and `==`:
`a ^ b + c` means `a ^ (b + c)`. Use parentheses, or the named functions.

`==` compares the floats exactly, which rarely holds after a rotation or a
normalization. Compare computed results with `a.RoundedEqual(b)`.

Different types can be added: `1.0f + line` is a `Motor`, `plane + point` a
`MultiVector`, and every element converts to a `MultiVector`.

## Using it in a CMake project

```cmake
# Either vendor it ...
add_subdirectory(FlyFish)
# ... or fetch it ...
include(FetchContent)
FetchContent_Declare(FlyFish
        GIT_REPOSITORY https://github.com/fdlombae/FlyFish.git
        GIT_TAG main)
FetchContent_MakeAvailable(FlyFish)
# ... or install it (cmake --install) and use find_package(FlyFish 2.0 REQUIRED)

target_link_libraries(your_target PRIVATE FlyFish::FlyFish)
```

The tests (and the googletest download) are only built when FlyFish is the
top-level project. Override with `-DFLYFISH_BUILD_TESTS=ON/OFF`.

## Using it without CMake

The library is the single file `include/flyfish/FlyFish.h`.

1. Copy the `include/flyfish` folder into your project (or add this
   repository's `include` folder to your include directories).
2. Turn on C++23 with the flag from the table above. In a Visual Studio
   project: Project > Properties > C/C++ > Language > C++ Language Standard >
   *Preview - Features from the Latest C++ Working Draft (/std:c++latest)*.
3. `#include <flyfish/FlyFish.h>`

## Building the tests

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
```