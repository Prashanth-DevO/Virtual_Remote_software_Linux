# linux-joystick

`linux-joystick` is a Linux C++ server that:
1. Creates a virtual gamepad with `/dev/uinput`.
2. Receives controller state over UDP.
3. Applies that state to the virtual gamepad.
4. Exposes a UDP discovery endpoint so clients can find the server.

The executable built by this repo is `virtual_remote_server`.

## Virtual Linux Remote 🎮

## System Architecture

![System Architecture](docs/demo/Welcome%20to%20FigJam%20%281%29.png)

## Server Initialization & Runtime Flow

![Server Initialization and Runtime Flow](docs/demo/Untitled%20%283%29.png)

## Discovery Protocol

![Discovery Protocol](docs/demo/Untitled%20%284%29.png)

## Demo Screenshots

![Demo Screenshot 1](docs/demo/Screenshot%20from%202026-03-07%2020-27-21.png)
![Demo Screenshot 2](docs/demo/Screenshot%20from%202026-03-07%2020-28-00.png)
![Demo Screenshot 3](docs/demo/Screenshot%20from%202026-03-07%2020-28-26.png)
![Demo Screenshot 4](docs/demo/Screenshot%20from%202026-03-07%2020-30-06.png)

## Demo Videos

Add demo video files under `docs/demo/` (for example: `demo.mp4`) and embed them here using:

```md
[Watch demo video](docs/demo/demo.mp4)
```

## What Is Implemented (Verified from Source)

- Binary controller protocol (`ControllerPacketV1`, 36 bytes), not text commands.
- UDP control receiver on port `9000`.
- Pair-lock to first sender IP until timeout.
- Input watchdog:
  - >200 ms silence: send neutral state.
  - >2000 ms silence: unlock paired IP and neutral again.
- UDP discovery service on port `9002` with request/response structs.
- Stable `server_id` generation from `/etc/machine-id` hash (fallback string if missing).
- Per-IP discovery response rate limiting (~1 response / 150 ms).
- Virtual gamepad axes/buttons mapped to Linux input event codes.

## Important Current Limitation

The TCP feedback server exists in code (`TcpFeedbackServer` on port `9001`) but is not started or used by `main`, so vibration forwarding is currently not active in runtime.

## Project Layout

- `CMakeLists.txt`: builds single target `virtual_remote_server` with C++17.
- `src/server_main.cpp`: process entry point, starts gamepad + UDP control + discovery service.
- `src/protocol.h`: packed binary control packet definition and button bit assignments.
- `src/controller_engine.h/.cpp`: packet validation, pairing lock, watchdog timeout, and input dispatch.
- `src/gamepad_mapping.h`: mapping from protocol controls to Linux `EV_KEY` / `EV_ABS` codes.
- `src/virtual_gamepad.h/.cpp`: `/dev/uinput` setup, device creation/destruction, event emission.
- `src/udp_receiver.h/.cpp`: UDP socket bind + datagram receive wrapper for control traffic.
- `src/discovery_protocol.h`: packed binary discovery request/response structs and flags.
- `src/discovery_service.h/.cpp`: UDP discovery socket thread, validation, status response building.
- `src/tcp_feedback_server.h/.cpp`: optional TCP client accept loop and `VIBRATE:<ms>:<strength>\n` sender.
- `include/`: currently empty.

## Network Ports

- `9000/udp`: control packets (`ControllerPacketV1`).
- `9002/udp`: discovery (`DiscoverReqV1` -> `DiscoverRespV1`).
- `9001/tcp`: defined for feedback in status/structs and `TcpFeedbackServer`, but not currently started from `main`.

## Control Packet Format (`src/protocol.h`)

`ControllerPacketV1` is `#pragma pack(push, 1)` and must be exactly 36 bytes.

Fields:
- `magic` (`uint32_t`): must equal `0x4F494E55` (`UNIO_MAGIC`).
- `version` (`uint16_t`): must equal `1`.
- `size` (`uint16_t`): must equal `sizeof(ControllerPacketV1)` (36).
- `seq` (`uint32_t`): sequence number.
- `ts_us` (`uint64_t`): timestamp in microseconds (client-provided).
- `lx, ly, rx, ry` (`int16_t`): stick axes in `-32768..32767`.
- `l2, r2` (`uint8_t`): trigger values in `0..255`.
- `dpad_x, dpad_y` (`int8_t`): each in `-1, 0, 1`.
- `buttons` (`uint32_t`): bitmask using `ButtonBits` enum.

Button bits:
- 0 `A`
- 1 `B`
- 2 `X`
- 3 `Y`
- 4 `L1`
- 5 `R1`
- 6 `L3`
- 7 `R3`
- 8 `SELECT`
- 9 `START`
- 10 `HOME` (defined in protocol, currently not applied in `ControllerEngine`)

## Controller Engine Behavior

`ControllerEngine::processPacket`:
1. Applies watchdog logic based on time since last valid packet.
2. Rejects packets shorter than 36 bytes.
3. Copies packet bytes and validates `magic/version/size`.
4. Pair-locks to first sender IP after unlocked state.
5. Rejects packets from other IPs while locked.
6. Sends all axes and button states to virtual gamepad.
7. Issues `sync()` after each accepted packet.

Neutral state sets:
- All sticks to `0`.
- Triggers to `0`.
- D-pad to `0`.
- Buttons `A/B/X/Y/L1/R1/L3/R3/SELECT/START` to released.

## Virtual Gamepad Details

Device path:
- `/dev/uinput` opened with `O_WRONLY | O_NONBLOCK`.

Enabled event types:
- `EV_KEY`
- `EV_ABS`

Buttons enabled:
- `GP_A`, `GP_B`, `GP_X`, `GP_Y`
- `GP_L1`, `GP_R1`
- `GP_START`, `GP_SELECT`
- `GP_L3`, `GP_R3`, `GP_HOME`

Axes configured:
- `GP_LX`, `GP_LY`, `GP_RX`, `GP_RY`: `-32768..32767`
- `GP_DPAD_X`, `GP_DPAD_Y`: `-1..1`
- `GP_L2`, `GP_R2`: `0..255`

uinput identity:
- Name: `Virtual Remote Gamepad`
- Bus: `BUS_USB`
- Vendor/Product: `0x1234 / 0x5678`

## Discovery Protocol

Magic/version:
- `kDiscoveryMagic = 0x53444A4C` (`LJDS` in little-endian bytes)
- `kDiscoveryVersion = 1`

Request (`DiscoverReqV1`):
- `magic`, `version`, `msg_type=DiscoverReq`, `reserved`, `nonce`

Response (`DiscoverRespV1`):
- `magic`, `version`, `msg_type=DiscoverResp`, `reserved`
- `nonce` (echoed from request)
- `server_id`
- `control_port`
- `feedback_port`
- `proto_ver`
- `name_len`
- `flags`
- followed by `name` bytes (not null-terminated, max 64)

Flags currently used:
- `kFlagPairedLocked` set when controller engine is locked.
- `kFlagSupportsFeedback` and `kFlagSupportsRumble` are defined but not set by current `main` status callback.

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Run

```bash
sudo ./build/virtual_remote_server
```

`sudo` (or equivalent permissions) is usually required for `/dev/uinput`.

## Runtime Output

On successful start, server prints:
- `Server running:`
- `UDP control   : 9000`
- `UDP discovery : 9002`

On first accepted controller sender, stderr logs lock event:
- `[lock] locking to ip=<numeric_ipv4_value>`
