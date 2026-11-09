# Robot Evens

Open-source snapshot of the Robot Evens desktop-robot project for M5Stack
StackChan/CoreS3. This repository is derived from the latest retained source
backup, created on 2026-10-07. Its firmware source identifies itself as
**V0.81**. It is source code only: it is not a statement that any particular
device has this version deployed.

The published snapshot deliberately excludes device flash dumps, device
settings snapshots, local configuration, credential references, and internal
project-management history.

The ESP32 is the hardware endpoint: it renders built-in eye expressions, moves the servos, controls RGB LEDs, and reports sensor state. The computer does complex AI/vision/audio work and sends high-level HTTP/JSON commands.

CoreS3's LTR-553ALS-WA proximity sensor is exposed through `/api/proximity/status` and `/api/proximity/set`. The firmware uses it as a low-priority "peek" attention layer: nearby people or objects can trigger short neutral glances with small servo motion, while emotion actions keep priority and normal idle behavior resumes whenever proximity attention is inactive.

Camera face presence is exposed through `/api/camera/status`, `/api/camera/set`, `/api/camera/catchphrase`, `/api/camera/face-report`, `/api/camera/wake`, and `/api/camera/clear`. The firmware uses the CoreS3 camera path for local face-presence detection when the camera driver is available. It captures from the current idle pose, returns to that snapshot pose for confirmation, then gently centers the face before playing the local catchphrase asset.

Set `ROBOT_LOCAL_FACE_CAMERA_AUTO_INIT` and the `ROBOT_LOCAL_FACE_CAMERA_PIN_*` values in `firmware/config.local.h` only when the camera driver is not initialized elsewhere. Face detection expects an RGB565 240x240 frame and uses ESP-WHO `HumanFaceDetectMSR01` + `HumanFaceDetectMNP01` when those headers are available.

## Layout

- `firmware/`: Arduino firmware
- `host/robot_client.py`: small Python HTTP client
- `host/console_server.py`: local computer-side browser console
- `host/web_console.html`: computer-side console page
- `ios/`: iPhone motion-control app and Swift package
- `tests/`: source-contract and host-client tests

## Local Wi-Fi Config

Create `firmware/config.local.h` from `firmware/config.example.h` and fill your local Wi-Fi credentials. This file is ignored by git.

## Standard Firmware Settings

The durable tuned defaults are captured in `firmware/standard_settings.json`.
It is generated from the latest device snapshot under `backups/` and excludes
Wi-Fi credentials, network diagnostics, sensor readings, battery readings,
cooldowns, camera detections, and uploaded runtime audio bytes.

Refresh the snapshot before changing the standard configuration:

```bash
python3 tools/capture_robot_settings.py --robot http://ROBOT_IP
```

## Build

```bash
python3 -B -m unittest discover -s tests
arduino-cli compile --fqbn esp32:esp32:m5stack-cores3 --build-path /private/tmp/robot-evens-build --build-property compiler.cpp.extra_flags=-std=gnu++17 firmware/
```

The V0.81 firmware in this published snapshot has been compiled successfully
with `esp32:esp32:m5stack-cores3`. Some historical Python tests are retained
for reference but are not currently green because the backup does not include
the old eye-glyph, voice-pack, and generator assets that those tests reference.

## Upload

```bash
arduino-cli upload --fqbn esp32:esp32:m5stack-cores3 --port /dev/cu.usbmodem1101 --build-path /private/tmp/robot-evens-build firmware/
```

After boot, keep the computer and robot on the same Wi-Fi network, then open the IP shown on the robot screen. The root page is a control console for eyes, motion, RGB, and sensor status.

The firmware also advertises `http://deskbot.local/` through mDNS when the network supports it. If mDNS does not resolve, use the numeric IP shown on the robot screen or printed in the serial log.

A fallback access point can be enabled manually by setting `ROBOT_AP_FALLBACK_ENABLED` to `1`, but it is disabled by default because normal control is designed to run through the shared router Wi-Fi.

## Computer Control Page

Run the local host action console on the computer, then open `http://127.0.0.1:8766/`:

```bash
python3 host/console_server.py --robot http://ROBOT_IP --port 8766
```

This console is for robot controls, camera controls, and realtime speech-to-action experiments. It proxies camera controls through local endpoints such as `/api/robot/camera/status`, `/api/robot/camera/set`, `/api/robot/camera/face-report`, `/api/robot/camera/wake`, and `/api/robot/camera/clear`.

