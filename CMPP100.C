/*
	リスト圧縮印字プログラム  怒涛のバージョンアップ
				  Made by Ken/ichiro
 */
#include <stdio.h>
#include <dos.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "opa.h"

#define ESC			"\033"
#define FS			"\x01c"

#define AP			0
#define VP			1
#define	UP			2
#define PR			3

#define NF			0
#define A4			1
#define B4			2
#define B5			3
#define FF			4
#define OT			5

#define MAXFILES	50
#define FNAMESIZE	80
#define LINEMAX		4000

char header1[85],
	 header2[85],
	 header3[35],
	 charfont[34],
	 readbuf[170],
	 title[]=	"�ｱ�｣�｣�｣�｣�｣�｣�｣�｣�｣�｣�｣�｣�｣�｣�｣�｣�｣�｣�｣�｣�ｵ\n"
				"�･ファイル圧縮印刷ユーティリティ ＣＭＰＰ �･\n"
				"�･           Made by Ken/ichiro           �･\n"
				"�ｹ�｣�｣�｣�｣�｣�｣�｣�｣�｣�｣�｣�｣�｣�｣�｣�｣�｣�｣�｣�｣�ｽ",
	helpmes[]=
	"使用法：\n"
		"\tＣＭＰＰ [スイッチ] [ファイル名...]\n"
	"プリンタ用紙を節約して印字します。\n"
	"使用できるオプションは次の通りです。(括弧内は初期値)\n\n"
	"プリンタの種類：\n"
		"\t/PR\tＮＥＣ８０桁モード\n"
		"\t/AP\tエプソン８０桁モード（初期値）\n"
		"\t/VP\tエプソン１３５桁モード\n"
		"\t/UP\tエプソンＵＰ−１３５Ｋ(PC)モード\n"
	"用紙の種類：\n"
		"\t/FF\tファンフォールド紙\n"
		"\t/A4\tＡ４単票用紙\n"
		"\t/B5\tＢ５単票用紙\n"
		"\t/B4\tＢ４単票用紙\n"
		"\t/NF\t連続用紙（初期値）\n"
		"\t/PIn\tインチ単位ページ長指定 n=1~21\n"
	"その他：\n"
		"\t/HTn\tタブ間隔　n=1~80(8、拡張子が.cか.hのとき4)\n"
		"\t/LPn\t１ページ１段に入れる行数を指定 n=1以上(用紙による)\n"
		"\t/SPn\t印刷開始ページを指定 n=1以上(1)\n"
		"\t/EPn\t印刷終了ページを指定 n=開始ページ以上(最終ページ)\n"
		"\t/DNn\t段組数を指定 n=1~3(AP,PRのとき2 VP,UPのとき3)\n"
		"\t/LN\t行番号をつけて印刷する\n"
		"\t/BF\t文字を太くして印刷する\n"
		"\t/NH\tヘッダ印刷を抑制する\n"
		"\t/CO\t印刷せずにページ数を表示する\n";

typedef union charint{
	uns char c[2];
	uns int  i;
}charint;

struct dosdate_t nowdate;
struct dostime_t nowtime;
union REGS regs;
charint charbuf[16];
FILE *fp;
uns long line_pointer[LINEMAX],
		 total=0;
uns short int	maxline,
				tabsize,
				tabflag=OFF,
				printer=AP,
				paper=NF,
				pagelen=65535U,
				lineparpage=65535U,
				numflag=OFF,
				boldflag=OFF,
				headflag=ON,
				countflag=OFF,
				startpage=1,
				endpage=65535U,
				dansuu=65535U,
				compflag=ON,
				nthbit[]={0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};

int iskanji(char c)
{
	return(0x81<=c && c<=0x9f || 0xe0<=c && c<=0xfc);
}

void sftstr(char s[],int w)
{
	int i;

	for(i=0;s[i];i++);
	for(;i>=0;i--)
		s[i+w]=s[i];
}

