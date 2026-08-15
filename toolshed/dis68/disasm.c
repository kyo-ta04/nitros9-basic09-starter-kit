/* write disassembler for 68k */
/* disasm disassemble one line of code */
/* number of bytes used is returned */
/* result left in mnebf */

#include <stdio.h>
#include "dis68.h"
#include "functbl.h"



u_short dat, dat1;
u_int pcl, ldat;
int addsz;			/* flag for immediate mode */
int from_moveal = 0;
char *warning = "** warning - not on 68000 **";

void disasm()
{
	/* return number of bytes read from cfp */
	/* mnebuf is concatenated with assembler code */

	(*topnib[(getdat(1) >> 12) & 0x000f]) ();
	opbcnt = 2 * opwcnt;
}


void huh()
{
	sprintf(str, "dc.w $%04X  ???", *opcode);
	strmne(str);
}

/* 0x00 types */
void imm()
{
	int tmp;

	addsz = 0;		/* default to byte */
	if (*opcode & 0x0100)
	{
		if ((*opcode & 0x0038) == 0x0008)
		{
			if (*opcode & 0x0040)
				strmne("movep.l ");
			else
				strmne("movep.w ");

			if (*opcode & 0x0080)
			{
				rio(SREG(*opcode));
				incomma();
				indn(DREG(*opcode));
			}
			else
			{
				indn(DREG(*opcode));
				incomma();
				rio(SREG(*opcode));
			}

			goto bail;
		}

		if (!((acdatan() && (!(*opcode & 0x00c0))) || acdtaltn()))
		{
			huh();
			goto bail;
		}

		strmne(bittyp[(*opcode & 0x00c0) >> 6]);
		indn(DREG(*opcode));
		incomma();
		effaddn();
		goto bail;
	}

	if ((*opcode & 0x0e00) == 0x0800)
	{
		if (!((acdatan() && (!(*opcode & 0x00c0))) || acdtaltn()))
		{
			huh();
			goto bail;
		}

		strmne(bittyp[(*opcode & 0x00c0) >> 6]);
		dat = getdat(1);
		sprintf(str, "#$%02X,", dat & 0x001f);
		strmne(str);
		effaddn();
		goto bail;
	}

	tmp = DREG(*opcode);
	if (((*opcode & 0x00bf) == 0x003c)
	    && ((tmp == 0) || (tmp == 1) || (tmp == 5)))
	{
		dat = getdat(1);
		strmne(imm_mne[tmp]);
		if (*opcode & 0x0040)
			sprintf(str, " #$%04X,sr", dat);
		else
			sprintf(str, " #$%02X,ccr", dat & 0x00ff);

		strmne(str);
		goto bail;
	}

	if (((*opcode & 0x00c0) == 0x00c0) || !acdtaltn())
	{
		huh();
		goto bail;
	}

	strmne(imm_mne[tmp]);
	isize(SIZE(*opcode));
	incomma();
	effaddn();
      bail:;
}

/* 0x10 types; move.b */
void moveb()
{
	int mode, reg;

	addsz = 0;
	mode = DMOD(*opcode);
	reg = DREG(*opcode);
	if (!(acdtalt(mode, reg) & acdatan()))
		huh();
	else
	{
		strmne("move.b ");
		effaddn();
		incomma();
		effadd(mode, reg);
	}
}


/* 0x20 types; move.l */
void movel()
{
	int mode, reg;

	addsz = 2;
	mode = DMOD(*opcode);
	reg = DREG(*opcode);
	if (!acalln())
		huh();
	else
	{
		if (mode == 1)
		{
			strmne("movea.l ");
			addsz = 2;
			from_moveal = 1;
			effaddn();
			from_moveal = 0;
			incomma();
			inan(reg);
		}
		else
		{
			if (!acdtalt(mode, reg))
				huh();
			else
			{
				strmne("move.l ");
				if ((mode == 7) && (reg == 1))	/* i.e. dest is Znnnnn */
					from_moveal = 1;

				effaddn();
				from_moveal = 0;
				incomma();
				effadd(mode, reg);
			}
		}
	}
}

/* 0x30 types; move.w */
void movew()
{
	int mode, reg;

	mode = DMOD(*opcode);
	reg = DREG(*opcode);
	addsz = 1;
	if (!acalln())
		huh();
	else
	{
		if (mode == 1)
		{
			strmne("movea.w ");
			effaddn();
			incomma();
			inan(reg);
		}
		else
		{
			if (!(acdtalt(mode, reg)))
				huh();
			else
			{
				strmne("move.w ");
				effaddn();
				incomma();
				effadd(mode, reg);
			}
		}
	}
}

