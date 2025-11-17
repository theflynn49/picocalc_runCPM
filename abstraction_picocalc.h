#ifndef ABSTRACT_H
#define ABSTRACT_H

#include <ctype.h>
#include <errno.h>
#include <glob.h>
#include <libgen.h>
// #include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
// #include <termios.h>
#include <time.h>
#include <unistd.h>
#define millis() clock() / 1000

#include "drivers/picocalc.h"
#include "drivers/lcd.h"
#include "drivers/clib.c"
#include "drivers/keyboard.h"
#include "drivers/sdcard.h"
#include "drivers/fat32.h"
#include "drivers/onboard_led.h"
#include "drivers/display.h"
#include "drivers/audio.h"

bool power_off_requested = false;
volatile bool user_interrupt = false ;
volatile bool user_freeze = false ;

fat32_file_t fat_dir;
char fat_dir_fullpath[6] ;
bool fat_dir_open = false ;
extern int long last_seek ;

#define HostOS 0x02

void set_onboard_led(uint8_t led)
{
    led_set(led & 0x01);
}

/* Filesystem (disk) abstraction functions */
/*===============================================================================*/
#define FOLDERCHAR '/'
#define FILEBASE "/"

typedef struct {
    uint8 dr;
    uint8 fn[8];
    uint8 tp[3];
    uint8 ex, s1, s2, rc;
    uint8 al[16];
    uint8 cr, r0, r1, r2;
} CPM_FCB;

typedef struct {
    uint8 dr;
    uint8 fn[8];
    uint8 tp[3];
    uint8 ex, s1, s2, rc;
    uint8 al[16];
} CPM_DIRENTRY;

uint8 sd_mkdir_filename(const char *dirname)
{
    fat32_file_t dir;
    if (dirname == NULL || strlen(dirname) == 0) return 1 ;
    fat32_error_t result = fat32_dir_create(&dir, dirname);
    if (result != FAT32_OK)
    {
        // printf("Error: %s\n", fat32_error_string(result));
        return 1 ;
    }
    fat32_close(&dir);
    return 0 ;
}

uint8 _sys_exists(uint8 *filename) {

    uint8 fullpath[128] = FILEBASE;
    strcat((char *)fullpath, (char *)filename);
    return (!access((const char *)fullpath, F_OK));
}

FILE *_sys_fopen_r(uint8 *filename) {
    uint8 fullpath[128] = FILEBASE;
    strcat((char *)fullpath, (char *)filename);
    return (fopen((const char *)fullpath, "rb"));
}

FILE *_sys_fopen_w(uint8 *filename) {
    uint8 fullpath[128] = FILEBASE;
    strcat((char *)fullpath, (char *)filename);
    return (fopen((const char *)fullpath, "wb"));
}

FILE *_sys_fopen_rw(uint8 *filename) {
    uint8 fullpath[128] = FILEBASE;
    strcat((char *)fullpath, (char *)filename);
    return (fopen((const char *)fullpath, "r+b"));
}

FILE *_sys_fopen_a(uint8 *filename) {
    uint8 fullpath[128] = FILEBASE;
    strcat((char *)fullpath, (char *)filename);
    return (fopen((const char *)fullpath, "a"));
}

int _sys_fseek(FILE *file, long delta, int origin) {
    return (fseek(file, delta, origin));
}

long _sys_ftell(FILE *file) {
    return (ftell(file));
}

long _sys_fread(void *buffer, long size, long count, FILE *file) {
    return (fread(buffer, size, count, file));
}

long _sys_fwrite(const void *buffer, long size, long count, FILE *file) {
    return (fwrite(buffer, size, count, file));
}

int _sys_fputc(int ch, FILE *file) {
    return (fputc(ch, file));
}

int _sys_feof(FILE *file) {
    return (feof(file));
}

int _sys_fflush(FILE *file) {
    return (fflush(file));
}

