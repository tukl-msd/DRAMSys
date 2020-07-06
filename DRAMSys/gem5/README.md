## DRAMSys with gem5

Install gem5 by following the instructions in the [gem5 documentation](https://www.gem5.org/documentation/general_docs/building). In order to allow a coupling without running into problems we recommend to use **commit a470ef5**. Optionally, use the scripts from [gem5.TnT](https://github.com/tukl-msd/gem5.TnT) to install gem5, build it, get some benchmark programs and learn more about gem5.

In order to understand the SystemC coupling with gem5 it is recommended to read the documentation in the gem5 repository *util/tlm/README* and [1].

The main steps for building gem5 and libgem5 follow:

```bash
$ cd gem5
$ scons build/ARM/gem5.opt
$ scons --with-cxx-config --without-python --without-tcmalloc build/ARM/libgem5_opt.so
```

In order to use gem5 with DRAMSys export the `GEM5` environment variable (gem5 root directory) and add the path of the library to `LD_LIBRARY_PATH`, then rerun CMake and rebuild the DRAMSys project.

### DRAMSys with gem5 ARM SE mode

All essential files for a functional example are provided. Execute a hello world application:

```bash
$ cd DRAMSys/build/gem5
$ ./DRAMSys_gem5 ../../DRAMSys/library/resources/simulations/ddr3-gem5-se.json ../../DRAMSys/gem5/gem5_se/hello-ARM/config.ini 1
```

A **Hello world!** message should be printed to the standard output.

### DRAMSys with gem5 X86 SE mode

Make sure you have built *gem5/build/X86/libgem5_opt.so*. Add the path of the library to `LD_LIBRARY_PATH` and remove the path of the ARM library.

Change the architecture in the [CMake file](DRAMSys/gem5/CMakeLists.txt) to *X86*, rerun CMake and rebuild the project. Test with a hello world application for X86:

```bash
$ cd DRAMSys/build/gem5
$ ./DRAMSys_gem5 ../../DRAMSys/library/resources/simulations/ddr3-gem5-se.json ../../DRAMSys/gem5/gem5_se/hello-X86/config.ini 1
```

A **Hello world!** message should be printed to the standard output.

## References

[1] System Simulation with gem5 and SystemC: The Keystone for Full Interoperability  
C. Menard, M. Jung, J. Castrillon, N. Wehn. IEEE International Conference on Embedded Computer Systems Architectures Modeling and Simulation (SAMOS), July, 2017, Samos Island, Greece.