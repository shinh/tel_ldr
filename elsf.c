#define _GNU_SOURCE 52
#include<dlfcn.h>/*s*/
#i\
nc\
lu\
de<string.h>/*('-' )*/
#include<sys/mman.h>/*
h*/
#i\
nc\
lude<stdlib.h>/*('')*/
#include<unistd.h>/**/

#i\
nc\
lu\
de\
<s\
td\
io\
.h>
#d\
ef\
ine A return/*( '-')*/
char T[]="stdout!dlsy"

"m!lseek!stderr!mmap!\
__libc_start_main",t,*H,**P,*O;

#define I int/*('-')*/


#define e(c,m)c?exit(\
(/*

ELF
ELF
ELF
ELF
*/perror\
(m),1)):1




I*Y(I*h,char*p){A dlsym(h?h:RTLD_DEFAULT,p);}
I*E,i,k,F,L;
I*W(void*a,size_t l,I p,I f,I d,I o){A mmap(a,l,p,(f&31)|(f&32?MAP_ANON:0),d,o);}
I*__rawmemchr(I*s,I c){A memchr(s,c,1<<30);}

I*V,J,D[99],M,Q,N,*K;

I X(I f,I o,I w){A lseek(f,o,w);}
;;I Z(I(*m)()){exit(m(Q,P,0));}
void*U[]={0,Y,X,0,W,Z};

I main(I S,char**R){
Q=S-1;P=R+1;e(!Q,"Usage: elf <elf>");F=fileno(fopen(*P,"r"));E=W(0,N=4095,7,S=MAP_PRIVATE,F,0);L=512;
e(7417633*159-*E,"not");e(E[4]-L*384-2,"\
not"" i\
386"" e\
xec");
;;
for(K=E+=13;K<E+E[-2]%65536*8;K+=8){U[3]=&stderr;O=(char*)K[2];
;
if(*K==1)O/**/-=M=K[2]&N,e(W(O,J=(K[5]+M+N)&~N,7,S|MAP_FIXED,F,K[1]-M)==MAP_FAILED,"mma\
p"),L=K[4]/**/+M,/*i"n*/*U=&std\
out,mem\
set(O+L,0,J-L);;
if(*K==2){I*G;for(;*O;O+=8)D[*O&63]=1[(I*)O];e(!D[6]|!D[5],"inv\
ali"/*h"*/"d");;for(i=-D[2];i<D[18];i+=8)t=M=1[V=(I*)((i<0?D[23]+D[2]:D[17])+i)],V=(I*)*V,O=strstr(T,H=*((char**)D[6]+M/256*4)+D[5]),G=O?U[(O-T)/6]:Y(0,H),*V=t-5?(t<7)**V+t%2*(I)G:*G;}}A(((I(*)())E[-7])());}