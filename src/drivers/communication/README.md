# Communication

Communication sisaldab side- ja andmesidekomponentidega seotud draivereid.

## Vastutus

- Andmeside seadmete juhtimine
- Ühenduste loomine
- Ühenduste jälgimine
- Sideparameetrite haldamine

## Näited

- CAN Driver
- LoRa Driver
- WiFi Driver
- Bluetooth Driver
- Ethernet Driver
- RS485 Driver

## Communication ei vastuta

- Võrguloogika eest
- Plug-and-play eest
- Seadmete registreerimise eest
- Rakendusloogika eest

Need ülesanded kuuluvad kõrgematele kihtidele.

## Seos teiste kihtidega

HAL
↓
Communication Driver
↓
Device
↓
Module
↓
Services