# Configuration

The DRAMSys executable supports one argument, which is a JSON file that contains certain arguments and the name of nested configuration files for the desired simulation. Alternatively, the contents of nested configuration files can also be added directly to the top configuration file instead of the file name.

The JSON code below shows an example configuration:

```json
{
    "simulation": {
        "simulationid": "ddr3-example",
        "simconfig": "example.json",
        "memspec": "MICRON_1Gb_DDR3-1600_8bit_G.json",
        "addressmapping": "am_ddr3_8x1Gbx8_dimm_p1KB_brc.json",
        "mcconfig":"fifoStrict.json",
        "tracesetup": [
            {
                "clkMhz": 300,
                "name": "example.stl"
            },
            {
                "clkMhz": 2000,
                "name": "gen0",
                "numRequests": 2000,
                "rwRatio": 0.85,
                "addressDistribution": "random",
                "seed": 123456,
                "maxPendingReadRequests": 8,
                "maxPendingWriteRequests": 8,
                "minAddress": 16384,
                "maxAddress": 32767
            },
            {
                "clkMhz": 1000,
                "name": "ham0",
                "numRequests": 4000,
                "rowIncrement": 2097152
            }
        ]	
    }
}
```
Field Descriptions:
- "simulationid": simulation file identifier
- "simconfig": configuration file for the DRAMSys simulator
- "memspec": memory device configuration file
- "addressmapping": address mapping configuration file
- "mcconfig": memory controller configuration file
- "tracesetup": The trace setup is only used in standalone mode. In library mode or gem5 mode the trace setup is ignored. Each device should be added as a json object inside the "tracesetup" array.

Each **trace setup** device configuration can be a **trace player**, a **traffic generator** or a **row hammer generator**. The type will be automatically concluded based on the given parameters.
All device configurations must define a **clkMhz** (operation frequency of the **traffic initiator**) and a **name** (in case of a trace player this specifies the **trace file** to play; in case of a generator this field is only for identification purposes).
The **maxPendingReadRequests** and **maxPendingWriteRequests** parameters define the maximum number of outstanding read/write requests. The current implementation delays all memory accesses if one limit is reached. The default value (0) disables the limit.

A **traffic generator** can be configured to generate **numRequests** requests in total, of which the **rwRatio** field defines the probability of one request being a read request. The length of a request (in bytes) can be specified with the **dataLength** parameter. The **seed** parameter can be used to produce identical results for all simulations. **minAddress** and **maxAddress** specify the address range, by default the whole address range is used. The parameter **addressDistribution** can either be set to **random** or **sequential**. In case of **sequential** the additional **addressIncrement** field must be specified, defining the address increment after each request. The address alignment of the random generator can be configured using the **dataAlignment** field. By default, the addresses will be naturally aligned at dataLength.

For more advanced use cases, the traffic generator is capable of acting as a state machine with multiple states that can be configured in the same manner as described earlier. Each state is specified as an element in the **states** array. Each state has to include an unique **id**. The **transitions** field describes all possible transitions from one state to another with their associated **probability**.
In the context of a state machine, there exists another type of generator: the idle generator. In an idle state no requests are issued. The parameter **idleClks** specifies the duration of the idle state.

An example of a state machine configuration with 3 states is shown below.

```json
{
    "clkMhz": 100,
    "maxPendingReadRequests": 8,
    "name": "StateMachine",
    "states": [
        {
            "addressDistribution": "sequential",
            "addressIncrement": 256,
            "id": 0,
            "maxAddress": 1024,
            "numRequests": 1000,
            "rwRatio": 0.5
        },
        {
            "id": 1,
            "idleClks": 1000
        },
        {
            "addressDistribution": "random",
            "id": 2,
            "maxAddress": 2048,
            "minAddress": 1024,
            "numRequests": 1000,
            "rwRatio": 0.75
        }
    ],
    "transitions": [
        {
            "from": 0,
            "probability": 1.0,
            "to": 1
        },
        {
            "from": 1,
            "probability": 1.0,
            "to": 2
        }
    ]
}
```

