# Event Bus

Event Bus võimaldab Cyber32 komponentidel omavahel suhelda ilma otsese sõltuvuseta.

Komponendid saadavad sündmusi ja kuulavad neid.

## Näited

* Moodul ühendati
* Seade eemaldati
* Aku tase muutus
* GPS sai fixi
* Viga tekkis

## Eesmärk

Vähendada komponentide omavahelist sõltuvust.

Selle asemel, et üks komponent kutsuks teist otse, saadetakse sündmus Event Bus-i kaudu.

## Näide

```text
GPS Device
    ↓
GPS_FIX_ACQUIRED
    ↓
Event Bus
    ↓
Navigation Service
```

## Event Bus ei vastuta

* Sündmuste salvestamise eest
* Seadmete juhtimise eest
* Süsteemi oleku hoidmise eest

Event Bus ainult vahendab sündmusi.
