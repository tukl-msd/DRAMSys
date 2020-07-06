DRAMSys4.0
===========

**DRAMSys4.0** [1] [2] [3] is a flexible DRAM subsystem design space exploration framework that consists of models reflecting the DRAM functionality, power consumption, temperature behavior and retention time errors.

## Basic Setup

Start using DRAMSys by cloning the repository.
Use the *--recursive* flag to initialize all submodules within the repository, namely **DRAMPower** [4], **SystemC** and **nlohmann json**.

### Dependencies

DRAMSys is based on the SystemC library. SystemC is included as a submodule and will be build automatically with the DRAMSys project. If you want to use an external SystemC version you have to export the environment variables `SYSTEMC_HOME` (SystemC root directory), `SYSTEMC_TARGET_ARCH` (e.g. linux64) and add the path of the library to `LD_LIBRARY_PATH`.

### Building DRAMSys
DRAMSys uses CMake for the build process, the minimum required version is **CMake 3.10**.

To build the standalone simulator for running memory trace files, create a build folder in the project root directory, then run CMake and make:

```bash
$ cd DRAMSys
$ mkdir build
$ cd build
$ cmake ../DRAMSys/
$ make
```

If you plan to integrate DRAMSys into your own SystemC/TLM project you can build the DRAMSys library only:

```bash
$ cd DRAMSys
$ mkdir build
$ cd build
$ cmake ../DRAMSys/library/
$ make
```

To build DRAMSys on Windows 10 we recommend to use the **Windows Subsystem for Linux (WSL)**.

### Executing DRAMSys

From the build directory use the commands below to execute the DRAMSys standalone.

```bash
$ cd simulator
$ ./DRAMSys
```

The default base config file is *ddr3-example.json* and located in *DRAMSys/library/resources/simulations*, the default resource folder for all nested config files is *DRAMSys/library/resources*.

To run DRAMSys with a specific base config file:

```bash
$ ./DRAMSys ../../DRAMSys/library/resources/simulations/ddr3-example.json
```

To run DRAMSys with a specific base config file and a resource folder somewhere else to the standard:

```bash
$ ./DRAMSys ../../DRAMSys/tests/example_ddr3/simulations/ddr3-example.json ../../DRAMSys/tests/example_ddr3/
```

### DRAMSys Configuration

The DRAMSys executable supports one argument which is a JSON file that contains certain arguments and the path of other configuration files for the desired simulation.

The JSON code below shows an example configuration:

```json
{
    "simulation": {
        "simulationid": "ddr3-example",
        "simconfig": "ddr3.json",
        "thermalconfig": "config.json",
        "memspec": "MICRON_1Gb_DDR3-1600_8bit_G.json",
        "addressmapping": "am_ddr3_8x1Gbx8_dimm_p1KB_brc.json",
        "mcconfig":"fifoStrict.json",
        "tracesetup": [
            {
                "clkMhz": 300,
                "name": "ddr3_example.stl"
            },
            {
                "clkMhz": 400,
                "name": "ddr3_example.stl"
            }
        ]	
    }
}
```
Fields Description:
- "simulationid": Simulation file identifier
- "simconfig": Configuration file for the DRAMSys Simulator
- "thermalconfig": Temperature Simulator Configuration File
- "memspec": Memory Device Specification File
- "addressmapping": Addressmapping Configuration of the Memory Controller File.
- "mcconfig": Memory Controller Configuration File.
- "tracesetup": The trace setup is only used in standalone mode. In library mode the trace setup is ignored. Each device should be added as a json object inside the "tracesetup" array. 

Each **trace setup** device configuration consists of two parameters, **clkMhz** (operation frequency of the **trace player**) and a trace file **name**. Most configuration fields reference other JSON files which contain more specialized chunks of the configuration like a memory specification, an address mapping and a memory controller configuration.


#### Trace Files

A **trace file** is a prerecorded file containing memory transactions. Each memory transaction has a time stamp that tells the simulator when it shall happen, a transaction type (*read* or *write*) and a hexadecimal memory address.

There are two different kinds of trace files. They differ in their timing behavior and are distinguished by their file extension.