int _sys_fclose(FILE *file) {
    return (fclose(file));
}

int _sys_remove(uint8 *filename) {
    uint8 fullpath[128] = FILEBASE;
    strcat((char *)fullpath, (char *)filename);
    return (remove((const char *)fullpath));
}

int _sys_rename(uint8 *name1, uint8 *name2) {
    uint8 fullpath1[128] = FILEBASE;
    strcat((char *)fullpath1, (char *)name1);
    uint8 fullpath2[128] = FILEBASE;
    strcat((char *)fullpath2, (char *)name2);
    return (rename((const char *)fullpath1, (const char *)fullpath2));
}

int _sys_select(uint8 *disk) {
    struct stat st;
    uint8 fullpath[128] = FILEBASE;
    strcat((char *)fullpath, (char *)disk);
    return ((stat((char *)fullpath, &st) == 0) && S_ISDIR(st.st_mode));
}

long _sys_filesize(uint8 *filename) {
    long l = -1;
    FILE *file = _sys_fopen_r(filename);
    if (file != NULL) {
        _sys_fseek(file, 0, SEEK_END) ;
        l = _sys_ftell(file);
        _sys_fclose(file);
    }
    return (l);
}

int _sys_openfile(uint8 *filename) {
    FILE *file = _sys_fopen_r(filename);
    if (file != NULL)
        _sys_fclose(file);
    return (file != NULL);
}

int _sys_makefile(uint8 *filename) {
    FILE *file = _sys_fopen_a(filename);
    if (file != NULL)
        _sys_fclose(file);
    return (file != NULL);
}

int _sys_deletefile(uint8 *filename) {
    return (!_sys_remove(filename));
}

int _sys_renamefile(uint8 *filename, uint8 *newname) {
    return (!_sys_rename(&filename[0], &newname[0]));
}

#ifdef DEBUGLOG
void _sys_logbuffer(uint8 *buffer) {
    FILE *file;
    #ifdef CONSOLELOG
    puts((char *)buffer);
    #else
    uint8 s = 0;
    while (*(buffer + s)) // Computes buffer size
        ++s;
    file = _sys_fopen_a((uint8 *)LogName);
    _sys_fwrite(buffer, 1, s, file);
    _sys_fclose(file);
    #endif
}
#endif

uint8 _sys_readseq(uint8 *filename, long fpos) {
    uint8 result = 0xff;
    uint8 bytesread;
    uint8 dmabuf[128];
    uint8 i;

    FILE *file = _sys_fopen_r(&filename[0]);
    if (file != NULL) {
        if (!_sys_fseek(file, fpos, 0)) {
            for (i = 0; i < 128; ++i)
                dmabuf[i] = 0x1a;
            bytesread = (uint8)_sys_fread(&dmabuf[0], 1, 128, file);
            if (bytesread) {
                for (i = 0; i < 128; ++i)
                    _RamWrite(dmaAddr + i, dmabuf[i]);
            }
            result = bytesread ? 0x00 : 0x01;
        } else {
            result = 0x01;
        }
        _sys_fclose(file);
    } else {
        result = 0x10;
    }

    return (result);
}

uint8 _sys_writeseq(uint8 *filename, long fpos) {
    uint8 result = 0xff;

    FILE *file = _sys_fopen_rw(&filename[0]);
    if (file != NULL) {
        if (!_sys_fseek(file, fpos, 0)) {
            if (_sys_fwrite(_RamSysAddr(dmaAddr), 1, 128, file))
                result = 0x00;
        } else {
            result = 0x01;
        }
        _sys_fclose(file);
    } else {
        result = 0x10;
    }

    return (result);
}