/* 0x40 types; miscellaneous */
void misc()
{
	if (0 == (*opcode & 0x0100))
		(*misctbl[DREG(*opcode)]) ();
	else
	{
		/* only possibilities are chk and lea */
		if (((*opcode & 0x00c0) == 0x0080) && acdatan())
		{
			addsz = 1;
			strmne("chk ");
			effaddn();
			incomma();
			sprintf(str, "d%1d", DREG(*opcode));
		}
		else if (((*opcode & 0x00c0) == 0x00c0) && acconn())
		{
			strmne("lea ");
			effaddn();
			incomma();
			inan(DREG(*opcode));
		}
		else
			huh();
	}
}


void negx()
{
	if (!acdtaltn())
		huh();
	else
	{
		if ((*opcode & 0x00c0) == 0x00c0)
			strmne("move sr,");
		else
		{
			strmne("negx");
			sizen();
		}

		effaddn();
	}
}

void clr()
{
	if (((*opcode & 0x00c0) == 0x00c0) || (!acdtaltn()))
		huh();
	else
	{
		strmne("clr");
		sizen();
		effaddn();
	}
}


void neg()
{
	if (!acdatan())
		huh();
	else
	{
		if ((*opcode & 0x00c0) == 0x00c0)
		{
			addsz = 1;
			strmne("move ");
			effaddn();
			strmne(",ccr");
		}
		else
		{
			if (!acaltn())
				huh();
			else
			{
				strmne("neg");
				sizen();
				effaddn();
			}
		}
	}
}

void not()
{
	if (!acdatan())
		huh();
	else
	{
		if ((*opcode & 0x00c0) == 0x00c0)
		{
			addsz = 1;
			strmne("move ");
			effaddn();
			strmne(",sr");
		}
		else
		{
			if (!acaltn())
				huh();
			else
			{
				strmne("not");
				sizen();
				effaddn();
			}
		}
	}
}


void tst()
{
	if (!acdtaltn())
		huh();
	else
	{
		if ((*opcode & 0x00c0) == 0x00c0)
			strmne("tas ");
		else
		{
			strmne("tst");
			sizen();
		}

		effaddn();
	}
}

void nbcd()
{
	u_int mask;
	int i;
	int flag = 0;

	if (*opcode & 0x0080)
	{
		if (*opcode & 0x0038)
		{
			if (!
			    ((acconn() & acaltn())
			     || ((*opcode & 0x38) == 0x20)))
			{
				huh();
				goto bail;
			}

			if (*opcode & 0x0040)
				strmne("movem.l ");
			else
				strmne("movem.w ");

			dat = getdat(1);
			for (i = 0, mask = 0x8000; i < 8;
			     mask = mask >> 1, i++)
				if (mask & dat)
				{
					if (flag)
						strmne("/");

					sprintf(str, "d%1d", i);
					strmne(str);
					flag = 1;
				}

			for (i = 0, mask = 0x0080; i < 8;
			     mask = mask >> 1, i++)
				if (mask & dat)
				{
					if (flag)
						strmne("/");

					sprintf(str, "a%1d", i);
					strmne(str);
					flag = 1;
				}

			incomma();
			effaddn();
			goto bail;
		}

		if (*opcode & 0x0040)
			strmne("ext.l ");
		else
			strmne("ext.w ");

		sprintf(str, "d%1d", SREG(*opcode));
		strmne(str);
		goto bail;
	}

	if ((*opcode & 0x0078) == 0x0040)
	{
		sprintf(str, "swap d%1d", SREG(*opcode));
		strmne(str);
		goto bail;
	}

	if ((*opcode & 0x0040))
	{
		if (!acconn())
		{
			huh();
			goto bail;
		}

		strmne("pea ");
	}
	else
	{
		if (!acdtaltn())
		{
			huh();
			goto bail;
		}

		strmne("nbcd ");
	}

	effaddn();
      bail:;
}

void movem()
{
	u_int mask;
	int i, rgmsk, flag = 0;

	if (!((acconn() || ((*opcode & 0x38) == 0x18)) && (*opcode & 0x80)))
	{
		huh();
		goto bail;
	}

	if (*opcode & 0x0040)
		strmne("movem.l ");
	else
		strmne("movem.w ");

	rgmsk = getdat(1);
	effaddn();
	incomma();
	dat = rgmsk;
	for (i = 0, mask = 0x0001; i < 8; mask = mask << 1, i++)
		if (mask & dat)
		{
			if (flag)
				strmne("/");

			sprintf(str, "d%1d", i);
			strmne(str);
			flag = 1;
		}

	for (i = 0, mask = 0x0100; i < 8; mask = mask << 1, i++)
		if (mask & dat)
		{
			if (flag)
				strmne("/");

			sprintf(str, "a%1d", i);
			strmne(str);
			flag = 1;
		}

      bail:;
}

