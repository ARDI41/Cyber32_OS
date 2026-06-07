# Modules

Modules esindab Cyber32 süsteemis füüsilisi või loogilisi plug-and-play mooduleid.

Moodul ühendab ühe või mitu Device objekti terviklikuks funktsionaalseks üksuseks.

## Vastutus

- Device objektide koondamine
- Mooduli võimekuste kirjeldamine
- Plug-and-play kasutuskogemuse pakkumine
- Mooduli identiteedi hoidmine

## Näited

- GPS Module
- Sensor Module
- Servo Module
- Power Module
- Planting Module
- Mower Module

## Module võib sisaldada mitut Device objekti

Näiteks:

GPS Module
├── GPS Device
├── Storage Device
└── Communication Device

## Modules ei vastuta

- Madala taseme riistvara juhtimise eest
- Draiverite teostuse eest
- Süsteemi registri eest
- Rakendusloogika eest

Need ülesanded kuuluvad teistele kihtidele.

## Seos teiste kihtidega

Drivers
↓
Devices
↓
Modules
↓
PNP
↓
Registry
↓
Logic