# Sensors metrics


 Set up:
 - web server, which communicates with BMP280 sensor using [device driver](../../devices/BMP280/n2_i2c_bmp280.c) to get sensor reading and exposes it as metrics;
 - prometheus to collect and store metrics;
 - grafana to visualise prometheus metrics;