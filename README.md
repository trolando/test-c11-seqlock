# Versioned Read Consistency Test

This project implements a small C11 benchmark to test the correctness of a **lock-free cache read** pattern using a **versioned read** (seqlock-style) approach.

## Overview

We simulate a cache bucket where:
- A single **writer thread** repeatedly updates a key-value tuple and a version field.
- A **reader thread** tries to read the tuple without locking, using the version field to detect concurrent writes.

There are different possible patterns depending on the memory model and compiler optimizations.

The most correct version following C11 semantics appears to be:
1. An acquire read of the version field (`s1`)
2. A load of the key/value data
3. A memory barrier (`atomic_thread_fence`) of order `seq_cst`
4. A second relaxed read of the version field (`s2`)
5. If `s1 != s2`, the reader discards the result and retries

This pattern should ensure that the reader only observes consistent data if no write occurred in between.

## Purpose

This project serves as a **litmus test** for:
- Memory consistency under relaxed atomics
- Correct usage of `atomic_thread_fence`
- Portability across Linux, macOS (including ARM/M1), and Windows

It is useful for validating memory ordering assumptions on your system and compiler.

## Findings

### Apple M1 Pro

We try different variations on an Apply M1 Pro chip.

Here are versions that did not work:
- `relaxed` first read, `acquire` second read, no fence 
- `relaxed` first read, `seq_cst` second read, no fence 
- `acquire` first read, `relaxed` second read, no fence
- `acquire` first read, `acquire` second read, no fence
- `acquire` first read, `seq_cst` second read, no fence

Here are versions where **no** data race was detected:
- `relaxed` first read, `relaxed` second read, no fence (??)
- `relaxed` first read, `relaxed` second read, `acquire` fence
- `relaxed` first read, `relaxed` second read, `seq_cst` fence
- `acquire` first read, `relaxed` second read, `acquire` fence
- `acquire` first read, `relaxed` second read, `seq_cst` fence

Performance-wise the versions with a `relaxed` first read run in 0.21 sec while the versions with an `acquire` first read run in 0.58 sec.

These results seem strange. One would expect that with no enforced ordering at all, the compiler and cpu would be allowed to reorder loads before the first load or after the second load, resulting in race conditions. The first read might allow `relaxed` because it is immediately followed by a branch test based on the loaded value, so perhaps this disables speculative execution? But one should not count on that.

### Intel x86

We try different variations on an Intel Core i5-13600KF.

In this test, there is **no** data race detected in any of the cases.
All versions appear to have the same performance.

## Building

You need a C11-compatible compiler and CMake 3.1+.

```sh
mkdir build
cd build
cmake ..
make
```

## Running

```sh
./test_atomics
```

You should see output like:

```
Detected no data races!
Reader time: 1.234567 seconds (324783.22 iterations/sec)
```

## Portability

- Works on Linux, macOS (Intel and ARM/M1), and Windows

## License

This code is released under the MIT License.
