#include "common.h"
#include "syscall.h"

int sys_write(int fd, void *buf, size_t len) {
    if(fd == 1 || fd == 2){
        char* c = (char*) buf;
        for(int i = 0; i < len; i++) {
            _putc(c[i]);
        }
        return len;
    }
    return -1;
}

_RegSet* do_syscall(_RegSet *r) {
  uintptr_t a[4];
  a[0] = SYSCALL_ARG1(r);
  a[1] = SYSCALL_ARG2(r);
  a[2] = SYSCALL_ARG3(r);
  a[3] = SYSCALL_ARG4(r);

  switch (a[0]) {
    case SYS_none:  r->eax = 1; break;
    case SYS_exit:  _halt(a[1]); break;
    case SYS_write: r->eax = sys_write(a[1], (void *)a[2], a[3]); break;
    case SYS_brk:   r->eax = 1; break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }

  return NULL;
}