char *getpath(char *d,char *s)
{
	int i,n;

	for(i=0,n=0;s[i];i++){
		if(s[i]=='\\' || s[i]==':')
			n=i+1;
		else if(iskanji(s[i]) && s[i+1]!=NULL)
			i++;
	}
	strncpy(d,s,n);
	d[n]=NULL;
	return(&s[n]);
}

int wildcard(char *buf,int maxfiles,int bufwidth,char *path,int *n)
{
	struct	find_t	f;
	char	dir[80],
			fn[20];

	strcpy(fn,getpath(dir,path));
/*
	if(strchr(fn,'.')==NULL)	/* 拡張子補完 */
		strcat(fn,".*");
	if(fn[0]=='.'){				/* ファイル名補完 */
		sftstr(fn,1);
		fn[0]='*';
	}
*/

	strcpy(buf,dir);
	strcat(buf,fn);
	if(_dos_findfirst(buf,_A_NORMAL | _A_RDONLY | _A_HIDDEN,&f))
		return(1);
	do{
		if(*n>=maxfiles)
			break;
		strcpy(buf,dir);
		strcat(buf,f.name);
		buf+=bufwidth;
		(*n)++;
	}while(_dos_findnext(&f)==0);

	return(0);
}

namecomp(char *p,char *q)
{
	return(stricmp(p,q));
}

void pputc(char c)
{
	regs.h.ah=0x11;
	regs.h.al=c;
	int86(0x1a,&regs,&regs);
}

void pputs(char *s)
{
	while(*s)
		pputc(*s++);
}

void fontget(int code,char far *x)
{
	if(code<256)
		code |= 0x8000;
	regs.x.dx=code;
	regs.x.bx=FP_SEG(x);
	regs.x.cx=FP_OFF(x);
	regs.h.ah=0x14;
	int86(0x18,&regs,&regs);
}

int cutline(char s[])
{
	int i,n;

	for(i=0,n=0;i<80 && s[n];n++,i++){
		if(iskanji(s[n])){
			if(s[n]>0x86 || s[n]<0x85 ||(s[n]==0x86 && s[n+1]>0x9e) )
				i++;
			n++;
		}else if(s[n]=='\t')
			i+=tabsize - i%tabsize -1;
		else if(s[n]=='\n')
			i--;
	}
	if(i==81)
		n-=2;
	else if(i<80 && !iskanji(s[n]))
		n++;
	return(n);
}

void setmaxline(void)
{
	int line=0;
	maxline=0;

	while(line_pointer[line]=ftell(fp),!feof(fp)){
		if(!fgets(readbuf,161,fp))
			break;
		if(maxline>=LINEMAX){
			printf("Sorry--- file too long.");
			break;
		}
		maxline++;
		fseek(fp,line_pointer[line++],SEEK_SET);
		fseek(fp,cutline(readbuf),SEEK_CUR);
	}
	fgets(readbuf,161,fp);
	if(strlen(readbuf)!=0){
		maxline++;
		line_pointer[++line]=ftell(fp);
	}
}

void tobold(void)
{
	int i;
	charint b;
	uns int mask;

	if(charfont[1]==1)
		for(i=2;i<18;i++){
			mask=charfont[i] | ~(charfont[i]>>1);
			charfont[i]|=charfont[i]<<1;
			charfont[i]&=mask;
		}
	else
		for(i=2;i<34;i+=2){
			b.c[1]=charfont[i];
			b.c[0]=charfont[i+1];

			mask=b.i | ~(b.i>>1);
			b.i|=b.i<<1;
			b.i&=mask;

			charfont[i]=b.c[1];
			charfont[i+1]=b.c[0];
		}
}

