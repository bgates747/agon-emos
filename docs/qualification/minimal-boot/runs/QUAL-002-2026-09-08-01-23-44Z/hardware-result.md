# Hardware result — ordinary EMOS smoke PASS

Recorded 2026-09-07 local / 2026-09-08 UTC. The Author supplied a photograph
of the physical display and reported:

> as far as i can tell this was the result of all three cold restarts. and yes, the original flash was successful.

This answers the preceding request for three cold boots and confirmation of
the successful flash/CRC/reset sequence. Flash success is operator-reported;
the supplied photograph shows the subsequent smoke, not the flasher output.

## Photograph observation

The display identifies `agon-emos-v0.2.0`, build
`agon-emos-v0.2.0-b2026-09-08-01-20-50Z`, status `candidate`, in both EMOS
status reports and identifies that same build in the smoke banner. Before and
after the program it shows Legacy, registry 0, generation 0, VDU route 0,
EDU inactive, adapter unavailable and mode generation 0. It visibly contains:

```text
BOOT SMOKE SD PASS
BOOT SMOKE CLOCK PASS
BOOT SMOKE PASS - returning to MOS
```

The final `/ *` MOS prompt and cursor are visible. No smoke failure appears.
The long identity line wraps onto the next display line, as in the accepted
emulator review. This is a manual reading of the conversation photograph;
it is not a serial capture or a newly generated image.

## Three-boot result

| Cold boot | Candidate identity | SD | Clock progress | Final PASS | Legacy / EDU inactive | Prompt return | Evidence |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | PASS | PASS | PASS | PASS | PASS | PASS | Author reports the pictured result on all three boots |
| 2 | PASS | PASS | PASS | PASS | PASS | PASS | Same operator report |
| 3 | PASS | PASS | PASS | PASS | PASS | PASS | Same operator report |

The photograph's individual boot number is unspecified; it is one supporting
image, not three separate captures. Raw image bytes were not available as a
local attachment, so no image filename or hash is fabricated. The photograph
remains conversation evidence; this tracked observation preserves its content.
Individual boot timestamps and measured startup durations were not supplied.

## Disposition and bounds

**PASS for the ordinary Legacy boot smoke**, based on the photograph and the
Author's report of all three cold boots and successful installation. EMOS
boots, provides onboard-VDP text output, loads and runs the ordinary SD
application, observes MOS-clock progress, retains Legacy/inactive-EDU state,
and returns to the prompt. Recovery was not needed.

The installation-command deviation remains explicit in the run manifest:
the operator corrected the invalid uppercase MOS argument and used a relative
payload filename. The candidate firmware and final smoke bytes were unchanged.
These runtime results do not validate the original installation script.
The photo does not show the onboard VDP version, GPIO-ribbon isolation,
individual boot durations or flash CRC text. Those limits do not become
invented measurements. This result does not establish measured 60 Hz timing,
full MOS compatibility, UART1/P4 operation or Exclusive Compatible mode.
Artifact status remains candidate pending the separate evidence/procedure
closeout; no version, registry revision or release claim changes here.