void trap()			/* $4exx */
{
	int n, extend, reg;
	char c, *s;

	if (*opcode & 0x0080)	/* jmp or jsr */
	{
		if (!acconn())
			huh();
		else
		{
			if (*opcode & 0x0040)
				strmne("jmp ");
			else
				strmne("jsr ");

			effaddn();
			if (*opcode & 0x0040)
				strmne("\n\n");
		}
	}
	else
	{
		switch (*opcode & 0x0070)
		{
		case 0x0040:	/* trap                                                 */
			if ((0 == (*opcode & 0xf)) /*&& (mdfmt != 0) */ )
			{
				dat = getdat(1);
				sprintf(str, "os9 $%04X", dat);
			}
			else
				sprintf(str, "trap #$%1X", *opcode & 0x000f);

			strmne(str);
			break;

		case 0x0050:	/* link, unlk                                   */
			if (*opcode & 0x0008)
				sprintf(str, "unlk a%1d", SREG(*opcode));
			else
			{
				dat = getdat(1);
				sprintf(str, "link a%1d,#$%04X",
					SREG(*opcode), dat);
			}

			strmne(str);
			break;

		case 0x0060:
			if (*opcode & 0x0008)	/* move <=> usp */
				sprintf(str, "move usp,a%1d", SREG(*opcode));
			else
				sprintf(str, "move a%1d,usp", SREG(*opcode));

			strmne(str);
			break;

		case 0x0070:
			if ((n = (*opcode & 0x000f)) <= 7)
			{
				strmne(msc_mne[n]);
				if (n == 2 || n == 3 || n == 5 || n == 7)
					strmne("\n\n");
			}
			else
			{
				extend = getdat(1);
				c = (extend & 0x8000) ? 'a' : 'd';
				reg = ((extend >> 12) & 7);
				if (extend & 0x0800)
					extend = 3 + (extend & 7);
				else
					extend = (extend & 7);
				s = movec_mne[extend];
				if (n == 0x000a)
				{
					sprintf(str, "movec %s,%c%d  %s", s,
						c, reg, warning);
					strmne(str);
				}
				else if (n == 0x000b)
				{
					sprintf(str, "movec %c%d,%s  %s", c,
						reg, s, warning);
					strmne(str);
				}
				else
					huh();
			}
			break;

		default:
			huh();
		}
	}
}

/* 0x50 types; addq, subq, scc, dbcc, trapcc */
void addq()
{
	if ((*opcode & 0x00c0) == 0x00c0)
	{
		if ((*opcode & 0x0038) == 0x0008)
		{
			short pcw;

			pcw = getdat(1);
			strmne("db");
			strmne(cc_mne[COND(*opcode)]);
			strmne(" ");
			indn(SREG(*opcode));
			incomma();
			pcl = ppc + pcw + 2;
			sprintf(str, "z%06x", pcl & 0xffffff);
			add_pc_sym(pcl);
			strmne(str);
		}
		else if (!acdtaltn())
			huh();
		else
		{
			strmne("s");
			strmne(cc_mne[COND(*opcode)]);
			strmne(" ");
			effaddn();
		}
	}
	else
	{
		if (!acaltn())
			huh();
		else
		{
			if (*opcode & 0x0100)
				strmne("subq");
			else
				strmne("addq");

			sizen();
			dat = DREG(*opcode);
			if (!dat)
				dat = 8;

			sprintf(str, "#%1d,", dat);
			strmne(str);
			effaddn();
		}
	}
}

/* 0x60 types; bcc, bsr, bra */
void bsr()
{
	int cc;
	char pcb;
	short pcw;

	cc = COND(*opcode);
	if (cc < 2)
	{
		if (cc)
			strmne("bsr");
		else
			strmne("bra");
	}
	else
	{
		strmne("b");
		strmne(cc_mne[cc]);
	}

	if (0 != DISP(*opcode))
	{
		pcb = DISP(*opcode);
		pcl = ppc + 2 + pcb;
		sprintf(str, ".s z%06x", pcl & 0xffffff);
	}
	else
	{
		pcw = getdat(1);
		pcl = ppc + 2 + pcw;
		sprintf(str, " z%06x", pcl & 0xffffff);
	}

	add_pc_sym(pcl);
	strmne(str);
	if (cc == 0)
		strmne("\n");
}


