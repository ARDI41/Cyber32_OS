# Compatibility

Compatibility vastutab moodulite ja Cyber32 süsteemi ühilduvuse kontrollimise eest.

Enne mooduli täielikku aktiveerimist peab süsteem veenduma, et moodul on toetatud ja kasutatav.

## Vastutus

- Ühilduvuse kontrollimine
- Versioonide kontrollimine
- Nõutud võimekuste kontrollimine
- Konfliktide tuvastamine
- Hoiatuste genereerimine

## Näited

- Cyber32 OS versioon sobib
- Mooduli protokolli versioon sobib
- Nõutud Device tüübid olemas
- Vajalikud Services olemas

## Näide

Moodul tuvastatakse
↓
Metadata loetakse
↓
Compatibility kontrollib nõudeid
↓
Moodul lubatakse süsteemi

või

Moodul blokeeritakse

## Compatibility ei vastuta

- Mooduli avastamise eest
- Metadata lugemise eest
- Registry haldamise eest
- Draiverite töö eest

Need ülesanded kuuluvad teistele PNP komponentidele.

## Eesmärk

Tagada, et Cyber32 süsteem jääks stabiilseks ka siis, kui ühendatakse uusi või vanemaid mooduleid.