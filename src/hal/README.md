# HAL



HAL tähendab Hardware Abstraction Layer.

HAL peidab konkreetse riistvara detailid ülejäänud Cyber32 süsteemi eest.

## Vastutus

- Ühtse ligipääsu pakkumine riistvarale
- ESP32/ESP32-S3 detailide peitmine
- Madala taseme sisendite ja väljundite haldamine
- Draiveritele lihtsa ja stabiilse liidese pakkumine

## HAL alamkomponendid

- GPIO
- PWM
- ADC
- I2C
- SPI
- UART
- CAN
- Storage
- Time

## HAL ei vastuta

- Servo loogika eest
- Andurite tõlgendamise eest
- Plug-and-play tuvastuse eest
- Kasutajaliidese eest
- Rakendusloogika eest

Need kuuluvad kõrgematesse kihtidesse.