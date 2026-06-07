# Hot Plug

Hot Plug vastutab töötava Cyber32 süsteemi ajal ühendatud ja eemaldatud moodulite käsitlemise eest.

Moodulite lisamine või eemaldamine ei tohiks nõuda süsteemi taaskäivitamist.

## Vastutus

- Uute moodulite lisamise käsitlemine
- Moodulite eemaldamise käsitlemine
- Süsteemi teavitamine muudatustest
- Registry uuendamise käivitamine
- Turvalise eemaldamise toetamine

## Näide

Moodul ühendatakse
↓
Discovery
↓
Identification
↓
Registration
↓
Moodul kasutatav

Moodul eemaldatakse
↓
Hot Plug tuvastab eemaldamise
↓
Registry uuendatakse
↓
Moodul eemaldatakse süsteemist

## Hot Plug ei vastuta

- Mooduli avastamise eest
- Metadata lugemise eest
- Draiverite töö eest
- Rakendusloogika eest

Need ülesanded kuuluvad teistele komponentidele.