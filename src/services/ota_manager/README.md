# OTA Manager

OTA Manager vastutab Cyber32 süsteemi tarkvarauuenduste haldamise eest.

OTA (Over-The-Air) võimaldab süsteemi uuendada ilma füüsilise ühenduseta.

## Vastutus

- Uuenduste kontrollimine
- Uuenduste allalaadimine
- Uuenduste valideerimine
- Uuenduste paigaldamine
- Uuenduste taastamine vea korral

## Näited

- Cyber32 OS uuendus
- Mooduli firmware uuendus
- Dashboardi uuendus
- Turvaparanduste paigaldamine

## OTA Manager võimaldab

- Kauguuendusi
- Versioonihaldust
- Turvalisi uuendusi
- Taastamist ebaõnnestunud uuenduse korral

## OTA Manager ei vastuta

- Võrguühenduse loomise eest
- Failide salvestamise eest
- Riistvara juhtimise eest
- Rakendusloogika eest

Need ülesanded kuuluvad teistele süsteemikomponentidele.

## Seos teiste kihtidega

Network Manager
↓
Storage Manager
↓
OTA Manager
↓
Runtime
↓
System State