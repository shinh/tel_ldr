#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

void error(const char* msg) {
  perror(msg);
  abort();
}

void undefined() {
  fprintf(stderr, "undefined function is called\n");
  abort();
}

int g_argc;
char** g_argv;
int H__libc_start_main(int (*m)(int, char**, char**),
                       int argc, char** argv /*,
                       void (*init)(void), void (*fini)(void),
                       void (*rtld_fini)(void),
                       void (*stack_end)
                       */
                       ) {
  if (g_argc) {
    argc = g_argc;
    argv = g_argv;
  }
  /*printf("%d %s\n", argc, argv[0]);*/
  exit(m(argc, argv, 0));
}

#ifdef __MACH__
void* __rawmemchr(const void* s, int c) {
  return memchr(s, c, -1);
}
#endif

int Hlseek(int fd, int off, int wh) {
  return lseek(fd, off, wh);
}

void* Hdlsym(void* h, char* p) {
  if(!h)
    h=RTLD_DEFAULT;
  return dlsym(h, p);
}

void* Hmmap(void* a, size_t l, int p, int f, int fd, int o) {
  return mmap(a, l, p, (f&0x1f)|(f&32?0x1000:0), fd, o);
}

int* H__errno_location() {
  return &errno;
}

struct {
  const char* n;
  void* f;
} T[] = {
#define H(n) { #n, (void*)&H ## n },
  H(__libc_start_main)
  H(__errno_location)
  H(mmap)
  H(dlsym)
  H(lseek)
  {0,0},
};

