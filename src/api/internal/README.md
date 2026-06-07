# Internal API

Internal API võimaldab Cyber32 sisemistel komponentidel omavahel suhelda.

See API ei ole mõeldud kasutajatele ega välistele süsteemidele.

## Vastutus

- Komponentidevahelise suhtluse standardiseerimine
- Teenuste ligipääsu pakkumine
- Sisemiste päringute vahendamine
- Süsteemi sõltuvuste vähendamine

## Näited

- Device Manager → Registry
- Module Manager → Registry
- Logic → Device Manager
- Dashboard → Services
- Runtime → Modules

## Eesmärk

Tagada, et Cyber32 komponendid suhtleksid ühtsel ja kontrollitud viisil.

## Internal API ei vastuta

- Väliste ühenduste eest
- REST teenuste eest
- WebSocket ühenduste eest
- Kasutajate autentimise eest

Need ülesanded kuuluvad teistele API komponentidele.