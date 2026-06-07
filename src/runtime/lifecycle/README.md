# Lifecycle

Lifecycle vastutab Cyber32 komponentide elutsükli haldamise eest.

Iga komponent liigub läbi kindlate olekute alates loomisest kuni eemaldamiseni.

## Vastutus

- Elutsükli olekute haldamine
- Komponentide käivitamine
- Komponentide peatamine
- Komponentide taaskäivitamine
- Komponentide eemaldamine

## Tüüpiline elutsükkel

CREATED
↓
INITIALIZED
↓
STARTING
↓
RUNNING
↓
STOPPING
↓
STOPPED

## Veaolukord

RUNNING
↓
ERROR
↓
RECOVERY
↓
RUNNING

või

ERROR
↓
STOPPED

## Kasutuskohad

- Modules
- Devices
- Services
- Tasks

## Lifecycle ei vastuta

- Riistvara juhtimise eest
- Plug-and-play eest
- Registri haldamise eest
- Rakendusloogika eest

Need ülesanded kuuluvad teistele süsteemikomponentidele.

## Eesmärk

Tagada, et kõik Cyber32 komponendid käituksid ühtse ja ennustatava elutsükli järgi.