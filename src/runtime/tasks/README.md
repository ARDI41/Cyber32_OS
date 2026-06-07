# Tasks

Tasks sisaldab Cyber32 süsteemi tööühikuid.

Task on konkreetne tegevus, mida Runtime saab käivitada.

## Vastutus

- Ülesannete kirjeldamine
- Ülesannete oleku hoidmine
- Ülesannete prioriteetide määramine
- Ülesannete elutsükli toetamine

## Näited

- Read GPS Position
- Read Battery Status
- Send Telemetry
- Update Dashboard
- Check Module Health

## Võimalikud olekud

- CREATED
- WAITING
- RUNNING
- COMPLETED
- FAILED
- CANCELLED

## Tasks ei vastuta

- Ajastamise eest
- Riistvara juhtimise eest
- Plug-and-play eest
- Registri haldamise eest

Need ülesanded kuuluvad teistele Runtime komponentidele.

## Seos teiste komponentidega

Scheduler
↓
Task
↓
Service / Module / Device