void fontp(void)
{
	int i,n;

	if(boldflag)
		tobold();

	for(i=0;i<16;i++)
		charbuf[i].i=0;

	if(printer==PR || printer==UP){
		if(charfont[1]==1){
			for(n=0;n<8;n++)
				for(i=17;i>1;i--){
					charbuf[n].i<<=1;
					charbuf[n].i+= ((charfont[i] & nthbit[n])!=0);
				}
			pputs(ESC"I0008");
			for(i=0;i<8;i++){
				pputc(charbuf[i].c[0]);
				pputc(charbuf[i].c[1]);
			}
		}else{
			for(n=0;n<8;n++)
				for(i=16;i>0;i--){
					charbuf[n].i<<=1;
					charbuf[n].i+=((charfont[i*2] & nthbit[n])!=0);
					charbuf[n+8].i<<=1;
					charbuf[n+8].i+=((charfont[i*2+1] & nthbit[n])!=0);
				}
			pputs(ESC"I0016");
			for(i=0;i<16;i++){
				pputc(charbuf[i].c[0]);
				pputc(charbuf[i].c[1]);
			}
		}
	}else{
		if(charfont[1]==1){
			for(n=0;n<8;n++)
				for(i=2;i<=17;i++){
					charbuf[n].i<<=1;
					charbuf[n].i+=((charfont[i] & nthbit[n])!=0);
				}
			pputs(ESC"*\x27\x8");pputc(0);
			for(i=0;i<8;i++){
				pputc(charbuf[i].c[1]);
				pputc(charbuf[i].c[0]);
				pputc(0);
			}
		}else{
			for(n=0;n<8;n++)
				for(i=1;i<=16;i++){
					charbuf[n].i<<=1;
					charbuf[n].i+=((charfont[i*2] & nthbit[n])!=0);
					charbuf[n+8].i<<=1;
					charbuf[n+8].i+=((charfont[i*2+1] & nthbit[n])!=0);
				}
			pputs(ESC"*\x27\x10");pputc(0);
			for(i=0;i<16;i++){
				pputc(charbuf[i].c[1]);
				pputc(charbuf[i].c[0]);
				pputc(0);
			}
		}
	}
}

void pputjis(char slo,char shi)
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

	if(compflag==OFF){
		if(printer==PR || printer==UP){
			pputc(27);pputc(75);
			pputc(jhi);pputc(jlo);
			pputc(27);pputc(72);
		}else{
			pputc(28);pputc(38);
			pputc(jhi);pputc(jlo);
			pputc(28);pputc(46);
		}
	}else{
		fontget(jhi*256+jlo,charfont);
		fontp();
	}
}

void _pputc(char c)
{
	if(compflag==OFF){
		pputc(c);
		return;
	}
	fontget(c,charfont);

	fontp();
}

void tabconv(char *s)
{
	uns int col,
			spc;

	for(col=0;*s;col++,s++){
		if(*s=='\t'){
			spc=tabsize-(col%tabsize)-1;
			sftstr(s+1,spc);
			setmem(s,spc+1,' ');
		}
	}
}

void line_print(char *s)
{
	for(;*s;s++)
	{
		if(iskanji(*s)){
			pputjis(*(s+1),*s);
			s++;
		}else if(!isspace(*s))
			_pputc(*s);
		else
			if(compflag)
				if(printer==PR || printer==UP){
					pputs(ESC"W0008");
					pputc(0);pputc(0);
				}else{
					pputs(ESC"\\\x8");
					pputc(0);
				}
			else
				pputc(' ');
	}
}

void _line_print(int l)
{
	int w;
	char lnum[6];

	if(numflag){
		w=boldflag;
		boldflag=OFF;
		sprintf(lnum,"%4d|",l+1);
		line_print(lnum);
		boldflag=w;
	}
	fseek(fp,line_pointer[l],SEEK_SET);
	w=line_pointer[l+1]-line_pointer[l]+1;
	fgets(readbuf,w,fp);
	readbuf[w]=NULL;
	tabconv(readbuf);
	line_print(readbuf);
}

