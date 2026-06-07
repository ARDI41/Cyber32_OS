# Module Manager

Module Manager vastutab Cyber32 süsteemis Module objektide haldamise eest.

Module Manager on peamine teenus, mille kaudu süsteem suhtleb registreeritud moodulitega.

## Vastutus

- Module objektide haldamine
- Module oleku jälgimine
- Module otsimine
- Module elutsükli jälgimine
- Module sündmuste vahendamine

## Näited

- GPS Module
- Sensor Module
- Power Module
- Planting Module
- Mower Module

## Module Manager võimaldab

- Leida mooduleid ID järgi
- Leida mooduleid tüübi järgi
- Leida mooduleid capability järgi
- Leida mooduliga seotud Device objektid
- Kontrollida mooduli olekut

## Module Manager ei vastuta

- Mooduli loomise eest
- Plug-and-play eest
- Driverite töö eest
- Riistvara juhtimise eest

Need ülesanded kuuluvad teistele süsteemikomponentidele.

## Seos teiste kihtidega

Registry
↓
Module Manager
↓
Devices
↓
Logic