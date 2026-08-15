/*
  This is our disassembler for OSK, written by Dan Poole and I long ago,
  when we were using OSK in product.  It was written for OSK on OSK using
  the Microware compiler, which was more or less K&R at the time.  It has
  since been updated to compile with a modern compiler on Linux and on
  Mac OS X.

  The dissassembler doesn't really know what to do without a parameter
  file to direct it.  It is not smart enough to figure out the contents
  of a binary by itself.  Normally one examines a hex dump of the binary
  to figure out what is text, what is code, and what is data.  It often
  takes a couple iterations to sort out. If you see garbage output instead
  of clean disassembly, it may be data or text instead of code. Fix the
  parameter file and try again.

  The parameter file does not have to be specified on the command line if
  it's name is the name of the binary with ".prm" tacked on. That is what
  dis68 will look for unless a name is provided with the -p option.  The
  format of the parm file is a letter specifying what this block should
  be and a hex address of the last byte of the block. For example, the
  parm file for the included demonstration binary, bsrl, is this:

  l 47
  t 4d
  c 5f

  Since it is an OS-9/68K module and dis68 is not told otherwise, it will first
  take apart the module header. Next, it will assume long (32 bit) data up
  to and including address 0x47, text up to and including address 0x4d,
  and code up to and including address 0x5f.

  I have included a trivial demonstration binary, bsrl, it parmeter file,
  bsrl.prm, the resulting dissassembly, bsrl.out, and the source that can
  be generated, bsrl.a.  In addition, I have included the paramerer file
  I used to disassemble the first part of the AT306 ROMs, rom.prm.

  Good luck and happy hacking!

  Carl Kreider
*/

/* main driver for 68K disassembler */
/*  -m  file is not module format */
/*  -s  create source do not print binary */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#ifdef WIN32
typedef unsigned int u_int;
typedef unsigned short u_short;
#endif
#include "dis68.h"

int prnflg = 1;			/* default to pretty printing on */
int mdfmt = 1, pso = 0;
char *fn, *an, *parmfile;

char mnebf[80];
char str[80];			/* temporary string */
u_short opcode[6];
int opwcnt;
int opbcnt;
u_int ppc;
char type;
int pass;
int pcsflg = 0;			/* pc symbol table empty flag */

static u_int psdofs = 0;
static u_int cend;
static u_int nxpcref;		/* next pc reference */

FILE *cfp, *pfp, *ofp;

static int help();
static void readtyp();
static void byte();
static void word();
static void lng();
static void offset();;
static void address();
static void text();
static void psect();
static void prnsym();
static void prnpcsym();
static void prnsrc();
static int htoi(char *s);



int main(int argc, char **argv)
{
	int opt;

	while (-1 != (opt = getopt(argc, argv, "mo:p:s")))
		switch (opt)
		{
		case 'm':
			mdfmt = 0;
			break;

		case 'o':
			psdofs = htoi(optarg);	/* get starting offset */
			break;

		case 'p':
			parmfile = optarg;	/* use different parmfile */
			break;	/* or say "none"                */

		case 's':
			prnflg = 0;	/* print for assembler */
			break;

		default:
			help();
		}

	if (argc == optind)
		help();

	fn = argv[optind + 0];
	if ((cfp = fopen(fn, "r")) == (FILE *) 0)
		exit(_errmsg(errno, "cannot open %s\n", fn));

	if (argc > (optind + 1))
	{
		an = argv[optind + 1];
		if ((ofp = fopen(an, "w")) == (FILE *) 0)
			exit(_errmsg(errno, "cannot open %s\n", an));
	}

	if (0 == parmfile)
		strcat(strcpy(str, fn), ".prm");
	else
		strcpy(str, parmfile);

	if ((pfp = fopen(str, "r")) == (FILE *) 0)
		exit(_errmsg(errno, "cannot open %s\n", str));

	for (pass = 1; pass < 3; pass++)
	{
		ppc = psdofs;
		if (pass == 2)
		{
			prnsym();	/* dump the register symbol table */
			nxpcref = any_pc_sym();
			prnpcsym();
		}

		if (mdfmt)
			psect();

		opwcnt = 0;
		while (!feof(pfp))
		{
			readtyp();
			while (ppc <= cend)
			{
				opwcnt = opbcnt = 0;
				*mnebf = 0;
				switch (type)
				{
				case 's':	/* skip (offset) */
					ppc = cend;	/* set new pc */
					cend = 0;	/* force termination of block */
					prnpcsym();
					continue;

				case 'b':	/* byte data */
					pso = 1;
					byte();
					break;

				case 'w':	/* word data */
					pso = 1;
					word();
					break;

				case 'l':	/* long data */
					pso = 1;
					lng();
					break;

				case 't':	/* text data */
					pso = 1;
					text();
					break;

				case 'o':	/* word offsets */
					pso = 1;
					offset();
					break;

				case 'a':	/* long offsets */
					pso = 1;
					address();
					break;

				case 'c':	/* code */
					if (pso)
					{
						pso = 0;
						if (pass == 2)
							printf("\n");
					}
					disasm();
					break;

				default:
					exit(_errmsg
					     (1,
					      "bad parameter file type %c\n",
					      type));
				}

				if (pass == 2)
					prnsrc();

				ppc += opbcnt;
			}
		}

		rewind(cfp);
		rewind(pfp);
	}

	ppc = 0xffffff;
	prnpcsym();
	if (mdfmt)
		printf(" ends\n\n");

	printf(" end\n");
}

