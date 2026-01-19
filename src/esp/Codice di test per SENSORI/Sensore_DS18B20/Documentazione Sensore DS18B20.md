# DS18B20

![Alt Text](https://www.electronicwings.com/storage/PlatformSection/TopicContent/422/description/DS18B20.jpg)

## PINOUT

- Dati (giallo): Canale dati digitali
- VCC (rosso): Positivo
- GND (nero): Negativo

## SPECIFICHE

- Chip: DS18B20
- Intervallo di temperatura: da -55°C a +125°C (da -67°F a 257°F)
- Interfaccia: 1-Wire (richiede un unico pin digitale per la comunicazione)
- Sonda: acciaio inossidabile, diametro 6mm
- Risoluzione: da 9 a 12 bit (configurabile dall'utente)
- Accuratezza: ±0,5°C (nell'intervallo tra -10°C e +85°C)
- Tempo di conversione (max): 750ms
- Supporto reti multi-punto: permette di collegare più sensori sullo stesso bus dati
- Grado di protezione: impermeabile (resistente all'acqua)
- Tensione di alimentazione: 3,0V ~ 5,5V
- Compatibilità e Programmazione: Arduino e altri microcontrollori
- Lunghezza del cavo: 1 metro
- Diametro della sonda: 6mm
- Peso: 15g

## LIBRERIE

- [milesburton/Arduino-Temperature-Control-Library v4.0.5](https://github.com/milesburton/Arduino-Temperature-Control-Library)
- [OneWire v2.3.8](https://www.pjrc.com/teensy/td_libs_OneWire.html)

## SCHEMA ELETTRICO

![Alt Text](https://www.electronicwings.com/storage/PlatformSection/TopicContent/422/description/DS18B20%20Interfacing%20Diagram%20with%20ESP32.png)

## CODICE
```
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 2

OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature sensors(&oneWire);

DeviceAddress insideThermometer;

void setup(void)
{
  sensors.begin();

  if (sensors.isParasitePowerMode());
  else Serial.println("OFF");
  sensors.setResolution(insideThermometer, 9);

  float sensor = sensors.getResolution(insideThermometer);
}

void printTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  if (tempC == DEVICE_DISCONNECTED_C)
  {
    // DEVICE NOT FOUND
    return;
  }

  DallasTemperature::toFahrenheit(tempC);
}

void loop(void)
{
  sensors.requestTemperatures(); // Send the command to get temperatures
  delay(1500);

  printTemperature(insideThermometer); // Use a simple function to print out the data
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}
```