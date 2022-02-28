#!/usr/bin/env python3

import gatt
import sys
import signal

DEVICE_MAC_ADDR = "04:91:62:A5:07:90"

class BleDevice(gatt.Device):
    def connect_succeeded(self):
        super().connect_succeeded()
        print("Connected to {}".format(self.mac_address))

    def connect_failed(self, error):
        super().connect_failed(error)
        print("Connection failed: {}".format(str(error)))

    def disconnect_succeeded(self):
        super().disconnect_succeeded()
        print("Disconnected")

    def services_resolved(self):
        super().services_resolved()

        for service in self.services:
            #print("Service {}".format(service.uuid))
            for characteristic in service.characteristics:
                tmpUUID = str(characteristic.uuid)
                if tmpUUID == "49535343-8841-43f4-a8d4-ecbe34729bb3" :
                    print("Tx Uart : UUID {}".format(tmpUUID))
                    BYTES_ARRAY = b'Hello server'
                    characteristic.write_value(BYTES_ARRAY)
                    print("Tx : {}".format(BYTES_ARRAY.decode("utf-8")))
        
                if tmpUUID == "49535343-1e4d-4bd9-ba61-23c647249616" :
                    print("Rx Uart : UUID {}".format(tmpUUID))
                    characteristic.enable_notifications()

    def characteristic_value_updated(self, characteristic, value):
        print("UUID {} ReadValue: {}".format(characteristic.uuid, value.decode("utf-8")))

    def characteristic_value_failed(self, characteristic, error):
        print("UUID {} ReadValue error: {}".format(characteristic.uuid, error.decode("utf-8")))

def main():
    deviceManager = gatt.DeviceManager(adapter_name='hci0')
    device = BleDevice(mac_address=DEVICE_MAC_ADDR, manager=deviceManager)

    def signal_handler(sig, frame):
        device.disconnect()
        deviceManager.stop()
        sys.exit(0)

    signal.signal(signal.SIGINT, signal_handler)
    device.connect()
    deviceManager.run() # infinite loop

if __name__ == '__main__':
    main()