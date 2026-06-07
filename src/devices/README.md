# Devices

Devices esindab Cyber32 süsteemis kasutatavaid seadmeid.

Device ühendab ühe või mitu draiverit terviklikuks funktsionaalseks objektiks.

## Vastutus

- Seadme oleku haldamine
- Draiverite ühendamine
- Seadme võimekuste kirjeldamine
- Ühtse liidese pakkumine ülejäänud süsteemile

## Näited

- Servo Device
- GPS Device
- Relay Device
- Battery Device
- Display Device

## Device võib kasutada mitut Driverit

Näiteks:

GPS Device
├── GPS Driver
├── Storage Driver
└── Communication Driver

## Devices ei vastuta

- Plug-and-play tuvastuse eest
- Registri haldamise eest
- Rakendusloogika eest
- Kasutajaliidese eest

Need ülesanded kuuluvad kõrgematele kihtidele.

## Seos teiste kihtidega

HAL
↓
Drivers
↓
Devices
↓
Modules
↓
Logic