
/*
 * Use at your own risk
 *
 * Uses modified comm routines from Jacek Lipkowski <sq5bpf@lipkowski.org>
 *
 * This program is licensed under the GNU GENERAL PUBLIC LICENSE v3
 * License text avaliable at: http://www.gnu.org/copyleft/gpl.html
 */

/*
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <vcl.h>
#include <inifiles.hpp>

#include <stdio.h>
//#incluve <limits.h>

//#include <stdlib.h>     // srand, rand
//#include <time.h>       // time

#pragma hdrstop

#include "Unit1.h"

#pragma package(smart_init)
#pragma link "CGAUGES"
#pragma resource "*.dfm"

TForm1 *Form1 = NULL;

typedef struct
{
	uint16_t MajorVer;
	uint16_t MinorVer;
	uint16_t ReleaseVer;
	uint16_t BuildVer;
} TVersion;

// ************************************************************************

#define MODE_NONE                            0            //
#define MODE_READ                            1            //
#define MODE_WRITE                           2            //
#define MODE_WRITE_MOST                      3            //
#define MODE_WRITE_ALL                       4            //
#define MODE_FLASH_DEBUG                     5            //
#define MODE_FLASH                           6            //

#define UVK5_PREPARE_TRIES                   4            //

#define UVK5_EEPROM_SIZE                     0x00001d00   // 7424
#define UVK5_MAX_EEPROM_SIZE                 0x00002000   // 8192    the radios calibration data is in 1D00 to 2000
#define UVK5_EEPROM_BLOCKSIZE                128          //

#define UVK5_FLASH_SIZE                      0x0000f000   // 61440
#define UVK5_MAX_FLASH_SIZE                  0x00010000   // 65536   the bootloader is in F000 to FFFF
#define UVK5_FLASH_BLOCKSIZE                 256          //

#define DEFAULT_SERIAL_SPEED                 38400        //
//#define DEFAULT_FILE_NAME                    "k5_eeprom.raw"
//#define DEFAULT_FLASH_NAME                   "k5_flash.raw"

static const int uvk5_writesx[][2] =
{
//	{ 0x0e70, 0x60 },
	{ 0x0eb0, 0x08 },
	{ 0,      0}
};

static const int uvk5_writes[][2] =
{
	{ 0x0e70, 0x60 },
	{ 0x0000, 0x10 },
	{ 0x0f50, 0x10 },
	{ 0x0010, 0x10 },
	{ 0x0f60, 0x10 },
	{ 0x0020, 0x10 },
	{ 0x0f70, 0x10 },
	{ 0x0030, 0x10 },
	{ 0x0f80, 0x10 },
	{ 0x0040, 0x10 },
	{ 0x0f90, 0x10 },
	{ 0x0050, 0x10 },
	{ 0x0fa0, 0x10 },
	{ 0x0060, 0x10 },
	{ 0x0fb0, 0x10 },
	{ 0x0070, 0x10 },
	{ 0x0fc0, 0x10 },
	{ 0x0080, 0x10 },
	{ 0x0fd0, 0x10 },
	{ 0x0090, 0x10 },
	{ 0x0fe0, 0x10 },
	{ 0x00a0, 0x10 },
	{ 0x0ff0, 0x10 },
	{ 0x00b0, 0x10 },
	{ 0x1000, 0x10 },
	{ 0x00c0, 0x10 },
	{ 0x1010, 0x10 },
	{ 0x00d0, 0x10 },
	{ 0x1020, 0x10 },
	{ 0x00e0, 0x10 },
	{ 0x1030, 0x10 },
	{ 0x00f0, 0x10 },
	{ 0x1040, 0x10 },
	{ 0x0100, 0x10 },
	{ 0x1050, 0x10 },
	{ 0x0110, 0x10 },
	{ 0x1060, 0x10 },
	{ 0x0120, 0x10 },
	{ 0x1070, 0x10 },
	{ 0x0130, 0x10 },
	{ 0x1080, 0x10 },
	{ 0x0140, 0x10 },
	{ 0x1090, 0x10 },
	{ 0x0150, 0x10 },
	{ 0x10a0, 0x10 },
	{ 0x0160, 0x10 },
	{ 0x10b0, 0x10 },
	{ 0x0170, 0x10 },
	{ 0x10c0, 0x10 },
	{ 0x0180, 0x10 },
	{ 0x10d0, 0x10 },
	{ 0x0190, 0x10 },
	{ 0x10e0, 0x10 },
	{ 0x01a0, 0x10 },
	{ 0x10f0, 0x10 },
	{ 0x01b0, 0x10 },
	{ 0x1100, 0x10 },
	{ 0x01c0, 0x10 },
	{ 0x1110, 0x10 },
	{ 0x01d0, 0x10 },
	{ 0x1120, 0x10 },
	{ 0x01e0, 0x10 },
	{ 0x1130, 0x10 },
	{ 0x01f0, 0x10 },
	{ 0x1140, 0x10 },
	{ 0x0200, 0x10 },
	{ 0x1150, 0x10 },
	{ 0x0210, 0x10 },
	{ 0x1160, 0x10 },
	{ 0x0220, 0x10 },
	{ 0x1170, 0x10 },
	{ 0x0230, 0x10 },
	{ 0x1180, 0x10 },
	{ 0x0240, 0x10 },
	{ 0x1190, 0x10 },
	{ 0x0250, 0x10 },
	{ 0x11a0, 0x10 },
	{ 0x0260, 0x10 },
	{ 0x11b0, 0x10 },
	{ 0x0270, 0x10 },
	{ 0x11c0, 0x10 },
	{ 0x0280, 0x10 },
	{ 0x11d0, 0x10 },
	{ 0x0290, 0x10 },
	{ 0x11e0, 0x10 },
	{ 0x02a0, 0x10 },
	{ 0x11f0, 0x10 },
	{ 0x02b0, 0x10 },
	{ 0x1200, 0x10 },
	{ 0x02c0, 0x10 },
	{ 0x1210, 0x10 },
	{ 0x02d0, 0x10 },
	{ 0x1220, 0x10 },
	{ 0x02e0, 0x10 },
	{ 0x1230, 0x10 },
	{ 0x02f0, 0x10 },
	{ 0x1240, 0x10 },
	{ 0x0300, 0x10 },
	{ 0x1250, 0x10 },
	{ 0x0310, 0x10 },
	{ 0x1260, 0x10 },
	{ 0x0320, 0x10 },
	{ 0x1270, 0x10 },
	{ 0x0330, 0x10 },
	{ 0x1280, 0x10 },
	{ 0x0340, 0x10 },
	{ 0x1290, 0x10 },
	{ 0x0350, 0x10 },
	{ 0x12a0, 0x10 },
	{ 0x0360, 0x10 },
	{ 0x12b0, 0x10 },
	{ 0x0370, 0x10 },
	{ 0x12c0, 0x10 },
	{ 0x0380, 0x10 },
	{ 0x12d0, 0x10 },
	{ 0x0390, 0x10 },
	{ 0x12e0, 0x10 },
	{ 0x03a0, 0x10 },
	{ 0x12f0, 0x10 },
	{ 0x03b0, 0x10 },
	{ 0x1300, 0x10 },
	{ 0x03c0, 0x10 },
	{ 0x1310, 0x10 },
	{ 0x03d0, 0x10 },
	{ 0x1320, 0x10 },
	{ 0x03e0, 0x10 },
	{ 0x1330, 0x10 },
	{ 0x03f0, 0x10 },
	{ 0x1340, 0x10 },
	{ 0x0400, 0x10 },
	{ 0x1350, 0x10 },
	{ 0x0410, 0x10 },
	{ 0x1360, 0x10 },
	{ 0x0420, 0x10 },
	{ 0x1370, 0x10 },
	{ 0x0430, 0x10 },
	{ 0x1380, 0x10 },
	{ 0x0440, 0x10 },
	{ 0x1390, 0x10 },
	{ 0x0450, 0x10 },
	{ 0x13a0, 0x10 },
	{ 0x0460, 0x10 },
	{ 0x13b0, 0x10 },
	{ 0x0470, 0x10 },
	{ 0x13c0, 0x10 },
	{ 0x0480, 0x10 },
	{ 0x13d0, 0x10 },
	{ 0x0490, 0x10 },
	{ 0x13e0, 0x10 },
	{ 0x04a0, 0x10 },
	{ 0x13f0, 0x10 },
	{ 0x04b0, 0x10 },
	{ 0x1400, 0x10 },
	{ 0x04c0, 0x10 },
	{ 0x1410, 0x10 },
	{ 0x04d0, 0x10 },
	{ 0x1420, 0x10 },
	{ 0x04e0, 0x10 },
	{ 0x1430, 0x10 },
	{ 0x04f0, 0x10 },
	{ 0x1440, 0x10 },
	{ 0x0500, 0x10 },
	{ 0x1450, 0x10 },
	{ 0x0510, 0x10 },
	{ 0x1460, 0x10 },
	{ 0x0520, 0x10 },
	{ 0x1470, 0x10 },
	{ 0x0530, 0x10 },
	{ 0x1480, 0x10 },
	{ 0x0540, 0x10 },
	{ 0x1490, 0x10 },
	{ 0x0550, 0x10 },
	{ 0x14a0, 0x10 },
	{ 0x0560, 0x10 },
	{ 0x14b0, 0x10 },
	{ 0x0570, 0x10 },
	{ 0x14c0, 0x10 },
	{ 0x0580, 0x10 },
	{ 0x14d0, 0x10 },
	{ 0x0590, 0x10 },
	{ 0x14e0, 0x10 },
	{ 0x05a0, 0x10 },
	{ 0x14f0, 0x10 },
	{ 0x05b0, 0x10 },
	{ 0x1500, 0x10 },
	{ 0x05c0, 0x10 },
	{ 0x1510, 0x10 },
	{ 0x05d0, 0x10 },
	{ 0x1520, 0x10 },
	{ 0x05e0, 0x10 },
	{ 0x1530, 0x10 },
	{ 0x05f0, 0x10 },
	{ 0x1540, 0x10 },
	{ 0x0600, 0x10 },
	{ 0x1550, 0x10 },
	{ 0x0610, 0x10 },
	{ 0x1560, 0x10 },
	{ 0x0620, 0x10 },
	{ 0x1570, 0x10 },
	{ 0x0630, 0x10 },
	{ 0x1580, 0x10 },
	{ 0x0640, 0x10 },
	{ 0x1590, 0x10 },
	{ 0x0650, 0x10 },
	{ 0x15a0, 0x10 },
	{ 0x0660, 0x10 },
	{ 0x15b0, 0x10 },
	{ 0x0670, 0x10 },
	{ 0x15c0, 0x10 },
	{ 0x0680, 0x10 },
	{ 0x15d0, 0x10 },
	{ 0x0690, 0x10 },
	{ 0x15e0, 0x10 },
	{ 0x06a0, 0x10 },
	{ 0x15f0, 0x10 },
	{ 0x06b0, 0x10 },
	{ 0x1600, 0x10 },
	{ 0x06c0, 0x10 },
	{ 0x1610, 0x10 },
	{ 0x06d0, 0x10 },
	{ 0x1620, 0x10 },
	{ 0x06e0, 0x10 },
	{ 0x1630, 0x10 },
	{ 0x06f0, 0x10 },
	{ 0x1640, 0x10 },
	{ 0x0700, 0x10 },
	{ 0x1650, 0x10 },
	{ 0x0710, 0x10 },
	{ 0x1660, 0x10 },
	{ 0x0720, 0x10 },
	{ 0x1670, 0x10 },
	{ 0x0730, 0x10 },
	{ 0x1680, 0x10 },
	{ 0x0740, 0x10 },
	{ 0x1690, 0x10 },
	{ 0x0750, 0x10 },
	{ 0x16a0, 0x10 },
	{ 0x0760, 0x10 },
	{ 0x16b0, 0x10 },
	{ 0x0770, 0x10 },
	{ 0x16c0, 0x10 },
	{ 0x0780, 0x10 },
	{ 0x16d0, 0x10 },
	{ 0x0790, 0x10 },
	{ 0x16e0, 0x10 },
	{ 0x07a0, 0x10 },
	{ 0x16f0, 0x10 },
	{ 0x07b0, 0x10 },
	{ 0x1700, 0x10 },
	{ 0x07c0, 0x10 },
	{ 0x1710, 0x10 },
	{ 0x07d0, 0x10 },
	{ 0x1720, 0x10 },
	{ 0x07e0, 0x10 },
	{ 0x1730, 0x10 },
	{ 0x07f0, 0x10 },
	{ 0x1740, 0x10 },
	{ 0x0800, 0x10 },
	{ 0x1750, 0x10 },
	{ 0x0810, 0x10 },
	{ 0x1760, 0x10 },
	{ 0x0820, 0x10 },
	{ 0x1770, 0x10 },
	{ 0x0830, 0x10 },
	{ 0x1780, 0x10 },
	{ 0x0840, 0x10 },
	{ 0x1790, 0x10 },
	{ 0x0850, 0x10 },
	{ 0x17a0, 0x10 },
	{ 0x0860, 0x10 },
	{ 0x17b0, 0x10 },
	{ 0x0870, 0x10 },
	{ 0x17c0, 0x10 },
	{ 0x0880, 0x10 },
	{ 0x17d0, 0x10 },
	{ 0x0890, 0x10 },
	{ 0x17e0, 0x10 },
	{ 0x08a0, 0x10 },
	{ 0x17f0, 0x10 },
	{ 0x08b0, 0x10 },
	{ 0x1800, 0x10 },
	{ 0x08c0, 0x10 },
	{ 0x1810, 0x10 },
	{ 0x08d0, 0x10 },
	{ 0x1820, 0x10 },
	{ 0x08e0, 0x10 },
	{ 0x1830, 0x10 },
	{ 0x08f0, 0x10 },
	{ 0x1840, 0x10 },
	{ 0x0900, 0x10 },
	{ 0x1850, 0x10 },
	{ 0x0910, 0x10 },
	{ 0x1860, 0x10 },
	{ 0x0920, 0x10 },
	{ 0x1870, 0x10 },
	{ 0x0930, 0x10 },
	{ 0x1880, 0x10 },
	{ 0x0940, 0x10 },
	{ 0x1890, 0x10 },
	{ 0x0950, 0x10 },
	{ 0x18a0, 0x10 },
	{ 0x0960, 0x10 },
	{ 0x18b0, 0x10 },
	{ 0x0970, 0x10 },
	{ 0x18c0, 0x10 },
	{ 0x0980, 0x10 },
	{ 0x18d0, 0x10 },
	{ 0x0990, 0x10 },
	{ 0x18e0, 0x10 },
	{ 0x09a0, 0x10 },
	{ 0x18f0, 0x10 },
	{ 0x09b0, 0x10 },
	{ 0x1900, 0x10 },
	{ 0x09c0, 0x10 },
	{ 0x1910, 0x10 },
	{ 0x09d0, 0x10 },
	{ 0x1920, 0x10 },
	{ 0x09e0, 0x10 },
	{ 0x1930, 0x10 },
	{ 0x09f0, 0x10 },
	{ 0x1940, 0x10 },
	{ 0x0a00, 0x10 },
	{ 0x1950, 0x10 },
	{ 0x0a10, 0x10 },
	{ 0x1960, 0x10 },
	{ 0x0a20, 0x10 },
	{ 0x1970, 0x10 },
	{ 0x0a30, 0x10 },
	{ 0x1980, 0x10 },
	{ 0x0a40, 0x10 },
	{ 0x1990, 0x10 },
	{ 0x0a50, 0x10 },
	{ 0x19a0, 0x10 },
	{ 0x0a60, 0x10 },
	{ 0x19b0, 0x10 },
	{ 0x0a70, 0x10 },
	{ 0x19c0, 0x10 },
	{ 0x0a80, 0x10 },
	{ 0x19d0, 0x10 },
	{ 0x0a90, 0x10 },
	{ 0x19e0, 0x10 },
	{ 0x0aa0, 0x10 },
	{ 0x19f0, 0x10 },
	{ 0x0ab0, 0x10 },
	{ 0x1a00, 0x10 },
	{ 0x0ac0, 0x10 },
	{ 0x1a10, 0x10 },
	{ 0x0ad0, 0x10 },
	{ 0x1a20, 0x10 },
	{ 0x0ae0, 0x10 },
	{ 0x1a30, 0x10 },
	{ 0x0af0, 0x10 },
	{ 0x1a40, 0x10 },
	{ 0x0b00, 0x10 },
	{ 0x1a50, 0x10 },
	{ 0x0b10, 0x10 },
	{ 0x1a60, 0x10 },
	{ 0x0b20, 0x10 },
	{ 0x1a70, 0x10 },
	{ 0x0b30, 0x10 },
	{ 0x1a80, 0x10 },
	{ 0x0b40, 0x10 },
	{ 0x1a90, 0x10 },
	{ 0x0b50, 0x10 },
	{ 0x1aa0, 0x10 },
	{ 0x0b60, 0x10 },
	{ 0x1ab0, 0x10 },
	{ 0x0b70, 0x10 },
	{ 0x1ac0, 0x10 },
	{ 0x0b80, 0x10 },
	{ 0x1ad0, 0x10 },
	{ 0x0b90, 0x10 },
	{ 0x1ae0, 0x10 },
	{ 0x0ba0, 0x10 },
	{ 0x1af0, 0x10 },
	{ 0x0bb0, 0x10 },
	{ 0x1b00, 0x10 },
	{ 0x0bc0, 0x10 },
	{ 0x1b10, 0x10 },
	{ 0x0bd0, 0x10 },
	{ 0x1b20, 0x10 },
	{ 0x0be0, 0x10 },
	{ 0x1b30, 0x10 },
	{ 0x0bf0, 0x10 },
	{ 0x1b40, 0x10 },
	{ 0x0c00, 0x10 },
	{ 0x1b50, 0x10 },
	{ 0x0c10, 0x10 },
	{ 0x1b60, 0x10 },
	{ 0x0c20, 0x10 },
	{ 0x1b70, 0x10 },
	{ 0x0c30, 0x10 },
	{ 0x1b80, 0x10 },
	{ 0x0c40, 0x10 },
	{ 0x1b90, 0x10 },
	{ 0x0c50, 0x10 },
	{ 0x1ba0, 0x10 },
	{ 0x0c60, 0x10 },
	{ 0x1bb0, 0x10 },
	{ 0x0c70, 0x10 },
	{ 0x1bc0, 0x10 },
	{ 0x0c80, 0x10 },
	{ 0x0c90, 0x10 },
	{ 0x0ca0, 0x10 },
	{ 0x0cb0, 0x10 },
	{ 0x0cc0, 0x10 },
	{ 0x0cd0, 0x10 },
	{ 0x0ce0, 0x10 },
	{ 0x0cf0, 0x10 },
	{ 0x0d00, 0x10 },
	{ 0x0d10, 0x10 },
	{ 0x0d20, 0x10 },
	{ 0x0d30, 0x10 },
	{ 0x0d40, 0x10 },
	{ 0x0d50, 0x10 },
	{ 0x0d60, 0x80 },
	{ 0x0de0, 0x50 },
	{ 0x0e40, 0x28 },
	{ 0x0ed0, 0x48 },
	{ 0x1c00, 0x80 },
	{ 0x1c80, 0x80 },
	{ 0x0f18, 0x08 },
	{ 0,      0}
};
/*
static const uint16_t CRC16_TABLE[] =
{
	0, 4129, 8258, 12387, 16516, 20645, 24774, 28903, 33032, 37161, 41290, 45419, 49548, 53677, 57806, 61935, 4657, 528, 12915, 8786, 21173, 17044, 29431, 25302,
	37689, 33560, 45947, 41818, 54205, 50076, 62463, 58334, 9314, 13379, 1056, 5121, 25830, 29895, 17572, 21637, 42346, 46411, 34088, 38153, 58862, 62927, 50604, 54669, 13907,
	9842, 5649, 1584, 30423, 26358, 22165, 18100, 46939, 42874, 38681, 34616, 63455, 59390, 55197, 51132, 18628, 22757, 26758, 30887, 2112, 6241, 10242, 14371, 51660, 55789,
	59790, 63919, 35144, 39273, 43274, 47403, 23285, 19156, 31415, 27286, 6769, 2640,14899, 10770, 56317, 52188, 64447, 60318, 39801, 35672, 47931, 43802, 27814, 31879,
	19684, 23749, 11298, 15363, 3168, 7233, 60846, 64911, 52716, 56781, 44330, 48395,36200, 40265, 32407, 28342, 24277, 20212, 15891, 11826, 7761, 3696, 65439, 61374,
	57309, 53244, 48923, 44858, 40793, 36728, 37256, 33193, 45514, 41451, 53516, 49453, 61774, 57711, 4224, 161, 12482, 8419, 20484, 16421, 28742, 24679, 33721, 37784, 41979,
	46042, 49981, 54044, 58239, 62302, 689, 4752, 8947, 13010, 16949, 21012, 25207, 29270, 46570, 42443, 38312, 34185, 62830, 58703, 54572, 50445, 13538, 9411, 5280, 1153, 29798,
	25671, 21540, 17413, 42971, 47098, 34713, 38840, 59231, 63358, 50973, 55100, 9939, 14066, 1681, 5808, 26199, 30326, 17941, 22068, 55628, 51565, 63758, 59695, 39368,
	35305, 47498, 43435, 22596, 18533, 30726, 26663, 6336, 2273, 14466, 10403, 52093, 56156, 60223, 64286, 35833, 39896, 43963, 48026, 19061, 23124, 27191, 31254, 2801,
	6864, 10931, 14994, 64814, 60687, 56684, 52557, 48554, 44427, 40424, 36297, 31782, 27655, 23652, 19525, 15522, 11395, 7392, 3265, 61215, 65342, 53085, 57212, 44955,
	49082, 36825, 40952, 28183, 32310, 20053, 24180, 11923, 16050, 3793, 7920
};
*/