uint8 _sys_readrand(uint8 *filename, long fpos) {
    uint8 result = 0xff;
    uint8 bytesread;
    uint8 dmabuf[128];
    uint8 i;
    long extSize;

    FILE *file = _sys_fopen_r(&filename[0]);
    if (file != NULL) {
        if (!_sys_fseek(file, fpos, 0)) {
            for (i = 0; i < 128; ++i)
                dmabuf[i] = 0x1a;
            bytesread = (uint8)_sys_fread(&dmabuf[0], 1, 128, file);
            if (bytesread) {
                for (i = 0; i < 128; ++i)
                    _RamWrite(dmaAddr + i, dmabuf[i]);
            }
            result = bytesread ? 0x00 : 0x01;
        } else {
            if (fpos >= 65536L * 128) {
                result = 0x06; // seek past 8MB (largest file size in CP/M)
            } else {
                _sys_fseek(file, 0, SEEK_END);
                extSize = _sys_ftell(file);
                // round file size up to next full logical extent
                extSize = 16384 * ((extSize / 16384) + ((extSize % 16384) ? 1 : 0));
                if (fpos < extSize)
                    result = 0x01; // reading unwritten data
                else
                    result = 0x04; // seek to unwritten extent
            }
        }
        _sys_fclose(file);
    } else {
        result = 0x10;
    }

    return (result);
}

uint8 _sys_writerand(uint8 *filename, long fpos) {
    uint8 result = 0xff;

    FILE *file = _sys_fopen_rw(&filename[0]);
    if (file != NULL) {
        if (!_sys_fseek(file, fpos, 0)) {
            if (_sys_fwrite(_RamSysAddr(dmaAddr), 1, 128, file))
                result = 0x00;
        } else {
            result = 0x06;
        }
        _sys_fclose(file);
    } else {
        result = 0x10;
    }

    return (result);
}

uint8 _Truncate(char *fn, uint8 rc) {
// CP/M doesn't support truncate
	printf("Err: no truncate\n") ;
/*
    uint8 result = 0x00;
    uint8 fullpath[128] = FILEBASE;
    strcat((char *)fullpath, (char *)fn);
    if (truncate((char *)fullpath, rc * 128))
        result = 0xff;
    return (result);
*/    
    return 1 ;
}

void _MakeUserDir() {
    uint8 dFolder = cDrive + 'A';
    uint8 uFolder = toupper(tohex(userCode));

    uint8 path[4] = {dFolder, FOLDERCHAR, uFolder, 0};
    uint8 fullpath[128] = FILEBASE;
    strcat((char *)fullpath, (char *)path);

    sd_mkdir_filename((char *)fullpath);
}

uint8 _sys_makedisk(uint8 drive) {
    uint8 result = 0;
    if (drive < 1 || drive > 16) {
        result = 0xff;
    } else {
        uint8 dFolder = drive + '@';
        uint8 disk[2] = {dFolder, 0};
        uint8 fullpath1[128] = FILEBASE;
        strcat((char *)fullpath1, (char *)disk);
        if (sd_mkdir_filename((char *)fullpath1)) {
            result = 0xfe;
        } else {
            uint8 path[4] = {dFolder, FOLDERCHAR, '0', 0};
            uint8 fullpath2[128] = FILEBASE;
            strcat((char *)fullpath2, (char *)path);
            if (sd_mkdir_filename((char *)fullpath2))
		    result = 0xfe ;
        }
    }
    return (result);
}

#ifndef POLLRDBAND
    #define POLLRDBAND 0
#endif
#ifndef POLLRDNORM
    #define POLLRDNORM 0
#endif

/* Memory abstraction functions */
/*===============================================================================*/
uint16 _RamLoad(uint8 *filename, uint16 address, uint16 maxsize) {
    long l=0L;
    FILE *file = _sys_fopen_r(filename);
    _sys_fseek(file, 0, SEEK_END);
    l = _sys_ftell(file);
    if (maxsize && l > maxsize)
        l = maxsize;
    _sys_fseek(file, 0, SEEK_SET);
    _sys_fread(_RamSysAddr(address), 1, l, file); // (todo) This can overwrite past RAM space

    _sys_fclose(file);
    return l ;
}