void page_print(int page,int maxpage)
{
	int i,j,dan,bline,hwid,vwid;
	char lpos[30];

	compflag=OFF;			/* 前処理 */
	sprintf(header3,"PAGE:%3d-%-3d TABS:%-3d\r\n",maxpage,page,tabsize);
	bline=(page-1)*lineparpage*dansuu;
	vwid=(maxline-bline)/dansuu+1;
	if(vwid>lineparpage)
		vwid=lineparpage;
	if(numflag==ON)
		hwid=680;
	else
		hwid=640;

	j=boldflag;
	boldflag=ON;

	if(printer==PR){			/* ヘッダを印字 */
		pputs(ESC"c1"ESC"T16");
		if(paper){
			pputs("\x1d\x41\x1");
			for(i=1;i<pagelen;i++)
				pputs("\x40\x1");
			pputs("\x41\x1\x1e");
		}
		if(headflag){
			line_print(header1);
			pputs(header3);
			line_print(header2);
			compflag=ON;
			line_print("          CMPP by Ken/ichiro");
			pputs("\r\n");
		}
		pputs(ESC"T12");
	}else if(printer==AP || printer==VP){
		pputs(ESC"@");
		if(paper){
			pputs(ESC"C");
			pputc(pagelen);
		}
		if(headflag){
			pputs(ESC"A\x8");
			line_print(header1);
			pputs(header3);
			line_print(header2);
			pputs(	ESC"p1"ESC"S1"
					"          CMPP by Ken/ichiro"
					ESC"T"ESC"p0\r"ESC"J\x10");
		}
		pputs(ESC"3\x10");
	}else{
		pputs(ESC"T16");
		if(paper){
			pputs("\x1d\x41\x1");
			for(i=1;i<pagelen;i++)
				pputs("\x40\x1");
			pputs("\x41\x1\x1e");
		}
		if(headflag){
			line_print(header1);
			pputs(header3);
			line_print(header2);
			compflag=ON;
			line_print("          CMPP by Ken/ichiro");
			pputs("\r\n");
		}
		pputs(ESC"T12");
	}
	boldflag=j;

	compflag=ON;
	for(i=0;i<dansuu;i++)		/* 横線を印刷 */
		if(printer==PR){
			sprintf(lpos,ESC"W%04d\x80\x01",hwid);
			pputs(lpos);
		}else if(printer==UP){
			sprintf(lpos,ESC"W%04d\x80\x01",hwid+1);
			pputs(lpos);
		}else{
			pputs(ESC"K");
			pputc((hwid+2)/3);
			pputc(0);
			for(j=0;j<(hwid+2)/3;j++)
				pputc(0x08);
		}
	pputc('\r');pputc('\n');

	for(i=0;i<vwid;i++){
		if(printer==PR || printer==UP)	/* 一番左の縦線を印刷 */
			pputs(ESC"S0001\xff");
		else{
			pputs(ESC"*\x27\1");
			pputc(0);
			pputs("\xff\xff");
			pputc(0);
		}

		for(dan=0;dan<dansuu;dan++){	/* ファイルの中身を印字 */
			if(bline+i+vwid*dan<maxline){
				_line_print(bline+i+vwid*dan);
			}
			if(printer==PR){			/* ２番目以降の縦線を印字 */
				sprintf(lpos,"\r"ESC"F%04d"ESC"S0001\xff",(dan+1)*hwid-1);
				pputs(lpos);
			}else if(printer==UP){
				sprintf(lpos,ESC"F%04d"ESC"S0001\xff",(dan+1)*hwid);
				pputs(lpos);
			}else{
				pputs(ESC"$");
				pputc(((dan+1)*(hwid+1)/3+1)%256);
				pputc(((dan+1)*(hwid+1)/3+1)/256);
				pputs(ESC"*\x27\1");pputc(0);pputs("\xff\xff");pputc(0);
			}
		}
		pputc('\r');pputc('\n');
		putchar(' ');
		putchar('\b');
	}

	for(i=0;i<dansuu;i++)		/* 下の横線を印字 */
		if(printer==PR){
			sprintf(lpos,ESC"W%04d\x80\x01",hwid);
			pputs(lpos);
		}else if(printer==UP){
			sprintf(lpos,ESC"W%04d\x80\x01",hwid+1);
			pputs(lpos);
		}else{
			pputs(ESC"K");
			pputc((hwid+2)/3);
			pputc(0);
			for(j=0;j<(hwid+2)/3;j++)
				pputc(0x40);
	   }
	pputc('\r');

	if(paper==NF)		/* 改ページ・間隔をあける */
		pputs("\n\n");
	else
		pputc(12);
}

