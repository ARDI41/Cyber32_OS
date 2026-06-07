# Storage

Storage sisaldab salvestusseadmetega seotud draivereid.

## Vastutus

- Salvestusseadmetega suhtlemine
- Andmete lugemine
- Andmete kirjutamine
- Seadmete oleku jälgimine
- Salvestusseadmete konfiguratsioon

## Näited

- Flash Driver
- EEPROM Driver
- FRAM Driver
- SD Card Driver
- External Memory Driver

## Storage ei vastuta

- Konfiguratsioonide haldamise eest
- Registrite haldamise eest
- Failisüsteemi poliitikate eest
- Rakendusloogika eest

Need ülesanded kuuluvad kõrgematele kihtidele.

## Seos teiste kihtidega

HAL
↓
Storage Driver
↓
Device
↓
Module
↓
Storage Manager