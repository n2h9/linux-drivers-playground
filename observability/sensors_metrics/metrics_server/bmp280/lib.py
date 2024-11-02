import sys
import time

def init_bmp280_sensor(device):
    print("bmp280: initialise sensor")
    with open(device, "w") as file:
        file.write("1")

def receive_bmp280_values(device):
    with open(device, "r") as file:
        # Read the first two lines
        cTemp_100 = file.readline().strip()
        pressure_256 = file.readline().strip()

        cTemp = int(cTemp_100) / 100 
        fTemp = cTemp * 1.8 + 32
        pressure = int(pressure_256) / 256 / 100 # in hPa

        return cTemp, fTemp, pressure

