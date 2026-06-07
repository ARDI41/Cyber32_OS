# Registry

Registry on Cyber32 süsteemi keskne kataloog.

Kõik süsteemis registreeritud objektid peavad olema leitavad Registry kaudu.

## Vastutus

- Device objektide hoidmine
- Module objektide hoidmine
- Service objektide hoidmine
- Capability objektide hoidmine
- Metadata hoidmine

## Registry eesmärk

Võimaldada süsteemil leida ja kasutada kõiki registreeritud komponente ühtsel viisil.

## Näide

Module
↓
Registration
↓
Registry

Registry
├── Devices
├── Modules
├── Services
├── Capabilities
└── Metadata

## Registry ei vastuta

- Plug-and-play eest
- Draiverite töö eest
- Riistvara juhtimise eest
- Rakendusloogika eest

Need ülesanded kuuluvad teistele kihtidele.