static char findNextDirName[128];
static uint16 fileRecords = 0;
static uint16 fileExtents = 0;
static uint16 fileExtentsUsed = 0;
static uint16 firstFreeAllocBlock;

uint8 _findnext(uint8 isdir) {
    uint8 result = 0xff;
    fat32_entry_t dir_entry;
    fat32_error_t fat_result ;
	
    int i;
    struct stat st;
    uint32 bytes;

    // printf("%s\n", filename) ;
    if (allExtents && fileRecords) {
        // _SearchFirst was called with '?' in the FCB's EX field, so
        // we need to return all file extents.
        // The last found file was large enough that in CP/M it would
        // have another directory entry, so mock up the next entry
        // for the file.
        _mockupDirEntry(1);
        result = 0;
    } else {
	if (!fat_dir_open ) return(0xff) ; // err dir not opened 
    	while(1)
    	{
            fat_result = fat32_dir_read(&fat_dir, &dir_entry);
            if (fat_result != FAT32_OK)
            {
            	printf("Dir read Error: %s\n", fat32_error_string(fat_result));
		break ;
            }
            if (dir_entry.filename[0])
            {
                // printf("fat readdir: %-28s\n", dir_entry.filename);
            	if (dir_entry.attr & (FAT32_ATTR_VOLUME_ID | FAT32_ATTR_HIDDEN | FAT32_ATTR_SYSTEM))
            	{
                // It's a volume label, hidden file, or system file, skip it
                	continue;
            	}
            	else if (dir_entry.attr & FAT32_ATTR_DIRECTORY)
            	{
			continue ;
            	}
                strcpy(findNextDirName, fat_dir_fullpath);
                strncpy(&findNextDirName[5], dir_entry.filename, sizeof(findNextDirName) - 6);
                findNextDirName[sizeof(findNextDirName) - 1] = 0;
                char *shortName = &findNextDirName[strlen(FILEBASE)+4];
                _HostnameToFCBname((uint8 *)shortName, fcbname);
                // printf("short/fcb: %s - %s - %s ", shortName, fcbname, findNextDirName);
                // if (stat(findNextDirName, &st) == 0) printf("stat OK\n") ; else printf("stat NOK err %d\n", errno )  ;
                if (match(fcbname, pattern) &&
                    (stat(findNextDirName, &st) == 0) &&
                    S_ISREG(st.st_mode)   
                    // isxdigit((uint8)shortName[2]) &&  (printf("-4") !=0) &&
                    // (isupper((uint8)shortName[2]) || isdigit((uint8)shortName[2]))
		    ) {
		    // printf(" -- ok\n") ;
                    if (allUsers)
                        currFindUser = isdigit((uint8)shortName[2]) ? shortName[2] - '0' : shortName[2] - 'A' + 10;
                    if (isdir) {
                        // account for host files that aren't multiples of the block size
                        // by rounding their bytes up to the next multiple of blocks
                        bytes = st.st_size;
                        if (bytes & (BlkSZ - 1))
                            bytes = (bytes & ~(BlkSZ - 1)) + BlkSZ;
                        // calculate the number of 128 byte records and 16K
                        // extents for this file. _mockupDirEntry will use
                        // these values to populate the returned directory
                        // entry, and decrement the # of records and extents
                        // left to process in the file.
                        fileRecords = bytes / BlkSZ;
                        fileExtents = fileRecords / BlkEX + ((fileRecords & (BlkEX - 1)) ? 1 : 0);
                        fileExtentsUsed = 0;
                        firstFreeAllocBlock = firstBlockAfterDir;
                        _mockupDirEntry(1);
                    } else {
                        fileRecords = 0;
                        fileExtents = 0;
                        fileExtentsUsed = 0;
                        firstFreeAllocBlock = firstBlockAfterDir;
                    }
                    _RamWrite(tmpFCB, filename[0] - '@');
                    _HostnameToFCB(tmpFCB, (uint8 *)shortName);
                    result = 0x00;
                    break;
                } // else printf(" %s nok\n", pattern) ; 
            } else {
        	fat32_close(&fat_dir);
		fat_dir_open = false ;
		break ;
	    }
	  }    
	}
    return (result);
}