#define POLY16 0x1021
static const uint16_t CRC16_TABLE[] =
{
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
	0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
	0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
	0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
	0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
	0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
	0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
	0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
	0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
	0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
	0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
	0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
	0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
	0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
	0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
	0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
	0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
	0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
	0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
	0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
	0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
	0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
	0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

// ************************************************************************

// commands:
//  0x14 - hello
//  0x1b - read eeprom
//  0x1d - write eeprom
//  0xdd - reset radio

// flash commands:
//  0x30 - say hello to the radio and present the version (reply is also 0x18)
//  0x19 - send flash block (reply from radio is 0x1a)
//
// from the radio:
//  0x18 - broadcast from the radio when flash mode is enabled

// CMD_GET_FW_VER    = b'\x14\x05' #0x0514 -> 0x0515
// CMD_READ_FW_MEM   = b'\x17\x05' #0x0517 -> 0x0518
// CMD_WRITE_FW_MEM  = b'\x19\x05' #0x0519 -> 0x051a // Only in bootloader
//
// CMD_READ_CFG_MEM  = b'\x1B\x05' #0x051B -> 0x051C
// CMD_WRITE_CFG_MEM = b'\x1D\x05' #0x051D -> 0x051E

const uint8_t session_id[] = {0x6a, 0x39, 0x57, 0x64};
//const uint8_t session_id[] = {0x46, 0x9c, 0x6f, 0x64};

// ************************************************************************

bool __fastcall getBuildInfo(String filename, TVersion *version)
{
	DWORD ver_info_size;
	char *ver_info;
	UINT buffer_size;
	LPVOID buffer;
	DWORD dummy;

	if (version == NULL || filename.IsEmpty())
		return false;

	memset(version, 0, sizeof(TVersion));

	ver_info_size = ::GetFileVersionInfoSizeA(filename.c_str(), &dummy);
	if (ver_info_size == 0)
		return false;

	ver_info = new char [ver_info_size];
	if (ver_info == NULL)
		return false;

	if (::GetFileVersionInfoA(filename.c_str(), 0, ver_info_size, ver_info) == FALSE)
	{
		delete [] ver_info;
		return false;
	}

	if (::VerQueryValue(ver_info, _T("\\"), &buffer, &buffer_size) == FALSE)
	{
		delete [] ver_info;
		return false;
	}

	PVSFixedFileInfo ver = (PVSFixedFileInfo)buffer;
	version->MajorVer   = (ver->dwFileVersionMS >> 16) & 0xFFFF;
	version->MinorVer   = (ver->dwFileVersionMS >>  0) & 0xFFFF;
	version->ReleaseVer = (ver->dwFileVersionLS >> 16) & 0xFFFF;
	version->BuildVer   = (ver->dwFileVersionLS >>  0) & 0xFFFF;

	delete [] ver_info;

	return true;
}

// ************************************************************************

__fastcall TForm1::TForm1(TComponent* Owner)
	: TForm(Owner)
{
}

void __fastcall TForm1::FormCreate(TObject *Sender)
{
	{
//		char username[64];
//		DWORD size = sizeof(username);
//		if (::GetUserNameA(username, &size) != FALSE && size > 1)
//			m_ini_filename = ChangeFileExt(Application->ExeName, "_" + String(username) + ".ini");
//		else
			m_ini_filename = ChangeFileExt(Application->ExeName, ".ini");
	}

	::GetSystemInfo(&m_system_info);
//	sprintf(SystemInfoStr,
//					"OEM id: %u"crlf
//					"num of cpu's: %u"crlf
//					"page size: %u"crlf
//					"cpu type: %u"crlf
//					"min app addr: %lx"crlf
//					"max app addr: %lx"crlf
//					"active cpu mask: %u"crlf,
//					m_system_info.dwOemId,
//					m_system_info.dwNumberOfProcessors,
//					m_system_info.dwPageSize,
//					m_system_info.dwProcessorType,
//					m_system_info.lpMinimumApplicationAddress,
//					m_system_info.lpMaximumApplicationAddress,
//					m_system_info.dwActiveProcessorMask);
//	MemoAddString(Memo1, SystemInfoStr);

	// the screen size is the phsyical screen size
	m_screen_width  = 0;
	m_screen_height = 0;
	HDC hDC = GetDC(0);
	if (hDC != NULL)
	{
		//ScreenBitsPerPixel = ::GetDeviceCaps(hDC, BITSPIXEL);
		m_screen_width  = ::GetDeviceCaps(hDC, HORZRES);
		m_screen_height = ::GetDeviceCaps(hDC, VERTRES);
		ReleaseDC(0, hDC);
	}

	{
		String s;
		TVersion version;
		getBuildInfo(Application->ExeName, &version);
		#ifdef _DEBUG
			s.printf("%s v%u.%u.%u.debug", Application->Title.c_str(), version.MajorVer, version.MinorVer, version.ReleaseVer);
		#else
			s.printf("%s v%u.%u.%u", Application->Title.c_str(), version.MajorVer, version.MinorVer, version.ReleaseVer);
		#endif
		this->Caption = s;
	}

	this->DoubleBuffered  = true;
	Panel1->DoubleBuffered = true;
	Panel2->DoubleBuffered = true;
	Memo1->DoubleBuffered = true;

	Memo1->Clear();

	m_thread = NULL;

	m_verbose = VerboseTrackBar->Position;

	m_serial.port_name = "";
	//m_serial.port;
	m_serial.rx_buffer.resize(2048);
	m_serial.rx_buffer_wr = 0;
	m_serial.rx_timer.mark();

	OpenDialog1->InitialDir = ExtractFilePath(Application->ExeName);
	SaveDialog1->InitialDir = ExtractFilePath(Application->ExeName);

	CGauge1->Progress = 0;

	// ******************

	{
		updateSerialPortCombo();
		SerialPortComboBox->ItemIndex = 0;

		const TNotifyEvent ne = SerialSpeedComboBox->OnChange;
		SerialSpeedComboBox->OnChange = NULL;
		SerialSpeedComboBox->Clear();
		SerialSpeedComboBox->AddItem("300",     (TObject *)300);
		SerialSpeedComboBox->AddItem("600",     (TObject *)600);
		SerialSpeedComboBox->AddItem("1200",    (TObject *)1200);
		SerialSpeedComboBox->AddItem("2400",    (TObject *)2400);
		SerialSpeedComboBox->AddItem("4800",    (TObject *)4800);
		SerialSpeedComboBox->AddItem("9600",    (TObject *)9600);
		SerialSpeedComboBox->AddItem("19200",   (TObject *)19200);
		SerialSpeedComboBox->AddItem("38400 - k5/k6", (TObject *)38400);
		SerialSpeedComboBox->AddItem("57600",   (TObject *)57600);
		SerialSpeedComboBox->AddItem("76800",   (TObject *)76800);
		SerialSpeedComboBox->AddItem("115200",  (TObject *)115200);
		SerialSpeedComboBox->AddItem("230400",  (TObject *)230400);
		SerialSpeedComboBox->AddItem("250000",  (TObject *)250000);
		SerialSpeedComboBox->AddItem("460800",  (TObject *)460800);
		SerialSpeedComboBox->AddItem("500000",  (TObject *)500000);
		SerialSpeedComboBox->AddItem("921600",  (TObject *)921600);
		SerialSpeedComboBox->AddItem("1000000", (TObject *)1000000);
		SerialSpeedComboBox->AddItem("1843200", (TObject *)1843200);
		SerialSpeedComboBox->AddItem("2000000", (TObject *)2000000);
		SerialSpeedComboBox->AddItem("3000000", (TObject *)3000000);
		SerialSpeedComboBox->ItemIndex = SerialSpeedComboBox->Items->IndexOfObject((TObject *)DEFAULT_SERIAL_SPEED);
		SerialSpeedComboBox->OnChange = ne;
	}

	comboBoxAutoWidth(SerialPortComboBox);
	comboBoxAutoWidth(SerialSpeedComboBox);

	// ******************

	::PostMessage(this->Handle, WM_INIT_GUI, 0, 0);
}

void __fastcall TForm1::FormDestroy(TObject *Sender)
{
	//
}

void __fastcall TForm1::FormClose(TObject *Sender, TCloseAction &Action)
{
	Timer1->Enabled = false;

	disconnect();

	saveSettings();
}

void __fastcall TForm1::WMWindowPosChanging(TWMWindowPosChanging &msg)
{
	const int thresh = 8;

	RECT work_area;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &work_area, 0);

	const int dtLeft   = Screen->DesktopRect.left;
	const int dtRight  = Screen->DesktopRect.right;
	const int dtTop    = Screen->DesktopRect.top;
	const int dtBottom = Screen->DesktopRect.bottom;
	const int dtWidth  = dtRight - dtLeft;
	const int dtHeight = dtBottom - dtTop;

//	const int waLeft = work_area.left;
//	const int waTop = work_area.top;
//	const int waRight = work_area.right;
//	const int waBottom = work_area.bottom;
	const int waWidth = work_area.right - work_area.left;
	const int waHeight = work_area.bottom - work_area.top;

	int x = msg.WindowPos->x;
	int y = msg.WindowPos->y;
	int w = msg.WindowPos->cx;
	int h = msg.WindowPos->cy;

	{	// sticky screen edges
		if (std::abs((int)(x - work_area.left)) < thresh)
			x = work_area.left;			// stick left to left side
		else
		if (std::abs((int)((x + w) - work_area.right)) < thresh)
			x = work_area.right - w;	// stick right to right side

		if (std::abs((int)(y - work_area.top)) < thresh)
			y = work_area.top;			// stick top to top side
		else
		if (std::abs((int)((y + h) - work_area.bottom)) < thresh)
			y = work_area.bottom - h;	// stick bottom to bottm side

		// stick the right side to the right side of the screen if the left side is stuck to the left side of the screen
		if (x == work_area.left)
			if ((w >= (waWidth - thresh)) && (w <= (waWidth + thresh)))
				w = waWidth;

		// stick the bottom to the bottom of the screen if the top is stuck to the top of the screen
		if (y == work_area.top)
			if ((h >= (waHeight - thresh)) && (h <= (waHeight + thresh)))
				h = waHeight;
	}

	{	// limit minimum size
		if (w < Constraints->MinWidth)
			 w = Constraints->MinWidth;
		if (h < Constraints->MinHeight)
			 h = Constraints->MinHeight;
	}

	{	// limit maximum size
		if (w > Constraints->MaxWidth && Constraints->MaxWidth > Constraints->MinWidth)
			 w = Constraints->MaxWidth;
		if (h > Constraints->MaxHeight && Constraints->MaxHeight > Constraints->MinHeight)
			 h = Constraints->MaxHeight;
	}

	{	// limit maximum size
		if (w > dtWidth)
			 w = dtWidth;
		if (h > dtHeight)
			 h = dtHeight;
	}

	if (Application->MainForm && this != Application->MainForm)
	{	// stick to our main form sides
		const TRect rect = Application->MainForm->BoundsRect;

		if (std::abs((int)(x - rect.left)) < thresh)
			x = rect.left;			// stick to left to left side
		else
		if (std::abs((int)((x + w) - rect.left)) < thresh)
			x = rect.left - w;	// stick right to left side
		else
		if (std::abs((int)(x - rect.right)) < thresh)
			x = rect.right;		// stick to left to right side
		else
		if (std::abs((int)((x + w) - rect.right)) < thresh)
			x = rect.right - w;	// stick to right to right side

		if (std::abs((int)(y - rect.top)) < thresh)
			y = rect.top;			// stick top to top side
		else
		if (std::abs((int)((y + h) - rect.top)) < thresh)
			y = rect.top - h;		// stick bottom to top side
		else
		if (std::abs((int)(y - rect.bottom)) < thresh)
			y = rect.bottom;		// stick top to bottom side
		else
		if (std::abs((int)((y + h) - rect.bottom)) < thresh)
			y = rect.bottom - h;	// stick bottom to bottom side
	}

	{	// stop it completely leaving the desktop area
		if (x < (dtLeft - Width + (dtWidth / 15)))
			  x = dtLeft - Width + (dtWidth / 15);
		if (x > (dtWidth - (Screen->Width / 15)))
			  x = dtWidth - (Screen->Width / 15);
		if (y < dtTop)
			 y = dtTop;
		if (y > (dtBottom - (dtHeight / 10)))
			  y = dtBottom - (dtHeight / 10);
	}

	msg.WindowPos->x  = x;
	msg.WindowPos->y  = y;
	msg.WindowPos->cx = w;
	msg.WindowPos->cy = h;
}

