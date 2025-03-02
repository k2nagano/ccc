class Lanio:
    def __init__(self):
        self.relays = {0: False, 1: False}

    def set_relay(self, id, on_off):
        self.relays[id] = on_off

    def get_relay(self, id):
        return self.relays[id]

    def get_leak(self):
        return 5.0  # 例: センサーデータを返すように変更してください

    def get_temperature(self):
        return [20.0, 21.0]  # 例: 2つの温度センサーデータを返すように変更してください
