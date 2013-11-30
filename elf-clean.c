#define _GNU_SOURCE
#include <dlfcn.h>
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

void error(const char* msg) {
  if (errno)
    perror(msg);
  else
    fprintf(stderr, "%s\n", msg);
  abort();
}

void undefined() {
  fprintf(stderr, "undefined function is called\n");
  abort();
}

int g_argc;
char** g_argv;
int H__libc_start_main(int (*m)(int, char**, char**),
                       int argc, char** argv
                       /*,
                         void (*init)(void), void (*fini)(void),
                         void (*rtld_fini)(void),
                         void (*stack_end) */) {
  if (g_argc) {
    argc = g_argc;
    argv = g_argv;
  }
  /*printf("%d %s\n", argc, argv[0]);*/
  exit(m(argc, argv, 0));
}

void* __rawmemchr(const void* s, int c) {
  return memchr(s, c, 1<<30);
}

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
  const char* name;
  void* sym;
} HOST_SYMS[] = {
  /* &stdin etc. are not compile time constants. We will fill them later */
  "stdin", NULL,
  "stdout", NULL,
  "stderr", NULL,
#define HOST_FUNC_ENTRY(n) { #n, (void*)&H ## n },
  HOST_FUNC_ENTRY(__libc_start_main)
  HOST_FUNC_ENTRY(__errno_location)
  HOST_FUNC_ENTRY(mmap)
  HOST_FUNC_ENTRY(dlsym)
  HOST_FUNC_ENTRY(lseek)
  {0, 0},
};

void relocate(const char* reloc_type,
              Elf32_Rel* rel, int relsz,
              Elf32_Sym* dsym, char* dstr) {
  int i;
  for (i = 0; i < relsz / sizeof(*rel); rel++, i++) {
    int* addr = (int*)rel->r_offset;
    int type = ELF32_R_TYPE(rel->r_info);
    Elf32_Sym* sym = dsym + ELF32_R_SYM(rel->r_info);
    char* sname = dstr + sym->st_name;
    void* val = 0;
    int k;
    for (k=0; HOST_SYMS[k].name; k++) {
      if (!strcmp(sname, HOST_SYMS[k].name)) {
        val = HOST_SYMS[k].sym;
        break;
      }
    }
    if (!val)
      val = dlsym(RTLD_DEFAULT, sname);

    printf("%s: %p %s(%d) %d => %p\n",
           reloc_type, (void*)addr, sname, sym, type, val);

    switch (type) {
    case R_386_32: {
      *addr += (int)val;
    }
    case R_386_COPY: {
      if (val) {
        *addr = *(int*)val;
      } else {
        fprintf(stderr, "undefined: %s\n", sname);
        abort();
      }
    }
    case R_386_GLOB_DAT: {
      break;
    }
    case R_386_JMP_SLOT: {
      if (val) {
        *addr = (int)val;
      } else {
        *addr = (int)&undefined;
      }
      break;
    }
    }
  }
}

int main(int argc, char* argv[]) {
  int i, fd;
  int entry, phoff, phnum;
  Elf32_Ehdr ehdr;

  HOST_SYMS[0].sym = &stdin;
  HOST_SYMS[1].sym = &stdout;
  HOST_SYMS[2].sym = &stderr;

  if (argc < 2)
    error("Usage: el <elf>");
  printf("loading %s\n", argv[1]);
  fd = open(argv[1], O_RDONLY);
  if (fd < 0)
    error("Usage: el <elf>");
  if (read(fd, &ehdr, sizeof(ehdr)) != sizeof(ehdr))
    error("reading ELF header failed");

  if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG))
    error("not elf");
  if (ehdr.e_type != ET_EXEC || ehdr.e_machine != EM_386)
    error("not i386 exec");

  entry = ehdr.e_entry;
  phoff = ehdr.e_phoff;
  phnum = ehdr.e_phnum;
  printf("entry=%x phoff=%x phnum=%x\n", entry, phoff, phnum);

  if (lseek(fd, phoff, SEEK_SET) != phoff)
    error("lseek failed");

  for (i = 0; i < phnum; i++) {
    int poff, paddr, pfsize, psize, pafsize, pflag;
    Elf32_Phdr phdr;
    if (read(fd, &phdr, sizeof(phdr)) != sizeof(phdr))
      error("reading program header failed");
    poff = phdr.p_offset;
    paddr = phdr.p_vaddr;
    pfsize = phdr.p_filesz;
    psize = phdr.p_memsz;
    pflag = phdr.p_flags;
    printf("%d %d %p %x\n", phdr.p_type, poff, paddr, pfsize);
    switch (phdr.p_type) {
    case PT_LOAD: {
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
    case PT_DYNAMIC: {
      char* dstr = NULL;
      Elf32_Sym* dsym = NULL;
      Elf32_Rel* rel = NULL;
      int relsz = 0, pltrelsz = 0;
      Elf32_Dyn* dyn;
      puts("PT_DYNAMIC");
      for (dyn = (Elf32_Dyn*)paddr; dyn->d_tag != DT_NULL; dyn++) {
        Elf32_Addr dval = dyn->d_un.d_ptr;
        switch (dyn->d_tag) {
        case DT_PLTRELSZ: {
          pltrelsz = dyn->d_un.d_val;
          printf("pltrelsz: %d\n", pltrelsz);
          break;
        }
        case DT_STRTAB: {
          dstr = (char*)dyn->d_un.d_ptr;
          printf("dstr: %p %s\n", dstr, dstr+1);
          break;
        }
        case DT_SYMTAB: {
          dsym = (Elf32_Sym*)dyn->d_un.d_ptr;
          printf("dsym: %p\n", dsym);
          break;
        }
        case DT_REL: {
          rel = (Elf32_Rel*)dyn->d_un.d_ptr;
          printf("rel: %p\n", rel);
          break;
        }
        case DT_RELSZ: {
          relsz = dyn->d_un.d_val;
          printf("relsz: %d\n", relsz);
          break;
        }
        case DT_RELENT: {
          int relent = dyn->d_un.d_val;
          printf("relent: %d\n", relent);
          if (relent != sizeof(*rel))
            error("unexpected RELENT");
          break;
        }
        case DT_PLTREL: {
          int pltrel = dyn->d_un.d_val;
          printf("pltrel: %d\n", pltrel);
          if (pltrel != DT_REL)
            error("unexpected PLTREL");
          break;
        }
        default:
          printf("unknown DYN %d %d\n", dyn->d_tag, dyn->d_un.d_val);
        }
      }
      if (!dsym || !dstr)
        error("no dsym or dstr");

      relocate("rel", rel, relsz, dsym, dstr);
      relocate("pltrel", rel + relsz / sizeof(*rel), pltrelsz, dsym, dstr);
    }
    default:
      printf("unknown PT %d\n", phdr.p_type);
    }
  }

  g_argc = argc - 1;
  g_argv = argv + 1;
  printf("start!: %s %x\n", argv[1], entry);
  ((void*(*)())entry)();
  return 1;
}