void file_print(void)
{
#define cm page
	uns int page,
			maxpage;

	setmaxline();

	maxpage=maxline/(lineparpage*dansuu)+1;

	if(countflag){
		if(paper){
			printf("\t%d ページ",maxpage);
			total+=maxpage;
		}else{
			cm=maxpage*8 + maxline/dansuu +1;
			cm=(printer==PR || printer==UP) ? +(cm*3)/5 : +(cm*8)/15;
			cm=+(cm*15)/38;
			if(cm>99)
				printf("\t%dm %dcm",cm/100,cm%100);
			else
				printf("\t%dcm",cm%100);
			total+=cm;
        }
	}else{
		for(page=startpage;page<=maxpage && page<=endpage;page++)
			page_print(page,maxpage);

		if(printer==PR)
			pputs(ESC"c1"ESC"T16");
		else if(printer==AP || printer==VP)
			pputs(ESC"@");
		else
			pputs(ESC"T16");
	}

#undef cm
}

void argfunc(char *a)
{
	a++;		/* プレフィクスをスキップ */
	if(*a=='?'){
		puts(title);
		puts(helpmes);
		exit(1);
	}else if(!stricmp(a,"NF"))
		paper=NF;
	else if(!stricmp(a,"FF"))
		paper=FF;
	else if(!stricmp(a,"A4"))
		paper=A4;
	else if(!stricmp(a,"B4"))
		paper=B4;
	else if(!stricmp(a,"B5"))
		paper=B5;
	else if(!stricmp(a,"PR"))
		printer=PR;
	else if(!stricmp(a,"AP"))
		printer=AP;
	else if(!stricmp(a,"VP"))
		printer=VP;
	else if(!stricmp(a,"UP"))
		printer=UP;
	else if(!stricmp(a,"LN"))
		numflag=ON;
	else if(!stricmp(a,"BF"))
		boldflag=ON;
	else if(!stricmp(a,"NH"))
		headflag=OFF;
	else if(!stricmp(a,"CO"))
		countflag=ON;
	else if(!strnicmp(a,"HT",2)){
		tabsize=atoi(&a[2]);
		tabflag=ON;
	}else if(!strnicmp(a,"PI",2)){
		paper=OT;
		pagelen=atoi(&a[2])*6;
	}else if(!strnicmp(a,"LP",2))
		lineparpage=atoi(&a[2]);
	else if(!strnicmp(a,"SP",2))
		startpage=atoi(&a[2]);
	else if(!strnicmp(a,"EP",2))
		endpage=atoi(&a[2]);
	else if(!strnicmp(a,"DN",2))
		dansuu=atoi(&a[2]);
	else{
		puts("変なスイッチを指定しないで下さい。");
		exit(1);
	}
}

