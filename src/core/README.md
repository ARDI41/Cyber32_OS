# Core

# Core

Core on Cyber32 OS-i süda.

Core vastutab süsteemi põhiloogika eest ning ei tohi sõltuda konkreetsetest anduritest, mootoritest ega moodulitest.

## Alamkomponendid

### Kernel

Süsteemi keskne juhtkomponent.

### Event Bus

Võimaldab komponentidel omavahel sündmuste kaudu suhelda.

### Logger

Süsteemi logimine ja diagnostika.

### Config

Süsteemi konfiguratsioonide haldus.

### Registry

Core taseme registrid ja süsteemiandmed.

### System State

Cyber32 süsteemi üldolek.

### Interfaces

Ühised liidesed, mida kasutavad kogu süsteemi komponendid.

## Core ei tohi sisaldada

* Servo juhtimist
* GPS lugemist
* WiFi ühendusi
* Dashboardi loogikat
* Konkreetsete seadmete draivereid

Need kuuluvad teistesse kihtidesse.
