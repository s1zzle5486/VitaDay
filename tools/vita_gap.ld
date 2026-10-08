/* vita-elf-create appends SCE module data to the RX segment.
   Leave at least one full page before the first RW section. */
SECTIONS
{
  . = ALIGN(0x1000) + 0x1000;
}
INSERT BEFORE .init_array;
