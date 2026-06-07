# Interfaces

Interfaces sisaldab Cyber32 ühiseid liideseid.

Liides määrab ära, kuidas komponendid omavahel suhtlevad, ilma et nad teaksid teineteise sisemist teostust.

## Vastutus

- Ühiste lepingute defineerimine
- Süsteemi komponentide standardiseerimine
- Sõltuvuste vähendamine
- Vahetatavate teostuste võimaldamine

## Näited

- IDevice
- IModule
- ILogger
- IStorage
- INetwork
- IService

## Eelised

Komponente saab vahetada ilma ülejäänud süsteemi muutmata.

Näiteks:

FileStorage
↕

SDStorage
↕

CloudStorage

Kõik võivad kasutada sama IStorage liidest.

## Interfaces ei sisalda

- Riistvara juhtimist
- Äriloogikat
- Seadmete implementatsiooni

Siin on ainult kokkulepped ja liidesed.