# Cyber32 Capability Catalog

This catalog defines the first official Cyber32 capability IDs.

Capability IDs are stable system contracts. Logic, Registry, API, Dashboard integrations, and AI tools must use these IDs instead of module names, device names, driver names, manufacturer names, or translated display names.

## Rules

- Capability IDs must start with `CAP_`.
- Capability IDs must use uppercase ASCII letters, numbers, and underscores.
- Capability IDs must describe what the system can do or observe.
- Capability IDs must not contain module names.
- Capability IDs must not contain manufacturer names.
- Capability IDs must not contain board, pin, or protocol details.

## Motion

| Capability ID | Name | Kind | Data Type | Access | Description |
|---|---|---|---|---|---|
| `CAP_MOTION_ENABLE` | Motion Enable | command | boolean | execute | Enable or disable motion output. |
| `CAP_MOTION_STOP` | Motion Stop | command | none | execute | Request controlled motion stop. |
| `CAP_MOTION_EMERGENCY_STOP` | Emergency Stop | command | none | execute | Request immediate safety stop. |
| `CAP_MOTOR_CONTROL` | Motor Control | actuator | object | execute | Control a motor or motor group. |
| `CAP_MOTOR_SPEED` | Motor Speed | actuator | float | read_write | Read or set motor speed. |
| `CAP_MOTOR_DIRECTION` | Motor Direction | actuator | string | read_write | Read or set motor direction. |
| `CAP_SERVO_POSITION` | Servo Position | actuator | float | read_write | Read or set servo position. |
| `CAP_STEERING_ANGLE` | Steering Angle | actuator | float | read_write | Read or set steering angle. |

## Sensors

| Capability ID | Name | Kind | Data Type | Access | Description |
|---|---|---|---|---|---|
| `CAP_DISTANCE` | Distance | sensor | float | read | Distance measurement. |
| `CAP_TEMPERATURE` | Temperature | sensor | float | read | Temperature measurement. |
| `CAP_HUMIDITY` | Humidity | sensor | float | read | Humidity measurement. |
| `CAP_PRESSURE` | Pressure | sensor | float | read | Pressure measurement. |
| `CAP_LIGHT_LEVEL` | Light Level | sensor | float | read | Light intensity measurement. |
| `CAP_IMU_ORIENTATION` | IMU Orientation | sensor | object | read | Orientation from an inertial measurement unit. |
| `CAP_ACCELERATION` | Acceleration | sensor | object | read | Acceleration vector. |
| `CAP_ANGULAR_VELOCITY` | Angular Velocity | sensor | object | read | Angular velocity vector. |
| `CAP_BUTTON_STATE` | Button State | sensor | boolean | read | Button or digital input state. |

## Position

| Capability ID | Name | Kind | Data Type | Access | Description |
|---|---|---|---|---|---|
| `CAP_POSITION` | Position | sensor | object | read | Current position from any position provider. |
| `CAP_POSITION_FIX` | Position Fix | state | boolean | read | Whether a valid position fix is available. |
| `CAP_SPEED` | Speed | sensor | float | read | Current speed. |
| `CAP_HEADING` | Heading | sensor | float | read | Current heading. |
| `CAP_ALTITUDE` | Altitude | sensor | float | read | Current altitude. |
| `CAP_TIME` | Time | sensor | string | read | Time source provided by a module or service. |
| `CAP_TARGET_POSITION` | Target Position | state | object | read_write | Desired target position. |

## Communication

| Capability ID | Name | Kind | Data Type | Access | Description |
|---|---|---|---|---|---|
| `CAP_NETWORK_STATUS` | Network Status | state | object | read | Current network connection state. |
| `CAP_NETWORK_CONNECT` | Network Connect | command | object | execute | Request connection to a network. |
| `CAP_NETWORK_DISCONNECT` | Network Disconnect | command | none | execute | Request network disconnection. |
| `CAP_TELEMETRY_STREAM` | Telemetry Stream | stream | object | stream | Stream telemetry data. |
| `CAP_MESSAGE_SEND` | Message Send | command | object | execute | Send a message through a communication provider. |
| `CAP_MESSAGE_RECEIVE` | Message Receive | event | object | read | Receive messages from a communication provider. |
| `CAP_SIGNAL_STRENGTH` | Signal Strength | sensor | float | read | Communication signal strength. |

## Power

| Capability ID | Name | Kind | Data Type | Access | Description |
|---|---|---|---|---|---|
| `CAP_BATTERY_LEVEL` | Battery Level | sensor | float | read | Battery charge level. |
| `CAP_BATTERY_VOLTAGE` | Battery Voltage | sensor | float | read | Battery voltage. |
| `CAP_BATTERY_CURRENT` | Battery Current | sensor | float | read | Battery current. |
| `CAP_CHARGING_STATE` | Charging State | state | string | read | Current charging state. |
| `CAP_POWER_MODE` | Power Mode | state | string | read_write | Current system power mode. |
| `CAP_POWER_OUTPUT_ENABLE` | Power Output Enable | actuator | boolean | read_write | Enable or disable a power output. |
| `CAP_POWER_CONSUMPTION` | Power Consumption | sensor | float | read | Current power consumption. |

## Storage

| Capability ID | Name | Kind | Data Type | Access | Description |
|---|---|---|---|---|---|
| `CAP_STORAGE_READ` | Storage Read | command | object | execute | Read data from storage. |
| `CAP_STORAGE_WRITE` | Storage Write | command | object | execute | Write data to storage. |
| `CAP_STORAGE_DELETE` | Storage Delete | command | object | execute | Delete stored data. |
| `CAP_STORAGE_STATUS` | Storage Status | state | object | read | Current storage state. |
| `CAP_STORAGE_CAPACITY` | Storage Capacity | sensor | integer | read | Total storage capacity. |
| `CAP_STORAGE_FREE_SPACE` | Storage Free Space | sensor | integer | read | Available storage space. |

## Display

| Capability ID | Name | Kind | Data Type | Access | Description |
|---|---|---|---|---|---|
| `CAP_DISPLAY_TEXT` | Display Text | command | string | execute | Show text on a display. |
| `CAP_DISPLAY_GRAPHICS` | Display Graphics | command | object | execute | Show graphics on a display. |
| `CAP_DISPLAY_CLEAR` | Display Clear | command | none | execute | Clear display output. |
| `CAP_DISPLAY_BRIGHTNESS` | Display Brightness | actuator | float | read_write | Read or set display brightness. |
| `CAP_DISPLAY_STATUS` | Display Status | state | object | read | Current display state. |

## Audio

| Capability ID | Name | Kind | Data Type | Access | Description |
|---|---|---|---|---|---|
| `CAP_AUDIO_PLAY_TONE` | Play Tone | command | object | execute | Play a tone. |
| `CAP_AUDIO_PLAY_SOUND` | Play Sound | command | object | execute | Play a sound asset or sound pattern. |
| `CAP_AUDIO_VOLUME` | Audio Volume | actuator | float | read_write | Read or set audio volume. |
| `CAP_AUDIO_MUTE` | Audio Mute | actuator | boolean | read_write | Read or set mute state. |
| `CAP_MICROPHONE_INPUT` | Microphone Input | stream | binary | stream | Stream microphone input. |

## Catalog Rule

This file is the starting official catalog. New capabilities may be added only when they describe a reusable system capability and follow `CAPABILITY_SCHEMA.md`.

Logic must bind to capability IDs from this catalog or future documented catalog additions.
