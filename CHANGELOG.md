## [5.3.1] - 2026-01-19

### 🐛 Bug Fixes

- Re-add SC_HAS_PROCESS for SC 2.3.1 compat
## [5.3.0] - 2026-01-16

### 🚀 Features

- Remove TraceAnalyzer timing dependencies
- *(traceanalyzer)* Show SID for HBM
- *(ecc)* Add support for write back for InlineEcc
- *(ecc)* Implement two-tevel ECC scheme
- *(python)* Introduce new simulation script
- HBM MemSpec output print is now more detailed
- *(cmake)* Add support for installing DRAMSys

### 🐛 Bug Fixes

- Fix segv when using data with child transacts
- Correctly instantiate banks when SID is used
- Remove undefined behavior in simulator
- *(ecc)* Handle multiple backpressure in InlineEcc
- Interpret nbrOfBanks as banks per pCH
- Apply correctly absolute numbering to stacks
- Correct bank count in exemplary HBM configs
- *(HBM3)* Add missing timing for HBM3

### 🚜 Refactor

- *(addressdecoder)* Simplify checking logic
- Use better example configs for HBM2/3
- *(ecc)* Move InlineEcc module into library
- *(ecc)* Make InlineEcc module generic
- *(ecc)* Remove memSpec from ECC modules
- *(ecc)* Decouple ECC modules from internals
- *(ecc)* Clean up ECC module code
- Show more DRAMPower stats in stdout
- *(python)* Remove old simulation script
- *(ecc)* Clean up ecc logic
- Move extension base configs location
- *(ta)* Remove configuration lib dependency
- Merge config lib sources with DRAMSys
- Move simulator and trace analyzer to apps
- Move DRAMSys sources to top-level src/
- *(cmake)* Rename the DRAMSys library target
- Remove tlm_protocol_checker config
- Clean up public API with forward declares

### ⚙️ Miscellaneous Tasks

- Add python script to remove standards
- Add our new contributors to acks
- Add scripts to mkpkg python script

### ◀️ Revert

- *(traceanalyzer)* Show SID for HBM
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
