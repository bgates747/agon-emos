# Added official MOS and previous full OTIRX comparison

The Author requested two more cases using the same fixture and retaining the
three completed Eirene cases in one package. Both additions are now qualified
and deployed in `/test/mos-uart0` on Console8's AGONC8 card, UUID8658-1C23.
No firmware was flashed by the agent. Only the two new physical cases remain.

XC-001, XC-002 and XC-003 are complete. Previous Eirene firmware, test.bin,
source.json, R0042..R0044, runs.log and next.txt remain byte-for-byte unchanged.
The package has32 files, including seven additions. Only readme.txt and
SHA256SUMS were replaced. previous.txt preserves the earlier instructions.
The next report remains R0045; the second new run should produce R0046.

## What to run

Keep the matched receiver-fixed VDP used for the first three cases. If it has
been replaced, the staged vdp-test.bin is the same required image. Use the
installed standard flash MOSlet and its normal checks. After each flash/reboot,
return to `/test/mos-uart0` and load the named app afresh.

Official stock MOS3.0.2:

```
flash mos mos-stock.bin
load stock.bin
run . 0
```

Previous full OTIRX candidate, borrowed/direct callback, batch16, copying OFF:

```
flash mos mos-ours.bin
load ours.bin
run . 2
```

Each runs three bulk and three paced trials with all8192payloadbytes checked
after each trial. Allow two minutes per run, require PERF1 PASS and return to
the prompt. The card's short readme contains the same instructions. Do not rerun
the old Eirene cases merely to populate this extended set; their data is retained.

## Exact inputs and comparison compatibility

| Added case | App build | Firmware bytes | Firmware SHA256 |
|---|---|---:|---|
|Official MOS3.0.2|2080fb37153fbf9a|108490|d564243283972690933a4554296ad6202ca4ef54572279533a942960846bebae|
|Previous full OTIRX|aa2a7706b769fdd7|112777|cb1d0f4e066395533a2f07c7454246fa3a1ed46134bf155b883bb8c6e8353128|

App SHA256s: stock.bin9ddfd9a30219cb0713bec304a2f4b5e2883e5073dd801d79c2970f06b3ab1cc6;
ours.bind99ba82ea227e3852fb0a57a3a7a0744b66fb0ebc7ff20571fd81fba60d33631.
Both MOS images were reused from the prior qualified artifact set; neither was
rebuilt or migrated to another compiler. The previous full candidate is the
ZDS II/Wine UX-001 firmware used by the earlier hardware balls demo. It uses
UTX1 borrowed/direct callbacks at batch16; this fixture does not activate copied
RST18. Its API adapter/discovery/configuration/close costs are included where
they occur during the measured write/finish path. Discovery is untimed.

The apps are built from the frozen PERF1 snapshot used for R0042..R0044.
All timed C helpers, source/payload construction, pacing, observation window,
receiver comparator and PRT code are unchanged. Only untimed identity/UI/case
checks and the old-API bridge differ. The compiler-generated1253-byte calculation
kernel is identical to the original in both added apps, SHA256
0ef1a25d91a263c567c20e10b104c68211f36d72740ba0e0c4e94438d3ebfcf6.
Linked addresses differ; source equality alone was not treated as machine-code
identity. Calibration remains per-run. This compares complete callable transport
paths, not just the OTIRX instruction in isolation.

A per-app selection guard refuses other mode arguments before any experimental
API call. It lives in a separate translation unit so the compiler cannot use it
to specialize the measured benchmark. Installed-ROM prefix checks remain active.

## Qualification, history and joint analysis

The exact deployed apps passed stock/full-candidate ×JIT/interpreter: four finite
actual-VDP-firmware Flexe cells,24timed trials and196608verifiedreceiverbytes.
Independent analysis checked full emitted payload/comparator streams, work
checksums, callbacks, timer agreement, ROM identities and zero backend/UART
faults. No physical result or speedup is claimed for these two added cases yet.

First extra001 build failed because C and assembly shared an object basename;
fresh snapshots use distinct names. Preliminary extra003/extra-flexe001 then
passed four cells. Review added the wrong-mode guard; a compiler-specialized
extra004 was built/staged but never executed/deployed. Final extra005 moves that
guard outside the measured translation unit and passed extra-flexe003. All
attempts remain in workspace history. Both Oryx leases224/225 were released
with fresh empty-host evidence; no next request waited. No lease is held.

Source/tooling commit1112c2a. Build inputs/outputs: `build/tm001/extra005`.
Raw final qualification: `build/tm001/extra-flexe003` on Pi and Oryx.
Deployment, prepared complete package and verified pre-write backup:
`build/tm001/extra-deploy001`. Pre-write archive SHA256
052369313b836cc6b399dedfd7c0286164bad0bf2ad272b313f7e5d4d3a5b32f.
The JSON companion contains deployment/readback hashes and full qualification.

cases.json explicitly maps all five case names, build IDs, mode arguments,
application hashes, source manifests and firmware pins. On returned-card
inspection, first archive/copy the complete appliance to the workspace, then:

```
python3 experiments/uart0-performance/extra/combined.py ARCHIVED_DIRECTORY --kind hardware
```

It requires all five registered cases and uses official MOS3.0.2 as the percentage
reference. A complete five-case MODEL dataset exercised this combined parser;
it is labeled emulator and is not substituted for the pending physical cases.
The original three-case analyzer/reports remain available without reinterpretation.
