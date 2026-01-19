# HX711 (~40kg)

- Pin 1 (VSUP): Ingresso alimentazione regolatore (2,7 ~ 5,5V).
- Pin 2 (BASE): Uscita controllo regolatore (non collegato se non usato).
- Pin 3 (AVDD): Alimentazione analogica (2,6 ~ 5,5V).
- Pin 4 (VFB): Ingresso controllo regolatore (collegare a AGND se non usato).
- Pin 5 (AGND): Massa analogica.
- Pin 6 (VBG): Uscita bypass riferimento.
- Pin 7 (INA-): Ingresso negativo Canale A.
- Pin 8 (INA+): Ingresso positivo Canale A.
- Pin 9 (INB-): Ingresso negativo Canale B.
- Pin 10 (INB+): Ingresso positivo Canale B.
- Pin 11 (PD_SCK): Controllo spegnimento (attivo alto) e ingresso clock seriale.
- Pin 12 (DOUT): Uscita dati seriale.
- Pin 13 (XO): I/O cristallo (non collegato se non usato).
- Pin 14 (XI): Ingresso cristallo o clock esterno (0 = usa oscillatore interno).
- Pin 15 (RATE): Controllo frequenza dati (0: 10Hz; 1: 80Hz).
- Pin 16 (DVDD): Alimentazione digitale (2,6 ~ 5,5V).

## SPECIFICHE TECNICHE

- Intervallo ingresso differenziale: ±0.5(AVDD/GUADAGNO) V.
- Ingresso modo comune: Da AGND + 1,2V a AVDD - 1,3V.
- Frequenza dati in uscita: 10Hz o 80Hz con oscillatore interno.
- Codifica dati: Complemento a 2, da 800000 a 7FFFFF HEX.
- Tempo di assestamento: 400ms (a 10Hz) o 50ms (a 80Hz).
- Rumore in ingresso (Guadagno 128): 50 nV(rms) a 10Hz o 90 nV(rms) a 80Hz.
- Deriva termica dell'offset: ±6 nV/°C.
- Reiezione dei disturbi (PSRR/CMRR): 100 dB.
- Frequenza clock esterno: Da 1 MHz a 20 MHz.
- Consumo corrente normale: Analogica 1400 µA, Digitale 100 µA.
- Consumo corrente "Power down": Analogica 0,3 µA, Digitale 0,2 µA.

![Alt Text](https://m.media-amazon.com/images/I/61O-sYI2d0L._AC_UF894,1000_QL80_.jpg)