void __fastcall TForm1::CMMouseEnter(TMessage &msg)
{
	TComponent *Comp = (TComponent *)msg.LParam;
	if (!Comp)
		return;

//	if (dynamic_cast<TControl *>(Comp) == NULL)
//		return;		// only interested in on screen controls

	if (dynamic_cast<TPaintBox *>(Comp) != NULL)
	{
//		TPaintBox *pb = (TPaintBox *)Comp;
//		pb->Invalidate();
	}
}

void __fastcall TForm1::CMMouseLeave(TMessage &msg)
{
	TComponent *Comp = (TComponent *)msg.LParam;
	if (!Comp)
		return;

//	if (dynamic_cast<TControl *>(Comp) == NULL)
//		return;		// only interested in on screen controls

	if (dynamic_cast<TPaintBox *>(Comp) != NULL)
	{
//		TPaintBox *pb = (TPaintBox *)Comp;
/*
		m_graph_mouse_x     = -1;
		m_graph_mouse_y     = -1;

		m_mouse_graph = -1;
		for (int i = 0; i < MAX_TRACES; i++)
			m_graph_mouse_index[i] = -1;
*/
//		pb->Invalidate();
	}
/*
	if (dynamic_cast<TPaintBox *>(Comp) != NULL)
	{
		TPaintBox *pb = PaintBox1;

		m_graph_mouse_x     = -1;
		m_graph_mouse_y     = -1;

		m_mouse_graph = -1;
		for (int i = 0; i < MAX_TRACES; i++)
			m_graph_mouse_index[i] = -1;

		pb->Invalidate();
	}
*/
}

void __fastcall TForm1::WMInitGUI(TMessage &msg)
{
	String s;

	loadSettings();

	//	BringToFront();
//	::SetForegroundWindow(Handle);

	if (Application->MainForm)
		Application->MainForm->Update();

//	HANDLE ble_service = openBleService();

//	Timer1->Enabled = true;

	// TEST ONLY
//	processConfigString("[ECG_TYPE3,v2023.06.12.10.30,500Hz,12bit,570,3328mV,int_bin,20,1,1,0,0,0,0,0]");

//	::PostMessage(this->Handle, WM_CONNECT, 0, 0);
}

void __fastcall TForm1::WMConnect(TMessage &msg)
{
	connect();
}

void __fastcall TForm1::WMDisconnect(TMessage &msg)
{
	disconnect();
}

