# Oynk Terminal V0

ESP32 firmware prototype for a low-power Oynk authorization terminal. V0 proves
device boot, Wi-Fi connectivity, 16x2 I2C status display, IR remote amount
entry, verified HTTPS, and authorization-service availability.

## Current target

The ACEBOTT controller uses an ESP-WROOM-32-class module and a CH340 USB serial
bridge, so the PlatformIO target is the generic `esp32dev` board. The current
prototype uses `/dev/ttyUSB0`, though the assigned port can change.

The attached LCD is a 16x2 I2C module at address `0x27`. The IR receiver uses
GPIO 32. The verified remote controls are:

- `0` through `9`: append an amount digit;
- `*`: delete the last digit, or begin a new draft from the confirmation screen;
- `OK`: confirm an unsent draft.

## Local configuration

Copy the example without committing the result:

```bash
cp include/secrets.example.h include/secrets.h
```

Set the development Wi-Fi, HTTPS health endpoint, and the endpoint's root CA in
`include/secrets.h`. The firmware deliberately has no insecure TLS fallback.
An empty API URL is allowed during hardware bring-up and is shown as not
configured rather than as a successful authorization service.

Without `secrets.h`, the firmware still builds and prints `CONFIG REQUIRED` over
USB serial.

## Build and flash

Install PlatformIO, then run from this directory:

```bash
pio run
pio run --target upload
pio device monitor
```

The monitor uses 115200 baud. A connected Linux user normally needs membership
in the `dialout` group to access `/dev/ttyUSB*` or `/dev/ttyACM*`.

## Scope boundary

V0 creates only a local, explicitly unsent payment draft. It sends no payment
request and holds no customer or delegated signing key. The eventual terminal
will submit minimal, transaction-bound authorization requests to the Oynk
backend. Policy enforcement, replay protection, cumulative limits, credential
verification, and signing remain server-side.
