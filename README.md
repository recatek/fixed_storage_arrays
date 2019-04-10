# fixed_storage_arrays
A set of fixed-sized C++ storage arrays with various features.

## Purpose

This header-only library provides the following fixed-sized data structures:

###### `nonstd::push_array<T, N>`

A reduced "array and a counter" fixed sized data structure for writing data to once and then copying. Trivially copyable. Nothing very fancy.

###### `nonstd::raw_buffer<T, N>`

An array built around `std::aligned_storage` for a given type. Abstracts the process of creating and destroying elements with placement new in otherwise uninitialized data. Does not keep track of which cells are occupied with what, and as such does *not* provide RAII safety. Serves as the backbone for the following structures.

###### `nonstd::slot_array<T, N>`

A fixed sized, pared down implementation of the [Slot Map data structure](https://www.youtube.com/watch?v=SHaAR7XPtNU), which provides the following properties:

- O(1) emplacement and deletion*. 

- Data is stored contiguously but unordered and can be iterated as such.

- Access is done via versioned keys to avoid dangling references.

- Does not perform or require default element construction for unused slots.

- Elements are automatically cleaned up upon deletion of the structure, similar to an `std::vector`.

(* - On deletion, some iteration may be done to clean up the free slot list.)

###### `nonstd::keyed_array<T, N>`

A deconstruction of the `slot_array` structure, intended for storing random access resources safely.

- O(1) emplacement and deletion.

- Data is not stored contiguously and can not be natively iterated.

- Unlike `slot_array`, no elements are moved or rearranged upon deletion (good for large storage).

- Does not perform or require default element construction for unused slots.

- Elements are automatically cleaned up upon deletion of the structure, similar to an `std::vector`.

###### `nonstd::packed_array<T, N>`

An unordered static/fixed vector of sorts. Provides contiguous data in-place.

- O(1) emplacement and deletion *from end*.

- Data is contiguous and ordered and can be natively iterated.

- Access is done via index. This structure does not use keys or versioning.

- Does not perform or require default element construction for unused slots.

- Elements are automatically cleaned up upon deletion of the structure, similar to an `std::vector`.

###### `nonstd::versioned_key`

A generic generational pointer key used for `slot_array` and `keyed_array`. Used to prevent dangling references. Can store a few bytes of "metadata" internally as a result of some spare room from alignment. The purpose of this data is left to the user.

## Usage

This is a header-only library with no nonstandard dependencies. Simply use the files provided in the `include/` directory as desired.

## Benchmarks

TBD.

## Testing

### Building Tests on Windows
This repository is tested with VS2017 and VS2019.

##### 1. Clone or download the repository.

##### 2. Get Premake 5 and drop the executable in the repository root.
You can download Premake 5 [here](https://premake.github.io/download.html).

##### 3. Drop the premake5.exe executable in the repository root.

##### 4. Generate solution and project files with Premake.
```
premake5 vs2017
```

##### 5. Open, build, and test in Visual Studio.
```
===============================================================================
All tests passed (6214 assertions in 28 test cases)
```

### Building Tests on Linux
This repository is tested with **GCC 8.3** and **Clang-8** under Ubuntu. Older compiler versions may have issues with the use of some language features (such as `std::launder`).

##### 1. Clone or download the repository.
```
$ git clone https://github.com/recatek/fixed_storage_arrays
```

##### 2. Get Premake 5 and drop the executable in the repository root.
You can download Premake 5 [here](https://premake.github.io/download.html).

##### 3. Generate the Makefiles with Premake.
```
$ ./premake5 gmake
```

##### 4. Make either config or release using your choice of compiler.
```
$ make config=release
```

##### 5. Run tests.
```
$ ./build/bin/release/tests
===============================================================================
All tests passed (6214 assertions in 28 test cases)
```

## License

MIT. Go nuts. Don't blame me.
