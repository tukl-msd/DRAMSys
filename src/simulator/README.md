# Simulator
The **standalone** simulator features a set of trace players, traffic generators and miscellaneous components that can be coupled with DRAMSys.

## Concept
Initiators in the simulator are split up into two disctinct components: **RequestProducers** and the **RequestIssuer**.

**RequestProducers** are simple C++ classes that implement the `RequestProducer` interface. Upon calling the `nextRequest()` method of a producer, a new `Request` is either generated on-the-fly or constructed from a trace file.

The **RequestIssuer** is the SystemC module that connect to DRAMSys. The Issuer has no knowledge of where the requests are coming from. It simply calls the `nextRequest()` method of its associated producer to obtain the next request to be sent to DRAMSys. Using this concept, the generation and the issuing of request is completely decoupled to make very flexible initiator designs possible.

**Requests** are an abstraction over the TLM payloads the issuer generates. A request describes whether it is a read or a write access or an internal `Stop` request that tells the initiator to terminate.

## Configuration
A detailed description on how to configure the traffic generators of the simulator can be found [here](../../configs/README.md).