For computer-side face detection, install the optional host vision dependencies and keep the MediaPipe BlazeFace model at `host/models/blaze_face_short_range.tflite`:

```bash
python3 -m pip install mediapipe opencv-python numpy
curl -L -o host/models/blaze_face_short_range.tflite https://storage.googleapis.com/mediapipe-models/face_detector/blaze_face_short_range/float16/latest/blaze_face_short_range.tflite
```

Open the console camera panel and use `Host 单步检测` for one computer-side frame check or `Host 开始追踪` for continuous tracking. Host vision reads `/api/camera/frame.bmp`, runs MediaPipe on the computer, then reports the normalized face box back through `/api/camera/face-report` so the firmware can lock and center the face. Leave `关闭固件本地检测` enabled when using this path.

## Robot Agent Scheduler

Run the host console to expose local workflow routes:

```bash
python3 host/console_server.py --robot http://ROBOT_IP --port 8766
```

Useful workflow endpoints:

```bash
curl http://127.0.0.1:8766/api/agent/status
curl -X POST http://127.0.0.1:8766/api/agent/invoke-safe-behavior -H 'Content-Type: application/json' -d '{"action_id":"behavior.emotion.thinking"}'
curl -X POST http://127.0.0.1:8766/api/agent/face-detect-and-center -H 'Content-Type: application/json' -d '{"max_steps":3}'
curl -X POST http://127.0.0.1:8766/api/agent/coding-event -H 'Content-Type: application/json' -d '{"type":"task_done","count":1}'
```

Run the MCP stdio server for Claude Code or another MCP client:

```bash
python3 -m host.agent_mcp_server --robot http://ROBOT_IP
```

The first MCP tool set is intentionally small and safe: status, catalog, safe
asset invocation, bounded pose movement, one-step face detection, bounded face
centering, greeting, and coding events.

## Today Coding Status Display

Run the local assistant status server on the computer:

```bash
python3 host/assistant_status_server.py --port 8770
```

The server exposes `GET /api/assistant/status` with date/time, today's coding
duration, Codex remaining percent when available, completed task count, and a
short-lived event prompt for the robot display. It stores local daily state in
`host/assistant_status.local.json`, which is ignored by git.

Useful local updates:

```bash
curl -X POST http://127.0.0.1:8770/api/assistant/tasks/done -H 'Content-Type: application/json' -d '{"count":1}'
curl -X POST http://127.0.0.1:8770/api/assistant/events -H 'Content-Type: application/json' -d '{"type":"task_failed"}'
curl -X POST http://127.0.0.1:8770/api/assistant/events -H 'Content-Type: application/json' -d '{"type":"waiting"}'
curl -X POST http://127.0.0.1:8770/api/assistant/events -H 'Content-Type: application/json' -d '{"type":"test_ok"}'
curl -X POST http://127.0.0.1:8770/api/assistant/events/clear -H 'Content-Type: application/json' -d '{}'
curl -X POST http://127.0.0.1:8770/api/assistant/settings -H 'Content-Type: application/json' -d '{"eventTtlSeconds":12,"lowCodexPercent":20}'
curl -X POST http://127.0.0.1:8770/api/assistant/codex/remaining -H 'Content-Type: application/json' -d '{"remainingPercent":64}'
curl -X POST http://127.0.0.1:8770/api/assistant/codex/remaining -H 'Content-Type: application/json' -d '{"available":false}'
```

Point the robot at the host status endpoint after the firmware is running:

```bash
curl -X POST http://ROBOT_IP/api/assistant-display/set -H 'Content-Type: application/json' -d '{"enabled":true,"assistantStatusUrl":"http://COMPUTER_LAN_IP:8770/api/assistant/status","pollIntervalMs":5000}'
```

The robot cycles compact bottom text for date/time, coding duration, Codex
remaining percent, completed tasks, and battery. Active event prompts such as
`TASK DONE +1`, `TASK FAILED`, `WAIT CONFIRM`, `TEST OK`, and `BUILD FAIL`
briefly override the normal rotation. If Codex remaining percent is unknown, the
robot displays `CODEX --%`.

## iPhone Motion Action App

The native iOS MVP lives in `ios/RobotEvensControl/`. Open `ios/RobotEvensControl/RobotEvensControl.xcodeproj` in Xcode, select the `RobotEvensControl` target, set your Apple signing team, and run it on an iPhone connected to the same Wi-Fi as the robot.

