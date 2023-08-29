# Simulator
The **standalone** simulator features a set of trace players, traffic generators and miscellaneous components that can be coupled with DRAMSys.

## Concept
Initiators in the simulator are split up into two disctinct components: **RequestProducers** and **RequestIssuers**.

**RequestProducers** are simple C++ classes that implement the `RequestProducer` interface. Upon calling the `nextRequest()` method of a producer, a new `Request` is either generated on-the-fly or constructed from a trace file.

**RequestIssuers** are the SystemC modules that connect to DRAMSys. Issuers have no knowledge of where the requests are coming from. They simply call their `nextRequest()` callback that it has been passed in the constructor to obtain the next request to be sent to DRAMSys. Using this concept, the generation and the issuing of request is completely decoupled to make very flexible initiator designs possible.

**Initiators** implement the `Initiator` interface, which describes how the initiator is bound to DRAMSys. This abstracts over the actual socket type used by the initiator.
Complex initiators may implement the interface directly, but for simple cases, there exists the templated `SimpleInitiator<Producer>` class. This specialized initiator consists of only one producer and one issuer that operate together. The `StlPlayer` and `RowHammer` issuers make use of the `SimpleInitiator`.
The `TrafficGenerator` is one example of a direct implementation of the `Initiator` interface, as it consists of many producers (which represent the states of the state machine) but of only one issuer.

**Requests** are an abstraction over the TLM payloads the issuer generates. A request describes whether it is a read or a write access or an internal `Stop` request that tells the initiator to terminate.
The **delay** field specifies the time that should pass between the issuance of the previous and the current request.

## Configuration
A detailed description on how to configure the traffic generators of the simulator can be found [here](../../configs/README.md).
