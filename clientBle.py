#!/usr/bin/env python3

import gatt
import sys
import signal
import time

DEVICE_MAC_ADDR = "04:91:62:A5:07:90"

class BleDevice(gatt.Device):
    def connect_succeeded(self):
        super().connect_succeeded()
        print("Connected to {}".format(self.mac_address))

    def connect_failed(self, error):
        super().connect_failed(error)
        print("Connection failed: {}".format(str(error)))
        sys.exit(0)

    def disconnect_succeeded(self):
        global deviceManager
        super().disconnect_succeeded()
        print("Disconnected")
        deviceManager.stop()
        sys.exit(0)

    def services_resolved(self):
        super().services_resolved()

        serviceUart = next(s for s in self.services if s.uuid == '49535343-fe7d-4ae5-8fa9-9fafd205e455')
        global charUartTx
        charUartTx = next(c for c in serviceUart.characteristics if c.uuid == '49535343-8841-43f4-a8d4-ecbe34729bb3')
        charUartRx = next(c for c in serviceUart.characteristics if c.uuid == '49535343-1e4d-4bd9-ba61-23c647249616')
        charUartRx.enable_notifications()

    def characteristic_value_updated(self, characteristic, value):
        recv = value.decode("utf-8")
        print("UUID {} ReadValue: {}".format(characteristic.uuid, recv))

        if recv == "RTC?" :
            epoch = 'RTC:{:X}'.format(int(time.time()))
            BYTES_ARRAY = bytes(epoch, "ascii")
            global charUartTx
            charUartTx.write_value(BYTES_ARRAY)
            print("Tx => {}".format(BYTES_ARRAY))
        elif recv.find("RTC") != -1:
            data = recv.split(':')
            date = time.gmtime(int(data[1], 16))
            print("Date {:02}-{:02}-{} Time {:02}:{:02}:{:02}".format(
                date.tm_mday, date.tm_mon, date.tm_year,
                date.tm_hour, date.tm_min, date.tm_sec))

    def characteristic_value_failed(self, characteristic, error):
        print("UUID {} ReadValue error: {}".format(characteristic.uuid, error.decode("utf-8")))

    def characteristic_write_value_succeeded(self, characteristic):
        print("UUID {} WriteValue success".format(characteristic.uuid))

    def characteristic_write_value_failed(self, characteristic, error):
        print("UUID {} WriteValue error: {}".format(characteristic.uuid, error.decode("utf-8")))

def main():
    global deviceManager
    deviceManager = gatt.DeviceManager(adapter_name='hci0')
    device = BleDevice(mac_address=DEVICE_MAC_ADDR, manager=deviceManager)

    def signal_handler(sig, frame):
        device.disconnect()

    signal.signal(signal.SIGINT, signal_handler)

    device.connect()
    deviceManager.run() # infinite loop

if __name__ == '__main__':
    main()