void __fastcall TForm1::loadSettings()
{
	int          i;
	float        f;
	String       s;
	bool         b;
	TNotifyEvent ne;

	TIniFile *ini = new TIniFile(m_ini_filename);
	if (ini == NULL)
		return;

	Top    = ini->ReadInteger("MainForm", "Top",     Top);
	Left   = ini->ReadInteger("MainForm", "Left",   Left);
	Width  = ini->ReadInteger("MainForm", "Width",  Width);
	Height = ini->ReadInteger("MainForm", "Height", Height);

	m_verbose = ini->ReadInteger("GUI", "Verbose", VerboseTrackBar->Position);
	VerboseTrackBar->Position = m_verbose;

	m_serial.port_name = ini->ReadString("SerialPort", "Name", SerialPortComboBox->Text);
	i = SerialPortComboBox->Items->IndexOf(m_serial.port_name);
	if (i >= 0)
	{
		ne = SerialPortComboBox->OnChange;
		SerialPortComboBox->OnChange = NULL;
		SerialPortComboBox->ItemIndex = i;
		SerialPortComboBox->OnChange = ne;
	}
	SerialPortComboBoxChange(SerialPortComboBox);

	ne = SerialSpeedComboBox->OnChange;
	SerialSpeedComboBox->OnChange = NULL;
	i = ini->ReadInteger("SerialPort", "Speed", DEFAULT_SERIAL_SPEED);
	i = SerialSpeedComboBox->Items->IndexOfObject((TObject *)i);
	if (i < 0)
		i = SerialSpeedComboBox->Items->IndexOfObject((TObject *)DEFAULT_SERIAL_SPEED);
	SerialSpeedComboBox->ItemIndex = i;
	SerialSpeedComboBox->OnChange = ne;

	delete ini;
}

void __fastcall TForm1::saveSettings()
{
	String s;
	int    i;

	DeleteFile(m_ini_filename);

	TIniFile *ini = new TIniFile(m_ini_filename);
	if (ini == NULL)
		return;

	ini->WriteInteger("MainForm", "Top",    Top);
	ini->WriteInteger("MainForm", "Left",   Left);
	ini->WriteInteger("MainForm", "Width",  Width);
	ini->WriteInteger("MainForm", "Height", Height);

	ini->WriteInteger("GUI", "Verbose",      m_verbose);

	ini->WriteString("SerialPort", "Name",  m_serial.port_name);
	ini->WriteInteger("SerialPort", "Speed", (int)SerialSpeedComboBox->Items->Objects[SerialSpeedComboBox->ItemIndex]);

	delete ini;
}

void __fastcall TForm1::comboBoxAutoWidth(TComboBox *comboBox)
{
	if (!comboBox)
		return;

	#define COMBOBOX_HORIZONTAL_PADDING	4

	int itemsFullWidth = comboBox->Width;

	// get the max needed with of the items in dropdown state
	for (int i = 0; i < comboBox->Items->Count; i++)
	{
		int itemWidth = comboBox->Canvas->TextWidth(comboBox->Items->Strings[i]);
		itemWidth += 2 * COMBOBOX_HORIZONTAL_PADDING;
		if (itemsFullWidth < itemWidth)
			itemsFullWidth = itemWidth;
	}

	if (comboBox->DropDownCount < comboBox->Items->Count)
		itemsFullWidth += ::GetSystemMetrics(SM_CXVSCROLL);

	::SendMessage(comboBox->Handle, CB_SETDROPPEDWIDTH, itemsFullWidth, 0);
}

void __fastcall TForm1::updateSerialPortCombo()
{
	std::vector <T_SerialPortInfo> serial_port_list;
	m_serial.port.GetSerialPortList(serial_port_list);

	const TNotifyEvent ne = SerialPortComboBox->OnChange;
	SerialPortComboBox->OnChange = NULL;

	SerialPortComboBox->Clear();
	SerialPortComboBox->AddItem("None", (TObject *)0xffffffff);
	for (unsigned int i = 0; i < serial_port_list.size(); i++)
		SerialPortComboBox->AddItem(serial_port_list[i].name, (TObject *)i);

	const int i = SerialPortComboBox->Items->IndexOf(m_serial.port_name);
	SerialPortComboBox->ItemIndex = (!m_serial.port_name.IsEmpty() && i >= 0) ? i : 0;

	SerialPortComboBox->OnChange = ne;

	SerialPortComboBoxChange(SerialPortComboBox);
}

void __fastcall TForm1::SerialPortComboBoxDropDown(TObject *Sender)
{
	updateSerialPortCombo();
}

void __fastcall TForm1::clearRxPacketQueue()
{
	CCriticalSection cs(m_thread_cs);

	for (size_t i = 0; i < m_rx_packet_queue.size(); i++)
		m_rx_packet_queue[i].clear();

	m_rx_packet_queue.resize(0);

	m_serial.rx_buffer_wr = 0;
}

void __fastcall TForm1::disconnect()
{
	if (m_thread != NULL)
	{
		if (!m_thread->FreeOnTerminate)
		{
			m_thread->Terminate();
			m_thread->WaitFor();
			delete m_thread;
			m_thread = NULL;
		}
		else
		{
			m_thread->Terminate();
			m_thread = NULL;
		}
	}

	if (m_serial.port.connected)
	{
		m_serial.port.Disconnect();

		String s;
		s.printf("'%s' closed", m_serial.port_name.c_str());
		Memo1->Lines->Add(s);
	}

	clearRxPacketQueue();

	SerialPortComboBox->Enabled  = true;
	SerialSpeedComboBox->Enabled = true;
}

bool __fastcall TForm1::connect(const bool clear_memo)
{
	String s;
	String port_name;

//	Beep(1000, 10);

	disconnect();

	if (clear_memo)
	{
		Memo1->Clear();
		Memo1->Update();
	}

	const int i = SerialPortComboBox->ItemIndex;
	if (i <= 0)
		return false;
	port_name = SerialPortComboBox->Items->Strings[i].Trim();
	if (port_name.IsEmpty() || port_name.LowerCase() == "none")
		return false;

	int baudrate = (int)SerialSpeedComboBox->Items->Objects[SerialSpeedComboBox->ItemIndex];
	if (baudrate <= 0)
	{	// error
		Memo1->Lines->Add("error: serial baudrate: [" + IntToStr(baudrate) + "]");
		return false;
	}

	m_serial.port.rts      = true;
	m_serial.port.dtr      = true;
	m_serial.port.byteSize = 8;
	m_serial.port.parity   = NOPARITY;
	m_serial.port.stopBits = ONESTOPBIT;
	m_serial.port.baudRate = baudrate;

	const int res = m_serial.port.Connect(port_name.c_str(), true);
	if (res != ERROR_SUCCESS)
	{
		s.printf("error: serial port open error [%d]", res);
		Memo1->Lines->Add(s);
		return false;
	}

	m_serial.port_name = port_name;

	s.printf("'%s' opened at %d Baud", m_serial.port_name.c_str(), baudrate);
	Memo1->Lines->Add(s);

	m_serial.rx_buffer_wr = 0;
	m_serial.rx_timer.mark();

	// flush the RX
	while (m_serial.port.RxBytes(&m_serial.rx_buffer[0], m_serial.rx_buffer.size()) > 0)
		Sleep(50);

	if (m_thread == NULL)
	{	// create & start the thread
		m_thread = new CThread(&threadProcess, tpNormal, 1, true, false);
//		m_thread = new CThread(&threadProcess, tpNormal, 1, true, true);
		if (m_thread == NULL)
		{
			Memo1->Lines->Add("");
			disconnect();
			return false;
		}
	}

	SerialPortComboBox->Enabled  = false;
	SerialSpeedComboBox->Enabled = false;

	return true;
}

void __fastcall TForm1::threadProcess()
{
	if (m_thread == NULL)
		return;

	if (!m_serial.port.connected)
	{	// serial is closed
		return;
	}

	CCriticalSection cs(m_thread_cs, !m_thread->Sync);

	// *********
	// save any rx'ed bytes from the serial port into our RX buffer

	int num_bytes = 0;

	const int rx_space = m_serial.rx_buffer.size() - m_serial.rx_buffer_wr;	// space we have left to store RX data
	if (rx_space > 0)
	{	// we have buffer space for more RX data

		num_bytes = m_serial.port.RxBytes(&m_serial.rx_buffer[m_serial.rx_buffer_wr], rx_space);
		if (num_bytes < 0)
		{	// error
			m_serial.port.Disconnect();
			::PostMessage(this->Handle, WM_DISCONNECT, 0, 0);
			m_serial.rx_buffer_wr = 0;
			return;
		}

		if (num_bytes > 0)
		{
			m_serial.rx_buffer_wr += num_bytes;
			m_serial.rx_timer.mark();
		}
	}

	// *********
	// extract any rx'ed packets found in our RX buffer

	if (m_serial.rx_buffer_wr > 0 && m_serial.rx_timer.millisecs() >= 5000)
		m_serial.rx_buffer_wr = 0;		// no data rx'ed fro 2 seconds .. empty the rx buffer

	while (m_serial.rx_buffer_wr >= 8)
	{
		// scan for the start of a packet
		if (m_serial.rx_buffer[0] != 0xAB || m_serial.rx_buffer[1] != 0xCD || m_serial.rx_buffer[2] == 0 || m_serial.rx_buffer[3] != 0x00)
		{	// slide the data down one byte
			memmove(&m_serial.rx_buffer[0], &m_serial.rx_buffer[1], m_serial.rx_buffer_wr - 1);
			m_serial.rx_buffer_wr--;
			continue;
		}

		const int data_len   = m_serial.rx_buffer[2];    // byte[2] is the payload size
		const int packet_len = 4 + data_len + 2 + 2;

		if ((int)m_serial.rx_buffer_wr < packet_len)
			break;	// not yet received the complete packet

		if (m_serial.rx_buffer[packet_len - 2] != 0xDC || m_serial.rx_buffer[packet_len - 1] != 0xBA)
		{	// slide the data down one byte
			memmove(&m_serial.rx_buffer[0], &m_serial.rx_buffer[1], m_serial.rx_buffer_wr - 1);
			m_serial.rx_buffer_wr--;
			continue;
		}

		// appear to have a complete packet

		if (data_len > 0)
		{
			std::vector <uint8_t> rx_data(data_len + 2);
			memcpy(&rx_data[0], &m_serial.rx_buffer[4], data_len + 2);

			#if 1
				if (m_thread->Sync && m_verbose > 2)
				{	// show the raw packet
					String s;
					s.printf("rx [%3u] ", packet_len);
					for (int i = 0; i < packet_len; i++)
					{
						String s2;
						s2.printf("%02X ", m_serial.rx_buffer[i]);
						s += s2;
					}
					Memo1->Lines->Add(s.Trim());
					Memo1->Update();
				}
			#endif

			// CRC bytes before de-obfuscate
			const uint16_t crc0 = ((uint16_t)rx_data[rx_data.size() - 1] << 8) | ((uint16_t)rx_data[rx_data.size() - 2] << 0);

			// descramble the payload (the data and CRC)
			xor_payload(&rx_data[0], rx_data.size());

			// CRC bytes after de-obfuscate
			const uint16_t crc1 = ((uint16_t)rx_data[rx_data.size() - 1] << 8) | ((uint16_t)rx_data[rx_data.size() - 2] << 0);

			#if 1
				if (m_thread->Sync && m_verbose > 2)
				{	// show the descrambled payload
					String s;
					s.printf("rx [%3u]             ", m_rx_packet_queue.size());
					for (size_t i = 0; i < rx_data.size(); i++)
					{
						String s2;
						s2.printf("%02X ", rx_data[i]);
						s += s2;
					}
					Memo1->Lines->Add(s.Trim());
					Memo1->Update();
				}
			#endif

			if (crc0 != 0xffff && crc1 != 0xffff)
			{	// compute and check the CRC
				const uint16_t crc2 = crc16xmodem(&rx_data[0], rx_data.size() - 2);
				if (crc2 != crc1)
					rx_data.resize(0);	// CRC error, dump the payload
			}

			if (!rx_data.empty())
			{	// we have a payload to save

				// drop the 16-bit CRC
				rx_data.resize(data_len);

				// append the rx'ed packet onto the rx queue
				//CCriticalSection cs(m_thread_cs);
				if (m_rx_packet_queue.size() < 4)
					m_rx_packet_queue.push_back(rx_data);
			}
		}

		// remove the spent packet from the RX buffer
		if (packet_len < (int)m_serial.rx_buffer_wr)
			memmove(&m_serial.rx_buffer[0], &m_serial.rx_buffer[packet_len], m_serial.rx_buffer_wr - packet_len);
		m_serial.rx_buffer_wr -= packet_len;
	}

	// *********
}

