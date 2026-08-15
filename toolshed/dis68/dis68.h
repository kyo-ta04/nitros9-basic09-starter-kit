
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#ifdef WIN32
typedef unsigned int u_int;
typedef unsigned short u_short;
#endif


#define SREG(a)		((a) & 7)			/* source register bits			*/
#define SMOD(a)		(((a) >> 3) & 7)	/* source mode bits				*/
#define DREG(a)		(((a) >> 9) & 7)	/* destination register bits	*/
#define DMOD(a)		(((a) >> 6) & 7)	/* destination mode bits		*/
#define COND(a)		(((a) >> 8) & 0xf)	/* branch contition bits		*/
#define DATA(a)		((a) & 0xff)		/* immediate data bits			*/
#define DISP(a)		((a) & 0xff)		/* displacement bits			*/
#define SIZE(a)		(((a) >> 6) & 3)	/* size bits					*/

extern int		opwcnt, opbcnt, addsz, from_moveal, pass, mdfmt;
extern u_int	ppc, pcl, ldat;
extern u_short	dat, dat1, opcode[6];
extern FILE		*cfp;
extern char		mnebf[80], str[80];
extern void		(*modetbl[])(int);
extern void		(*spaddtbl[])(void);

int		_errmsg(int code, char *fmt, ...);

// from disasm.c
void	disasm(void);
void	huh(void);
void	imm(void);
void	moveb(void);
void	movel(void);
void	movew(void);
void	misc(void);
void	negx(void);
void	clr(void);
void	neg(void);
void	not(void);
void	tst(void);
void	nbcd(void);
void	movem(void);
void	trap(void);
void	addq(void);
void	bsr(void);
void	moveq(void);
void	or(void);
void	sub(void);
void	cmp(void);
void	and(void);
void	add(void);
void	shift(void);

// from addmod.c
void	sizen(void);
void	isize(int);
void	effaddn(void);
void	effadd(int, int);
void	drd(int);
void	ard(int);
void	ri(int);
void	piri(int);
void	pdri(int);
void	rio(int);
void	irio(int);
void	spadd(int);
void	as(void);
void	al(void);
void	relo(void);
void	relio(void);
void	aimm(void);
int		acdtaltn(void);
int		acdtalt(int, int);
int		acdatan(void);
int		acmemn(void);
int		acconn(void);
int		acaltn(void);
int		acalln(void);
u_int	getldat(int);
u_short	getdat(int);
void	eatdat(int);
void	incomma(void);
void	inan(int);
void	indn(int);
void	strmne(char *);

// from symtbl.c
void	add_pc_sym(u_int);
u_int	any_pc_sym();
u_int	next_pc_sym(void);
void	add_reg_sym(int, u_int);
int		any_reg_sym(int);
u_int	next_reg_sym(int);

