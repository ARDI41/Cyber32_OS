# Sensor Devices

Sensor Devices sisaldab Cyber32 süsteemi anduriseadmeid.

Device ühendab ühe või mitu anduriga seotud draiverit terviklikuks kasutatavaks seadmeks.

## Vastutus

- Anduri oleku haldamine
- Anduri andmete koondamine
- Anduri võimekuste kirjeldamine
- Ühtse liidese pakkumine süsteemile

## Näited

- Distance Sensor Device
- Temperature Sensor Device
- Humidity Sensor Device
- IMU Device
- GPS Device
- Light Sensor Device
- Pressure Sensor Device

## Device ei vastuta

- Madala taseme I2C, SPI või UART suhtluse eest
- Andurispetsiifilise draiveri eest
- Otsuste tegemise eest
- Automaatika eest

Need ülesanded kuuluvad teistele kihtidele.

## Seos teiste kihtidega

Sensor Drivers
↓
Sensor Device
↓
Module
↓
Logic