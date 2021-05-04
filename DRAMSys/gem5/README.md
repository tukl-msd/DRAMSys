## DRAMSys with gem5 SE Mode

Install gem5 by following the instructions in the [gem5 documentation](https://www.gem5.org/documentation/general_docs/building). In order to allow a coupling without running into problems we recommend to use the **develop** branch. Optionally, use the scripts from [gem5.TnT](https://github.com/tukl-msd/gem5.TnT) to install gem5, build it, get some benchmark programs and learn more about gem5.

In order to understand the SystemC coupling with gem5 it is recommended to read the documentation in the gem5 repository *util/tlm/README* and [1].

The main steps for building the gem5 executable and shared library follow (either choose ARM or x86 ISA):

```bash
$ cd gem5
$ scons build/<ARM/X86>/gem5.opt
$ scons --with-cxx-config --without-python --without-tcmalloc USE_SYSTEMC=False build/<ARM/X86>/libgem5_opt.so
```

In order to use gem5 with DRAMSys export the root directory as `GEM5` environment variable and add the path of the library to `LD_LIBRARY_PATH`, then rerun CMake with the additional variable `-DDRAMSYS_WITH_GEM5=ON` and rerun Make.

```bash
$ export GEM5=/path/to/gem5
$ export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/path/to/gem5/build/<ARM/X86>
$ cd DRAMSys/build
$ cmake ../DRAMSys/ -DDRAMSYS_WITH_GEM5=ON
$ make
```

Adjust the *path/to/gem5/configs/example/se.py* as shown below:

```bash
...
if options.tlm_memory:
    system.physmem = SimpleMemory()
MemConfig.config_mem(options, system)
...

```

Next, a gem5 configuration file must be generated. A "Hello world!" example is provided in the gem5 repository. To run the example execute the following commands:

```bash
$ cd DRAMSys/build/gem5
$ ./path/to/gem5/build/<ARM/X86>/gem5.opt path/to/gem5/configs/example/se.py \
  -c path/to/gem5/tests/test-progs/hello/bin/<arm/x86>/linux/hello \ 
  --mem-size=512MB --mem-channels=1 --caches --l2cache --mem-type=SimpleMemory \
  --cpu-type=TimingSimpleCPU --num-cpu=1 \
  --tlm-memory=transactor
```

The error message `fatal: Can't find port handler type 'tlm_slave'` is printed because a TLM memory is not yet connected to gem5. However, a configuration file that corresponds to the simulation will already be created inside the *m5out* directory. This configuration file can now be used to run the same simulation with gem5 coupled to DRAMSys:

```bash
$ ./DRAMSys_gem5 ../../DRAMSys/library/resources/simulations/ddr3-gem5-se.json m5out/config.ini 1
```

`Hello world!` should now be printed to the standard output.

If you want to switch to another ISA simply modify the `LD_LIBRARY_PATH` environment variable accordingly and create a new gem5 configuration file.

## References

[1] System Simulation with gem5 and SystemC: The Keystone for Full Interoperability  
C. Menard, M. Jung, J. Castrillon, N. Wehn. IEEE International Conference on Embedded Computer Systems Architectures Modeling and Simulation (SAMOS), July, 2017, Samos Island, Greece.