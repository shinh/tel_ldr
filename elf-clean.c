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

#if defined(__x86_64__)
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
# define Elf_Dyn Elf64_Dyn
# define Elf_Rel Elf64_Rela
# define Elf_Sym Elf64_Sym
# define Elf_Addr Elf64_Addr
# define ELF_R_TYPE ELF64_R_TYPE
# define ELF_R_SYM ELF64_R_SYM
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
# define Elf_Dyn Elf32_Dyn
# define Elf_Rel Elf32_Rel
# define Elf_Sym Elf32_Sym
# define Elf_Addr Elf32_Addr
# define ELF_R_TYPE ELF32_R_TYPE
# define ELF_R_SYM ELF32_R_SYM
#endif

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
              Elf_Rel* rel, int relsz,
              Elf_Sym* dsym, char* dstr,
              long base_addr) {
  int i;
  for (i = 0; i < relsz / sizeof(*rel); rel++, i++) {
    long* addr = (long*)((char*)rel->r_offset + base_addr);
    int type = ELF_R_TYPE(rel->r_info);
    Elf_Sym* sym = dsym + ELF_R_SYM(rel->r_info);
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
#if defined(__x86_64__)
    case R_X86_64_RELATIVE: {
      *addr += (long)base_addr;
      break;
    }
    case R_X86_64_GLOB_DAT: {
      *addr = (long)val;
      break;
    }
    case R_X86_64_JUMP_SLOT: {
      if (val) {
        *addr = (long)val;
      } else {
        *addr = (long)&undefined;
      }
      break;
    }
#else
    case R_386_32: {
      *addr += (long)val;
    }
    case R_386_COPY: {
      if (val) {
        *addr = *(long*)val;
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
        *addr = (long)val;
      } else {
        *addr = (long)&undefined;
      }
      break;
    }
#endif
    }
  }
}

int main(int argc, char* argv[]) {
  int i, fd;
  long entry, phoff, phnum;
  Elf_Ehdr ehdr;
  long base_addr = 0;

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

  if (ehdr.e_type == ET_DYN) {
    base_addr = 0x10000;
  } else if (ehdr.e_type != ET_EXEC) {
    error("not exec");
  }
#if defined(__x86_64__)
  if (ehdr.e_machine != EM_X86_64)
    error("not x86-64");
#else
  if (ehdr.e_machine != EM_386)
    error("not i386");
#endif

  entry = ehdr.e_entry + base_addr;
  phoff = ehdr.e_phoff;
  phnum = ehdr.e_phnum;
  printf("entry=%x phoff=%x phnum=%x\n", entry, phoff, phnum);

  if (lseek(fd, phoff, SEEK_SET) != phoff)
    error("lseek failed");

  for (i = 0; i < phnum; i++) {
    long poff, paddr, pfsize, psize, pafsize, pflag;
    Elf_Phdr phdr;
    if (read(fd, &phdr, sizeof(phdr)) != sizeof(phdr))
      error("reading program header failed");
    poff = phdr.p_offset;
    paddr = phdr.p_vaddr + base_addr;
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
      Elf_Sym* dsym = NULL;
      Elf_Rel* rel = NULL;
      int relsz = 0, pltrelsz = 0;
      Elf_Dyn* dyn;
      puts("PT_DYNAMIC");
      for (dyn = (Elf_Dyn*)paddr; dyn->d_tag != DT_NULL; dyn++) {
        Elf_Addr dptr = dyn->d_un.d_ptr + base_addr;
        switch (dyn->d_tag) {
        case DT_PLTRELSZ: {
          pltrelsz = dyn->d_un.d_val;
          printf("pltrelsz: %d\n", pltrelsz);
          break;
        }
        case DT_STRTAB: {
          dstr = (char*)dptr;
          printf("dstr: %p %s\n", dstr, dstr+1);
          break;
        }
        case DT_SYMTAB: {
          dsym = (Elf_Sym*)dptr;
          printf("dsym: %p\n", dsym);
          break;
        }
        case DT_REL:
        case DT_RELA: {
          rel = (Elf_Rel*)dptr;
          printf("rel: %p\n", rel);
          break;
        }
        case DT_RELSZ:
        case DT_RELASZ: {
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
#if defined(__x86_64__)
          if (pltrel != DT_RELA)
            error("unexpected PLTREL");
#else
          if (pltrel != DT_REL)
            error("unexpected PLTREL");
#endif
          break;
        }
        default:
          printf("unknown DYN %d %d\n", dyn->d_tag, dyn->d_un.d_val);
        }
      }
      if (!dsym || !dstr)
        error("no dsym or dstr");

      relocate("rel", rel, relsz, dsym, dstr, base_addr);
      relocate("pltrel", rel + relsz / sizeof(*rel), pltrelsz, dsym, dstr, base_addr);
    }
    default:
      printf("unknown PT %d\n", phdr.p_type);
    }
  }

  g_argc = argc - 1;
  g_argv = argv + 1;
  fflush(stdout);
  printf("start!: %s %x\n", argv[1], entry);
  ((void*(*)())entry)();
  return 1;
}
