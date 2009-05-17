/*
		プリンタ用紙節約プログラムＣＭＰＰ

			 By Ken/ichiro(OPA)
 */
#define VERSION		"2.01"

#include <stdio.h>
#include <dos.h>
#include <string.h>
#include <farstr.h>
#include <stdlib.h>
#include <ctype.h>
#include "opa.h"

#define UNDEF		(65535U)

#define ESC			"\033"
#define FS			"\x01c"

#define LINEMAX		20000	/* 最大ファイル行数(default 20000)*/
#define BUF16SIZE	9800
#define BUFPRNSIZE	(BUF16SIZE*3)
#define PBIOS_CALL	YES
#define LOCALCACHE	OFF

enum Printers{
	P2,						/* ＮＥＣ２４ドット */
	P4,						/* ＮＥＣ４８ドット */
	E2,						/* エプソン２４ドット */
	E4,						/* エプソン４８ドット */
	MAX_PRINTERS
};

enum Papers{
	NF,						/* 連続用紙 */
	A4,						/* Ａ４ */
	B4,						/* Ｂ４ */
	B5,						/* Ｂ５ */
	FF,						/* ファンフォールド */
	OT,						/* その他 */
	MAX_PAPERS
};

extern char
	*_argv0,
	*_environ;

FILE
	*fp;
struct dosdate_t
	nowdate;
struct dostime_t
	nowtime;
uchar
	helpmes[]=
		"使用法：\n"
			"\tCMPP [スイッチ|ファイル名]...\n"
		"プリンタ用紙を節約して印字します。\n"
		"使用できるオプションは次の通りです。(括弧内は初期値)\n\n"
		"プリンタの種類：\n"
			"\t/P2\tPC-PR ２４ドット\n"
			"\t/E2\tESC/P ２４ドット\n"
			"\t/E4\tESC/P ４８ドット\n"
		"用紙の種類：\n"
			"\t/FF\tファンフォールド紙\n"
			"\t/A4\tＡ４単票用紙\n"
			"\t/B5\tＢ５単票用紙\n"
			"\t/B4\tＢ４単票用紙\n"
			"\t/NF\t連続用紙\n"
			"\t/PIn\tインチ単位ページ長指定 n=1-127\n"
		"その他：\n"
			"\t/DNn\t段組数を指定 n=1-7(2)\n"
			"\t/LPn\t１ページ１段に入れる行数 n=1以上\n"
			"\t/66x\t１段の行数を６６の倍数にする(-)\n"
			"\t/HTn\tタブ間隔 - で自動　n=1-80(-)\n"
			"\t/PAn-m\t印刷するページを指定 n,m=1以上(-)\n"
			"\t/HEx\tヘッダ印刷をする(+)\n"
			"\t/LNx\t行番号をつける(-)\n"
			"\t/BFx\t文字を太くする(-)\n"
			"\t/NC\tＣＧウィンドウを使わない\n"
			"\t/COx\t印刷せずにページ数を表示する(-)\n"
			"\t/WA\t一旦停止する\n"
			"n は正の整数、x は'+'または'-'。",
	er_prn[]=
		"er:プリンタ種類を指定してください : [",
	er_num[]=
		"er:値が不適当です : [",
	er_plu[]=
		"er:'+'または'-'が必要です : [",
	*papername[MAX_PAPERS]={"連続用紙","Ａ４","Ｂ４","Ｂ５","ＦＦ紙","その他"},
	decor_caseis_upper=TRUE,
	header1[85],
	header2[85],
	readbuf[170],
	charfont[34],
	_line_pointer[LINEMAX],
#if LOCALCACHE
	prt_pq[4096],
#endif
	buf16[BUF16SIZE];
uchar far
	bufprn[BUFPRNSIZE];
uint
	now_line,
	maxline,
	maxpage,
	printed_line,
	print_line,
	printed_files=0,
	counted_files=0,
	error_files=0,
	pagecount[MAX_PAPERS],
	startpage=0,
	endpage=LINEMAX,
	printer=UNDEF,
	paper=UNDEF,
	pageline=UNDEF,
	lineparpage=UNDEF,
	dansuu=2,
	dpi,
	tabsize,
	tabflag=OFF,
	boldflag=OFF,
	countflag=OFF,
	headerflag=ON,
	cgwflag=ON,
	numflag=OFF,
	buf16_x,
	bufprn_y,
	head_dots,
	h_dots,
#if LOCALCACHE
	prt_t=0,
	prt_e=0,
#endif
	filedate,
	filetime;
ulong
	pagecount_nf=0,
	pagedot,
	now_pointer;

int iskanji(char c)
{
	return(0x81<=c && c<=0x9f || 0xe0<=c && c<=0xfc);
}

void sftstr(char s[],int w)
{
	memmove(&s[w],&s[0],strlen(s)+1);
}

char *nextstr(char *s)
{
	return(s + strlen(s) + 1);
}

void backelase(int n)
{
	for(;n>0;n--)
		printf("\b \b");
}

