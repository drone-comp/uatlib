import random

class MyAgent:


    def __init__(self, mission):
        self.mission = mission
        self.remaining = 0


    def bid_phase(self, t, bid, status, seed):
        if status(self.mission[0], t + 1) == 'available' and \
                status(self.mission[1], t + 2) == 'available':
            random.seed(seed)
            price = 1.0 + random.random()
            bid(self.mission[0], t + 1, price)
            bid(self.mission[1], t + 2, price)
            self.remaining = 2


    def on_bought(self, region, t, value):
        self.remaining -= 1


    def stop(self, t, seed):
        return self.remaining == 0
