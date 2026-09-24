# Hardware bring-up

The supplied photos show:

- an ACEBOTT controller with an ESP-WROOM-32-class module;
- the ACEBOTT expansion/connector board;
- an ACEBOTT 16x2 LCD with a PCF8574 I2C backpack at address `0x27`;
- an ACEBOTT IR receiver connected to GPIO 32 and its NEC remote;
- breadboard, jumper wires, resistors, LEDs, buzzer, buttons, IR remote and
  receiver, PIR and ultrasonic sensors, servo, and USB cable.

V0 targets the controller as a generic PlatformIO `esp32dev` board. USB serial
remains the diagnostic output while the LCD provides the user-facing status.

Planned bring-up order:

1. Connect the ESP32 over USB and identify its USB-to-serial controller and
   `/dev/ttyUSB*` or `/dev/ttyACM*` port.
2. Flash and monitor the serial-only firmware.
3. Detect the LCD on the I2C bus and confirm its address.
4. Display boot, network, API, and amount-entry states on the LCD.
5. Decode the IR remote and use it as the temporary numeric keypad.
6. Add a status LED and buzzer after the input flow is stable.

The terminal must never store a customer delegation key or reusable plaintext
credential. Production hardware also needs secure boot, signed firmware, flash
encryption, device identity, rollback protection, and server-side revocation.
