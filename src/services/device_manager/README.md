# Device Manager

Device Manager vastutab Cyber32 süsteemis Device objektide haldamise eest.

Device Manager on peamine teenus, mille kaudu süsteem suhtleb registreeritud seadmetega.

## Vastutus

- Device objektide haldamine
- Device oleku jälgimine
- Device otsimine
- Device elutsükli jälgimine
- Device sündmuste vahendamine

## Näited

- Servo Device
- GPS Device
- Battery Device
- Relay Device
- Display Device

## Device Manager võimaldab

- Leida seadmeid ID järgi
- Leida seadmeid tüübi järgi
- Leida seadmeid capability järgi
- Kontrollida seadme olekut

## Device Manager ei vastuta

- Device loomise eest
- Plug-and-play eest
- Driverite töö eest
- Riistvara juhtimise eest

Need ülesanded kuuluvad teistele süsteemikomponentidele.

## Seos teiste kihtidega

Registry
↓
Device Manager
↓
Modules
↓
Logic