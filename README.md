<img src="docs/images/dramsys_logo.png" width="350" style="float: left;" alt="DRAMSys Logo"/>  

**DRAMSys** is a flexible DRAM subsystem design space exploration framework based on SystemC TLM-2.0. It was developed by the [Microelectronic Systems Design Research Group](https://eit.rptu.de/en/fgs/ems/home/seite) at [RPTU Kaiserslautern-Landau](https://rptu.de/en/) and by [Fraunhofer IESE](https://www.iese.fraunhofer.de/en.html).

\>> [Official Website](https://www.iese.fraunhofer.de/en/innovation_trends/autonomous-systems/memtonomy/DRAMSys.html) <<

## Disclaimer

This is the public read-only mirror of an internal DRAMSys repository. Pull requests will not be merged but the changes might be added internally and published with a future commit. Both repositories are synchronized from time to time.

The user DOES NOT get ANY WARRANTIES when using this tool. This software is released under the BSD 3-Clause License. By using this software, the user implicitly agrees to the licensing terms.

If you decide to use DRAMSys in your research please cite the papers [2] [3]. To cite the TLM methodology of DRAMSys use the paper [1].

## Key Features

- **Standalone** simulator with trace players and traffic generators, **gem5**-coupled simulator and **TLM-AT-compliant library**
- Support for **DDR3/4**, **LPDDR4**, **Wide I/O 1/2**, **GDDR5/5X/6** and **HBM1/2**
- Support for **DDR5**, **LPDDR5** and **HBM3** (please contact [Matthias Jung](mailto:matthias.jung@iese.fraunhofer.de) for more information) 
- Automatic source code generation for new JEDEC standards [3] [9] from DRAMml DSL
- Various scheduling policies
- Open, closed and adaptive page policies [8]
- All-bank, same-bank, per-bank and per-2-bank refresh
- Staggered power down [5]
- Coupling to **DRAMPower** [4] and for power simulation
- **Trace Analyzer** for visual and metric-based result analysis

## Video
The linked video shows the background of DRAMSys and some examples of how simulations can be performed.

[![DRAMSys Video](https://img.youtube.com/vi/xdfaGv7MPVo/0.jpg)](https://www.youtube.com/watch?v=xdfaGv7MPVo)

## Trace Analyzer Consulting and Custom-Tailored Modifications

To provide better analysis capabilities for DRAM subsystem design space exploration than the usual performance-related outputs to the console, DRAMSys offers the Trace Analyzer. 

All requests, responses and DRAM commands can be recorded in an SQLite trace database during a simulation and visualized with the tool afterwards. An evaluation of the trace databases can be performed with the powerful Python interface of the Trace Analyzer. Different metrics are described as SQL statements and formulas in Python, which can be customized or extended without recompilation.

The Trace Analyzer's main window is shown below.

If you are interested in the Trace Analyzer, if you need support with the setup of DRAMSys in a virtual platform of your company, or if you require custom modifications of the simulator please contact [Matthias Jung](mailto:matthias.jung@iese.fraunhofer.de).

![Trace Analyzer Main Window](docs/images/traceanalyzer.png) 

## Basic Setup

Start using DRAMSys by cloning the repository.

### Dependencies

DRAMSys requires a **C++17** compiler. The build process is based on **CMake** (minimum version **3.24**). Furthermore, the simulator is based on **SystemC**. SystemC is included with FetchContent and will be build automatically with the project. If you want to use a preinstalled SystemC version, export the environment variable `SYSTEMC_HOME` (SystemC installation directory). Also make sure that the SystemC library was built with the same C++ version.

### Building DRAMSys

To build the standalone simulator for running memory trace files or traffic generators, create a build folder in the project root directory, then run CMake and make:

```bash
$ cd DRAMSys
$ mkdir build
$ cd build
$ cmake ..
$ make
```

To include **DRAMPower** in your build enable the CMake option `DRAMSYS_WITH_DRAMPOWER`. If you plan to integrate DRAMSys into your own SystemC TLM-2.0 project you can build only the DRAMSys library by disabling the CMake option `DRAMSYS_BUILD_CLI`.

To build DRAMSys on Windows 10 we recommend to use the **Windows Subsystem for Linux (WSL)**.

### Executing DRAMSys

From the build directory use the commands below to execute the DRAMSys standalone.

```bash
$ cd bin
$ ./DRAMSys
```

The default configuration file is *ddr4-example.json* located in *configs/*, the default folder for all nested configuration files is *configs/*.

To run DRAMSys with a specific configuration file:

```bash
$ ./DRAMSys ../../configs/lpddr4-example.json
```

To run DRAMSys with a specific configuration file and configuration folder:

```bash
$ ./DRAMSys ../../tests/tests_regression/DDR3/ddr3-example.json ../../tests/tests_regression/DDR3/
```

More information on the configuration can be found [here](configs/README.md).

## gem5 Coupling
There are two ways to couple DRAMSys with **gem5**:
- Use the official integration of DRAMSys in gem5. More information can be found in `ext/dramsys` of the gem5 repository.
- (Deprecated) Compile gem5 as a shared library and link it with DRAMSys, which is only supported in older versions of DRAMSys (tag v4.0).

## Acknowledgements

The development of DRAMSys was supported by the German Research Foundation (DFG) as part of the priority program [Dependable Embedded Systems SPP1500](http://spp1500.itec.kit.edu) and the DFG grant no. [WE2442/10-1](https://www.uni-kl.de/en/3d-dram/). Furthermore, it was supported within the Fraunhofer and DFG cooperation program (grant no. [WE2442/14-1](https://www.iese.fraunhofer.de/en/innovation_trends/autonomous-systems/memtonomy.html)) and by the [Fraunhofer High Performance Center for Simulation- and Software-Based Innovation](https://www.leistungszentrum-simulation-software.de/en.html). Special thanks go to all listed contributors for their work and commitment during seven years of development. 

Shama Bhosale  
Derek Christ  
Luiza Correa  
Peter Ehses  
Johannes Feldmann  
Robert Gernhardt  
Doris Gulai  
Matthias Jung  
Frederik Lauer  
Ana Mativi  
Felipe S. Prado  
Iron Prando  
Tran Anh Quoc  
Janik Schlemminger  
Lukas Steiner  
Thanh C. Tran  
Norbert Wehn  
Christian Weis  
Éder F. Zulian

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

[8] Simulating DRAM controllers for future system architecture exploration  
A. Hansson, N. Agarwal, A. Kolli, T. Wenisch, A. N. Udipi. IEEE International Symposium on Performance Analysis of Systems and Software (ISPASS), 2014, Monterey, USA.

[9] Fast Validation of DRAM Protocols with Timed Petri Nets  
M. Jung, K. Kraft, T. Soliman, C. Sudarshan, C. Weis, N. Wehn. ACM International Symposium on Memory Systems (MEMSYS 2019), October, 2019, Washington, DC, USA.
