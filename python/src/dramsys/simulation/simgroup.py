from dramsys.simulation.simulator import Simulator

class SimGroup:
    def __init__(self, simulators: list[Simulator]):
        self.simulators = simulators

    def run(self):
        for simulator in self.simulators:
            simulator.run()
