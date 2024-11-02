import os
import sys
import time
import schedule
import threading

from prometheus_client import Gauge
from prometheus_flask_exporter import PrometheusMetrics
from bmp280.lib import init_bmp280_sensor, receive_bmp280_values

device = os.getenv("BMP280_DEVICE_PATH")
if device is None:
    raise TypeError("bmp280: device is not specified")

# Create a gauge metric
temp_celc_value = Gauge('bmp280_temperature_celsius', 'Temperature in celsius from bmp280 sensor')
temp_fahrenheit_value = Gauge('bmp280_temperature_fahrenheit', 'Temperature in fahrenheit from bmp280 sensor')
pressure_hectopascal_value = Gauge('bmp280_pressure_hectopascal', 'Pressure in hecto pascal (hPa) from bmp280 sensor')

def bmp280_update_metrics_schedule_thread_func():
    _init_device()
    # Schedule the job to run every minute
    schedule.every(1).minutes.do(_update_metrics_job)
    
    while True:
        schedule.run_pending()  # Check if any scheduled tasks are pending
        time.sleep(1)           # Sleep for a second to avoid busy waiting

def _update_metrics_job():
    cTemp, fTemp, pressure =  receive_bmp280_values(device)
    temp_celc_value.set(cTemp)
    temp_fahrenheit_value.set(fTemp)
    pressure_hectopascal_value.set(pressure)
    print("bmp280: sensor metrics updated")

def _init_device():
    # run once to initialise the sensor
    init_bmp280_sensor(device)
    # need to sleep here to have a suitable delay between sensor init and first measurement 
    time.sleep(1)
    # run to update the metrics value
    _update_metrics_job()

