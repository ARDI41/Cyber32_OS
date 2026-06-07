# Cyber32 Bus

Cyber32 Bus on Cyber32 süsteemi ühine moodulite tuvastuse ja suhtluse standard.

Cyber32 Bus ei ole ainult üks füüsiline ühendus, vaid põhimõte, kuidas moodulid süsteemis ennast kirjeldavad ja suhtlevad.

## v0.1 põhimõte

Cyber32 toetab mitut tuvastustaset.

## Tuvastustasemed

### Level 1: Passive ID

Lihtsate ja odavate moodulite tuvastus takisti, kondensaatori või muu passiivse komponendi abil.

Sobib lihtsatele moodulitele.

Näited:

- nupp
- LED moodul
- relee moodul
- lihtne servo adapter

### Level 2: I2C Metadata

Targemad moodulid saavad anda enda kohta infot I2C kaudu.

Moodul võib kirjeldada:

- nime
- versiooni
- tüüpi
- capabilities
- vajalikke draivereid
- konfiguratsiooni

### Level 3: Memory / ID Chip

Tulevikus võib igal moodulil olla oma mälukiip või ID kiip.

See võimaldab:

- unikaalset Module ID-d
- tootja infot
- seerianumbrit
- konfiguratsiooni
- kalibreerimisandmeid
- turvalisuse kontrolli

## Eesmärk

Cyber32 Bus võimaldab moodulitel olla plug-and-play.

Kasutaja ühendab mooduli ja Cyber32 OS saab aru, mis moodul see on ja mida see oskab.

## Cyber32 Bus ei vastuta

- konkreetse füüsilise siini elektrilise teostuse eest
- draiverite sisemise töö eest
- kasutajaliidese eest
- rakendusloogika eest

Need kuuluvad teistele kihtidele.