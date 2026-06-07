# Discovery

Discovery vastutab Cyber32 süsteemis uute moodulite avastamise eest.

Discovery on Plug and Play protsessi esimene etapp.

## Vastutus

- Uute moodulite leidmine
- Ühendatud moodulite jälgimine
- Eemaldatud moodulite tuvastamine
- Discovery sündmuste tekitamine

## Võimalikud avastusmeetodid

- I2C skaneerimine
- CAN sõnumid
- UART tuvastus
- Mälu-ID lugemine
- Tulevased Cyber32 Bus lahendused

## Näide

Moodul ühendatakse
↓
Discovery leiab mooduli
↓
Mooduli aadress või ID saadakse
↓
Algab identifitseerimine

## Discovery ei vastuta

- Metadata lugemise eest
- Mooduli registreerimise eest
- Ühilduvuse kontrollimise eest

Need ülesanded kuuluvad järgmistele PNP komponentidele.