std::vector <String> __fastcall TForm1::stringSplit(String s, String separator)
{
	std::vector <String> strings;

	if (separator.IsEmpty())
		return strings;

	while (s.Length() > 0)
	{
		const int p = s.Pos(separator);
		if (p <= 0)
			break;
		String s2 = s.SubString(1, p - 1);
		s = s.SubString(p + separator.Length(), s.Length());
		strings.push_back(s2);
	}

	if (!s.IsEmpty())
		strings.push_back(s);

	return strings;
}

void __fastcall TForm1::FormKeyDown(TObject *Sender, WORD &Key,
		TShiftState Shift)
{
	switch (Key)
	{
		case VK_ESCAPE:
			Key = 0;
			if (m_serial.port.connected)
				m_serial.port.Disconnect();
			else
				Close();
			break;
		case VK_SPACE:
//			Key = 0;
			break;
		case VK_UP:		// up arrow
//			Key = 0;
			break;
		case VK_DOWN:	// down arrow
//			Key = 0;
			break;
		case VK_LEFT:	// left arrow
//			Key = 0;
			break;
		case VK_RIGHT:	// right arrow
//			Key = 0;
			break;
		case VK_PRIOR:	// page up
//			Key = 0;
			break;
		case VK_NEXT:	// page down
//			Key = 0;
			break;
	}
}

void __fastcall TForm1::Timer1Timer(TObject *Sender)
{
	String s = FormatDateTime(" dddd dd mmm yyyy  hh:nn:ss", Now());
	if (StatusBar1->Panels->Items[0]->Text != s)
	{
		StatusBar1->Panels->Items[0]->Text = s;
		StatusBar1->Update();
	}
}

int __fastcall TForm1::saveFile(String filename, const uint8_t *data, const size_t size)
{
	String s;

	if (data == NULL || size == 0)
		return 0;

	const int file_handle = FileCreate(filename);
	if (file_handle <= 0)
		return 0;

	const int bytes_written = FileWrite(file_handle, &data[0], size);

	FileClose(file_handle);

	return bytes_written;
}

size_t __fastcall TForm1::loadFile(String filename)
{	// load a saved file

	String s;

	// ****************************
	// load the file

	const int file_handle = FileOpen(filename, fmOpenRead | fmShareDenyNone);
	if (file_handle <= 0)
	{
		Application->BringToFront();
		Application->NormalizeTopMosts();
		Application->MessageBox("error: file not opened", Application->Title.c_str(), MB_ICONERROR | MB_OK);
		Application->RestoreTopMosts();
		return 0;
	}

	const int file_size = FileSeek(file_handle, 0, 2);
	FileSeek(file_handle, 0, 0);

	if (file_size <= 0)
	{
		FileClose(file_handle);

		Application->BringToFront();
		Application->NormalizeTopMosts();
		Application->MessageBox("error: empty file", Application->Title.c_str(), MB_ICONERROR | MB_OK);
		Application->RestoreTopMosts();
		return 0;
	}

	std::vector <uint8_t> input_buffer(file_size);

	const int bytes_loaded = FileRead(file_handle, &input_buffer[0], file_size);

	FileClose(file_handle);

	if (bytes_loaded != (int)input_buffer.size())
	{
		Application->BringToFront();
		Application->NormalizeTopMosts();
		Application->MessageBox("error: file not loaded", Application->Title.c_str(), MB_ICONERROR | MB_OK);
		Application->RestoreTopMosts();
		return 0;
	}

	// ***************************

	m_loadfile_name = filename;
	m_loadfile_data = input_buffer;

	return m_loadfile_data.size();
}

void __fastcall TForm1::ClearButtonClick(TObject *Sender)
{
	Memo1->Clear();
}

// ************************************************************************

#if 0

	uint16_t __fastcall TForm1::crc16xmodem(const uint8_t *data, const int size)
	{
		uint16_t crc = 0;
		if (data != NULL && size > 0)
		{
			for (int i = 0; i < size; i++)
			{
				crc ^= (uint16_t)data[i] << 8;
				for (int k = 0; k < 8; k++)
					crc = (crc & 0x8000) ? (crc << 1) ^ POLY16 : crc << 1;
			}
		}
		return crc;
	}

#else

	uint16_t __fastcall TForm1::crc16xmodem(const uint8_t *data, const int size)
	{
		uint16_t crc = 0;
		if (data != NULL && size > 0)
		{
			for (int i = 0; i < size; i++)
				crc = (crc << 8) ^ CRC16_TABLE[(crc >> 8) ^ data[i]];
		}
		return crc;
	}

#endif

void __fastcall TForm1::destroy_k5_struct(struct k5_command *cmd)
{
	if (cmd == NULL)
		return;

	if (cmd->cmd != NULL)
	{
		free(cmd->cmd);
		cmd->cmd = NULL;
	}

	if (cmd->obfuscated_cmd != NULL)
	{
		free(cmd->obfuscated_cmd);
		cmd->obfuscated_cmd = NULL;
	}

	free(cmd);
}

void __fastcall TForm1::hdump(const uint8_t *buf, const int len)
{
	String     s;
	char       adump[75];
	int        tmp3 = 0;
	uint8_t    sss;
	const char hexz[] = "0123456789ABCDEF";
	int        last_tmp = 0;

	if (buf == NULL || len <= 0)
		return;

	#if 0
		s.printf("Offset  00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F  Text", len);
		Memo1->Lines->Add(s);
		s.printf("-------------------------------------------------------------------------");
		Memo1->Lines->Add(s);
	#endif

	memset(adump, 0, sizeof(adump));
	memset(adump, ' ', 74);

	int tmp1;
	for (tmp1 = 0; tmp1 < len; tmp1++)
	{
		const int tmp2 = tmp1 % 16;
		if (tmp2 == 0)
		{
			if (tmp1 != 0)
			{
				s.printf("%06X  %.65s", tmp3, adump);
				Memo1->Lines->Add(s);
				last_tmp = tmp1;
			}

			memset(adump, 0, sizeof(adump));
			memset(adump, ' ', 74);

			tmp3 = tmp1;
		}

		sss = buf[tmp1];
		adump[(tmp2 * 3) + 0] = hexz[sss / 16];
		adump[(tmp2 * 3) + 1] = hexz[sss % 16];
		adump[tmp2 + 49] = isprint(sss) ? sss : '.';
	}

	#if 0
		if ((tmp1 % 16) != 0 || len == 16)
		{
			s.printf("%06X  %.65s", tmp3, adump);
			Memo1->Lines->Add(s);
		}
	#else
		if (last_tmp != tmp1)
		{
			s.printf("%06X  %.65s", tmp3, adump);
			Memo1->Lines->Add(s);
		}
	#endif
}

// hexdump a k5_command struct
void __fastcall TForm1::k5_hexdump(const struct k5_command *cmd)
{
	String s;

	if (cmd == NULL)
		return;

	s.printf("command hex dump   obf_len %d   clear_len %d   crc_ok %d", cmd->obfuscated_len, cmd->len, cmd->crc_ok);
	Memo1->Lines->Add(s);

	if (cmd->obfuscated_cmd != NULL)
	{
//		Memo1->Lines->Add("");
		Memo1->Lines->Add("* obfuscated");
		hdump(cmd->obfuscated_cmd, cmd->obfuscated_len);
	}

	if (cmd->cmd != NULL)
	{
//		Memo1->Lines->Add("");
		Memo1->Lines->Add("* clear");
		hdump(cmd->cmd, cmd->len);
	}
}

void __fastcall TForm1::hex_dump(const struct k5_command *cmd, const bool tx)
{
	String s;

	if (cmd == NULL)
		return;

	s.printf("%s crc_ok %d", tx ? "tx" : "rx", cmd->crc_ok);
	Memo1->Lines->Add(s);

	if (cmd->obfuscated_cmd != NULL)
	{
		s.printf("%s obfuscated [%3d] ", tx ? "tx" : "rx", cmd->obfuscated_len);
		for (int i = 0; i < cmd->obfuscated_len; i++)
		{
			String s2;
			s2.printf("%02X ", cmd->obfuscated_cmd[i]);
			s += s2;
		}
		Memo1->Lines->Add(s.Trim());
	}

	if (cmd->cmd != NULL)
	{
		s.printf("%s clear      [%3d]             ", tx ? "tx" : "rx", cmd->len);
		for (int i = 0; i < (cmd->len + 2); i++)
		{
			String s2;
			s2.printf("%02X ", cmd->cmd[i]);
			s += s2;
		}
		Memo1->Lines->Add(s.Trim());
	}
}

// (de)obfuscate firmware data
void __fastcall TForm1::xor_firmware(uint8_t *data, const int len)
{
	const uint8_t k5_xor_pattern[] =
	{
		0x47, 0x22, 0xc0, 0x52, 0x5d, 0x57, 0x48, 0x94, 0xb1, 0x60, 0x60, 0xdb, 0x6f, 0xe3, 0x4c, 0x7c,
		0xd8, 0x4a, 0xd6, 0x8b, 0x30, 0xec, 0x25, 0xe0, 0x4c, 0xd9, 0x00, 0x7f, 0xbf, 0xe3, 0x54, 0x05,
		0xe9, 0x3a, 0x97, 0x6b, 0xb0, 0x6e, 0x0c, 0xfb, 0xb1, 0x1a, 0xe2, 0xc9, 0xc1, 0x56, 0x47, 0xe9,
		0xba, 0xf1, 0x42, 0xb6, 0x67, 0x5f, 0x0f, 0x96, 0xf7, 0xc9, 0x3c, 0x84, 0x1b, 0x26, 0xe1, 0x4e,
		0x3b, 0x6f, 0x66, 0xe6, 0xa0, 0x6a, 0xb0, 0xbf, 0xc6, 0xa5, 0x70, 0x3a, 0xba, 0x18, 0x9e, 0x27,
		0x1a, 0x53, 0x5b, 0x71, 0xb1, 0x94, 0x1e, 0x18, 0xf2, 0xd6, 0x81, 0x02, 0x22, 0xfd, 0x5a, 0x28,
		0x91, 0xdb, 0xba, 0x5d, 0x64, 0xc6, 0xfe, 0x86, 0x83, 0x9c, 0x50, 0x1c, 0x73, 0x03, 0x11, 0xd6,
		0xaf, 0x30, 0xf4, 0x2c, 0x77, 0xb2, 0x7d, 0xbb, 0x3f, 0x29, 0x28, 0x57, 0x22, 0xd6, 0x92, 0x8b
	};

	if (data == NULL || len <= 0)
		return;

	for (int i = 0; i < len; i++)
		data[i] ^= k5_xor_pattern[i % sizeof(k5_xor_pattern)];
}

// (de)obfuscate communications data
void __fastcall TForm1::xor_payload(uint8_t *data, const int len)
{
	const uint8_t k5_xor_pattern[] =
	{
		0x16, 0x6c, 0x14, 0xe6, 0x2e, 0x91, 0x0d, 0x40, 0x21, 0x35, 0xd5, 0x40, 0x13, 0x03, 0xe9, 0x80
	};

	if (data == NULL || len <= 0)
		return;

	for (int i = 0; i < len; i++)
		data[i] ^= k5_xor_pattern[i % sizeof(k5_xor_pattern)];
}

int __fastcall TForm1::k5_obfuscate(struct k5_command *cmd)
{
	uint16_t c;

	if (cmd == NULL)
		return 0;

	if (cmd->cmd == NULL)
		return 0;

	if (cmd->obfuscated_cmd != NULL)
	{
		free(cmd->obfuscated_cmd);
		cmd->obfuscated_cmd = NULL;
	}

	cmd->obfuscated_len    = cmd->len + 8;       // header + length + data + crc + footer
	cmd->obfuscated_cmd    = (uint8_t *)calloc(cmd->obfuscated_len, 1);

	cmd->obfuscated_cmd[0] = 0xAB;
	cmd->obfuscated_cmd[1] = 0xCD;

	cmd->obfuscated_cmd[2] = (cmd->len >> 0) & 0xff;
	cmd->obfuscated_cmd[3] = (cmd->len >> 8) & 0xff;

	memcpy(cmd->obfuscated_cmd + 4, cmd->cmd, cmd->len);

	c = crc16xmodem(cmd->obfuscated_cmd + 4, cmd->len);
	cmd->obfuscated_cmd[cmd->len + 4] = (c >> 0) & 0xff;
	cmd->obfuscated_cmd[cmd->len + 5] = (c >> 8) & 0xff;

	xor_payload(cmd->obfuscated_cmd + 4, cmd->len + 2);

	cmd->obfuscated_cmd[cmd->len + 6] = 0xDC;
	cmd->obfuscated_cmd[cmd->len + 7] = 0xBA;

	cmd->crc_ok = 1;

	return 1;
}

