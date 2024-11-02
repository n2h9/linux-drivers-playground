import sys
from lib import receive_bmp280_values

cTemp, fTemp, pressure =  receive_bmp280_values(sys.argv[1])

# Output data to screen
print ("Temperature in Celsius : %.2f C" %cTemp)
print ("Temperature in Fahrenheit : %.2f F" %fTemp)
print ("Pressure : %.2f hPa " %pressure)