The app uses manual robot addressing in this first version. Enter `http://ROBOT_IP` or `http://deskbot.local`, tap connect, tap calibrate while holding the phone in the neutral pose, then enable live control. Phone roll maps to robot yaw and phone pitch maps to robot pitch. Use stop/home if the pose needs to return to rest.

Action recording is local to the iPhone in this MVP. Pick one start sound asset, tap record, move the phone to capture servo keyframes, press expression buttons to add face events, then save and preview. Saved actions use compact JSON and reference sound IDs; audio is not embedded in the action file.

Core logic is tested as a Swift package:

```bash
cd ios/RobotEvensCore
swift test
```

Command-line simulator builds can use the target directly:

```bash
xcodebuild -project ios/RobotEvensControl/RobotEvensControl.xcodeproj -target RobotEvensControl -sdk iphonesimulator CODE_SIGNING_ALLOWED=NO build
```

## Realtime Gesture Action Router

The speech-to-action integration runs on the computer, not on the ESP32. In this control path, the robot does not play model-generated speech. The Realtime session chooses a constrained `robot_action`, and the host validates that action before sending normal HTTP commands to the robot.

Install optional live dependencies on the host computer:

```bash
python3 -m pip install websocket-client sounddevice
```

Set the API key and robot URL:

```bash
export OPENAI_API_KEY="sk-..."
export ROBOT_BASE_URL="http://ROBOT_IP"
```

Dry-run a known action through the router:

```bash
python3 -m host.realtime_robot --robot "$ROBOT_BASE_URL" --context-json '{"known_user":"owner"}' dry-action --action-json '{"action_id":"nod_yes","intensity":0.8}'
```

Run a text smoke test through the Realtime API before enabling the microphone:

```bash
python3 -m host.realtime_robot --robot "$ROBOT_BASE_URL" --context-json '{"known_user":"owner"}' text-smoke "你认识我吗？"
```

Local face presence is firmware-owned when camera support is available. If the camera or inference headers are missing, the same camera state machine can still be tested through `/api/camera/face-report`.

## Voice API Settings Console

The old realtime voice runtime has been removed. The dedicated voice console now only keeps API settings and basic robot/listening status while the voice base is redesigned:

```bash
python3 host/voice_console_server.py --robot http://ROBOT_IP --port 8767
```

Open `http://127.0.0.1:8767/`. API keys stay on the computer; no robot microphone gateway, browser microphone stream, or realtime TTS playback route is started by this console.

## Host Client Examples

```bash
python3 host/robot_client.py --host http://ROBOT_IP status
python3 host/robot_client.py --host http://ROBOT_IP face happy --color '#00f2ff'
python3 host/robot_client.py --host http://ROBOT_IP react --look-x 0.3 --look-y -0.2 --audio 0.5
python3 host/robot_client.py --host http://ROBOT_IP pose --yaw 0 --pitch 120 --speed 300
python3 host/robot_client.py --host http://ROBOT_IP rotate 0
python3 host/robot_client.py --host http://ROBOT_IP camera-status
python3 host/robot_client.py --host http://ROBOT_IP camera-set --enabled 1 --capture-interval-ms 12000 --detect-timeout-ms 1500
python3 host/robot_client.py --host http://ROBOT_IP camera-catchphrase --asset-id halo --listen-after 1
python3 host/robot_client.py --host http://ROBOT_IP camera-face-report --scan-id 1 --detected 1 --confidence 0.86 --face-center-x 0.55 --face-center-y 0.48
python3 host/robot_client.py --host http://ROBOT_IP camera-wake
curl http://ROBOT_IP/api/proximity/status
curl http://ROBOT_IP/api/camera/status
curl -X POST http://ROBOT_IP/api/camera/catchphrase -H 'Content-Type: application/json' -d '{"assetId":"halo","listenAfter":true}'
curl -X POST http://ROBOT_IP/api/camera/face-report -H 'Content-Type: application/json' -d '{"scanId":1,"faceDetected":true,"confidence":0.86,"faceCenterX":0.55,"faceCenterY":0.48,"faceWidth":0.24,"faceHeight":0.32}'
curl -X POST http://ROBOT_IP/api/camera/wake -H 'Content-Type: application/json' -d '{"source":"voice"}'
```

## First Built-In Eye Moods

`neutral`, `happy`, `angry`, `sad`, `sleeping`, `tired`, `surprised`, `love`, `dizzy`, `wink`, `dead`, `thinking`, `curious`, `loading`.

## License

MIT. See [LICENSE](LICENSE).