char *getpath(char *d,char *s)
{
	int i,n;
	char c;

	for(i=0,n=0;s[i];i++){
		c=s[i];
		if(c=='\\' || c==':' || c=='/')
			n=i+1;
		else if(iskanji(c))
			i++;
	}
	strncpy(d,s,n);
	d[n]=NULL;
	return(&s[n]);	/* パスを除いたファイル名を返す */
}

long get_lp(uint line)				/* かの有名なファイルポインタを得る関数 */
{
	if(line<now_line){
		while(line<now_line)
			now_pointer-=_line_pointer[--now_line];
	}else{
		if(line>maxline)
			line=maxline;
		while(line>now_line)
			now_pointer+=_line_pointer[now_line++];
	}

	return(now_pointer);
}

end(int rcode,int aflag)
{
	if(aflag)
		printf("\nProcess abort : ");
	else
		printf("\nProcess end : ");
	if(printed_files)
		printf("print %d , ",printed_files);
	if(counted_files)
		printf("count %d , ",counted_files);
	if(error_files)
		printf("error %d.\n",error_files);
	else
		printf("no error.\n");

	exit(rcode);
}

uint bgetc(void)								/* BIOS経由１文字読み込み */
{
	uint _asm_();

	return(
		_asm_(	"\n"
			"	xor		ax,ax		\n"
			"	int		18h			\n"
			)
		);
}

int __pstat(void)					/* プリンタのスティタス取得 */
{
	void _asm_();

#if PBIOS_CALL
	_asm_(	"\n"
			"	mov		ah,12h		\n"
			"	int		1ah			\n"
			"	mov		al,ah		\n"
			);
#else
	_asm_(	"\n"
			"	in		al,42h		\n"
			"	and		al,4		\n"
			"	mov		ah,al		\n"
			);
#endif
}

void __pputc(char c)					/* プリンタに１バイト送出 */
{
	void _asm_();

#if PBIOS_CALL
	_asm_(	"\n"
			"	mov		ah,11h		\n"
			"	int		1ah			\n"
			);
#else
	_asm_(	"\n"
			"	mov		ah,al		\n"
			"A::in		al,42h		\n"
			"	test	al,4		\n"
			"	jz		A			\n"
			"	mov		al,ah		\n"
			"	out		40h,al		\n"
			"	in		al,44h		\n"
			"	and		al,7fh		\n"
			"	out		44h,al		\n"
			"	or		al,80h		\n"
			"	jmp		$+2			\n"
			"	out		44h,al		\n"
			);
#endif
}

void _pflush(void)
{
#if LOCALCACHE
	while(prt_t!=prt_e){
		__pputc(prt_pq[prt_t++]);
		if(prt_t>=4096)
			prt_t=0;
	}
#endif
}

void _pputc(char c)
{
#if LOCALCACHE
	if(((prt_t-prt_e+4096)%4096)==1)
		_pflush();

	prt_pq[prt_e++]=c;
	if(prt_e>=4096)
		prt_e=0;

	while(prt_t!=prt_e && __pstat()){
		__pputc(prt_pq[prt_t++]);
		if(prt_t>=4096)
			prt_t=0;
	}
#else
	__pputc(c);
#endif
}

void pputc(uint c)								/* プリンタに１文字印刷要求 */
{
	if(c>255){
		if(printer==P2){
			_pputc(27);  _pputc(75);
			_pputc(c>>8);_pputc((char)c);
			_pputc(27);  _pputc(72);
		}else{
			_pputc(28);  _pputc(38);
			_pputc(c>>8);_pputc((char)c);
			_pputc(28);  _pputc(46);
		}
	}else{
		_pputc(c);
	}
}

uint sfttojis(char shi,char slo)
{
	char jhi,jlo,k;

	if(shi<=0x9f)
		k=0x71;
	else
		k=0xb1;

	if(slo<=0x9e){
		jhi=(shi-k)*2+1;
		if(slo>=0x80)
			jlo=slo-0x20;
		else
			jlo=slo-0x1f;
	}else{
		jhi=(shi-(k-1))*2;
		jlo=slo-0x7e;
	}

	return((jhi<<8)+jlo);
}

void pputs(char *s)									/* プリンタに１行送出 */
{
	while(*s)
		if(iskanji(*s)){
			pputc(sfttojis(*s,*(s+1)));
			s+=2;
		}else
			pputc(*s++);
}

