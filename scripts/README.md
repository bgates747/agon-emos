# EMOS tool index

Use the [current documentation](../docs/README.md) for resident and foreground
utility contracts. These tools have different scopes; a successful old review
does not qualify today's firmware or a physical board.

## Build and artifact preparation

| Tool | Scope |
|---|---|
| [prepare_boot_review.py](prepare_boot_review.py) | Identified firmware/ordinary boot-smoke bundle through repository build wrappers; no physical deployment |
| [prepare_sdserve.py](prepare_sdserve.py) | Identified **ordinary application** listener bundle, not the MOSlet layout. Follow the [listener build guide](../projects/sdserve/README.md#build-layouts) for the current `/emos/sdserve.bin` form |
| [verify_hardware_capture.py](verify_hardware_capture.py) | Validates recorded PORT-203 capture structure/artifact hashes; does not perform hardware tests or confer current acceptance |

The repository's `make firmware-check` and selected source profile remain the
firmware build boundary. Neither these helpers nor a raw compile override
identity, caller-admission, SD-placement or current qualification requirements.

## Retained emulator review helpers

| Tools | Purpose and limits |
|---|---|
| [review_boot.py](review_boot.py) | Isolated stock/EMOS boot profiles from a frozen draft bundle |
| [review_keyboard.py](review_keyboard.py) | Absent-peer keyboard-admission failure and mainboard recovery, not working browser input |
| [review_general_poll.py](review_general_poll.py), [review_uart_flow.py](review_uart_flow.py), [review_uart_probe.py](review_uart_probe.py), [review_visible_text.py](review_visible_text.py) | Bounded absent-peer diagnostic failure followed by a graphical review profile |
| [prepare_keyboard_api_review.py](prepare_keyboard_api_review.py), [keyboard_api_peer.py](keyboard_api_peer.py), [keyboard_wire_peer.py](keyboard_wire_peer.py) | Ordinary SD keyboard fixtures and controlled UART1 peers. Real resident EMOS effects execute; P4 firmware and physical timing do not |
| [prepare_browser_typing_review.py](prepare_browser_typing_review.py), [browser_typing_peer.py](browser_typing_peer.py) | Earlier SD typing sample with retained serializer bytes, not qualification of the current browser session/arbitration UI |
| [prepare_usb_cli_review.py](prepare_usb_cli_review.py), [usb_cli_peer.py](usb_cli_peer.py) | Ordinary CLI input using retained USB mapping/serializer output; no physical USB-host proof |
| [stage_emos_media.py](stage_emos_media.py) | Historical `.emo` provider media only. External provider loading was cancelled; this is **not** the foreground `/emos` utility installer |

Before reusing a review helper, inspect its pinned firmware/runtime, output
paths and owning task. Old `/bin` fixture and root result-file layouts need
refresh to the Extender SD-placement contract before physical use. Do not
confuse a prepared profile with completed human visual review. Helpers which
feed Fab CLI must preserve their bounded stdin/EOF handling; emulator timeout
and instruction results are not physical UART timing measurements.

This index classifies the existing tools; it does not change them, execute a
review, or establish that each historical launcher works with current inputs.
