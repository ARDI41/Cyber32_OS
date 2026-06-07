# Storage Manager

Storage Manager vastutab Cyber32 süsteemi andmete ja konfiguratsioonide salvestamise haldamise eest.

Storage Manager pakub ühtset liidest sõltumata sellest, kas andmed asuvad Flash mälus, EEPROM-is, FRAM-is, SD kaardil või mõnes tulevases salvestusseadmes.

## Vastutus

- Andmete salvestamise koordineerimine
- Konfiguratsioonide haldamine
- Andmete laadimine
- Andmete varundamise toetamine
- Salvestusruumi jälgimine

## Näited

- Süsteemi konfiguratsioonid
- Moodulite konfiguratsioonid
- Logid
- Telemeetria andmed
- Kasutaja seaded

## Storage Manager võimaldab

- Salvestada andmeid ühtsel viisil
- Vahetada salvestusseadet ilma rakendusi muutmata
- Hallata konfiguratsioone
- Kontrollida salvestusruumi kasutust

## Storage Manager ei vastuta

- Madala taseme lugemise ja kirjutamise eest
- Salvestusseadme draiverite eest
- Riistvara juhtimise eest
- Rakendusloogika eest

Need ülesanded kuuluvad teistele süsteemikomponentidele.

## Seos teiste kihtidega

Storage Devices
↓
Storage Manager
↓
Config
↓
Modules
↓
Services