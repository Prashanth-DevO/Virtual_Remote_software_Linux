# Virtual Remote (Linux Side)

Linux service for Android remote game controls.

## Protocol
- **UDP 9000**: phone -> Linux controller input.
- **TCP 9001**: Linux -> phone vibration messages (`VIBRATE:<duration_ms>:<strength>\n`).
- **UDP 9002**: discovery request/response.

### Discovery
Phone sends:
- `WHO_IS_CONTROLLER`

Linux replies:
- `CONTROLLER_AVAILABLE;udp=9000;tcp=9001`

### Control messages (UDP 9000)
Examples:
- `BTN_A:1`, `BTN_A:0`
- `BTN_B:1`, `BTN_X:1`, `BTN_Y:0`
- `BTN_L1:1`, `BTN_R1:0`
- `BTN_START:1`, `BTN_SELECT:0`
- `LX:-12000`, `LY:8000`, `RX:0`, `RY:0`
- `L2:200`, `R2:15`
- `DPAD_X:-1`, `DPAD_Y:1`

### Vibration forward command
If Linux receives this packet on UDP 9000:
- `VIBRATE:120:200`

it forwards to connected phone TCP client on port 9001.

## Build
```bash
cmake -S . -B build
cmake --build build
```

## Run
```bash
sudo ./build/virtual_remote_server
```

> `sudo` is normally required for `/dev/uinput` access.
