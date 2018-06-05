/* Flash Split for 4M chips */
/* sketch 1019KB */
/* empty  1024KB */
/* spiffs 2028KB */
/* eeprom 20KB */

/* MB: Copied from https://raw.githubusercontent.com/esp8266/Arduino/master/tools/sdk/ld/eagle.flash.4m2m.ld
 * All my test ESP modules have this, but would be nice to try to get 1MB size to work also
 * (I will conceded 512K is too small considering just core ESP8266 firmware with few options
 *  was around ~300K last I checked, and mbedtls is reasonable to be larger than a few 100k)
 */

MEMORY
{
  dport0_0_seg :                        org = 0x3FF00000, len = 0x10
  dram0_0_seg :                         org = 0x3FFE8000, len = 0x14000
  iram1_0_seg :                         org = 0x40100000, len = 0x8000
  irom0_0_seg :                         org = 0x40201010, len = 0xfeff0
}

PROVIDE ( _SPIFFS_start = 0x40400000 );
PROVIDE ( _SPIFFS_end = 0x405FB000 );
PROVIDE ( _SPIFFS_page = 0x100 );
PROVIDE ( _SPIFFS_block = 0x2000 );

INCLUDE "../ld/eagle.app.v6.common.ld"