<img src="docs/images/dramsys_logo.png" width="350" style="float: left;" alt="DRAMSys Logo"/>

**DRAMSys** is a flexible DRAM subsystem design space exploration framework based on SystemC TLM-2.0. It was developed by the [Microelectronic Systems Design Research Group](https://eit.rptu.de/en/fgs/ems/home/seite) at [RPTU Kaiserslautern-Landau](https://rptu.de/en/), by [Fraunhofer IESE](https://www.iese.fraunhofer.de/en.html) and by the [Computer Engineering Group](https://www.informatik.uni-wuerzburg.de/ce/) at [JMU Würzburg](https://www.uni-wuerzburg.de/en/home/).

\>> [Official Website](http://dramsys.de) <<

## Disclaimer

This is the public read-only mirror of an internal DRAMSys repository. Pull requests will not be merged but the changes might be added internally and published with a future commit. Both repositories are synchronized from time to time.

The user DOES NOT get ANY WARRANTIES when using this tool. This software is released under the BSD 3-Clause License. By using this software, the user implicitly agrees to the licensing terms.

If you decide to use DRAMSys in your research please cite the paper [2] or [3]. To cite the TLM methodology of DRAMSys use the paper [1].

## Included Features

- **Standalone** simulator with trace players and traffic generators or **TLM-2.0-compliant library**
- **Trace Analyzer** for visual and metric-based result analysis
- Coupling to **gem5** supported
- Cycle-accurate **DDR3/4**, **LPDDR4**, **Wide I/O 1/2**, **GDDR5/5X/6** and **HBM1/2** modelling
- Bit-granular address mapping with optional XOR connections [7]
- Various scheduling policies
- Open, closed and adaptive page policies [8]
- All-bank, same-bank, per-bank and per-2-bank refresh, postponed and pulled in refresh commands
- Refresh management
- Staggered power down [5]
- Coupling to **DRAMPower** [4] for power simulation

## Additional Features

- Cycle-accurate **DDR5**, **LPDDR5** and **HBM3** modelling
- Extended analysis features for the **Trace Analyzer**
- **Free academic** or **commercial** licenses available (please contact [DRAMSys@iese.fraunhofer.de](mailto:DRAMSys@iese.fraunhofer.de) for more information)

## Video

The linked video shows the background of DRAMSys and some examples of how simulations can be performed.

[![DRAMSys Video](https://img.youtube.com/vi/xdfaGv7MPVo/0.jpg)](https://www.youtube.com/watch?v=xdfaGv7MPVo)

## Trace Analyzer Consulting and Custom-Tailored Modifications

To provide better analysis capabilities for DRAM subsystem design space exploration than the usual performance-related outputs to the console, DRAMSys offers the Trace Analyzer.

All requests, responses and DRAM commands can be recorded in an SQLite trace database during a simulation and visualized with the tool afterwards. An evaluation of the trace databases can be performed with the powerful Python interface of the Trace Analyzer. Different metrics are described as SQL statements and formulas in Python, which can be customized or extended without recompilation.

The Trace Analyzer's main window is shown below.

A basic version of Trace Analyzer is included in the open source release of DRAMSys.
However, the full version of the Trace Analyzer includes many additional analysis features:
- Detailed transaction-level latency analysis
- Power, bandwidth, and buffer utilization analysis over simulation time intervals
- Timing dependency analysis at the DRAM command level
- Calculation of numerous predefined and user-defined metrics
- VCD export of generated trace

If you are interested in the full version of Trace Analyzer, if you need support with the setup of DRAMSys in a virtual platform of your company, or if you require custom modifications of the simulator please contact [DRAMSys@iese.fraunhofer.de](mailto:DRAMSys@iese.fraunhofer.de).

![Trace Analyzer Main Window](docs/images/traceanalyzer.png)

## Basic Setup

To use DRAMSys, first clone the repository.

### Dependencies

DRAMSys requires a **C++17** compiler. The build process is based on **CMake** (minimum version **3.24**). Furthermore, the simulator is based on **SystemC**. SystemC is included with FetchContent and will be build automatically with the project. If you want to use a preinstalled SystemC version, export the environment variable `SYSTEMC_HOME` (SystemC installation directory) and enable the `DRAMSYS_USE_EXTERNAL_SYSTEMC` CMake option. Also make sure that the SystemC library was built with the same C++ version.

If you choose to enable the compilation of the Trace Analyzer, a system installation of development packages for Qt5, Qwt and Python is required.
For example, on Ubuntu, these dependencies can be satisfied by installing the `qtbase5-dev`, `libqwt-qt5-dev` and `python3-dev` packages.

### Building DRAMSys

To build the standalone simulator for running memory trace files or traffic generators, first configure the project using CMake, then build the project:

```console
$ cd DRAMSys
$ cmake -S . -B build
$ cmake --build build
```

To include **DRAMPower** in your build enable the CMake option `DRAMSYS_WITH_DRAMPOWER`:
```console
$ cmake -B build -D DRAMSYS_WITH_DRAMPOWER=Y
```

If you plan to integrate DRAMSys into your own SystemC TLM-2.0 project you can build only the DRAMSys library by disabling the CMake option `DRAMSYS_BUILD_CLI`.

To include the Trace Analyzer in the build process, enable the CMake option `DRAMSYS_BUILD_TRACE_ANALYZER`.

In order to include any proprietary extensions such as the extended features of Trace Analyzer, enable the CMake option `DRAMSYS_ENABLE_EXTENSIONS`.

To build DRAMSys on Windows 10 we recommend to use the **Windows Subsystem for Linux (WSL)**.

### Executing DRAMSys

From the build directory use the commands below to execute the DRAMSys standalone.

```console
$ cd bin
$ ./DRAMSys
```

The default configuration file is *ddr4-example.json* located in *configs/*, the default folder for all nested configuration files is *configs/*.

To run DRAMSys with a specific configuration file:

```console
$ ./DRAMSys ../../configs/lpddr4-example.json
```

To run DRAMSys with a specific configuration file and configuration folder:

```console
$ ./DRAMSys ../../tests/tests_regression/DDR3/ddr3-example.json ../../tests/tests_regression/DDR3/
```

More information on the configuration can be found [here](configs/README.md).

## gem5 Coupling

There are two ways to couple DRAMSys with **gem5**:
- Use the official integration of DRAMSys in gem5. More information can be found in `ext/dramsys` of the gem5 repository.
- (Deprecated) Compile gem5 as a shared library and link it with DRAMSys, which is only supported in older versions of DRAMSys (tag v4.0).

## Development

Some additional development sources required for tests may be obtained using Git LFS.
Make sure to have Git LFS installed through your system's package manager and set up for your user:
```console
$ git lfs install
```

To make the additional files available, run:
```console
$ git lfs pull
```

## Third-party libraries

This application uses several third-party libraries.
For detailed license information, please refer to the [NOTICE](NOTICE) file.

## Contact Information

Further questions about the simulator and the Trace Analyzer can be directed to:

[DRAMSys@iese.fraunhofer.de](mailto:DRAMSys@iese.fraunhofer.de)

Feel free to ask for updates to the simulator's features and please do report any bugs and errors you encounter.
This will encourage us to continuously improve the tool.

## Acknowledgements

The development of DRAMSys is supported by the German Federal Ministry of Education and Research (BMBF) under grant [16ME0934K (DI-DERAMSys)](https://www.elektronikforschung.de/projekte/di-deramsys).

![Bundesministerium für Bildung und Forschung](docs/images/bmbf_logo.svg)

Moreover, the development was supported by the German Research Foundation (DFG) as part of the priority program [Dependable Embedded Systems SPP1500](http://spp1500.itec.kit.edu) and the DFG grant no. [WE2442/10-1](https://www.uni-kl.de/en/3d-dram/). Furthermore, it was supported within the Fraunhofer and DFG cooperation program (grant no. [WE2442/14-1](https://www.iese.fraunhofer.de/en/innovation_trends/autonomous-systems/memtonomy.html)) and by the [Fraunhofer High Performance Center for Simulation- and Software-Based Innovation](https://www.leistungszentrum-simulation-software.de/en.html). Special thanks go to all listed contributors for their work and commitment during seven years of development.

Shama Bhosale      \
Derek Christ       \
Luiza Correa       \
Peter Ehses        \
Johannes Feldmann  \
Robert Gernhardt   \
Doris Gulai        \
Matthias Jung      \
Frederik Lauer     \
Ana Mativi         \
Felipe S. Prado    \
Iron Prando        \
Tran Anh Quoc      \
Janik Schlemminger \
Lukas Steiner      \
Thanh C. Tran      \
Norbert Wehn       \
Christian Weis     \
Éder F. Zulian

## References

[1] TLM Modelling of 3D Stacked Wide I/O DRAM Subsystems, A Virtual Platform for Memory Controller Design Space Exploration
M. Jung, C. Weis, N. Wehn, K. Chandrasekar. International Conference on High-Performance and Embedded Architectures and Compilers 2013 (HiPEAC), Workshop on: Rapid Simulation and Performance Evaluation: Methods and Tools (RAPIDO), January, 2013, Berlin.

[2] DRAMSys4.0: A Fast and Cycle-Accurate SystemC/TLM-Based DRAM Simulator
L. Steiner, M. Jung, F. S. Prado, K. Bykov, N. Wehn. International Conference on Embedded Computer Systems: Architectures, Modeling, and Simulation (SAMOS), July, 2020, Samos Island, Greece.

[3] DRAMSys4.0: An Open-Source Simulation Framework for In-Depth DRAM Analyses
L. Steiner, M. Jung, F. S. Prado, K. Bykov, N. Wehn. International Journal of Parallel Programming (IJPP), Springer, 2022.

[4] DRAMPower: Open-source DRAM Power & Energy Estimation Tool
K. Chandrasekar, C. Weis, Y. Li, S. Goossens, M. Jung, O. Naji, B. Akesson, N. Wehn, K. Goossens. URL: http://www.drampower.info

[5] Optimized Active and Power-Down Mode Refresh Control in 3D-DRAMs
M. Jung, M. Sadri, C. Weis, N. Wehn, L. Benini. VLSI-SoC, October, 2014, Playa del Carmen, Mexico.

[6] Retention Time Measurements and Modelling of Bit Error Rates of WIDE-I/O DRAM in MPSoCs
C. Weis, M. Jung, P. Ehses, C. Santos, P. Vivet, S. Goossens, M. Koedam, N. Wehn. IEEE Conference Design, Automation and Test in Europe (DATE), March, 2015, Grenoble, France.

[7] Efficient Generation of Application Specific Memory Controllers
M. V. Natale, M. Jung, K. Kraft, F. Lauer, J. Feldmann, C. Sudarshan, C. Weis, S. O. Krumke, N. Wehn. ACM/IEEE International Symposium on Memory Systems (MEMSYS 2020), October, 2020, virtual conference.

[8] Simulating DRAM controllers for future system architecture exploration
A. Hansson, N. Agarwal, A. Kolli, T. Wenisch, A. N. Udipi. IEEE International Symposium on Performance Analysis of Systems and Software (ISPASS), 2014, Monterey, USA.

[9] Fast Validation of DRAM Protocols with Timed Petri Nets
M. Jung, K. Kraft, T. Soliman, C. Sudarshan, C. Weis, N. Wehn. ACM International Symposium on Memory Systems (MEMSYS 2019), October, 2019, Washington, DC, USA.