void cgw_fontget(uint jcode)		/* フォントを得る　ＣＧウィンドウ使う */
{									/* 漢字のみ。 */
	char far
		*win=(char far *)0xa4000000L;
	int
		i;

/*	if(jcode<256){				/* ＡＮＫ */
		charfont[0]=2;
		charfont[1]=1;

		outp8(0x68,0xb);
		outp8(0xa1,0);
		outp8(0xa3,(char)jcode);

		for(i=0;i<16;i++)
			charfont[2+i]=win[(i << 1)+1];

		outp8(0x68,0xa);
	}else*/ if(jcode>=0x2920 && jcode<0x2b80){	/* 半角漢字 */
		charfont[0]=2;
		charfont[1]=1;

		outp8(0x68,0xb);
		outp8(0xa3,(jcode >> 8)-0x20);
		outp8(0xa1,(char)jcode);
		outp8(0xa5,0x20);

		for(i=0;i<16;i++)
			charfont[2+i]=win[(i << 1)+1];

		outp8(0x68,0xa);
#if 0
	}else if(	(jcode>=0x2800 && jcode<0x3000) || 	/* 判定を厳密に */
				(jcode>=0x7600 && jcode<0x7700) || 
				(jcode>=0x7900 && jcode<0x7D00)){
#else
	}else if((jcode>=0x2800 && jcode<0x3000)||jcode>=0x7600){/* 判定を簡単に */
#endif											/* 特殊漢字 */
		charfont[0]=2;
		charfont[1]=2;

		outp8(0x68,0xb);
		outp8(0xa3,(jcode >> 8)-0x20);
		outp8(0xa1,(char)jcode);

		outp8(0xa5,0x20);						/* 左 */
		for(i=0;i<16;i++)
			charfont[2+(i << 1)]=win[(i << 1)+1];

		outp8(0xa5,0);							/* 右 */
		for(i=0;i<16;i++)
			charfont[3+(i << 1)]=win[(i << 1)+1];

		outp8(0x68,0xa);
	}else{										/* 通常漢字 */
		charfont[0]=2;
		charfont[1]=2;

		outp8(0x68,0xb);
		outp8(0xa3,(jcode >> 8)-0x20);
		outp8(0xa1,(char)jcode);

		for(i=0;i<32;i++)
			charfont[2+i]=win[i];

		outp8(0x68,0xa);
	}
}

void nocgw_fontget(uint jcode)				/* フォントを得る　安全かつ遅い */
{
	union REGS
		regs;

	if(jcode<256)
		jcode |= 0x8000;
	regs.x.dx=jcode;
	regs.x.bx=FP_SEG(charfont);
	regs.x.cx=FP_OFF(charfont);
	regs.h.ah=0x14;
	int86(0x18,&regs,&regs);
}

void fontget(uint jcode)
{
	if(cgwflag && jcode>=256)
		cgw_fontget(jcode);
	else
		nocgw_fontget(jcode);
}

void decor_fn(char *f)							/* ファイル名を美しく？する */
{
	char c;

	while(c=*f){
		if(iskanji(c))
			f++;
		else if( decor_caseis_upper && islower(c))
			*f=_toupper(c);
		else if(!decor_caseis_upper && isupper(c))
			*f=_tolower(c);
		else if(c=='\\')
			*f='/';
		f++;
	}
}

uint getanum(char **s)					/* 文字列→数字 オプション解析で使用 */
{
	uint r=0;
	char c;

	while(c=**s,(c && c!='/' && c!='-' && !isdigit(c)))
		(*s)++;			/* スペースをスキップ */

	if(!isdigit(**s))
		return(0xffff);

	while(isdigit(**s)){
		r*=10;
		r+=**s-'0';
		(*s)++;
	}
	return(r);
}

void tabconv(char *s)
{
	uint col,
		 spc;
	char c;

	for(col=0;*s;col++,s++){
		c=*s;
		if(c=='\t'){
			spc=tabsize-(col%tabsize)-1;
			sftstr(s+1,spc);
			setmem(s,spc+1,' ');
		}else if(iskanji(c)){
			if(c>0x86 || c<0x85 ||(c==0x86 && *(s+1)>0x9e) )
				col++;
			s++;
		}
	}
}

int cutline(char s[],int len)
{
	int i,n;
	char c;

/*		n:何バイト目か (di)
		i:何文字目か(0から)(cx)		*/

	for(i=0,n=0;;n++){
		c=s[n];
		if(iskanji(c)){
			if(c>0x86 || c<0x85 ||(c==0x86 && s[n+1]>0x9e) )
				i++;
			n++;
		}else if(c=='\t'){
			i+=tabsize - i%tabsize -1;
		}else if(c==0x0d){			/* CR */
			if(s[n+1]==0x0a)
				n++;
			break;
		}else if(c==0x0a)			/* LF */
			break;
		if(++i>=80)
			break;
	}
	n++;
	while(i>80){
		i--,i--;	/* Is't tricky? */
		n--,n--;
	}

	if(n>len)
		n=len;
/*	i=-1;
	while(c=s[n+i],(c==0x0a || c==0x0d || c==0x1a))
		i--;
	s[n+i+1]=NULL;
*/
	return(n);
}

void setmaxline(void)
{
	long
		filelen;
	int
		len,
		count=0;

	maxline=0;
	now_pointer=0;
	now_line=0;

	_line_pointer[0]=0;
	filelen=filelength(fileno(fp));
	if(filelen==0)
		return;

	if(countflag)
		printf("read      ");
	else
		printf("init      ");

	fseek(fp,0,SEEK_SET);
	while(1){
		if(++count>50){
			count=0;
			printf("\b\b\b\b%3u%%",(int)(now_pointer*100/filelen));
		}

		len=fread(readbuf,sizeof(char),170,fp);
		if(len==0)
			break;
		if(maxline>=LINEMAX){
			printf("  Sorry--- file too long.(over %5u)\x1b[37D",(int)LINEMAX);
			break;
		}
		len=cutline(readbuf,len);
		_line_pointer[maxline++]=len;
		fseek(fp,get_lp(maxline),SEEK_SET);
	}

	_line_pointer[maxline]=0;
	backelase(10);
}