static int help()
{
	fprintf(stderr, "Usage: dis68 [options] infile [outfile]\n");
	fprintf(stderr, "  -m         file not module format\n");
	fprintf(stderr, "  -o[=]nnnn  use logical offset of nnnn (hex)\n");
	fprintf(stderr, "  -p name    use 'name' for parmfile\n");
	fprintf(stderr, "  -s         create assembler source\n");
	exit(1);
}


static void readtyp()
{
	char tmp[80];

	type = getc(pfp);
	fgets(tmp, 79, pfp);
	cend = psdofs + htoi(tmp);
}


static void byte()
{
	int i;
	u_short dat;

	strmne("dc.b ");
	dat = getdat(1);
	sprintf(str, "$%02x,$%02x", (dat >> 8) & 0xff, dat & 0xff);
	strmne(str);
	for (i = 1; i < 4 && ((ppc + 2 * i) <= cend); i++)
	{
		if ((ppc + 2 * i) >= nxpcref)
			break;

		dat = getdat(1);
		sprintf(str, ", $%02x,$%02x", (dat >> 8) & 0xff, dat & 0xff);
		strmne(str);
	}

	opbcnt = opwcnt << 1;
}


static void word()
{
	u_short dat;

	strmne("dc.w ");
	dat = getdat(1);
	sprintf(str, "$%04x", dat);
	strmne(str);
	opbcnt = opwcnt << 1;
}

static void lng()
{
	u_int ldat;
	char str[20];

	strmne("dc.l ");
	ldat = getldat(1);
	sprintf(str, "$%08x", ldat);
	strmne(str);
	opbcnt = opwcnt << 1;
}


static void offset()
{
	u_short dat;

	strmne("dc.w ");
	dat = getdat(1);
	dat &= 0xffff;
	sprintf(str, "z%06hx", dat);
	strmne(str);
	add_pc_sym(dat);
	opbcnt = opwcnt << 1;
}


static void address()
{
	u_int pcl;
	char str[20];

	strmne("dc.l ");
	pcl = getldat(1);
	sprintf(str, "z%06x", pcl & 0xffffff);
	strmne(str);
	add_pc_sym(pcl);
	opbcnt = opwcnt << 1;
}

static void text()
{
	int i, flag;
	u_short dat;

	strmne("dc.b ");
	flag = 0;
	for (i = 0; i < 32 && ((ppc + i) <= cend); i++)
	{
		opbcnt += 1;
		dat = getc(cfp);
		if ((dat & 0x7f) < 0x20)
		{
			if (flag)
				strmne("\",");

			flag = 0;
			sprintf(str, "$%02x", 0xff & dat);
			strmne(str);
			break;
		}

		if (!flag)
		{
			strmne("\"");
			flag = 1;
		}

		sprintf(str, "%c", (char) dat);
		strmne(str);
	}

	if (flag)
		strmne("\"");
}

