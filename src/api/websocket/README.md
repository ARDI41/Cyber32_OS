# WebSocket API

WebSocket API võimaldab reaalajas kahepoolset suhtlust Cyber32 süsteemi ja klientide vahel.

Erinevalt REST API-st jääb ühendus avatuks ning andmeid saab saata mõlemas suunas.

## Vastutus

- Reaalaja andmevahetus
- Telemeetria voogedastus
- Sündmuste edastamine
- Käsu- ja vastusevahetus
- Ühenduse oleku jälgimine

## Näited

- Aku taseme reaalaja jälgimine
- GPS asukoha voogedastus
- Mooduli ühendamise sündmus
- Dashboardi reaalaja uuendused
- Roboti juhtimine

## Eelised

- Väike viivitus
- Reaalajas uuendused
- Vähem päringuid võrreldes REST API-ga
- Sobib juhtimiseks ja telemeetriaks

## WebSocket API ei vastuta

- Autentimise eest
- Riistvara juhtimise eest
- Plug-and-play eest
- Registri haldamise eest

Need ülesanded kuuluvad teistele süsteemikomponentidele.

## Eesmärk

Tagada Cyber32 süsteemi ja kasutajaliidese vaheline kiire ja reaalajas suhtlus.