// deobfuscate a k5 datagram and verify it
int __fastcall TForm1::k5_deobfuscate(struct k5_command *cmd)
{
	String s;

	if (cmd == NULL)
		return 0;

	if (cmd->obfuscated_cmd == NULL)
		return 0;

	if (cmd->cmd != NULL)
	{
		free(cmd->cmd);
		cmd->cmd = NULL;
	}

	// check the obfuscated datagram

	if (cmd->obfuscated_cmd[0] != 0xAB || cmd->obfuscated_cmd[1] != 0xCD)
	{	// invalid header
		if (m_verbose > 2)
		{
			Memo1->Lines->Add("error: invalid header");
			k5_hexdump(cmd);
		}
		return 0;
	}

	if (cmd->obfuscated_cmd[cmd->obfuscated_len - 2] != 0xDC || cmd->obfuscated_cmd[cmd->obfuscated_len - 1] != 0xBA)
	{	// invalid footer
		if (m_verbose > 2)
		{
			Memo1->Lines->Add("error: invalid footer");
			k5_hexdump(cmd);
		}
		return 0;
	}

	cmd->len = cmd->obfuscated_len - 6;       // header + length + data + crc + footer
	cmd->cmd = (uint8_t *)calloc(cmd->len, 1);
	if (cmd->cmd == NULL)
	{
		Memo1->Lines->Add("error: calloc()");
		return 0;
	}

	memcpy(cmd->cmd, cmd->obfuscated_cmd + 4, cmd->len);

	// de-obfuscate
	xor_payload(cmd->cmd, cmd->len);

	if (m_verbose > 2)
	{
		s.printf("de-obfuscate [%d]:", cmd->len);
		Memo1->Lines->Add("");
		Memo1->Lines->Add(s);
		Memo1->Update();

		hdump(cmd->cmd, cmd->len);
	}

	const uint16_t crc1 = crc16xmodem(cmd->cmd, cmd->len - 2);
	const uint16_t crc2 = ((uint16_t)cmd->cmd[cmd->len - 1] << 8) | ((uint16_t)cmd->cmd[cmd->len - 2] << 0);

	//if ((*cmd->cmd[*cmd->cmd - 2] == ((c << 0) & 0xff)) && (*cmd->cmd[*cmd->cmd - 2] == ((c << 8) & 0xff)))
	// the protocol looks like it would use crc from the radio to the pc, but instead the radio sends 0xffff
	if (crc2 == 0xffff)
	{
		cmd->crc_ok = 1;
		cmd->len -= 2;    // skip crc
		return 1;
	}

	if (crc2 == crc1)
	{
		Memo1->Lines->Add("* the protocol actually uses proper crc on datagrams from the radio, please inform the author of the radio/firmware version");
		k5_hexdump(cmd);
	}

	cmd->crc_ok = 0;

	if (m_verbose > 2)
	{
		s.printf("invalid crc   rx'ed 0x%04X   computed 0x%04X", crc2, crc1);
		Memo1->Lines->Add(s);
		k5_hexdump(cmd);
	}

	cmd->len -= 2;    // skip crc

	return 0;
}

// obfuscate a command, send it
int __fastcall TForm1::k5_send_cmd(struct k5_command *cmd)
{
	String s;

	if (!m_serial.port.connected || cmd == NULL)
		return 0;

	if (k5_obfuscate(cmd) <= 0)
	{
		Memo1->Lines->Add("error: k5_obfuscate()");
		return 0;
	}

	if (m_verbose > 1)
//		k5_hexdump(cmd);
		hex_dump(cmd, true);

	if (cmd->obfuscated_cmd != NULL && cmd->obfuscated_len > 0)
	{
		const int len = m_serial.port.TxBytes(cmd->obfuscated_cmd, cmd->obfuscated_len);
		if (len > 0 && m_verbose > 2)
		{
			s.printf("sent %d bytes", len);
			Memo1->Lines->Add(s);
		}
	}

	return 1;
}

int __fastcall TForm1::k5_send_buf(const uint8_t *buf, const int len)
{
	if (!m_serial.port.connected || buf == NULL || len <= 0)
		return 0;

	// delete any previously rx'ed/saved packets
	clearRxPacketQueue();

	struct k5_command *cmd = (struct k5_command *)calloc(sizeof(struct k5_command), 1);
	if (cmd == NULL)
		return 0;

	cmd->len = len;
	cmd->cmd = (uint8_t *)malloc(cmd->len);
	memcpy(cmd->cmd, buf, len);

	const int s_len = k5_send_cmd(cmd);

	destroy_k5_struct(cmd);

	return s_len;
}

int __fastcall TForm1::k5_read_eeprom(uint8_t *buf, const int len, const int offset)
{
	String             s;
	int                l;
	uint8_t            buffer[4 + 4 + 4];
	struct k5_command *cmd;

	if (!m_serial.port.connected || buf == NULL || len <= 0 || len > 128)
		return 0;

	if (m_verbose > 1)
	{
		s.printf("read_eeprom  offset %04X  len %d", offset, len);
		Memo1->Lines->Add(s);
		Memo1->Update();
	}

	buffer[0] = 0x1B;
	buffer[1] = 0x05;
	buffer[2] = 8;
	buffer[3] = 0x00;

	buffer[4] = (offset >> 0) & 0xff;   // addr LS-Byte
	buffer[5] = (offset >> 8) & 0xff;   // addr MS-Byte
	buffer[6] = len;                    // len
	buffer[7] = 0x00;                   // ?

	memcpy(&buffer[8], session_id, 4);

	int r = k5_send_buf(buffer, sizeof(buffer));
	if (r <= 0)
		return 0;

	// wait for a rx'ed packet
	const DWORD tick = GetTickCount();
	while ((GetTickCount() - tick) <= 3000)
	{
		if (m_thread->Sync)
			Application->ProcessMessages();
		else
			Sleep(1);

		if (m_rx_packet_queue.empty())
			continue;

		// fetch the rx'ed packet
		const int      rx_data_size = m_rx_packet_queue[0].size();
		const uint8_t *rx_data      = &m_rx_packet_queue[0][0];

		if (m_verbose > 2)
			hdump(rx_data, rx_data_size);

		if (rx_data_size < (8 + len))
		{
			CCriticalSection cs(m_thread_cs);
			m_rx_packet_queue[0].clear();
			m_rx_packet_queue.erase(m_rx_packet_queue.begin() + 0);
			continue;
		}

		if (rx_data[0] != 0x1C ||
			 rx_data[1] != 0x05 ||
			 rx_data[4] != buffer[4] ||
			 rx_data[5] != buffer[5])
		{
			CCriticalSection cs(m_thread_cs);
			m_rx_packet_queue[0].clear();
			m_rx_packet_queue.erase(m_rx_packet_queue.begin() + 0);
			continue;
		}

		memcpy(buf, &rx_data[8], len);

		return 1;
	}

	if (m_verbose > 1)
		Memo1->Lines->Add("error: k5_read_eeprom() no valid packet received");

	return 0;
}

int __fastcall TForm1::k5_write_eeprom(uint8_t *buf, const int len, const int offset)
{
	String  s;
	uint8_t buffer[4 + 4 + 4 + 128];

	if (!m_serial.port.connected || buf == NULL || len <= 0 || len > 128)
		return 0;

	if (m_verbose > 2)
	{
//		s.printf("write_eeprom  offset %04X  len %d", offset, len);
//		Memo1->Lines->Add(s);
//		Memo1->Update();
	}

	buffer[0] = 0x1D;
	buffer[1] = 0x05;
	buffer[2] = 8 + len;
	buffer[3] = 0x00;

	buffer[4] = (offset >> 0) & 0xff;   // addr LS-Byte
	buffer[5] = (offset >> 8) & 0xff;   // addr MS-Byte
	buffer[6] = len;                    // len
	buffer[7] = 0x01;                   // ?

	memcpy(&buffer[8], session_id, 4);

	memcpy(&buffer[12], buf, len);

	const int r = k5_send_buf(buffer, 4 + 4 + 4 + len);
	if (r <= 0)
		return 0;

	// wait for a rx'ed packet
	const DWORD tick = GetTickCount();
	while ((GetTickCount() - tick) <= 2000)
	{
		if (m_thread->Sync)
			Application->ProcessMessages();
		else
			Sleep(1);

		if (m_rx_packet_queue.empty())
			continue;

		// fetch the rx'ed packet
		const int      rx_data_size = m_rx_packet_queue[0].size();
		const uint8_t *rx_data      = &m_rx_packet_queue[0][0];

		if (m_verbose > 2)
			hdump(rx_data, rx_data_size);

		if (rx_data_size < 6)
		{
			CCriticalSection cs(m_thread_cs);
			m_rx_packet_queue[0].clear();
			m_rx_packet_queue.erase(m_rx_packet_queue.begin() + 0);
			continue;
		}

		if (rx_data[0] != 0x1E ||
			 rx_data[1] != 0x05 ||
			 rx_data[4] != buffer[4] ||
			 rx_data[5] != buffer[5])
		{
			CCriticalSection cs(m_thread_cs);
			m_rx_packet_queue[0].clear();
			m_rx_packet_queue.erase(m_rx_packet_queue.begin() + 0);
			continue;
		}

		return 1;
	}

	if (m_verbose > 1)
		Memo1->Lines->Add("error: k5_write_eeprom() no valid packet received");

	return 0;
}

int __fastcall TForm1::wait_flash_message()
{
	if (!m_serial.port.connected)
		return 0;

	m_bootloader_ver = "";

	// delete any previously rx'ed/saved packets
	clearRxPacketQueue();

	// wait for a rx'ed packet
	const DWORD tick = GetTickCount();
	while ((GetTickCount() - tick) <= 4000)
	{
		if (m_thread->Sync)
			Application->ProcessMessages();
		else
			Sleep(1);

		if (m_rx_packet_queue.empty())
			continue;

		// fetch the rx'ed packet
		const int      rx_data_size = m_rx_packet_queue[0].size();
		const uint8_t *rx_data      = &m_rx_packet_queue[0][0];

		if (m_verbose > 2)
			hdump(rx_data, rx_data_size);

		if (rx_data_size < 36)
		{
			CCriticalSection cs(m_thread_cs);
			m_rx_packet_queue[0].clear();
			m_rx_packet_queue.erase(m_rx_packet_queue.begin() + 0);
			continue;
		}

		//  0x000000: 18 05 20 00 01 02 02 06 1c 53 50 4a 37 47 ff 0f   .. ......SPJ7G..
		//  0x000010: 8c 00 53 00 32 2e 30 30 2e 30 36 00 34 0a 00 00   ..S.2.00.06.4...
		//  0x000020: 00 00 00 20                                       ...

		if (rx_data[0] != 0x18 ||
			 rx_data[1] != 0x05 ||
			 rx_data[2] != 32 ||
			 rx_data[3] != 0x00 ||
			 rx_data[4] != 0x01 ||
			 rx_data[5] != 0x02 ||
			 rx_data[6] != 0x02)
		{
			CCriticalSection cs(m_thread_cs);
			m_rx_packet_queue[0].clear();
			m_rx_packet_queue.erase(m_rx_packet_queue.begin() + 0);
			continue;
		}

		char buf[17];
		memset(buf, 0, sizeof(buf));

		for (int i = 0; i < ((int)sizeof(buf) - 1); i++)
		{
			const int k = i + 20;
			if (k >= rx_data_size)
				break;

			const char c = rx_data[k];
			if (!isprint(c))
				break;
			buf[i] = c;
		}

		m_bootloader_ver = String(buf);

		Memo1->Lines->Add("");
		Memo1->Lines->Add("Bootloader version '" + m_bootloader_ver + "'");

		return 1;
	}

	if (m_verbose > 1)
		Memo1->Lines->Add("error: wait_flash_message() no valid packet received");

	return 0;
}