uint pagetopline(int page)		/* そのページは一体何行目から始まるのかな？ */
{
	uint r;

	r=page*lineparpage*dansuu;
	if(r>maxline)
		return(maxline);
	else
		return(r);
}

void _pout_buf(int start,int width)	   /* startからwidthドットプリンタに転送 */
{
	void prnout_P2(int s,int w);
	void prnout_E2(int s,int w);
	void prnout_E4(int s,int w);
	int
		w;
	char
		lpos[30];

	if(printer==P2){
		sprintf(lpos,ESC"F%04d"ESC"J%04d",start,width);
		pputs(lpos);

		prnout_P2(start,width);
	}else if(printer==E2){
		w=start % 3;
		start-=w;
		width+=w;

		pputs(ESC"$");
		_pputc((char)(start / 3));
		_pputc((start / 3) >> 8);

		pputs(ESC"*\x27");
		_pputc((char)width);
		_pputc(width >> 8);
		prnout_E2(start,width);
	}else if(printer==E4){
		w=start % 6;
		start-=w;
		width+=w;

		pputs(ESC"$");
		_pputc((char)(start / 6));
		_pputc((start / 6) >> 8);

		pputs(ESC"*\x48");
		_pputc((char)width);
		_pputc(width >> 8);
		prnout_E4(start,width);
	}
}

int bufstat(int x)				/* xのあるバイトの状態（なんのこっちゃ？）*/
{
	char far
		*b;
	char
		mask;
	int
		i;

	if((x & 7)<=3)
		mask=0xf0;
	else
		mask=0x0f;

	x>>=3;
	x*=48;

	b=&bufprn[x];

	for(i=0;i<head_dots;i++)
		if(b[i] & mask)
			return(1);		/* 将来の拡張（あるか？）のため明示的に１を使う */

	return(0);
}

void print_bufprn(void)
{
	int
		mwidth=h_dots*dansuu,
		w,
		start;

	if(dpi==160 && dansuu==2)
		;
	else
		mwidth++;

	for(start=0;start<mwidth;){
		if(bufstat(start)){
			w=4;
			while(start+w<mwidth && bufstat(start+w))
				w+=4;
			_pout_buf(start,min(w,mwidth-start));
			start+=w;
		}else{
			start+=4;
		}
	}

	pputs("\r\n");
}

void oneline_cpy(int l16,int lprn)
{
	uint
		o1,
		op,
		n;

	n=(h_dots*dansuu)/8+1;

	o1=l16;
	op=lprn;
	for(;n>0;n--){
		bufprn[op]|=buf16[o1];
		o1+=16;
		op+=48;
	}
}

void clear_linebuf()						/* １６ドットバッファをクリア */
{
	setmem(buf16,BUF16SIZE,0x00);
	buf16_x=0;
}

void clear_printbuf()								/* 印刷バッファをクリア */
{
	far_setmem(bufprn,BUFPRNSIZE,0x00);
	bufprn_y=0;
}

void flush_linebuf()			/* １６ドットバッファを印刷バッファに転送 */
{								/* 一杯になれば印刷、その後クリア */
	int i;

	for(i=0;i<16;i++){
		oneline_cpy(i,bufprn_y++);					/* 適当な関数名だなぁ */
		if(bufprn_y>=head_dots){
			print_bufprn();
			clear_printbuf();
		}
	}

	clear_linebuf();
}

void flush_printbuf()						/* 印刷バッファを印字後クリア */
{
	if(bufprn_y>0)
		print_bufprn();
	clear_printbuf();
}

void set_hline(void)							/* 横線をバッファにセット */
{
	int
		i,
		j;

	j=bufprn_y;
	for(i=(h_dots*dansuu+7)/8;i>0;i--){
		bufprn[j]=0xff;								/* 注意：大雑把である。 */
		j+=48;
	}

	if(++bufprn_y>=head_dots){
		print_bufprn();
		clear_printbuf();
	}
}

void set_vline(void)			/* 各段の区切り、縦棒を１６ドットバッファに */
{
	int
		i,off;
	char
		c;

	off=16*(buf16_x >> 3);
	c=0x80 >> (buf16_x & 7);

	for(i=0;i<16;i++)
		buf16[off+i]|=c;

	if(dpi==160 && dansuu==2)				/* PC-P2 ２段のときは数えない */
		;
	else
		buf16_x++;
}

