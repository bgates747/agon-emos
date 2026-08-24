OUTPUT_FORMAT("elf32-z80")
ENTRY(_emos_provider_entry)

MEMORY
{
    MODULE (rx) : ORIGIN = 0x0B0080, LENGTH = 0x007F80
}

SECTIONS
{
    .entry :
    {
        KEEP(*(.entry))
    } > MODULE

    .text :
    {
        *(.text .text.*)
        *(.rodata .rodata.*)
    } > MODULE

    .data :
    {
        *(.data .data.*)
    } > MODULE

    .bss (NOLOAD) :
    {
        *(.bss .bss.*)
        *(COMMON)
    } > MODULE

    /DISCARD/ :
    {
        *(.comment)
        *(.note .note.*)
        *(.eh_frame .eh_frame.*)
        *(.gnu.attributes)
    }
}

ASSERT(ADDR(.entry) == ORIGIN(MODULE),
       "provider entry must be the first payload byte")
ASSERT(SIZEOF(.entry) > 0, "provider must define an entry section")
ASSERT(SIZEOF(.text) + SIZEOF(.entry) <= LENGTH(MODULE),
       "provider payload exceeds the EMOS module area")
ASSERT(SIZEOF(.data) == 0 && SIZEOF(.bss) == 0,
       "transient example provider must not require writable static storage")