The **row hammer generator** is a special traffic generator that mimics a row hammer attack. It generates **numRequests** alternating read requests to two different addresses. The first address is 0x0, the second address is specified by the **rowIncrement** parameter and should decode to a different row in the same bank. Since only one outstanding request is allowed, the controller cannot perform any reordering, forcing a row switch (precharge and activate) for each access. That way the number of activations on the target rows are maximized.

Most configuration fields reference other JSON files which contain more specialized chunks of the configuration like a memory specification, an address mapping and a memory controller configuration.


## Trace Files

A **trace file** is a prerecorded file containing memory transactions. Each memory transaction has a time stamp that tells the simulator when it shall happen, a transaction type (*read* or *write*) and a hexadecimal memory address. The optional length parameter (in bytes) allows sending transactions with a custom length that does not match the length of a single DRAM burst access. In this case a length converter has to be added. Write transactions also have to specify a data field when storage is enabled in DRAMSys.

There are two different kinds of trace files. They differ in their timing behavior and are distinguished by their file extension.

### STL Traces (.stl)

The time stamp corresponds to the time the request is to be issued and it is given in cycles of the bus master device. Example: The device is an FPGA with a frequency of 200 MHz (clock period of 5 ns). If the time stamp is 10 the request is to be issued when time is 50 ns.

Syntax example:

```
# Comment lines begin with #
# cycle: [(length)] command hex-address [hex-data]
31: read 0x400140
33: read 0x400160
56: write 0x7fff8000 0x123456789abcdef...
81: (128) read 0x400180
```

### Relative STL Traces (.rstl)

The time stamp corresponds to the time the request is to be issued relative to the end of the previous transaction. This results in a simulation in which the trace player is able to react to possible delays due to DRAM bottlenecks.

Syntax example:

```
# Comment lines begin with #
# cycle: [(length)] command hex-address [hex-data]
31: read 0x400140
2: (512) read 0x400160
23: write 0x7fff8000 0x123456789abcdef...
10: read 0x400180
```

## Trace Player

A trace player is equivalent to a bus master device (processor, FPGA, etc.). It reads an input trace file and translates each line into a new memory request. By adding a new device element into the trace setup section one can specify a new trace player, its operating frequency and its trace file.

## Configuration File Sections

The main configuration file is divided into self-contained sections. Each of these sections refers to sub-configuration files. Below, the sub-configurations are listed and explained.

### Simulator Configuration

The content of [ddr3.json](simconfig/example.json) is presented below as an example.

```json
{
    "simconfig": {
        "SimulationName": "example",
        "Debug": false,
        "DatabaseRecording": true,
        "PowerAnalysis": false,
        "EnableWindowing": false,
        "WindowSize": 1000,
        "SimulationProgressBar": true,
        "CheckTLM2Protocol": false,
        "UseMalloc": false,
        "AddressOffset": 0,
        "StoreMode": "NoStorage"
    }
}
```

- *SimulationName* (string)
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
- *SimulationProgressBar* (boolean)
    - true: enables the simulation progress bar
    - false: disables the simulation progress bar
- *CheckTLM2Protocol* (boolean)
    - true: enables the TLM-2.0 Protocol Checking
    - false: disables the TLM-2.0 Protocol Checking
- *UseMalloc* (boolean)
    - false: model storage using mmap() (DEFAULT)
    - true: allocate memory for modeling storage using malloc()
- *AddressOffset* (unsigned int)
    - Address offset of the DRAM subsystem (required for the gem5 coupling).
- *StoreMode* (string)
    - "NoStorage": no storage
    - "Store": store data without error model

### Memory Specification

A file with memory specifications. Timings and currents come from data sheets and measurements and usually do not change.  
The fields inside "mempowerspec" can be written directly as a **double** type, "memoryId" and "memoryType" are **string**, all other fields are **unsigned int**.

