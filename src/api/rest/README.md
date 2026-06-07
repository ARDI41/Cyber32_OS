# REST API

REST API võimaldab Cyber32 süsteemiga suhelda standardsete HTTP päringute kaudu.

REST API sobib konfiguratsiooniks, andmete pärimiseks ja süsteemi haldamiseks.

## Vastutus

- HTTP päringute vastuvõtmine
- Andmete väljastamine
- Käskude vastuvõtmine
- Ressursside haldamine
- Integratsioonide toetamine

## Näited

GET /modules

GET /devices

GET /telemetry

POST /logic/start

POST /system/restart

PUT /config

## Eelised

- Lihtne kasutada
- Lai tugi erinevates süsteemides
- Sobib konfiguratsiooniks
- Sobib integratsioonideks

## REST API ei vastuta

- Reaalaja andmeedastuse eest
- Riistvara juhtimise eest
- Plug-and-play eest
- Registri haldamise eest

Need ülesanded kuuluvad teistele süsteemikomponentidele.

## Eesmärk

Pakkuda standardset ja laialt toetatud ligipääsu Cyber32 funktsioonidele.