void readline(uint l)
{
	int w;

	fseek(fp,get_lp(l),SEEK_SET);
	w=_line_pointer[l];
	if(w)
		fread(readbuf,w,sizeof(char),fp);
	readbuf[w--]=NULL;

	while(w>=0 && (readbuf[w]==0x0d || readbuf[w]==0x0a || readbuf[w]==0x1a))
		readbuf[w--]=NULL;
	tabconv(readbuf);
}

void set_str(char *s,int bf)					/* s を１６ドットバッファに */
{
	uint
		off,
		sft,
		work,
		mask,
		d,
		i;

	off=16*(buf16_x >> 3);
	sft=8-(buf16_x & 7);

	while(*s){
		if(*s==' '){
			off+=16;
			s++;
			continue;
		}else if(iskanji(*s)){
			fontget(sfttojis(*s,*(s+1)));
			s+=2;
		}else{
			fontget(*s);
			s++;
		}
		if(charfont[1]==1){											/* ank */
			for(i=2;i<18;i++){
				work=charfont[i];
				if(bf){
					mask=work | ~(work>>1);
					work|=work << 1;
					work&=mask;
				}
				d=work << sft;
				buf16[off   ]|=d >> 8;
				buf16[off+16]|=(char)d;
				off++;
			}
		}else{														/* kanji */
			for(i=2;i<33;i+=2){
				work=(charfont[i] << 8)| charfont[i+1];
				if(bf){
					mask=work | ~(work>>1);
					work|=work << 1;
					work&=mask;
				}

				d=work >> (8-sft);
				buf16[off   ]|=d >> 8;
				buf16[off+16]|=d;
				d=work << sft;
				buf16[off+32]|=(char)d;
				off++;
			}
			off+=16;
		}
	}
}

void set_line(uint line)		/* line行を読み込んで１６ドットバッファに */
{
	char
		lnum[10],
		*s=readbuf;

	if(numflag){
		sprintf(lnum,"%6u�I",line+1);
		set_str(lnum,NO);
		buf16_x+=8*7;
	}

	readline(line);

	set_str(s,boldflag);
	buf16_x+=8*80;
}

void init_page(void)						/* 各ページ印刷前の初期化処理 */
{
	int i;

	if(__pstat()==0){
		i=0;
		printf("(BUSY)");
		while(__pstat()==0){
			if(i++>20000){
				i=0;
				putchar(' ');
				putchar('\b');
			}
		}
		backelase(6);
	}

	if(printer==P2){	/* 初期化と、ページ長の設定 */
		pputs(ESC"c1"ESC"T18");
		if(paper){
			pputs("\x1d\x41\x1");
			for(i=1;i<pageline/6;i++)
				pputs("\x40\x1");
			pputs("\x41\x1\x1e");
		}
	}else if(printer==E2 || printer==E4){
		pputs(ESC"@"ESC"3\x18");
		if(paper){
			pputs(ESC"C");
			pputc(0);
			pputc(pageline/6);
		}
	}

	h_dots=(numflag)?696:640;
	if(dpi==160 && dansuu==2)				/* PC-P2 ２段のときは数えない */
		;
	else
		h_dots++;

	clear_linebuf();
	clear_printbuf();
}

void print_header(int page)				/* 各ページの頭に来るヘッダを印刷 */
{
	char
		header3[30];

	sprintf(header3,"PAGE:%3d-%-3d TABS:%-3d\r\n",maxpage+1,page+1,tabsize);

	if(printer==P2){
		pputs(header1);
		pputs(header3);
		pputs(header2);
		pputs(	ESC"P"ESC"s2"
				"              CMPP V" VERSION
				ESC"s0"ESC"P\r\n");
	}else if(printer==E2 || printer==E4){
		pputs(header1);
		pputs(header3);
		pputs(header2);
		pputs(	ESC"p1"ESC"S1"
				"              CMPP V" VERSION
				ESC"T"ESC"p0\r\n");
	}
}

void new_page()									/* ページ印字終了の後始末 */
{
	flush_printbuf();

	if(paper==NF)
		pputs("\n\n");
	else
		pputc(12);

	if(printer==P2)
		pputs(ESC"c1"ESC"T16");
	else if(printer==E2 || printer==E4)
		pputs(ESC"@");

	_pflush();				/* ローカルスプール時・スプールバッファクリア */
}

void print_page(int page)									/* １ページ印刷 */
{
	int
		offset,
		vwid,
		i,j;

	offset=pagetopline(page);
	vwid=(maxline-offset-1)/dansuu+1;
	if(vwid>lineparpage)
		vwid=lineparpage;

	init_page();
	if(headerflag)
		print_header(page);

	bufprn_y+=2;	/* 893 */

	set_hline();

	for(i=0;i<vwid;i++){
		printf("\b\b\b\b%3u%%",(int)printed_line*100L/print_line);
		for(j=0;j<dansuu;j++){
			set_vline();
			set_line(offset + vwid*j + i);
			printed_line++;
		}
		if(dpi==160 && dansuu==2)
			buf16_x--;
		set_vline();
		flush_linebuf();
	}

	set_hline();

	new_page();
}

