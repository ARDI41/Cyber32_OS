# State Machine

State Machine vastutab Cyber32 süsteemi olekute ja olekumuutuste haldamise eest.

State Machine tagab, et komponendid liiguvad ühest olekust teise kontrollitud viisil.

## Vastutus

- Olekute kirjeldamine
- Olekumuutuste haldamine
- Lubatud üleminekute kontrollimine
- Süsteemi käitumise ühtlustamine

## Näited

BOOTING
↓
INITIALIZING
↓
READY

READY
↓
ERROR

ERROR
↓
RECOVERY
↓
READY

## Võimalikud kasutuskohad

- System State
- Module State
- Device State
- Service State
- Task State

## State Machine ei vastuta

- Riistvara juhtimise eest
- Plug-and-play eest
- Registri haldamise eest
- Rakendusloogika eest

Need ülesanded kuuluvad teistele süsteemikomponentidele.

## Eesmärk

Tagada, et Cyber32 süsteemi käitumine oleks ennustatav ja kontrollitav.