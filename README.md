# ESP32 Pool Manager

This project aims to provide a simple but complete __pool manager system__. Starting simple, it has so far :

- Air temperature capture (DS18 sensor, one wire)
- Water temperature capture (DS18 sensor, one wire)
- Fitration pump relay control

## Filtration pump algorithm

Variables :
- PumpingTime<sub>per period</sub> : this is the pumping time calculated
- Temp<sub>air</sub> : this is the air temperature
- Period : the period calculation time : 24h...

According to pool specialists, filtration pump command must be driven by following formula: 

### Temp<sub>air</sub> < 25°C
> PumpingTime<sub>per period</sub> =  Temp<sub>air</sub> / 2

### Temp<sub>air</sub> > 25°C
> PumpingTime<sub>per period</sub> =  Period

This very last case means that if Temp<sub>air</sub> > 25°C, __filtration pump must be ON all the time__.