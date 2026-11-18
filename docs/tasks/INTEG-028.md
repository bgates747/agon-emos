# Bounded UART0 CTS progress on the foreground SD PARLIO profile

Prepare an offline P084 EMOS candidate from the current production PARLIO
source. A busy receiver can defer UART0 without holding the eZ80 in its TX
interrupt. The RFC25 application ABI and production SD listener stay paired.

- INTEG-028-P084-01 [x] Port the retained CTS-edge deferral as the opt-in
  `EMOS_UART0_CTS_IRQ=1` composition for guarded RFC25 and foreground SD PARLIO.

- INTEG-028-P084-02 [ ] Verify the original ROM with the option off. Build and
  test the option-on image through the native wrapper and retain its exact pins.

- INTEG-028-P084-03 [ ] Return the candidate and compatibility limits to the
  FSIM owner. Physical deployment and promotion are separate decisions.

The TX IRQ owns burst copies and source release. On STOP it publishes pending,
masks TXIE, arms modem interrupts, acknowledges MSR, resamples PD3 and returns.
The modem IRQ and VBlank fallback only rearm. The owner restores the original
interrupt-enable state when checking pending, TXIE and TEMT together. Keep the
source through callback, the stream through TEMT and the staging identity
through the matching VDP receipt. A held peer supplies no completion deadline.