/* 0x70 types; moveq */
void moveq()
{
	if (*opcode & 0x0100)
		huh();
	else
	{
		sprintf(str, "moveq #$%02X,d%1d", DATA(*opcode),
			DREG(*opcode));
		strmne(str);
	}
}

/* 0x80 types; or, div, sbcd */
void or()
{
	if ((*opcode & 0x01f0) == 0x0100)
	{
		strmne("sbcd ");
		if (*opcode & 0x0008)
			sprintf(str, "-(a%1d),-(a%1d)", SREG(*opcode),
				DREG(*opcode));
		else
			sprintf(str, "d%1d,d%1d", SREG(*opcode),
				DREG(*opcode));

		strmne(str);
	}
	else
	{
		if ((*opcode & 0x00c0) == 0x00c0)
		{
			if (!acdatan())
			{
				huh();
				goto bail;
			}
			else
			{
				if (*opcode & 0x0100)
					strmne("divs ");
				else
					strmne("divu ");

				addsz = 1;
				effaddn();
				incomma();
				indn(DREG(*opcode));
			}
		}
		else
		{
			if (!(*opcode & 0x0100))
			{
				if (!acdatan())
					huh();
				else
				{
					strmne("or");
					sizen();
					effaddn();
					incomma();
					indn(DREG(*opcode));
				}
			}
			else
			{
				if (!(acaltn() & acmemn()))
					huh();
				else
				{
					strmne("or");
					sizen();
					indn(DREG(*opcode));
					incomma();
					effaddn();
					goto bail;
				}
			}
		}
	}
      bail:;
}

/* 0x90 types; sub, subx */
void sub()
{
	if (((*opcode & 0x0130) == 0x0100) && ((*opcode & 0x00c0) != 0x00c0))
	{
		strmne("subx");
		sizen();
		if (*opcode & 0x0100)
			sprintf(str, "-(a%1d),-(a%1d)", SREG(*opcode),
				DREG(*opcode));
		else
			sprintf(str, "d%1d,d%1d", SREG(*opcode),
				DREG(*opcode));

		strmne(str);
		goto bail;
	}

	if ((*opcode & 0x00c0) == 0x00c0)
	{
		if (!acalln())
		{
			huh();
			goto bail;
		}

		strmne("suba");
		if (*opcode & 0x0100)
		{
			strmne(".l ");
			addsz = 2;
		}
		else
		{
			strmne(".w ");
			addsz = 1;
		}

		effaddn();
		incomma();
		inan(DREG(*opcode));
		goto bail;
	}

	if (*opcode & 0x0100)
	{
		if (!(acmemn() & acaltn()))
		{
			huh();
			goto bail;
		}

		strmne("sub");
		sizen();
		indn(DREG(*opcode));
		incomma();
		effaddn();
		goto bail;
	}
	else
	{
		if ((!acalln()) || ((*opcode & 0x00f8) == 0x0008))
		{
			huh();
			goto bail;
		}

		strmne("sub");
		sizen();
		effaddn();
		incomma();
		indn(DREG(*opcode));
	}
      bail:;
}

/* 0xb0 types; cmp, eor */
void cmp()
{
	if ((*opcode & 0x00c0) == 0x00c0)
	{
		if (!acalln())
		{
			huh();
			goto bail;
		}

		strmne("cmpa");
		if (*opcode & 0x0100)
		{
			addsz = 2;
			strmne(".l ");
		}
		else
		{
			addsz = 1;
			strmne(".w ");
		}

		effaddn();
		incomma();
		inan(DREG(*opcode));
		goto bail;
	}

	if ((*opcode & 0x0138) == 0x0108)
	{
		strmne("cmpm");
		sizen();
		sprintf(str, "(a%1d)+,(a%1d)+", SREG(*opcode), DREG(*opcode));
		strmne(str);
		goto bail;
	}

	if (*opcode & 0x0100)
	{
		if (!acdtaltn())
		{
			huh();
			goto bail;
		}

		strmne("eor");
		sizen();
		indn(DREG(*opcode));
		incomma();
		effaddn();
		goto bail;
	}

	if ((!acalln()) || ((*opcode & 0x00f8) == 0x0008))
	{
		huh();
		goto bail;
	}

	strmne("cmp");
	sizen();
	effaddn();
	incomma();
	indn(DREG(*opcode));
      bail:;
}