static void psect()
{
	int typelang, attrrev, edtion;
	u_int mname, stksz, mexec, mtrap, mmem;

	eatdat(12);		/* get M$ID, M$SysRev, M$Size, M$Owner */
	mname = getldat(0);	/* M$Name */
	eatdat(2);		/* M$Accs */
	typelang = getdat(0) & 0xffff;	/* typelang */
	attrrev = getdat(0) & 0xffff;	/* attrrev */
	edtion = getdat(0);	/* M$Edit */
	stksz = getldat(0);	/* M$Usage */
	eatdat(20);		/* M$symbol through M$Parity */
	mexec = getldat(0);	/* M$Exec */
	add_pc_sym(mexec);
	mtrap = getldat(0);	/* M$expt */
	mmem = getldat(0);	/* M$Mem */
	ppc += 0x3c;

	(void) mname;		/* unused for now */

	if (pass == 2)
	{
		if (prnflg)
		{
			printf("%04X            Type_Lang equ       $%04X\n",
			       typelang, typelang);
			printf("%04X            Attr_Rev  equ       $%04X\n",
			       attrrev, attrrev);
			printf("%04X            Edition   equ       $%04X\n",
			       edtion, edtion);
			printf("%06X           stksz     equ       $%06X\n",
			       stksz, stksz);
			printf("%06X           mexec     equ       Z%06x\n",
			       mexec, mexec);
			if (mtrap)
				printf("%06X           mtrap    equ       $%06X\n", mtrap, mtrap);

			printf("                         ");
			printf(" psect mname,Type_Lang,Attr_Rev,Edition,stksz,mexec");
			if (mtrap)
				printf(",mtrap");

			putchar('\n');
			if (mmem)
			{
				printf("                         ");
				printf(" vsect\n");
				printf("                         ");
				printf(" rmb $%06X\n", mmem);
				printf("                         ");
				printf(" ends\n\n");
			}
		}
		else
		{
			printf("Type_Lang equ $%04X\n", typelang);
			printf("Attr_Rev equ $%04X\n", attrrev);
			printf("Edition equ $%04X\n", edtion);
			printf("stksz equ $%08X\n", stksz);
			printf("mexec equ Z%06x\n", mexec);
			if (mtrap)
				printf("mtrap equ $%06X\n", mtrap);

			printf(" psect mname,Type_Lang,Attr_Rev,Edition,stksz,mexec");
			if (mtrap)
				printf(",mtrap");

			putchar('\n');
			if (mmem)
				printf(" vsect\n ds.b $%06X\n ends\n", mmem);
		}
	}
}

/*
 * print out register symbol tables
 */

static void prnsym()
{
	u_int dat;
	int i, n;

	for (i = 0; i < 8; i++)
	{
		if (any_reg_sym(i))
		{
			while ((dat = next_reg_sym(i)) != -1)
			{
				n = dat;
				if (prnflg)
					printf("%04X             a%1d%04x   equ       $%04X\n", n, i, n, n);
				else
					printf("a%1d%04x equ $%04X\n", i, n,
					       n);
			}

			putchar('\n');
		}
	}
}


/*
 * flush the pc symbol table up to current pc
 */

static void prnpcsym()
{
	if ((pass >= 2) && (!pcsflg))
	{
		do
		{
			nxpcref &= 0xffffff;
			if (prnflg)
				printf("%06X           z%06x  equ       $%06X\n", nxpcref, nxpcref, nxpcref);
			else
				printf("z%06x equ $%06X\n", nxpcref, nxpcref);

			if ((nxpcref = next_pc_sym()) == -1)
				break;

		}
		while (nxpcref < ppc);

		printf("\n\n");
	}
}

static void prnsrc()
{
	int i, n;
	char *strptr;

	if (prnflg)
	{
		if (!pcsflg)
		{
			if (ppc > nxpcref)
				printf("%06X           z%06x  equ       *-$%X\n", nxpcref, nxpcref, ppc - nxpcref);
		}

		printf("%06X %04X ", ppc, *opcode);
		if (opwcnt <= 1)
			printf("     ");
		else
			printf("%04X ", opcode[1]);

		if ((ppc == nxpcref) && !pcsflg)
			printf("z%06x  ", nxpcref);
		else
			printf("         ");

		if ((ppc >= nxpcref) && !pcsflg)
		{
			if ((nxpcref = next_pc_sym()) == -1)
				pcsflg = 1;
			else
				nxpcref &= 0xffffff;
		}

		if (0 == (strptr = strchr(mnebf, ' ')))
			printf("%s\n", mnebf);
		else
		{
			*strptr++ = 0;
			printf("%s", mnebf);
			n = 10 - strlen(mnebf);
			for (i = 0; i < n; i++)
				printf(" ");

			printf("%s\n", strptr);
		}
	}
	else
	{
		if (!pcsflg)
		{
			if (ppc > nxpcref)
				printf("z%06x equ *-$%X\n", nxpcref,
				       ppc - nxpcref);

			if (ppc == nxpcref)
				printf("z%06x", nxpcref);

			if (ppc >= nxpcref)
			{
				if ((nxpcref = next_pc_sym()) == -1)
					pcsflg = 1;
				else
					nxpcref &= 0xffffff;
			}
		}

		printf(" %s\n", mnebf);
	}
}

#include <stdarg.h>
/*
 * generic error message handler
 */

int _errmsg(int code, char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	return (code);
}




/*
 * Get a hex digit from 's' into 'x'.
 *  accepts free form data
 *     ace2
 *     xace2
 *     0xace2
 */

#include <ctype.h>

static int htoi(char *s)
{
	char ch;
	u_int x = 0;

	while (*s == ' ' || *s == '\t')
		++s;

	if ('0' == *s)
		++s;

	if (('X' == toupper((unsigned char)*s)) || ('$' == *s))
		++s;

	while (isxdigit((unsigned char)(ch = *s++)))
		x = (x << 4) + toupper((unsigned char)ch) - (ch > '9' ? '7' : '0');

	return (x);
}
