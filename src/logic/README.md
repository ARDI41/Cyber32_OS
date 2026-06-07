# Logic

Logic sisaldab Cyber32 süsteemi automatiseerimise ja otsustusloogika kihti.

Cyber32 Logic on capability-põhine.

Logic ei tööta otse moodulite ega seadmetega, vaid kasutab Capability objekte.

## Vastutus

- Reeglite käivitamine
- Tingimuste hindamine
- Tegevuste käivitamine
- Töövoogude haldamine
- Automatiseerimise toetamine

## Põhimõte

Logic kasutab Capability objekte.

Näiteks:

Position

Distance

Battery Level

Temperature

Video Stream

Mitte:

GPS Module

RTK Module

Battery Module

## Näide

IF Position Changed
↓
Save Position

IF Battery Level < 20%
↓
Send Warning

IF Distance < 10cm
↓
Stop Motion

## Logic ei vastuta

- Riistvara juhtimise eest
- Plug-and-play eest
- Registri haldamise eest
- Kasutajaliidese eest

Need ülesanded kuuluvad teistele süsteemikomponentidele.

## Eesmärk

Võimaldada luua loogikat sõltumata konkreetsest riistvarast.