int main(int argc,char *argv[])
{
	uns int i=0,
			j=0,
			date,
			time;

	static char fname[MAXFILES][FNAMESIZE];

	_dos_gettime(&nowtime);
	_dos_getdate(&nowdate);

	for(i=1;i<argc;i++){
		if(argv[i][0]=='-' || argv[i][0]=='/')
			argfunc(argv[i]);
		else{
			if(stricmp("STDIN",argv[i])==0 || stricmp("CON",argv[i])==0){
				if(j<MAXFILES)
					strcpy(fname[j++],"STDIN");
			}else{
				if(wildcard(fname[j],MAXFILES,FNAMESIZE,argv[i],&j)){
					puts("ファイルが見つからないのですが。");
					exit(1);
				}
			}
		}
	}

	if(j==0)
		if(isatty(0))		/* 0 is handle number of STDIN */
			argfunc("/?");	/* Do help */
		else
			strcpy(fname[j++],"STDIN");
	else
		puts(title);

	if(j>=MAXFILES)
		puts("ファイル数が多いため印刷されないファイルがあります。");

	if(pagelen==65535U){
		if(paper==FF)
			pagelen=66;
		else if(paper==A4)
			pagelen=71;
		else if(paper==B4)
			pagelen=86;
		else if(paper==B5)
			pagelen=61;
		else
			pagelen=49563;
	}

	if(lineparpage==65535U)
		if(pagelen==49563)
			lineparpage=132;
		else{
			if(printer==PR || printer==UP)
				lineparpage=+((pagelen-7)*5)/3;
			else
				lineparpage=+((pagelen-7)*15)/8;
		}

	if(pagelen==49563)
		if(printer==PR || printer==UP)
			pagelen=87;
		else
			pagelen=78;
	else if(headflag==OFF)
		lineparpage+=3;

	if(dansuu==65535U)
		if(printer==PR || printer==AP)
			dansuu=2;
		else
			dansuu=3;

	if(tabflag && (tabsize<1 || tabsize>80)){
		puts("あまり複雑なことをさせないで下さい。");
		exit(1);
	}
	if(pagelen<5 || pagelen>768){
		puts("変わった紙があるんですな。");
		exit(1);
	}
	if(lineparpage<1 || lineparpage>LINEMAX){
		puts("ヘッダだけ無限に印刷するのですか？");
		exit(1);
	}
	if(startpage<1 || endpage<1 || startpage>endpage){
		puts("反転印刷なんて機能はついてないんです。");
		exit(1);
	}
	if(dansuu<1 || dansuu>3){
		puts("そういう印刷はしない方があなたのためです。");
		exit(1);
	}
	qsort(fname,j,FNAMESIZE,namecomp);

	for(i=0;i<j;i++){
		if(!stricmp(fname[i],"STDIN"))
			fp=stdin;
		else
			fp=fopen(fname[i],"rt");

		if(tabflag==OFF)
			if(strstr(fname[i],".C") || strstr(fname[i],".H"))
				tabsize=4;
			else
				tabsize=8;

		if(fp==NULL){
			puts("申し訳ありません。ファイルが開けません。");
		}else{
			_dos_getftime(fileno(fp),&date,&time);

			sprintf(header1," 　ファイル名：%-45s",
							fname[i]);
			sprintf(header2,"　　作成時刻：%04d-%02d-%02d %2d:%02d:%02d"
							"　　印刷時刻：%04d-%02d-%02d %2d:%02d:%02d",
							(date>>9)+1980,(date>>5) & 15,date & 31,
							time>>11,(time>>5)&63,(time & 31)*2,
							nowdate.year,nowdate.month,nowdate.day,
							nowtime.hour,nowtime.minute,nowtime.second);
			if(countflag)
				printf("ファイル : %s",fname[i]);
			else
				printf("印 刷 中 : %s",fname[i]);
			file_print();
			if(countflag)
				putchar('\n');
			else
				printf("\r印刷終了 : %s\n",fname[i]);
			fclose(fp);
		}
	}
	if(countflag && j>1)
		if(paper)
			printf("合計： %d ページ\n",total);
		else{
			if(total>99){
				i=total/100;
				j=total%100;
				printf("合計： %d m %d cm\n",i,j);
			}else
				printf("合計： %d cm\n",total);
	   }
}