void printfile_err(void)
{
	static int flag=0;

	uint r;

	if(flag==1)
		return;

	printf("印刷を中止しますか？[YN]");
	while(1){
		r=bgetc();

		if(r==0x2b03 || (r>>8)==0x2e){	/* ESC , N */
			backelase(24);
			flag=1;
			return;
		}else if((r>>8)==0x15){			/* Y */
			backelase(24);
			end(-1,YES);
		}
	}
}

void print_file(char *filename)						/* 印刷・ファイルレベル */
{
	ulong
		lwork,
		lwork2;
	uint
		page;

	if(error_files>0)
		printfile_err();

	if(stricmp("STDIN",filename)==0 || stricmp("CON",filename)==0)
		fp=stdin;
	else
		fp=fopen(filename,"rb"); /* 厳密に動作させるためバイナリモードでopen */

	if(fp==NULL){
		error_files++;
		printf("er:開けません : [%s]\n",filename);
		return;
	}

	if(tabflag==OFF)
		if(strstr(filename,".C") || strstr(filename,".H"))
			tabsize=4;
		else
			tabsize=8;

	if(countflag)											/* 本格印刷処理 */
		printf("ファイル : [%s] ",filename);
	else
		printf("印 刷 中 : [%s] ",filename);

	setmaxline();										/* まず行数を数えて */

	if(maxline<dansuu)									/* 何頁になるか計算 */
		maxpage=0;
	else{
		maxpage=maxline-1;
		maxpage/=lineparpage*dansuu;
	}

	if(maxpage>endpage)
		maxpage=endpage;

	if(countflag){
		counted_files++;
		if(paper==NF){
			lwork=0;
			for(page=startpage;page<=maxpage;page++){
				lwork2=(pagetopline(page+1)-pagetopline(page)+dansuu-1)/dansuu;
				lwork2*=16;
				lwork2+=head_dots+1;
				lwork2/=head_dots;
				lwork2*=head_dots;
				lwork2+=head_dots*((headerflag)?4:2);

				lwork+=lwork2;
			}
			lwork*=60;				/* ちなみに60という数字に特に根拠はない */
			lwork/=dpi;			/* ここ迄で、lwork:=1/60in単位の長さ(のはず) */
			lwork*=254;							/* longならではの強引な計算 */
			lwork/=600;

			pagecount_nf+=lwork;
			if(lwork>=1000){
				printf("%dm %d.%dcm",(int)lwork/1000,
						(int)(lwork/10) % 100,(int)lwork % 10);
			}else{
				printf("%d.%dcm",(int)lwork/10,(int)lwork % 10);
			}
		}else{
			page=maxpage-startpage+1;
			pagecount[paper]+=page;
			printf("%u ページ",page);
		}
		putchar('\n');
	}else{
		printed_files++;

		_dos_getftime(fileno(fp),&filedate,&filetime);
		sprintf(header1," 　ファイル名：%-45s",filename);
		sprintf(header2,"　　作成時刻：%04d-%02d-%02d %2d:%02d:%02d"
						"　　印刷時刻：%04d-%02d-%02d %2d:%02d:%02d",
						(filedate>>9)+1980,(filedate>>5) & 15,filedate & 31,
						filetime>>11,(filetime>>5)&63,(filetime & 31)*2,
						nowdate.year,nowdate.month,nowdate.day,
						nowtime.hour,nowtime.minute,nowtime.second);

		printed_line=0;
		print_line=pagetopline(maxpage+1)-pagetopline(startpage);

		printf("print     ");

		for(page=startpage;page<=maxpage;page++)			/* さあ印刷だぁ */
			print_page(page);

		backelase(10);
		printf("\r印刷終了\n");
	}

	fclose(fp);
}

void exec_file(char *s)								/* s のファイルを印刷 */
{
	struct find_t
		f;
	char
		orgname[128],
		path[128],
		fn[20];
	int
		i=0;

	while(*s && *s!=' ' && *s!='\t')		/* 元になるファイル名を取り出す */
		orgname[i++]=*(s++);
	orgname[i]=NULL;

	decor_fn(orgname);											/* 趣味だね */

	if(paper==UNDEF || printer==UNDEF){
		error_files++;				/* プリンタ種類と用紙種類は指定が必須 */
		printf("er:用紙種類とプリンタ種類は指定が必須です : [%s]\n",orgname);
		return;
	}

	strcpy(fn,getpath(path,orgname));	/* それをパスとファイル名に分割して */

	if(fn[0]=='.'){								/* 手抜きのファイル名も許す */
		sftstr(fn,1);
		fn[0]='*';
	}

	strcpy(orgname,path);						/* path\filename を作って */
	strcat(orgname,fn);

	if(_dos_findfirst(orgname,_A_NORMAL | _A_RDONLY | _A_HIDDEN,&f)){/* 検索 */
		print_file(orgname);			/* 見つからなくても印刷しようとする */
		return;
	}

	do{
		strcpy(orgname,path);
		strcat(orgname,f.name);
		print_file(orgname);
	}while(_dos_findnext(&f)==0);							/* どんどん印刷 */
}

