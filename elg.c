/* 1167 */
#define _GNU_SOURCE
#include<dlfcn.h>
#include<stdio.h>
#include<string.h>
#include<sys/mman.h>
#include<unistd.h>
#define e(c,m)c?perror(m),exit(1):1
#define I int
#define A return

I*E,i,k,F,*ph,pfsize,psize,pbias,*addr,*val,D[99],g_argc,pagesize,exit();
char*paddr,t,*sname,**g_argv,T[]="stdout!dlsym!lseek!stderr!mmap!__libc_start_main";

I H__libc_start_main(I(*m)()){
/*printf("%d %s\n",argc,argv[0]);*/
  exit(m(g_argc,g_argv,0));
}

I*__rawmemchr(I*s,I c){
  A memchr(s,c,1<<30);
}

I Hlseek(I f,I o,I w){
  A lseek(f,o,w);
}

I*Hdlsym(I*h,char*p){
  A dlsym(h?h:RTLD_DEFAULT,p);
}

I*Hmmap(void*a,size_t l,I p,I f,I d,I o){
  A mmap(a,l,p,f&31|(f&32?MAP_ANON:0),d,o);
}

void*U[]={0,Hdlsym,Hlseek,0,Hmmap,H__libc_start_main};

/*
I*H__errno_location(){
  A &errno;
}
*/

I main(I argc,char**argv){
  *U=&stdout;
  U[3]=&stderr;
  g_argc=argc-1;
  g_argv=argv+1;

  e(!g_argc,"Usage: el <elf>");
/*printf("loading %s\n",argv[1]);*/
  F=fileno(fopen(*g_argv,"r"));
  E=Hmmap(0,pagesize=4095,7,argc=MAP_PRIVATE,F,0);
  e(*E-0x464c457f,"not elf");
  e(E[4]-196610,"not i386 exec");

  for(ph=E+=13;ph<E+E[-2]%65536*8;ph+=8){
    paddr=(char*)ph[2];
    if(*ph==1)
      paddr-=pbias=ph[2]&pagesize,
        e(Hmmap(paddr,psize=(ph[5]+pbias+pagesize)&~pagesize,7,argc|MAP_FIXED,F,ph[1]-pbias)==MAP_FAILED,"mmap"),
        pfsize=ph[4]+pbias,
        memset(paddr+pfsize,0,psize-pfsize);
    if(*ph==2){
      for(;*paddr;paddr+=8)
        D[*paddr&63]=1[(I*)paddr];
      e(!D[6]|!D[5],"no dsym/dstr");

      for(i=-D[2];i<D[18];i+=8)
/*addr=(I*)((i<D[18]?D[17]:D[23]-D[18])+i);*/
        t=pbias=1[addr=(I*)((i<0?D[23]+D[2]:D[17])+i)],
          addr=(I*)*addr,
          paddr=strstr(T,sname=*((char**)D[6]+pbias/256*4)+D[5]),
          val=paddr?U[(paddr-T)/6]:Hdlsym(0,sname),
          *addr=t-5?(t<7)**addr+t%2*(I)val:*val;
    }
  }

  ((I(*)())E[-7])();
}
