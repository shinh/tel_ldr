#define _GNU_SOURCE
#include<dlfcn.h>
#include<stdio.h>
#include<string.h>
#include<sys/mman.h>
#include<unistd.h>
#define I int
#define A return
I*E,i,k,F,*K,L,J,M,*V,*G,D[99],Q,N,exit();char*O,t,*H,**P,T[]="stdout!dlsym!lseek!stderr!mmap!__libc_start_main";I Z(I(*m)()){exit(m(Q,P,0));}I*__rawmemchr(I*s,I c){A memchr(s,c,1<<30);}I X(I f,I o,I w){A lseek(f,o,w);}I*Y(I*h,char*p){A dlsym(h?h:RTLD_DEFAULT,p);}I*W(void*a,size_t l,I p,I f,I d,I o){A mmap(a,l,p,f&31|(f&32?MAP_ANON:0),d,o);}void*U[]={0,Y,X,0,W,Z};I main(I S,char**R){*U=&stdout;U[3]=&stderr;Q=S-1;P=R+1;F=fileno(fopen(*P,"r"));E=W(0,N=4095,7,S=MAP_PRIVATE,F,0);for(K=E+=13;K<E+E[-2]%65536*8;K+=8){O=(char*)K[2];if(*K==1)O-=M=K[2]&N,W(O,J=(K[5]+M+N)&~N,7,S|MAP_FIXED,F,K[1]-M),L=K[4]+M,memset(O+L,0,J-L);if(*K==2){for(;*O;O+=8)D[*O&63]=1[(I*)O];for(i=-D[2];i<D[18];i+=8)t=M=1[V=(I*)((i<0?D[23]+D[2]:D[17])+i)],V=(I*)*V,O=strstr(T,H=*((char**)D[6]+M/256*4)+D[5]),G=O?U[(O-T)/6]:Y(0,H),*V=t-5?(t<7)**V+t%2*(I)G:*G;}}((I(*)())E[-7])();}