void setlineparpage(void)
{
	if(headerflag)
		lineparpage=(pagedot-3*head_dots)/16;
	else
		lineparpage=(pagedot			)/16;
}

void setop_P2(void)
{
	printer=P2;
	dpi=160;
	paper=UNDEF;
	head_dots=24;
}

void setop_E2(void)
{
	printer=E2;
	dpi=180;
	paper=UNDEF;
	head_dots=24;
}

void setop_E4(void)
{
	printer=E4;
	dpi=360;
	paper=UNDEF;
	head_dots=48;
}

void setop_FF(void)
{
	if(printer==UNDEF){
		error_files++;
		printf(er_prn);
		printf("/FF]\n");
	}else{
		paper=FF;
		pagedot=11*dpi;
		pageline=66;
		setlineparpage();
	}
}

void setop_A4(void)
{
	if(printer==UNDEF){
		error_files++;
		printf(er_prn);
		printf("/A4]\n");
	}else{
		paper=A4;
		pagedot=dpi/6;
		pagedot*=71;
		pageline=71;
		setlineparpage();
	}
}

void setop_B5(void)
{
	if(printer==UNDEF){
		error_files++;
		printf(er_prn);
		printf("/B5]\n");
	}else{
		paper=B5;
		pagedot=dpi/6;
		pagedot*=61;
		pageline=61;
		setlineparpage();
	}
}

void setop_B4(void)
{
	if(printer==UNDEF){
		error_files++;
		printf(er_prn);
		printf("/B4]\n");
	}else{
		paper=B4;
		pagedot=dpi/6;
		pagedot*=86;
		pageline=86;
		setlineparpage();
	}
}

void setop_NF(void)
{
	if(printer==UNDEF){
		error_files++;
		printf(er_prn);
		printf("/NF]\n");
	}else{
		paper=NF;
		pagedot=30*180*78;
		pagedot/=dpi;
		pageline=180*78;
		pageline/=dpi;
		setlineparpage();
	}
}

void setop_PI(char **s)
{
	uint i;

	(*s)+=2;

	i=getanum(s);
	if(printer==UNDEF){
		error_files++;
		printf(er_prn);
		printf("/PI]\n");
	}else{
		if(i<=1 || i>128){
			error_files++;
			printf(er_num);
			printf("/PIn]\n");
		}else{
			paper=OT;
			pagedot=i*dpi;
			pageline=i*6;
			setlineparpage();
		}
	}
}

void setop_WA(void)
{
	uint r;

	printf("/WA:何かキーを押してください。");
	r=bgetc();
	backelase(30);

	if(r==0x2b03 || r==0x001b)
		end(-1,YES);
}

void setop_HT(char **s)
{
	uint i;

	(*s)+=2;

	if(isdigit(**s)){
		i=getanum(s);
		if(i<1 || i>80){
			error_files++;
			printf(er_num);
			printf("/HTn]\n");
		}else{
			tabsize=i;
			tabflag=ON;
		}
	}else if(**s=='-'){
		tabflag=OFF;
		(*s)++;
	}
}

void setop_LP(char **s)
{
	uint i;

	(*s)+=2;

	i=getanum(s);
	if(i<1 || i>LINEMAX){
		error_files++;
		printf(er_num);
		printf("/LPn]\n");
	}else{
		lineparpage=i;
	}
}

void setop_PA(char **s)
{
	uint i;

	(*s)+=2;

	if(**s=='-'){
		startpage=1;
		(*s)++;
		if(isdigit(**s))
			endpage=getanum(s);
		else
			endpage=LINEMAX;
	}else if(isdigit(**s)){
		startpage=getanum(s);
		if(**s=='-'){
			(*s)++;
			if(isdigit(**s))
				endpage=getanum(s);
			else
				endpage=LINEMAX;
		}else
			endpage=startpage;
	}

	startpage--;
	endpage--;

	if(startpage>LINEMAX || endpage>LINEMAX){
		error_files++;
		printf("er:ページ番号が解釈不能です。 : [/PAn-m]\n");
	}else if(startpage>endpage){
		i=startpage;
		startpage=endpage;
		endpage=i;
		printf("er:ページ番号の指定に矛盾があるので交換します。 : [/PAn-m]\n");
	}
}

void setop_DN(char **s)
{
	uint i;

	(*s)+=2;

	i=getanum(s);
	if(i<1 || i>7){
		error_files++;
		printf(er_num);
		printf("/DNn]\n");
	}else{
		dansuu=i;
	}
}

void setop_LN(char **s)
{
	(*s)+=2;

	if(**s=='+'){
		numflag=ON;
		(*s)++;
	}else if(**s=='-'){
		numflag=OFF;
		(*s)++;
	}else{
		error_files++;
		printf(er_plu);
		printf("/LNx]\n");
	}
}

void setop_BF(char **s)
{
	(*s)+=2;

	if(**s=='+'){
		boldflag=ON;
		(*s)++;
	}else if(**s=='-'){
		boldflag=OFF;
		(*s)++;
	}else{
		error_files++;
		printf(er_plu);
		printf("/BFx]\n");
	}
}