### Address Mapping

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
    "addressmapping": {
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

### Memory Controller

An example follows.

```json
{
    "mcconfig": {
        "PagePolicy": "Open", 
        "Scheduler": "Fifo", 
        "SchedulerBuffer": "ReadWrite",
        "RequestBufferSize": 8, 
        "CmdMux": "Oldest", 
        "RespQueue": "Fifo", 
        "RefreshPolicy": "AllBank", 
        "RefreshMaxPostponed": 8, 
        "RefreshMaxPulledin": 8, 
        "PowerDownPolicy": "NoPowerDown", 
        "Arbiter": "Fifo",
        "MaxActiveTransactions": 128,
        "RefreshManagement": true
    }
}
```

- *PagePolicy* (string)
    - "Open": no auto-precharge is performed after read or write commands
    - "OpenAdaptive": auto-precharge after read or write commands is only performed if further requests for the targeted bank are stored in the scheduler and all the requests are row misses
    - "Closed": auto-precharge is performed after each read or write command
    - "ClosedAdaptive": auto-precharge after read or write commands is performed if all further requests for the targeted bank stored in the scheduler are row misses or if there are no further requests stored
- *Scheduler* (string)
    - all policies are applied locally to one bank, not globally to the whole channel
    - "Fifo": first in, first out policy
    - "FrFcfs": first-ready - first-come, first-served policy (row hits are preferred to row misses)
    - "FrFcfsGrp": first-ready - first-come, first-served policy with additional grouping of read and write requests
- *SchedulerBuffer* (string)
    - "Bankwise": requests are stored in bankwise buffers
    - "ReadWrite": read and write requests are stored in different buffers
    - "Shared": all requests are stored in one shared buffer
- *RequestBufferSize* (unsigned int)
    - depth of a single scheduler buffer entity, total buffer depth depends on the selected scheduler buffer policy
- *CmdMux* (string)
    - "Oldest": from all commands that are ready to be issued in the current clock cycle the one that belongs to the oldest transaction has the highest priority; commands from refresh managers have a higher priority than all other commands, commands from power down managers have a lower priority than all other commands
    - "Strict": based on "Oldest", in addition, read and write commands are strictly issued in the order their corresponding requests arrived at the channel controller (can only be used in combination with the "Fifo" scheduler)
- *RespQueue* (string)
    - "Fifo": the original request order is not restored for outgoing responses
    - "Reorder": the original request order is restored for outgoing responses (only within the channel)
- *RefreshPolicy* (string)
    - "NoRefresh": refresh is disabled
    - "AllBank": all-bank refresh commands are issued (per rank)
    - "PerBank": per-bank refresh commands are issued (only available in combination with LPDDR4, Wide I/O 2, GDDR5/5X/6 or HBM2)
    - "SameBank": same-bank refresh commands are issued (only available in combination with DDR5)
- *RefreshMaxPostponed* (unsigned int)
    - maximum number of refresh commands that can be postponed (with per-bank refresh the number is internally multiplied with the number of banks, with same-bank refresh the number is internally multiplied with the number of banks per bank group)
- *RefreshMaxPulledin* (unsigned int)
    - maximum number of refresh commands that can be pulled in (with per-bank refresh the number is internally multiplied with the number of banks, with same-bank refresh the number is internally multiplied with the number of banks per bank group)
- *PowerDownPolicy* (string)
    - "NoPowerDown": power down disabled
    - "Staggered": staggered power down policy [5]
- *Arbiter* (string)
    - "Simple": simple forwarding of transactions to the right channel or initiator
    - "Fifo": transactions can be buffered internally to achieve a higher throughput especially in multi-initiator-multi-channel configurations
    - "Reorder": based on "Fifo", in addition, the original request order is restored for outgoing responses (separately for each initiator and globally to all channels)
- *MaxActiveTransactions* (unsigned int)
    - maximum number of active transactions per initiator (only applies to "Fifo" and "Reorder" arbiter policy)
- *RefreshManagement* (boolean)
    - enable the sending of refresh management commands when the number of activates to one bank exceeds a certain management threshold (only supported in DDR5 and LPDDR5)
