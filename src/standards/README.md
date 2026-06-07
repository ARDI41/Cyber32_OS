# Cyber32 Capability Standards

Cyber32 kasutab capability ID-sid.

Capability ID on süsteemi sisemine püsiv tunnus, mille järgi Logic, API, Registry ja Dashboard saavad aru, mida seade või moodul teha oskab.

Capability nimi on ainult inimesele kuvamiseks.

## Põhireegel

Süsteem kasutab:

CAP_POSITION

mitte:

Position

## Miks ID-d?

- ID ei muutu tõlkimisel
- ID ei sõltu mooduli nimest
- ID ei sõltu tootjast
- ID sobib API jaoks
- ID sobib Logic süsteemi jaoks
- ID võimaldab mooduleid vahetada ilma loogikat rikkumata

## Näited

| Capability ID | Inimesele kuvatav nimi |
|---|---|
| CAP_POSITION | Position |
| CAP_SPEED | Speed |
| CAP_TIME | Time |
| CAP_TEMPERATURE | Temperature |
| CAP_DISTANCE | Distance |
| CAP_BATTERY_LEVEL | Battery Level |
| CAP_VIDEO_STREAM | Video Stream |
| CAP_MOTOR_CONTROL | Motor Control |
| CAP_RELAY_CONTROL | Relay Control |

## Reegel Logic kihile

Logic kasutab ainult capability ID-sid.

Näide:

IF CAP_BATTERY_LEVEL < 20%

THEN CAP_SEND_NOTIFICATION

## Reegel Dashboardile

Dashboard võib kuvada inimesele tõlgitud nime.

Näide:

CAP_POSITION

võib kuvada kui:

- Position
- Asukoht
- Position
- Standort

## Eesmärk

Cyber32 peab töötama capability-põhiselt, mitte konkreetsete moodulite või seadmete nimede järgi.