int __fastcall TForm1::k5_send_flash_version_message(const char *ver)
{
	uint8_t buffer[4 + 16 + 1];

	if (!m_serial.port.connected || ver == NULL || strlen(ver) > 16)
		return 0;

	memset(buffer, 0, sizeof(buffer));

	buffer[0] = 0x30;
	buffer[1] = 0x05;
	buffer[2] = 16;
	buffer[3] = 0x00;

	strcpy(buffer + 4, ver);

	const int r = k5_send_buf(buffer, 4 + 16);
	if (r <= 0)
		return 0;

	// wait for a rx'ed packet
	const DWORD tick = GetTickCount();
	while ((GetTickCount() - tick) <= 2000)
	{
		if (m_thread->Sync)
			Application->ProcessMessages();
		else
			Sleep(1);

		if (m_rx_packet_queue.empty())
			continue;

		// fetch the rx'ed packet
		const int      rx_data_size = m_rx_packet_queue[0].size();
		const uint8_t *rx_data      = &m_rx_packet_queue[0][0];

		if (m_verbose > 2)
			hdump(rx_data, rx_data_size);

		if (rx_data_size < 36)
		{
			CCriticalSection cs(m_thread_cs);
			m_rx_packet_queue[0].clear();
			m_rx_packet_queue.erase(m_rx_packet_queue.begin() + 0);
			continue;
		}

		//  0x000000: 18 05 20 00 01 02 02 06 1c 53 50 4a 37 47 ff 0f   .. ......SPJ7G..
		//  0x000010: 8c 00 53 00 32 2e 30 30 2e 30 36 00 34 0a 00 00   ..S.2.00.06.4...
		//  0x000020: 00 00 00 20                                       ...

		if (rx_data[0] != 0x18 ||
			 rx_data[1] != 0x05 ||
			 rx_data[2] != 32 ||
			 rx_data[3] != 0x00 ||
			 rx_data[4] != 0x01 ||
			 rx_data[5] != 0x02 ||
			 rx_data[6] != 0x02)
		{
			CCriticalSection cs(m_thread_cs);
			m_rx_packet_queue[0].clear();
			m_rx_packet_queue.erase(m_rx_packet_queue.begin() + 0);
			continue;
		}

		Memo1->Lines->Add("");
		Memo1->Lines->Add("sent firmware version '" + String((char *)buffer + 4) + "'");

		return 1;
	}

	if (m_verbose > 1)
		Memo1->Lines->Add("error: k5_send_flash_version_message() no valid packet received");

	return 0;
}
/*
int __fastcall TForm1::k5_readflash(uint8_t *buf, const int len, const int offset)
{
	String s;
	uint8_t buffer[16];
	int ok = 0;
	int r;

	if (buf == NULL || len <= 0 || len > 256)
		return 0;

	if (m_verbose > 1)
	{
		s.printf("read_flash offset %04X  len %d", offset, len);
		Memo1->Lines->Add(s);
	}

	memset(buffer, 0, sizeof(buffer));

	// 0x19 0x05 0x0c 0x01 0x8a 0x8d 0x9f 0x1d
	buffer[ 0] = 0x17;
	buffer[ 1] = 0x05;
//	buffer[ 2] = 0x0C;      // bytes 2, 3: length is 0x010C (12 + 256)
//	buffer[ 3] = 0x01;




}
*/
int __fastcall TForm1::k5_write_flash(const uint8_t *buf, const int len, const int offset, const int firmware_size)
{
	String s;
	uint8_t buffer[16 + 256];

	if (!m_serial.port.connected || buf == NULL || len <= 0 || len > 256)
		return 0;

	if (m_verbose > 1)
	{
		s.printf("write_flash  offset %04X  len %d", offset, len);
		Memo1->Lines->Add(s);
		Memo1->Update();
	}

	memset(buffer, 0, sizeof(buffer));

	buffer[ 0] = 0x19;
	buffer[ 1] = 0x05;
	buffer[ 2] = 0x0C;      // bytes 2, 3: length is 0x010C (256 + 12)
	buffer[ 3] = 0x01;

	buffer[ 4] = 0x8A;
	buffer[ 5] = 0x8D;
	buffer[ 6] = 0x9F;
	buffer[ 7] = 0x1D;

	buffer[ 8] = (offset >> 8) & 0xff;
	buffer[ 9] = (offset >> 0) & 0xff;
	buffer[10] = 0xF0;
//	buffer[10] = ((firmware_size + 255) >> 8) & 0xff;
	buffer[11] = 0x00;

	buffer[12] = (len >> 8) & 0xff;
	buffer[13] = (len >> 0) & 0xff;
	buffer[14] = 0x00;
	buffer[15] = 0x00;

	// add up to 256 bytes of data
	memcpy(&buffer[16], buf, len);

	int r = k5_send_buf(buffer, sizeof(buffer));
	if (r <= 0)
		return 0;

	// wait for a rx'ed packet
	const DWORD tick = GetTickCount();
	while ((GetTickCount() - tick) <= 3000)
	{
		if (m_thread->Sync)
			Application->ProcessMessages();
		else
			Sleep(1);

		if (m_rx_packet_queue.empty())
			continue;

		// fetch the 1st packet in the queue
		const int      rx_data_size = m_rx_packet_queue[0].size();
		const uint8_t *rx_data      = &m_rx_packet_queue[0][0];

		if (m_verbose > 2)
			hdump(rx_data, rx_data_size);

		if (rx_data_size < 12)
		{
			CCriticalSection cs(m_thread_cs);
			m_rx_packet_queue[0].clear();
			m_rx_packet_queue.erase(m_rx_packet_queue.begin() + 0);
			continue;
		}

		// good replies:
		// 1A 05 08 00 8A 8D 9F 1D 00 00 00 00
		// 1A 05 08 00 8A 8D 9F 1D 01 00 00 00
		// 1A 05 08 00 8A 8D 9F 1D 02 00 00 00

		// error replies:
		// 1A 05 08 00 00 00 00 00 00 00 01 00
		// 1A 05 08 00 00 00 00 00 00 00 01 00
		// 1A 05 08 00 00 00 00 00 00 00 01 00

		if (rx_data[0] != 0x1A ||
			 rx_data[1] != 0x05 ||
			 rx_data[2] != 8 ||
			 rx_data[3] != 0x00 ||
			 rx_data[4] != buffer[4] ||
			 rx_data[5] != buffer[5] ||
			 rx_data[6] != buffer[6] ||
			 rx_data[7] != buffer[7] ||
			 rx_data[8] != buffer[8] ||
			 rx_data[9] != buffer[9])
		{
			CCriticalSection cs(m_thread_cs);
			m_rx_packet_queue[0].clear();
			m_rx_packet_queue.erase(m_rx_packet_queue.begin() + 0);
			continue;
		}

		return 1;
	}

	if (m_verbose > 1)
		Memo1->Lines->Add("error: k5_write_flash() no valid packet received");

	return 0;
}

int __fastcall TForm1::k5_prepare(const int retry)
{
	String s;
	uint8_t buffer[8];

	if (!m_serial.port.connected)
		return 0;

	m_firmware_ver = "";

	// 'hello' packet
	buffer[0] = 0x14;
	buffer[1] = 0x05;
	buffer[2] = 4;
	buffer[3] = 0x00;

	memcpy(&buffer[4], session_id, 4);

	if (m_verbose > 0)
	{
		if (retry >= 0)
			s.printf("k5_prepare [%d]", retry);
		else
			s.printf("k5_prepare");
		Memo1->Lines->Add("");
		Memo1->Lines->Add(s);
	}

	const int r = k5_send_buf(buffer, sizeof(buffer));
	if (r <= 0)
		return 0;

	// wait for a rx'ed packet
	const DWORD tick = GetTickCount();
	while ((GetTickCount() - tick) <= 1000)
	{
		if (m_thread->Sync)
			Application->ProcessMessages();
		else
			Sleep(1);

		if (m_rx_packet_queue.empty())
			continue;

		// fetch the rx'ed packet
		const int      rx_data_size = m_rx_packet_queue[0].size();
		const uint8_t *rx_data      = &m_rx_packet_queue[0][0];

		if (m_verbose > 2)
			hdump(rx_data, rx_data_size);

		if (rx_data[0] == 0x18 && rx_data[1] == 0x05)
		{	// radio is in firmware mode
			return -1;
		}

		if (rx_data[0] != 0x15 || rx_data[1] != 0x05)
		{
			CCriticalSection cs(m_thread_cs);
			m_rx_packet_queue[0].clear();
			m_rx_packet_queue.erase(m_rx_packet_queue.begin() + 0);
			continue;
		}

		if (rx_data_size < (4 + 16))
		{
			CCriticalSection cs(m_thread_cs);
			m_rx_packet_queue[0].clear();
			m_rx_packet_queue.erase(m_rx_packet_queue.begin() + 0);
			continue;
		}

		char buf[17] = {0};
		memcpy(buf, &rx_data[4], 16);
		m_firmware_ver = String(buf);

		Memo1->Lines->Add("");
		Memo1->Lines->Add("firmware version '" + m_firmware_ver + "'");

		return 1;
	}

	if (m_verbose > 1)
		Memo1->Lines->Add("error: k5_prepare() no valid packet received");

	return 0;
}

int __fastcall TForm1::k5_reboot()
{
	uint8_t buffer[8];

	if (!m_serial.port.connected)
		return 0;

	buffer[0] = 0xDD;
	buffer[1] = 0x05;
	buffer[2] = 0;
	buffer[3] = 0x00;

	if (m_verbose > 0)
	{
		Memo1->Lines->Add("");
		Memo1->Lines->Add("* rebooting radio");
	}

	return k5_send_buf(buffer, sizeof(buffer));
}

void __fastcall TForm1::VerboseTrackBarChange(TObject *Sender)
{
	m_verbose = VerboseTrackBar->Position;
}

void __fastcall TForm1::ReadEEPROMButtonClick(TObject *Sender)
{
	String  s;
	uint8_t eeprom[UVK5_MAX_EEPROM_SIZE];

	disconnect();

	if (!connect())
		return;

	m_firmware_ver = "";

	int r = 0;
	for (int i = 0; i < UVK5_PREPARE_TRIES; i++)
	{
		r = k5_prepare(i);
		if (r != 0)
			break;
	}
	Memo1->Lines->Add("");
	if (r < 0)
	{
		disconnect();
		Memo1->Lines->Add("");
		Memo1->Lines->Add("error: radio is in firmware mode - turn the radio off, then back on whilst NOT pressing the PTT");
		return;
	}
	if (r == 0)
	{	// no valid reply
		Memo1->Lines->Add("");
		disconnect();
		return;
	}

	const int verbose = m_verbose;
	if (m_verbose > 1)
		m_verbose = 1;

	const int size      = sizeof(eeprom);
	const int block_len = UVK5_EEPROM_BLOCKSIZE;

	CGauge1->MaxValue = size;
	CGauge1->Progress = 0;
	CGauge1->Update();

	for (unsigned int i = 0; i < size; i += block_len)
	{
		if (m_verbose > 0)
		{
			s.printf("read ERPROM  %04X  len %3d  %3d%%", i, block_len, (100 * (i + block_len)) / size);
			Memo1->Lines->Add(s);
			Memo1->Update();
		}

		r = k5_read_eeprom(&eeprom[i], block_len, i);
		if (r <= 0)
		{
			m_verbose = verbose;
			s.printf("error: k5_read_eeprom() [%d]", r);
			Memo1->Lines->Add(s);
			Memo1->Lines->Add("");
			disconnect();
			return;
		}

		CGauge1->Progress = i + block_len;
		CGauge1->Update();
	}

	Memo1->Lines->Add("read EEPROM complete");
	Memo1->Lines->Add("");

	m_verbose = verbose;

	disconnect();

	Memo1->Lines->Add("");
	Memo1->Update();

	if (m_verbose > 2)
	{
		Memo1->Lines->BeginUpdate();
		hdump(&eeprom[0], size);
		Memo1->Lines->EndUpdate();
		Memo1->Lines->Add("");
		Memo1->Update();
	}

	// *****************

	Application->BringToFront();
	Application->NormalizeTopMosts();
	SaveDialog1->Title = "Select EEPROM filename to save too";
	const bool ok = SaveDialog1->Execute();
	Application->RestoreTopMosts();
	if (!ok)
		return;

	String name = SaveDialog1->FileName;
	String ext  = ExtractFileExt(name).LowerCase();
	if (ext.IsEmpty())
	{
		ext = ".bin";
		name += ext;
	}

	saveFile(name, &eeprom[0], size);
}

