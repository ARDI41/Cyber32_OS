# Conditions

Conditions sisaldab Cyber32 süsteemi tingimusi.

Condition otsustab, kas tegevus võib käivituda või mitte.

## Vastutus

- Tingimuste hindamine
- Tingimuste kombineerimine
- Võrdluste teostamine
- Loogiliste operatsioonide toetamine

## Näited

Battery Level < 20%

Temperature > 50°C

Distance < 10cm

Position Available

Module Connected

## Loogilised operaatorid

AND

OR

NOT

## Condition struktuur

Input
↓
Evaluation
↓
True / False

## Conditions ei vastuta

- Triggerite jälgimise eest
- Tegevuste käivitamise eest
- Riistvara juhtimise eest
- Registri haldamise eest

Need ülesanded kuuluvad teistele Logic komponentidele.

## Eesmärk

Otsustada, kas Cyber32 peab konkreetse tegevuse käivitama.