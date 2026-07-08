## [5.6.0] - 2026-07-08

### 🚀 Features

- *(LPDDR6)* Add LPDDR6 support
- *(standard)* Add HBM4 support
- Implement public API for serde
- Implement gearing/slotting mechanism for the controller frontend
- Implement statistic framework
- Add simulation statistics for Arbiter
- Enable external physical storage support

### 🐛 Bug Fixes

- *(configs)* Correct tCK of MICRON_6Gb_LPDDR4-3200_32bit_A
- *(build)* Support build without DRAMPower
- *(tests)* Link DRAMPower as public again
- Apply phyDelayBW for read commands only
- Resolve deprecation warning regarding the SQLite::SQLite3 alias
- *(simulator)* Stop simulation also when DRAMSys is already idle
- Resolve build issue with DRAMPower support disabled
- Only access physical memory for CAS commands
- Add missing response status in b_transport
- Make bandwidth calculation time resolution independent
- Add DRAMSys support for TLM_COMPLETED early completion
- *(ta)* Remove unused variable in tracedb.cpp
- Resolve build issue with DRAMPower
- DRAMPower generator
- DRAMPower generator assertion
- Remove non-standard SystemC headers
- Prevent infinite loop when refresh triggers during power-down exit
- Add missing include
- Use fmt::print correctly in PrettyFormat
- Add SC_HAS_PROCESS for issuer again for SystemC 2 compat
- Use compiler-independent RNG and distribution implementations
- Use RNG with uint64 range

### 💼 Other

- *(deps)* Update DRAMSys Extensions
- Provide DRAMSysConfig.cmake for consuming the library
- Add libfmt for better formatting
- *(vcpkg)* Update baseline
- Update the dramsys-extensions version
- Enable all install targets when using FetchContent

### 🚜 Refactor

- Update DRAMPower to v6.0.0 and migrate to new its API
- *(tests)* Clean up some tests
- Remove unused memoryId in MemSpec
- *(power)* Migrate to new API of DRAMPower
- *(config)* Remove unused UseMalloc config
- Implement DRAM access callbacks
- Remove dramsys_modules CMake target
- Forward declare Dram in DRAMSys.h
- Ignore some clangd warnings for unused headers
- Use raw string for DRAMSys logo
- Move print of memory configuration out of constructor
- Simplify bandwidth calculation in controller
- Move simulation modules into main library
- Add return value fore address decoder plausibility check
- Split up controllerMethod into separate functions
- Use a const interface for the CmdMux
- Port away from own scMaxTime const
- Port away from ReadyCommand tuple
- Simplify logic in CmdMuxStrict
- Move command creation to power variant
- TargetCoordinate calculation DRAMPowerWrapper
- Explicitly serde all top-level DRAMSys components
- Rename Statistics namespace to Stats
- Move USE_DRAMPOWER define out of public header

### 📚 Documentation

- Refine terms for academic licences

### 🧪 Testing

- Add test for storage of physical data
- Delete reference databases on download failure
- Update regression reference databases

### ⚙️ Miscellaneous Tasks

- *(python)* Update dependency lock file
- Update DRAMSysExtensions dependency
- Provide sample gem5 config
- Fix running of regression tests for standards in extensions
- *(python)* Update dependency lock file
- Add workspaces to gitignore
- *(ta)* Migrate to new Python CLI API and remove pybind dependency
- Update gitignore
- Ignore all tdb files
- *(python)* Reduce minimum required Python version to 3.11
- Add .zed to gitignore
## [5.5.0] - 2026-03-27

### 🐛 Bug Fixes

- *(benches)* Restore SystemC 3 compatibility

### 💼 Other

- Require FetchContent module
- Remove FetchContent option for internal deps

### 🚜 Refactor

- Modularize DRAMSys extensions
- Clean up repository
- Move proprietary files to extensions
- Separate dramsys Python package
- Move extensions to external repository

### 🧪 Testing

- Switch regression tests to use generator
- Fetch reference databases from package repo

### ⚙️ Miscellaneous Tasks

- Update dependency versions
- Clean up .gitignore
- Disable git lfs
## [5.4.0] - 2026-03-09
## 🚨 Breaking Changes

- **Interpret STL data as little-endian words**\
The data given in an STL file will no longer be
interpret as a byte array, but as a hex value. Internally, little-endian
arrangement is used, resulting in a reversed byte order in the memory
compared to previous versions.

### 🚀 Features

- *(LPDDR5)* Add read-to-write timings (tRTW) to LPDDR5
- Add API to return total memory size

### 🐛 Bug Fixes

- Resolve compilation issues with MSVC compiler
- Fix UB in CmdMuxOldest with HBM
- *(databaseRecording)* Use SC_THREAD instead of SC_METHOD
- *(TlmRecorder)* Ensure constness of referenced transactions
- Update hbm2 regression tests
- Apply tCCDR only in same pseudo-channel
- [**breaking**] Interpret STL data as little-endian words
- *(TlmRecorder)* Fix a bug where a fatal error was thrown before the database was closed
- Stop simulation only when DRAMSys is idle
- *(tests)* Fix incomplete type in b_transport test

### 💼 Other

- Update DRAMUtils version to 1.13.0
- Add CMake preset for also for Mac

### 🚜 Refactor

- Remove redundant destructors

### 📚 Documentation

- Update config readme to new XOR format
- Remove broken pipeline status from readme

### ⚙️ Miscellaneous Tasks

- Update DRAMUtils version
- Add template line breaks to clang-format
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