##### STL Trace (.stl)

The times tamp corresponds to the time the request is to be issued and it is given in cycles of the bus master device. Example: the device is an FPGA with a frequency of 200 MHz (clock period of 5 ns). If the time stamp is 10 it means that the request is to be issued when time is 50 ns.

Here is an example syntax:

```
# Comment lines begin with #
# [clock-cyle]: [write|read] [hex-address] [hex-data (optional)]
31:	read	0x400140
33:	read	0x400160
56:	write	0x7fff8000 0x123456789abcdef
81:	read	0x400180
```

##### Relative STL Traces (.rstl)

The time stamp corresponds to the time the request is to be issued relative to the end of the transaction before or the beginning of the trace. This results in a simulation in which the trace player is able to react to possible delays due to DRAM bottlenecks.

Here is an example syntax:

```
# Comment lines begin with #
# [clock-cyle]: [write|read] [hex-address] [hex-data (optional)]
31:	read	0x400140
2:	read	0x400160
23:	write	0x7fff8000 0x123456789abcdef
25:	read	0x400180
```

#### Trace Player

A **trace player** is **equivalent** to a bus master **device** (processor, FPGA, etc.). It reads an input trace file and translates each line into a new memory request. By adding a new device element into the trace setup section one can specify a new trace player, its operating frequency and the trace file for that trace player.

#### Configuration File Sections

The main configuration file is divided into self-contained sections. Each of these sections refers to sub-configuration files.

Below, the sub-configurations are listed and explained.

##### Simulator Configuration

The content of [ddr3.json](DRAMSys/library/resources/configs/simulator/ddr3.json) is presented below as an example.