int main(int argc, char* argv[]) {
  int i;
  int fd, len;
  char* elf;
  int entry, phoff, phnum;
  int* ph;
  if (argc < 2)
    error("Usage: el <elf>");
  printf("loading %s\n", argv[1]);
  fd = open(argv[1], O_RDONLY);
  if (fd < 0)
    error("Usage: el <elf>");
  len = lseek(fd, 0, SEEK_END);
  elf = malloc(len);
  lseek(fd, 0, SEEK_SET);
  read(fd, elf, len);

  if (*(int*)elf != 0x464c457f)
    error("not elf");
  if (*(int*)(elf+16) != 0x30002)
    error("not i386 exec");

  entry = *(int*)(elf+24);
  phoff = *(int*)(elf+28);
  phnum = *(int*)(elf+42);
  printf("%x %x %x\n", entry, phoff, phnum);

  ph = (int*)(elf + phoff);
  for (i = 0; i < phnum >> 16; i++) {
    int poff, paddr, pfsize, psize, pafsize, pflag /*, palign */;
    poff = ph[1];
    paddr = ph[2];
    pfsize = ph[4];
    psize = ph[5];
    pflag = ph[6];
    /*palign = ph[7];*/
    switch (ph[0]) {
    case 1: {
      int prot = 0;
      if (pflag & 1)
        prot |= PROT_EXEC;
      if (pflag & 2)
        prot |= PROT_WRITE;
      if (pflag & 4)
        prot |= PROT_READ;
      psize += paddr & 0xfff;
      pfsize += paddr & 0xfff;
      poff -= paddr & 0xfff;
      paddr &= ~0xfff;
      pafsize = (pfsize + 0xfff) & ~0xfff;
      psize = (psize + 0xfff) & ~0xfff;
      printf("PT_LOAD size=%d fsize=%d flag=%d addr=%x prot=%d poff=%d\n",
             psize, pafsize, pflag, paddr, prot, poff);
      if (mmap((void*)paddr, pafsize, prot, MAP_FILE|MAP_PRIVATE|MAP_FIXED,
               fd, poff) == MAP_FAILED) {
        error("mmap(file)");
      }
      if ((prot & PROT_WRITE)) {
        printf("%p\n", (char*)paddr);
        for (; pfsize < pafsize; pfsize++) {
          char* p = (char*)paddr;
          p[pfsize] = 0;
        }
        if (pfsize != psize) {
          if (mmap((void*)(paddr + pfsize),
                   psize - pfsize, prot, MAP_ANON|MAP_PRIVATE,
                   -1, 0) == MAP_FAILED) {
            error("mmap(anon)");
          }
        }
      }
      break;
    }
    case 2: {
      char* dyn;
      char* dstr = NULL;
      char* dsym = NULL;
      char* rel = NULL;
      char* pltrel = NULL;
      int relsz, relent, pltrelsz = 0;
      int needed[999] = {}, *neededp = needed;
      puts("PT_DYNAMIC");
      dyn = elf + poff;
      for (;;) {
        short dtag = *(short*)dyn;
        int dval = *(int*)(dyn + 4);
        dyn += 8;
        if (dtag == 0)
          break;
        switch (dtag) {
        case 1: {  /* DT_NEEDED */
          *neededp++ = dval;
        }
        case 2: {
          pltrelsz = dval;
          printf("pltrelsz: %d\n", pltrelsz);
          break;
        }
        case 5: {
          dstr = (char*)dval;
          printf("dstr: %p %s\n", dstr, dstr+1);
          break;
        }
        case 6: {
          dsym = (char*)dval;
          printf("dsym: %p\n", dsym);
          break;
        }
        case 17: {
          rel = (char*)dval;
          printf("rel: %p\n", rel);
          break;
        }
        case 18: {
          relsz = dval;
          printf("relsz: %d\n", relsz);
          break;
        }
        case 19: {
          relent = dval;
          printf("relent: %d\n", relent);
          break;
        }
        case 20: {
          pltrel = (char*)dval;
          printf("pltrel: %p\n", pltrel);
          break;
        }
        default:
          printf("unknown DYN %d %d\n", dtag, dval);
        }
      }
      if (!dsym || !dstr) {
        error("no dsym or dstr");
      }

      for (neededp = needed; *neededp; neededp++) {
        printf("needed: %s\n", dstr + *neededp);
        dlopen(dstr + *neededp, RTLD_NOW | RTLD_GLOBAL);
      }

      {
        int i, j;
        for (j = 0; j < 2; j++) {
          for (i = 0; i < relsz; rel += relent, i += relent) {
            int* addr = *(int**)rel;
            int info = *(int*)(rel + 4);
            int sym = info >> 8;
            int type = info & 0xff;

            int* ds = (int*)(dsym + 16 * sym);
            char* sname = dstr + *ds;
            void* val=0;
            int k;
            for(k=0;T[k].n;k++){
              if(!strcmp(sname,T[k].n)){
                 val = T[k].f;
                 break;
              }
            }
            if(!val){
              if (!strcmp(sname,"stdout"))
                val = &stdout;
              else if (!strcmp(sname,"stderr"))
                val = &stderr;
              /*
              else if (!strcmp(sname, "__environ"))
                val = &environ;
              */
              else
                val = dlsym(RTLD_DEFAULT, sname);
            }

            printf("%srel: %p %s(%d) %d => %p\n",
                   j ? "plt" : "", (void*)addr, sname, sym, type, val);

            switch (type) {
            case 1: {
              *addr += (int)val;
            }
            case 5: {
              if (val) {
                *addr = *(int*)val;
              } else {
                fprintf(stderr, "undefined: %s\n", sname);
                abort();
              }
            }
            case 6: {
              if (val) {
                *addr = (int)val;
              } else {
                fprintf(stderr, "undefined data %s\n", sname);
              }
              break;
            }
            case 7: {
              if (val) {
                *addr = (int)val;
              } else {
                *addr = (int)&undefined;
              }
              break;
            }
            }
          }

          if ((int)pltrel != 17) {
            rel = pltrel;
          }
          relsz = pltrelsz;
        }
      }

      break;
    }
    default:
      printf("unknown PT %d\n", ph[0]);
    }
    ph += 8;
  }

  g_argc = argc-1;
  g_argv = argv+1;
  printf("start!: %s %x\n", argv[1], entry);
  ((void*(*)())entry)();
  /*((void*(*)(int, char**))entry)(argc, argv);*/
  return 1;
}
