ENTRY(_start)

MEMORY
{
  APP (rx) : ORIGIN = 0x040000, LENGTH = 0x010000
}

SECTIONS
{
  .text :
  {
    *(.text*)
  } > APP

  /DISCARD/ :
  {
    *(.data*)
    *(.bss*)
  }
}

ASSERT(_start == ORIGIN(APP), "PORT-008 fixture entry moved")
ASSERT(_port008_fixture_end - _port008_fixture_bytes == 106,
       "PORT-008 fixture byte count changed")