```json
{
    "simconfig": {
        "SimulationName": "ddr3",
        "Debug": false,
        "DatabaseRecording": true,
        "PowerAnalysis": false,
        "EnableWindowing": false,
        "WindowSize": 1000,
        "ThermalSimulation": false,
        "SimulationProgressBar": true,
        "CheckTLM2Protocol": false,
        "ECCControllerMode": "Disabled",
        "UseMalloc": false,
        "AddressOffset": 0,
        "ErrorCSVFile": "",
        "ErrorChipSeed": 42,
        "StoreMode": "NoStorage"
    }
}
```

  - *SimulationName* (boolean)
    - Give the name of the simulation for distinguishing from other simulations.
  - *Debug* (boolean)
    - true: enables debug output on console (only supported by a debug build)
    - false: disables debug output
  - *DatabaseRecording* (boolean)
    - true: enables output database recording for the Trace Analyzer tool
    - false: disables output database recording
  - *PowerAnalysis* (boolean)
    - true: enables live power analysis with DRAMPower
    - false: disables power analysis
  - *EnableWindowing* (boolean)
    - true: enables temporal windowing
    - false: disables temporal windowing
  - *WindowSize* (unsigned int)
    - Size of the window in clock cycles used to evaluate average bandwidth and average power consumption
  - *ThermalSimulation* (boolean)
    - true: enables thermal simulation ([more information](#dramsys-with-thermal-simulation))
    - false: static temperature during simulation
  - *SimulationProgressBar* (boolean)
    - true: enables the simulation progress bar
    - false: disables the simulation progress bar
  - *CheckTLM2Protocol* (boolean)
    - true: enables the TLM-2.0 Protocol Checking
    - false: disables the TLM-2.0 Protocol Checking
  - *ECCControllerMode* (string)
    - "Disabled": No ECC controller is used
    - "Hamming": Enables an ECC Controller with classic SECDED implementation using Hamming Code
  - *UseMalloc* (boolean)
    - false: model storage using mmap() (DEFAULT)
    - true: allocate memory for modeling storage using malloc()
- *AddressOffset* (unsigned int)
  - Address offset of the DRAM subsystem (required for the gem5 coupling).
- *ErrorChipSeed* (unsigned int)
  - Seed to initialize the random error generator.
  - *ErrorCSVFile* (string)
    - CSV file with error injection information.
  - *StoreMode* (string)
    - "NoStorage": no storage
    - "Store": store data without error model
    - "ErrorModel": store data with error model [6]

##### Temperature Simulator Configuration

The content of [config.json](DRAMSys/library/resources/configs/thermalsim/config.json) is presented below as an example.

```json
{
    "thermalsimconfig": {
        "TemperatureScale": "Celsius",
        "StaticTemperatureDefaultValue": 89,
        "ThermalSimPeriod": 100,
        "ThermalSimUnit": "us",
        "PowerInfoFile": "powerInfo.json",
        "IceServerIp": "127.0.0.1",
        "IceServerPort": 11880,
        "SimPeriodAdjustFactor": 10,
        "NPowStableCyclesToIncreasePeriod": 5,
        "GenerateTemperatureMap": true,
        "GeneratePowerMap": true
    }
}
```
  - *TemperatureScale* (string)
    - "Celsius"
    - "Fahrenheit"
    - "Kelvin"
  - *StaticTemperatureDefaultValue* (int)    
    - Temperature value for simulations with static temperature
  - *ThermalSimPeriod* (double)   
    - Period of the thermal simulation
  - *ThermalSimUnit* (string)
    - "s": seconds
    - "ms": millisecond
    - "us": microseconds
    - "ns": nanoseconds
    - "ps": picoseconds
    - "fs": femtoseconds
  - *PowerInfoFile* (string)    
    - File containing power related information: devices identifiers, initial power values and power thresholds.
  - *IceServerIp* (string)    
    - 3D-ICE server IP address
  - *IceServerPort* (unsigned int)    
    - 3D-ICE server port
  - *SimPeriodAdjustFactor* (unsigned int)    
    - When substantial changes in power occur (i.e., changes that exceed the thresholds), then the simulation period will be divided by this number causing the thermal simulation to be executed more often.
  - *NPowStableCyclesToIncreasePeriod* (unsigned int)    
    - Wait this number of thermal simulation cycles with power stability (i.e., changes that do not exceed the thresholds) to start increasing the simulation period back to its configured value.
  - *GenerateTemperatureMap* (boolean)
    - true: generate temperature map files during thermal simulation
    - false: do not generate temperature map files during thermal simulation
  - *GeneratePowerMap* (boolean)
    - true: generate power map files during thermal simulation
    - false: do not generate power map files during thermal simulation


##### Memory Specification

A file with memory specifications. Timings and currents come from data sheets and measurements, and usually do not change.  
The fields inside "mempowerspec" can be written directly as a **double** type. "memoryId" and "memoryType" are **string**. The others are **unsigned int**.

##### Address Mapping

DRAMSys uses the **ConGen** [7] format for address mappings. It provides bit-wise granularity. It also provides the possibility to XOR address bits in order to map page misses to different banks and reduce latencies. 

Used fields:  
- "XOR": Defines an XOR connection of a "FIRST" and a "SECOND" bit
- "BYTE_BIT": Address bits that are connected to the byte bits in ascending order
- "COLUMN_BIT": Address bits that are connected to the column bits in ascending order
- "ROW_BIT": Address bits that are connected to the row bits in ascending order
- "BANK_BIT": Address bits that are connected to the bank bits in ascending order
- "BANKGROUP_BIT": Address bits that are connected to the bank group bits in ascending order
- "RANK_BIT": Address bits that are connected to the rank bits in ascending order
- "CHANNEL_BIT": Address bits that are connected to the channel bits in ascending order

```json
{
    "CONGEN": {
        "XOR": [
            {
                "FIRST": 13,
                "SECOND": 16
            }
        ],
        "BYTE_BIT": [0,1,2],
        "COLUMN_BIT": [3,4,5,6,7,8,9,10,11,12],
        "BANK_BIT": [13,14,15],
        "ROW_BIT": [16,17,18,19,20,21,22,23,24,25,26,27,28,29]
    }	
}

```

##### Memory Controller Configuration

An example follows.

```json
{
    "mcconfig": {
        "PagePolicy": "Open", 
        "Scheduler": "Fifo", 
        "RequestBufferSize": 8, 
        "CmdMux": "Oldest", 
        "RespQueue": "Fifo", 
        "RefreshPolicy": "Rankwise", 
        "RefreshMaxPostponed": 8, 
        "RefreshMaxPulledin": 8, 
        "PowerDownPolicy": "NoPowerDown", 
        "PowerDownTimeout": 100
    }
}
```

  - *PagePolicy* (string)
      - "Open"
      - "OpenAdaptive"
      - "Closed"
      - "ClosedAdaptive"
  - *Scheduler* (string)
    - "Fifo": first in, first out
    - "FrFcfs": first-ready - first-come, first-served
    - "FrFcfsGrp": first-ready - first-come, first-served with grouping of read and write requests
  - RequestBufferSize (unsigned int)
    - buffer size of the scheduler
  - *CmdMux* (string)
      - "Oldest": oldest payload has the highest priority
      - "Strict": read and write commands are issued in the same order as their corresponding requests arrived at the channel controller (can only be combined with "Fifo" scheduler) 
  - *RespQueue* (string)
      - "Strict": outgoing responses are not reordered
      - "Reorder": outgoing responses are reordered
  - *RefreshPolicy* (string)
      - "NoRefresh": refresh disabled
      - "Rankwise": all-bank refresh commands, issued per rank
      - "Bankwise": per-bank refresh commands (only supported by LPDDR4, Wide I/O 2, GDDR5/5X/6, HBM2)
  - *RefreshMaxPostponed*
      - maximum number of refresh commands that can be postponed (usually 8, with per-bank refresh the number is automatically multiplied by the number of banks)
  - *RefreshMaxPulledin*
      - maximum number of refresh commands that can be pulled in (usually 8, with per-bank refresh the number is automatically multiplied by the number of banks)
  - *PowerDownPolicy* (string)
    - "NoPowerDown": power down disabled
    - "Staggered": staggered power down policy [5]
- PowerDownTimeout (unsigned int)
  - currently unused

## DRAMSys with Thermal Simulation

The thermal simulation is performed by a **3D-ICE** [8] server accessed through the network. Therefore users interested in thermal simulation during their DRAMSys simulations need to make sure they have a 3D-ICE server up and running before starting. For more information about 3D-ICE visit the [official website](https://www.epfl.ch/labs/esl/open-source-software-projects/3d-ice/).

#### Installing 3D-ICE

Install SuperLU dependencies:

```bash
$ sudo apt-get install build-essential git bison flex libblas-dev
```

Download and install SuperLU:

```bash
$ wget http://crd.lbl.gov/~xiaoye/SuperLU/superlu_4.3.tar.gz
$ tar xvfz superlu_4.3.tar.gz
$ cd SuperLU_4.3/
$ cp MAKE_INC/make.linux make.inc
```

Make sure the *SuperLUroot* variable in *make.inc* is properly set. For example, if you downloaded it to your home folder set as follows:

```bash
SuperLUroot = $(HOME)/SuperLU_4.3
```

Compile the library:

```bash
$ make superlulib
```

Download and install bison-2.4.1:

```bash
$ wget http://ftp.gnu.org/gnu/bison/bison-2.4.1.tar.gz
$ tar xvzf bison-2.4.1.tar.gz
$ cd bison-2.4.1
$ ./configure --program-suffix=-2.4.1
$ make
$ sudo make install
```

[Download](https://www.epfl.ch/labs/esl/open-source-software-projects/3d-ice/3d-ice-download/) the lastest version of 3D-ICE. Make sure you got version 2.2.6 or greater.

Unzip the archive and go to the 3D-ICE directory:

```bash
$ unzip 3d-ice-latest.zip
$ cd 3d-ice-latest/3d-ice-2.2.6
```

Open the file makefile.def and set some variables.

```bash
SLU_MAIN = $(HOME)/SuperLU_$(SLU_VERSION)
YACC = bison-2.4.1
SYSTEMC_ARCH = linux64
SYSTEMC_MAIN = $(HOME)/systemc-2.3.x
```

Compile 3D-ICE with SystemC TLM-2.0 support:

```bash
$ make SYSTEMC_WRAPPER=y
```

Export the environment variable `LIBTHREED_ICE_HOME`:

```bash
$ export LIBTHREED_ICE_HOME=${HOME}/3d-ice-latest/3d-ice-2.2.6
```

#### Running DRAMSys with Thermal Simulation

In order to run DRAMSys with thermal simulation you have to rerun CMake and rebuild the project. The example input trace file can be generated with a Perl script:

 ```bash
$ cd DRAMSys/DRAMSys/library/resources/traces
$ ./generateErrorTest.pl > test_error.stl
 ```

Before starting DRAMSys it is necessary to run the 3D-ICE server passing to it two arguments: a suitable configuration file and a socket port number. And then wait until the server is ready to receive requests.

```bash
$ cd DRAMSys/DRAMSys/library/resources/configs/thermalsim
$ ~/3d-ice-latest/3d-ice-2.2.6/bin/3D-ICE-Server stack.stk 11880
Preparing stk data ... done !
Preparing thermal data ... done !
Creating socket ... done !
Waiting for client ...
```

In another terminal or terminal tab start DRAMSys with the special thermal simulation config:

```bash
$ cd DRAMSys/build/simulator/
$ ./DRAMSys ../../DRAMSys/library/resources/simulations/wideio-thermal.json
```

## DRAMSys with gem5

Further information about the usage of DRAMSys with gem5 can be found [here](DRAMSys/gem5/README.md). 

## Trace Analyzer

If you want to use the database recording feature and the Trace Analyzer tool for result analysis please contact [Matthias Jung](mailto:matthias.jung@iese.fraunhofer.de).

## Disclaimer

This is the public read-only mirror of an internal DRAMSys repository. Pull requests will not be merged but the changes might be added internally and published with a future commit. The repositories are synchronized from time to time.

The user DOES NOT get ANY WARRANTIES when using this tool. This software is released under the BSD 3-Clause License. By using this software, the user implicitly agrees to the licensing terms.

## References

[1] TLM Modelling of 3D Stacked Wide I/O DRAM Subsystems, A Virtual Platform for Memory Controller Design Space Exploration  
M. Jung, C. Weis, N. Wehn, K. Chandrasekar. International Conference on High-Performance and Embedded Architectures and Compilers 2013 (HiPEAC), Workshop on: Rapid Simulation and Performance Evaluation: Methods and Tools (RAPIDO), January, 2013, Berlin.

[2] DRAMSys: A flexible DRAM Subsystem Design Space Exploration Framework  
M. Jung, C. Weis, N. Wehn. IPSJ Transactions on System LSI Design Methodology (T-SLDM), October, 2015.

[3] DRAMSys4.0: A Fast and Cycle-Accurate SystemC/TLM-Based DRAM Simulator  
L. Steiner, M. Jung, F. S. Prado, K. Bykov, N. Wehn. International Conference on Embedded Computer Systems: Architectures, Modeling, and Simulation (SAMOS), July, 2020, Samos Island, Greece.

[4] DRAMPower: Open-source DRAM Power & Energy Estimation Tool  
K. Chandrasekar, C. Weis, Y. Li, S. Goossens, M. Jung, O. Naji, B. Akesson, N. Wehn, K. Goossens. URL: http://www.drampower.info

[5] Optimized Active and Power-Down Mode Refresh Control in 3D-DRAMs  
M. Jung, M. Sadri, C. Weis, N. Wehn, L. Benini. VLSI-SoC, October, 2014, Playa del Carmen, Mexico.

[6] Retention Time Measurements and Modelling of Bit Error Rates of WIDE-I/O DRAM in MPSoCs  
C. Weis, M. Jung, P. Ehses, C. Santos, P. Vivet, S. Goossens, M. Koedam, N. Wehn. IEEE Conference Design, Automation and Test in Europe (DATE), March, 2015, Grenoble, France.

[7] ConGen: An Application Specific DRAM Memory Controller Generator
M. Jung, I. Heinrich, M. Natale, D. M. Mathew, C. Weis, S. Krumke, N. Wehn. International Symposium on Memory Systems (MEMSYS 2016), October, 2016, Washington, DC, USA.