void setop_66(char **s)
{
	(*s)+=2;

	if(**s=='+'){
		(*s)++;
		if(lineparpage<66){
			error_files++;
			printf("er:用紙が短いため不可能です : [/66+]\n");
		}else{
			lineparpage/=66;
			lineparpage*=66;
		}
	}else if(**s=='-'){
		(*s)++;
		setlineparpage();
	}else{
		error_files++;
		printf(er_plu);
		printf("/66x]\n");
	}

}

void setop_HE(char **s)
{
	(*s)+=2;

	if(**s=='+'){
		headerflag=ON;
		(*s)++;
		setlineparpage();
	}else if(**s=='-'){
		headerflag=OFF;
		(*s)++;
		setlineparpage();
	}else{
		error_files++;
		printf(er_plu);
		printf("/HEx]\n");
	}
}

void setop_NC(void)
{
	cgwflag=OFF;
}

void setop_CO(char **s)
{
	(*s)+=2;

	if(**s=='+'){
		countflag=ON;
		(*s)++;
	}else if(**s=='-'){
		countflag=OFF;
		(*s)++;
	}else{
		error_files++;
		printf(er_plu);
		printf("/COx]\n");
	}
}

void exec_option(char **s)					/* *s のオプションを解釈して実行 */
{
	uint
		opname;

	if(**s=='?'){
		(*s)++;
		puts(helpmes);
		exit(1);
	}
	opname=toupper(*(*s+1)) | (toupper(**s) << 8);
	switch(opname){
	case 'P2':	setop_P2();	 break;
	case 'E2':	setop_E2();	 break;
	case 'E4':	setop_E4();	 break;
	case 'FF':	setop_FF();	 break;
	case 'A4':	setop_A4();	 break;
	case 'B5':	setop_B5();	 break;
	case 'B4':	setop_B4();	 break;
	case 'NF':	setop_NF();	 break;
	case 'PI':	setop_PI(s); break;
	case 'WA':	setop_WA();	 break;
	case 'HT':	setop_HT(s); break;
	case 'LP':	setop_LP(s); break;
	case 'PA':	setop_PA(s); break;
	case 'DN':	setop_DN(s); break;
	case 'LN':	setop_LN(s); break;
	case 'BF':	setop_BF(s); break;
	case '66':	setop_66(s); break;
	case 'HE':	setop_HE(s); break;
	case 'NC':	setop_NC();	 break;
	case 'CO':	setop_CO(s); break;
	default:
		error_files++;					/* ファイルのエラーじゃないけどね */
		printf("er:解釈不能なオプションです : [");
		(*s)--;
		while(**s && **s!=' ' && **s!='\t')
			if(iskanji(**s)){
				putchar(*(*s)++);
				putchar(*(*s)++);
			}else{
				putchar(toupper(*(*s)++));
			}

		printf("]\n");
	break;
	}

	while(**s && **s!=' ' && **s!='\t' && **s!='-' && **s!='/')
		(*s)++;
}

void exec(char *str)									/* s を解釈して印字 */
{
	char
		*s=str;

	while(*s){
		while(*s==' ' || *s=='\t')			/* はじめのスペースを無視して */
			s++;
		if(*s==NULL)
			break;

		if(*s=='/' || *s=='-'){	
			s++;										/* オプションを実行 */
			exec_option(&s);
		}else{
			exec_file(s);							/* ファイル印刷を実行 */
			while(*s && *s!=' ' && *s!='\t')
				s++;
		}
	}
}

int main(char *av)										/* 嘘じゃないもん */
{
	int
		i;
	char
		*ev=_environ;

	for(i=0;i<MAX_PAPERS;i++)				/* 必ず初期化する主義なのじゃ */
		pagecount[i]=0;

	printf("�b�l�o�o Version "VERSION"  Copyright by Ken/ichiro (OPA)\n\n");

	_dos_gettime(&nowtime);							/* まず印刷時刻を得て */
	_dos_getdate(&nowdate);

	while(ev[0]){									/* 環境変数CMPPの処理 */
		if(strncmp("CMPP=",ev,5)==0){
			exec(&ev[5]);
			break;
		}
		ev=nextstr(ev);
	}

	exec(av);										/* コマンドラインを印字 */

	if(!isatty(fileno(stdin)))
		exec("STDIN");									/* 標準入力を印字 */

	if(counted_files){
		printf("\n必要な用紙：\n");
		if(pagecount_nf>1000)
			printf("\t%8s : %dm %d.%dcm\n",papername[NF],
					(int)pagecount_nf/1000,
					(int)(pagecount_nf/10) % 100,(int)pagecount_nf % 10);
		else if(pagecount_nf)
			printf("\t%8s : %d.%dcm\n",papername[NF],
					(int)pagecount_nf/10,(int)pagecount_nf % 10);
		for(i=1;i<MAX_PAPERS;i++)
			if(pagecount[i])
				printf("\t%8s : %d 枚\n",papername[i],pagecount[i]);
	}

	end(error_files,NO);
}
