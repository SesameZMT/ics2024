#include "common.h"
#include "fs.h"

#define DEFAULT_ENTRY ((void *)0x8048000)

extern uint8_t ramdisk_start;
extern uint8_t ramdisk_end;

#define RAMDISK_SIZE ((&ramdisk_end) - (&ramdisk_start))

extern void ramdisk_read(void *buf, off_t offset, size_t len);
extern size_t get_ramdisk_size();

uintptr_t loader(_Protect *as, const char *filename) {
  // TODO();
//   size_t len = get_ramdisk_size();
//   ramdisk_read(DEFAULT_ENTRY,0,len);
//   return (uintptr_t)DEFAULT_ENTRY;
    int fd = fs_open(filename, 0, 0);
    Log("filename=%s,fd=%d",filename,fd);
    fs_read(fd, DEFAULT_ENTRY, fs_filesz(fd));
    fs_close(fd);
    return (uintptr_t)DEFAULT_ENTRY;
}