uint8 _findfirst(uint8 isdir) {
    uint8 path[6] = {'/', '?', FOLDERCHAR, '?', FOLDERCHAR, 0};
    path[1] = filename[0];
    path[3] = filename[2];
    if (fat_dir_open)
        fat32_close(&fat_dir) ;
    fat32_error_t fat_result = fat32_open(&fat_dir, (char *)path);
    if (fat_result != FAT32_OK)
    {
       	printf("Dir Error: %s\n", fat32_error_string(fat_result));
       	return 0xff;
    }
    strcpy(fat_dir_fullpath, (char *)path) ;
    fat_dir_open = true ;
    _HostnameToFCBname(filename, pattern);
    fileRecords = 0;
    fileExtents = 0;
    fileExtentsUsed = 0;
    return (_findnext(isdir));
}

uint8 _findnextallusers(uint8 isdir) {
    return _findnext(isdir);
}

uint8 _findfirstallusers(uint8 isdir) {
    uint8 path[3] = {'/', '?', 0};
    path[1] = filename[0];
    if (fat_dir_open)
        fat32_close(&fat_dir) ;
    fat32_error_t fat_result = fat32_open(&fat_dir, (char *)path);
    if (fat_result != FAT32_OK)
    {
       	printf("Dir Error: %s\n", fat32_error_string(fat_result));
       	return 0xff;
    }
    fat_dir_open = true ;
    strcpy((char *)pattern, "???????????");
    fileRecords = 0;
    fileExtents = 0;
    fileExtentsUsed = 0;
    return (_findnextallusers(isdir));
}

/* Hardware abstraction functions */
/*===============================================================================*/

void beep(void) {
  audio_play_sound_blocking(440, 440, 100) ;
}

void _HardwareInit(void) {
    int led_init_result = led_init();

    stdio_init_all();
    picocalc_init();
    lcd_set_font(&font_4x10);
    audio_init() ;
    display_set_bell_callback(beep) ;
#ifdef UART_DEBUG
    serial_init(UART_BAUDRATE, UART_DATABITS, UART_STOPBITS, UART_PARITY) ;
#endif
} 

void _HardwareOut(const uint32 Port, const uint32 Value) {
}

uint32 _HardwareIn(const uint32 Port) {
    return 0;
}

/* Console abstraction functions */
/*===============================================================================*/

void _console_init(void) {
}

void _console_reset(void) {
}

/* ==============================================================================*/

int _kbhit(void) {
    return keyboard_key_available() || user_interrupt ;
}

uint8 _getch(void) {
    return getchar() ;
}


void _putch(uint8 ch) {
    static int nbout = 0 ; 
    char buf[10] ;

    while(user_freeze) ; // wait for Ctrl-Q
    putchar(ch);
#if UART_DEBUG
    if ((ch>0x20) && (ch<0x80)) 
    {
    	serial_put_char(ch) ;
        serial_put_char(' ') ;
    } else {
	sprintf(buf, "%02X", ch) ;
    	serial_put_char(buf[0]) ;
    	serial_put_char(buf[1]) ;
    }
    serial_put_char(' ') ;
    if (nbout++>32)
    {
	    serial_put_char('\x0A') ;
	    serial_put_char('\x0D') ;
	    nbout=0 ;
    }
#endif
}

uint8 _getche(void) {
    uint8 ch = _getch();
    _putch(ch);
    return ch;
}

void _clrscr(void) {
    _putch(0x1b) ;
    _putch('[') ;
    _putch('2') ;
    _putch('J') ;
    // lcd_clear_screen() ;
}

#endif
