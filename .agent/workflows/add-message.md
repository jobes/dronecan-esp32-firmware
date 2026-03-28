---
description: Add a new DroneCAN message/service definition in the manual message system.
---

## Add Message Workflow

Use this workflow to add a new DroneCAN message or service to the shared `dronecan_mini` component.

### 1. Identify Message/Service (DSDL)
Identify the DroneCAN message or service you want to add (refer to [DroneCAN DSDL](https://github.com/DroneCAN/DSDL)).

### 2. Create Header & C File
Copy an existing one from `components/dronecan_mini/messages/` to follow the project's layout and style.

Example for a simple message:
- Header: Define the message structure and its `ID`.
- Source: Implement the `encode` and `decode` routines using `canard`.

### 3. Register with Receiver
If it's an incoming message or service request, register the handler in `components/dronecan_mini/dronecan_receiver.c`.

### 4. Rebuild & Test
Run a full clean build to ensure the new files are picked up by the ESP-IDF build system.

// turbo
```bash
cd firmwares/<target-firmware>
idf.py fullclean build
```
