STAMP for TardisTM
==================

Requires [TardisTM](https://github.com/ddcc/tardisTM) at `../tardisTM`. When testing with hardware transactional memory or GCC libitm, requires [libcpuidoverride](https://github.com/ddcc/libcpuidoverride) at `../libcpuidoverride` to hide AVX2 or RTM capabilities under certain conditions. See [scripts/run-configs.sh](https://github.com/ddcc/stamp/blob/master/scripts/run-configs.sh) for an automated script that builds and runs the benchmarks with different configurations.

# Build

`./scripts/build.stm.sh`

# Run

`./scripts/abort.sh`