/* 0xc0 types; and, bul, abcd, exg */
void and()
{
	if ((*opcode & 0x00c0) == 0x00c0)
	{
		if (!acdatan())
		{
			huh();
			goto bail;
		}

		if (*opcode & 0x0100)
			strmne("muls ");
		else
			strmne("mulu ");

		addsz = 1;
		effaddn();
		incomma();
		indn(DREG(*opcode));
		goto bail;
	}

	if ((*opcode & 0x0130) == 0x0100)
	{
		if (!(*opcode & 0x00c0))
		{
			strmne("abcd ");
			if (*opcode & 0x0008)
				sprintf(str, "-(a%1d),-(a%1d)", SREG(*opcode),
					DREG(*opcode));
			else
				sprintf(str, "d%1d,d%1d", SREG(*opcode),
					DREG(*opcode));

			strmne(str);
			goto bail;
		}

		if (*opcode & 0x0080)
		{
			if (*opcode & 0x0008)
				sprintf(str, "exg d%1d,a%1d", DREG(*opcode),
					SREG(*opcode));
			else
			{
				huh();
				goto bail;
			}
		}
		else
		{
			if (*opcode & 0x0008)
				sprintf(str, "exg a%1d,a%1d", DREG(*opcode),
					SREG(*opcode));
			else
				sprintf(str, "exg d%1d,d%1d", DREG(*opcode),
					SREG(*opcode));
		}

		strmne(str);
		goto bail;
	}

	if (*opcode & 0x0100)
	{
		if (!(acmemn() & acaltn()))
		{
			huh();
			goto bail;
		}

		strmne("and");
		sizen();
		indn(DREG(*opcode));
		incomma();
		effaddn();
		goto bail;
	}
	else
	{
		if (!acdatan())
		{
			huh();
			goto bail;
		}

		strmne("and");
		sizen();
		effaddn();
		incomma();
		indn(DREG(*opcode));
	}
      bail:;
}

/* 0xd0 types; add, addx */
void add()
{
	if (((*opcode & 0x0130) == 0x0100) && ((*opcode & 0x00c0) != 0x00c0))
	{
		strmne("addx");
		sizen();
		if (*opcode & 0x0100)
			sprintf(str, "-(a%1d),-(a%1d)", SREG(*opcode),
				DREG(*opcode));
		else
			sprintf(str, "d%1d,d%1d", SREG(*opcode),
				DREG(*opcode));

		strmne(str);
		goto bail;
	}

	if ((*opcode & 0x00c0) == 0x00c0)
	{
		if (!acalln())
		{
			huh();
			goto bail;
		}

		strmne("adda");
		if (*opcode & 0x0100)
		{
			strmne(".l ");
			addsz = 2;
		}
		else
		{
			strmne(".w ");
			addsz = 1;
		}

		effaddn();
		incomma();
		inan(DREG(*opcode));
		goto bail;
	}

	if (*opcode & 0x0100)
	{
		if (!(acmemn() & acaltn()))
		{
			huh();
			goto bail;
		}

		strmne("add");
		sizen();
		indn(DREG(*opcode));
		incomma();
		effaddn();
		goto bail;
	}
	else
	{
		if ((!acalln()) || ((*opcode & 0x00f8) == 0x0008))
		{
			huh();
			goto bail;
		}

		strmne("add");
		sizen();
		effaddn();
		incomma();
		indn(DREG(*opcode));
	}
      bail:;
}

/* 0xe0 types; shift, rotate, bit field */
void shift()
{
	int shftcnt;

	if ((*opcode & 0x00c0) == 0x00c0)
	{
		if (!(acmemn() & acaltn()))
		{
			huh();
			goto bail;
		}

		strmne(shft_mne[(*opcode >> 9) & 0x3]);
		if (*opcode & 0x0100)
			strmne("l ");
		else
			strmne("r ");

		effaddn();
		goto bail;
	}
	else
	{
		strmne(shft_mne[(*opcode >> 3) & 0x3]);
		if (*opcode & 0x0100)
			strmne("l");
		else
			strmne("r");

		sizen();
		shftcnt = DREG(*opcode);
		if (*opcode & 0x0020)
			sprintf(str, "d%1d,", shftcnt);
		else
		{
			if (!shftcnt)
				shftcnt = 8;

			sprintf(str, "#%1d,", shftcnt);
		}

		strmne(str);
		indn(SREG(*opcode));
	}
      bail:;
}
