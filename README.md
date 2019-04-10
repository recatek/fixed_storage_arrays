# fixed_storage_arrays
A set of fixed-sized C++ storage arrays with various features.

## Building Tests on Windows
This repository is tested with VS2017 and VS2019.

#### 1. Clone or download the repository.

#### 2. Get Premake 5 and drop the executable in the repository root.
You can download Premake 5 [here](https://premake.github.io/download.html).

#### 3. Drop the premake5.exe executable in the repository root.

#### 4. Generate solution and project files with Premake.
```
premake5 vs2017
```

#### 5. Open, build, and test in Visual Studio.
```
===============================================================================
All tests passed (6214 assertions in 28 test cases)
```

## Building Tests on Linux
This repository is tested with **GCC 8.3** and **Clang-8** under Ubuntu. Older compiler versions may have issues with the use of some language features (such as `std::launder`).

#### 1. Clone or download the repository.
```
$ git clone https://github.com/recatek/fixed_storage_arrays
```

#### 2. Get Premake 5 and drop the executable in the repository root.
You can download Premake 5 [here](https://premake.github.io/download.html).

#### 3. Generate the Makefiles with Premake.
```
$ ./premake5 gmake
```

#### 4. Make either config or release using your choice of compiler.
```
$ make config=release
```

#### 5. Run tests.
```
$ ./build/bin/release/tests
===============================================================================
All tests passed (6214 assertions in 28 test cases)
```