void __fastcall TForm1::WriteFirmwareButtonClick(TObject *Sender)
{
	String s;
	uint8_t flash[UVK5_MAX_FLASH_SIZE];

	disconnect();

	Application->BringToFront();
	Application->NormalizeTopMosts();
	OpenDialog1->Title = "Select a FIRMWARE file to load";
	const bool ok = OpenDialog1->Execute();
	Application->RestoreTopMosts();
	if (!ok)
		return;

	String name = OpenDialog1->FileName;
	String ext  = ExtractFileExt(name).LowerCase();
	if (ext.IsEmpty())
	{
		ext = ".bin";
		name += ext;
	}

	if (loadFile(name) == 0)
	{
		Application->BringToFront();
		Application->NormalizeTopMosts();
		Application->MessageBox("No data loaded", Application->Title.c_str(), MB_ICONERROR | MB_OK);
		Application->RestoreTopMosts();
		return;
	}

	m_bootloader_ver = "";
	m_firmware_ver   = "";

	s.printf("Loaded %u bytes from '%s'", m_loadfile_data.size(), m_loadfile_name.c_str());
	Memo1->Lines->Add("");
	Memo1->Lines->Add(s);

	if (m_loadfile_data.size() < 1000)
	{
		Application->BringToFront();
		Application->NormalizeTopMosts();
		Application->MessageBox("File appears to be to small to be a firmware file", Application->Title.c_str(), MB_ICONERROR | MB_OK);
		Application->RestoreTopMosts();
		return;
	}

	bool encrypted = true;

	const uint16_t crc1 = crc16xmodem(&m_loadfile_data[0], m_loadfile_data.size() - 2);
	const uint16_t crc2 = ((uint16_t)m_loadfile_data[m_loadfile_data.size() - 1] << 8) | ((uint16_t)m_loadfile_data[m_loadfile_data.size() - 2] << 0);

	if (m_loadfile_data[0] == 0x88 && m_loadfile_data[1] == 0x13 && m_loadfile_data[2] == 0x00 && m_loadfile_data[3] == 0x20)
		encrypted = false;
	else
	if (m_loadfile_data[0] == 0x88 && m_loadfile_data[1] == 0x11 && m_loadfile_data[2] == 0x00 && m_loadfile_data[3] == 0x20)
		encrypted = false;
	else
	if (m_loadfile_data[0] == 0xF0 && m_loadfile_data[1] == 0x3F && m_loadfile_data[2] == 0x00 && m_loadfile_data[3] == 0x20)
		encrypted = false;

	if (encrypted && crc1 == crc2)
	{	// the file appears to be encrypted

		// drop the 16-bit CRC
		m_loadfile_data.resize(m_loadfile_data.size() - 2);

		// decrypt it
		xor_firmware(&m_loadfile_data[0], m_loadfile_data.size());

		if (m_loadfile_data[0] == 0x88 && m_loadfile_data[1] == 0x13 && m_loadfile_data[2] == 0x00 && m_loadfile_data[3] == 0x20)
			encrypted = false;
		else
		if (m_loadfile_data[0] == 0x88 && m_loadfile_data[1] == 0x11 && m_loadfile_data[2] == 0x00 && m_loadfile_data[3] == 0x20)
			encrypted = false;
		else
		if (m_loadfile_data[0] == 0xF0 && m_loadfile_data[1] == 0x3F && m_loadfile_data[2] == 0x00 && m_loadfile_data[3] == 0x20)
			encrypted = false;

		if (!encrypted && m_loadfile_data.size() >= (0x2000 + 16))
		{	// extract and remove the 16-byte version string

			char firmware_ver[17] = {0};
			memcpy(firmware_ver, &m_loadfile_data[0x2000], 16);

			if (m_loadfile_data.size() > (0x2000 + 16))
				memmove(&m_loadfile_data[0x2000], &m_loadfile_data[0x2000 + 16], m_loadfile_data.size() - 0x2000 - 16);
			m_loadfile_data.resize(m_loadfile_data.size() - 16);

			m_firmware_ver = String(firmware_ver);

			Memo1->Lines->Add("decrypted version '" + m_firmware_ver + "'");
		}
	}

	if (encrypted)
	{
		Application->BringToFront();
		Application->NormalizeTopMosts();
		Application->MessageBox("File appears to be encrypted", Application->Title.c_str(), MB_ICONERROR | MB_OK);
		Application->RestoreTopMosts();
		return;
	}

	if (m_loadfile_data.size() > UVK5_MAX_FLASH_SIZE)
	{
		Application->BringToFront();
		Application->NormalizeTopMosts();
		Application->MessageBox("File is to large to be a firmware file (max 65536)", Application->Title.c_str(), MB_ICONERROR | MB_OK);
		Application->RestoreTopMosts();
		return;
	}

	if (m_loadfile_data.size() > UVK5_FLASH_SIZE)
	{
		#if 0
			Application->BringToFront();
			Application->NormalizeTopMosts();
			const int res = Application->MessageBox("File runs into bootloader area, continue ?", Application->Title.c_str(), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2);
			Application->RestoreTopMosts();
			switch (res)
			{
				case IDYES:
					break;
				case IDNO:
				case IDCANCEL:
					return;
			}
		#else
			Application->BringToFront();
			Application->NormalizeTopMosts();
			Application->MessageBox("File runs into bootloader area. Upload currently prevented.", Application->Title.c_str(), MB_ICONERROR | MB_OK);
			Application->RestoreTopMosts();
			return;
		#endif
	}

	if (m_verbose > 2)
	{
		Memo1->Lines->BeginUpdate();
		hdump(&m_loadfile_data[0], m_loadfile_data.size());
		Memo1->Lines->EndUpdate();
		Memo1->Lines->Add("");
	}

	if (!connect(false))
		return;

	int r = 0;
	for (int i = 0; i < UVK5_PREPARE_TRIES; i++)
	{
		r = k5_prepare(-1);
		if (r != 0)
			break;
	}
	Memo1->Lines->Add("");
	if (r >= 0)
	{
		disconnect();
		Memo1->Lines->Add("");
		Memo1->Lines->Add("error: radio is not in firmware mode - turn the radio off, then back on whilst pressing the PTT");
		return;
	}

	// overcome version problems
	// the radios bootloader can refuse the chosen firmware version, so fool the booloader if so
	if (m_firmware_ver.Length() >= 3 && m_bootloader_ver.Length() >= 3)
		if (m_firmware_ver[1] > m_bootloader_ver[1])
			m_firmware_ver[1] = '*';

	r = k5_send_flash_version_message(!m_firmware_ver.IsEmpty() ? m_firmware_ver.c_str() : "2.01.26");
	if (r <= 0)
	{
		Memo1->Lines->Add("error: send firmware version");
		Memo1->Lines->Add("");
		disconnect();
		return;
	}

	Memo1->Lines->Add("");

	const int verbose = m_verbose;
	if (m_verbose > 1)
		m_verbose = 1;

	CGauge1->MaxValue = m_loadfile_data.size();
	CGauge1->Progress = 0;
	CGauge1->Update();

	for (int i = 0; i < (int)m_loadfile_data.size(); i += UVK5_FLASH_BLOCKSIZE)
	{
		int len = (int)m_loadfile_data.size() - i;
		if (len > UVK5_FLASH_BLOCKSIZE)
			 len = UVK5_FLASH_BLOCKSIZE;

		if (m_verbose > 0)
		{
			s.printf("write FLASH  %04X  len %3d   %3d%%", i, len, (100 * (i + len)) / m_loadfile_data.size());
			Memo1->Lines->Add(s);
			Memo1->Update();
		}

		r = k5_write_flash(&m_loadfile_data[i], len, i, m_loadfile_data.size());
		if (r <= 0)
		{
			s.printf("error: k5_write_flash() [%d]", r);
			Memo1->Lines->Add(s);
			Memo1->Lines->Add("");
			m_verbose = verbose;
			disconnect();
			return;
		}

		CGauge1->Progress = i + len;
		CGauge1->Update();
	}

	Memo1->Lines->Add("write FLASH complete");
	Memo1->Update();

	m_verbose = verbose;

	k5_reboot();

	// give the serial port time to complete the data TX
	Sleep(50);

	Memo1->Lines->Add("");
	disconnect();
}

void __fastcall TForm1::WriteEEPROMButtonClick(TObject *Sender)
{
	String s;

	disconnect();

	Memo1->Lines->Add("");

	Application->BringToFront();
	Application->NormalizeTopMosts();
	OpenDialog1->Title = "Select an EEPROM file to load";
	const bool ok = OpenDialog1->Execute();
	Application->RestoreTopMosts();
	if (!ok)
		return;

	String name = OpenDialog1->FileName;
	String ext  = ExtractFileExt(name).LowerCase();
	if (ext.IsEmpty())
	{
		ext = ".bin";
		name += ext;
	}

	if (loadFile(name) == 0)
	{
		Application->BringToFront();
		Application->NormalizeTopMosts();
		Application->MessageBox("No data loaded", Application->Title.c_str(), MB_ICONERROR | MB_OK);
		Application->RestoreTopMosts();
		return;
	}

	s.printf("Loaded %u bytes from '%s'", m_loadfile_data.size(), m_loadfile_name.c_str());
	Memo1->Lines->Add(s);

	if (m_loadfile_data.size() <= 1000)
	{
		Application->BringToFront();
		Application->NormalizeTopMosts();
		Application->MessageBox("File appears to be to small to be an eeprom file", Application->Title.c_str(), MB_ICONERROR | MB_OK);
		Application->RestoreTopMosts();
		return;
	}

	if (m_loadfile_data.size() > UVK5_MAX_EEPROM_SIZE)
	{
		Application->BringToFront();
		Application->NormalizeTopMosts();
		Application->MessageBox("File is to large to be an eeprom file", Application->Title.c_str(), MB_ICONERROR | MB_OK);
		Application->RestoreTopMosts();
		return;
	}

	if (m_loadfile_data.size() > UVK5_EEPROM_SIZE)
	{
		Application->BringToFront();
		Application->NormalizeTopMosts();
		const int res = Application->MessageBox("File is larger than normal, continue ?", Application->Title.c_str(), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2);
		Application->RestoreTopMosts();
		switch (res)
		{
			case IDYES:
				break;
			case IDNO:
			case IDCANCEL:
				return;
		}
	}

	if (!connect())
		return;

	int r = 0;
	for (int i = 0; i < UVK5_PREPARE_TRIES; i++)
	{
		r = k5_prepare(i);
		if (r != 0)
			break;
	}
	Memo1->Lines->Add("");
	if (r < 0)
	{
		disconnect();
		Memo1->Lines->Add("");
		Memo1->Lines->Add("error: radio is in firmware mode - turn the radio off, then back on whilst NOT pressing the PTT");
		return;
	}
	if (r == 0)
	{
		disconnect();
		return;
	}

	if (m_verbose > 2)
	{
		Memo1->Lines->BeginUpdate();
		hdump(&m_loadfile_data[0], m_loadfile_data.size());
		Memo1->Lines->EndUpdate();
		Memo1->Lines->Add("");
	}

	Memo1->Lines->Add("writing eeprom area ..");

	int size = 0;
//	while (uvk5_writes[size][1] > 0)
//		size++;

	size = m_loadfile_data.size();

	const int verbose = m_verbose;
	if (m_verbose > 1)
		m_verbose = 1;

	CGauge1->MaxValue = size;
	CGauge1->Progress = 0;
	CGauge1->Update();

	for (int i = 0; i < size; i += UVK5_EEPROM_BLOCKSIZE)
	{
//		const int addr = uvk5_writes[i][0];
//		const int len  = uvk5_writes[i][1];
//		if (len <= 0)
//			break;

		const int addr = i;
		const int len  = UVK5_EEPROM_BLOCKSIZE;

		if (m_verbose > 0)
		{
			s.printf("write EEPROM  %04X  len %3d   %3d%%", addr, len, (100 * (i + len)) / size);
			Memo1->Lines->Add(s);
			Memo1->Update();
		}

		const int r = k5_write_eeprom(&m_loadfile_data[addr], len, addr);
		if (r <= 0)
		{
			s.printf("error: k5_write_eeprom() [%d]", r);
			Memo1->Lines->Add(s);
			Memo1->Lines->Add("");
			Memo1->Update();
			m_verbose = verbose;
			disconnect();
			return;
		}

		CGauge1->Progress = i + len;
		CGauge1->Update();
	}

	Memo1->Lines->Add("write EEPROM complete");
	Memo1->Update();

	m_verbose = verbose;

	k5_reboot();

	Sleep(50);

	disconnect();
}

void __fastcall TForm1::SerialPortComboBoxChange(TObject *Sender)
{
	ReadEEPROMButton->Enabled    = SerialPortComboBox->ItemIndex > 0;
	WriteEEPROMButton->Enabled   = SerialPortComboBox->ItemIndex > 0;
	WriteFirmwareButton->Enabled = SerialPortComboBox->ItemIndex > 0;
}

