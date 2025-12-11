## [5.2.0] - 2025-12-10
## 🚨 Breaking Changes

- **Switch to BURST_BITs for LPDDR and HBM**\
For LPDDR and HBM, the json config files have to be
updated. The value for nbrOfColumns has to be divided by the value for
the burst length. Additionally BYTE_BITs should be renamed to BURST_BITs
and the lowest-log2(BL) bits from the COLUMN_BIT vector have to be moved
to the BURST_BIT vector.

### 🚀 Features

- *(python)* Introduce dramsys Python package
- *(build)* Make DRAMPower optional
- Introduce burst bits

### 🐛 Bug Fixes

- Incorrect memory size calculation for LP4/5
- Remove non-standard type conversions
- Memory freeing in memory manager
- *(generator)* Honor minAddress and maxAddress
- Fix warnings in Memspec.h
- *(build)* Remove non-standard SystemC headers
- Allow for non-power-of-two rows and stacks

### 💼 Other

- Re-add support for legacy SystemC installations

### 🚜 Refactor

- *(cmake)* Clean up some CMake files
- Remove unused variable in Arbiter
- Remove unused includes from DRAMPower
- Remove const from MemSpec.h
- Add {default/max}DataBytesPerBurst
- *(controller)* Clean up splitting logic
- *(am)* Improve checking and reporting
- Move helper function into AddressDecoder
- Remove deprecated pearl scripts
- *(addressdecoder)* Remove redundant check
- *(addressdecoder)* Fix multiple clang-tidy warnings
- *(addressdecoder)* Streamline decAddr
- [**breaking**] Switch to BURST_BITs for LPDDR and HBM

### ⚙️ Miscellaneous Tasks

- Update version in vcpkg
- Define dev dependencies in vcpkg
- Bump CMake version to 3.25
- *(vcpkg)* Remove Python and pybind11 from manifest
- *(vcpkg)* Update manifest baseline
- Update DRAMUtils version
- Run pipeline only on merge request events
- Add git cliff config
- Remove old LPDDR1/2/3 memspec files
- Add breaking changes section to changelog
