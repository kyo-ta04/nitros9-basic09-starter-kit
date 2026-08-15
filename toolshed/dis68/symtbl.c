/* store symbols in binary tree */

#include <stdlib.h>
#include "dis68.h"


typedef struct tnode
{
	u_int addr;
	struct tnode *left;
	struct tnode *right;
	struct tnode *parent;
} TNode;


static TNode *pcsym, *regsym[8];
static TNode *stsym(TNode * p, u_int x);
static TNode *incsym(TNode * p);
static TNode *uright(TNode * p);
static TNode *dleft(TNode * p);
static TNode *talloc(void);
static int lcomp(u_int x, u_int y);

/*
 * add a pc symbol/value pair.
 *  no return value (can't fail ;) )
 */

void add_pc_sym(u_int val)
{
//printf("%s: %p %p\n", __func__, pcsym, val);
	if (pass == 1)
		pcsym = stsym(pcsym, val);
}


/*
 * set to first pc symbol and return value or -1 at end
 */
// FIXME?  why return -1
u_int any_pc_sym()
{
//printf("%s: before %p\n", __func__, pcsym);
	pcsym = dleft(pcsym);
//printf("%s: after %p\n", __func__, pcsym);
	if (pcsym)
		return (pcsym->addr);
	else
		return (-1);
}


/*
 * return the next pc symbol value or -1 at the end
 */

u_int next_pc_sym()
{
	if (pcsym && (pcsym = incsym(pcsym)))
		return (pcsym->addr);
	else
		return (-1);
}

/*
 * add a reg symbol/value pair.
 *  no return value (can't fail ;) )
 */

void add_reg_sym(int reg, u_int val)
{
	if (pass == 1)
		regsym[reg] = stsym(regsym[reg], val);
}


/*
 * set to first symbol and return true if any reg symbols
 */

int any_reg_sym(int reg)
{
	TNode *tval;
	int retval;

	retval = 0;
	if (0 != (tval = regsym[reg]))
	{
		regsym[reg] = dleft(regsym[reg]);
		retval = 1;
	}

	return (retval);
}


/*
 * return the next reg symbol value or -1 at the end
 */

u_int next_reg_sym(int reg)
{
	if (regsym[reg] && (regsym[reg] = incsym(regsym[reg])))
		return (regsym[reg]->addr);
	else
		return (-1);
}

static TNode *stsym(TNode * p, u_int x)	/* install x in p */
{
//printf("%s: p = %p, x = %p\n", __func__, p, x);
	if (0 == p)		/* new word */
	{
		p = talloc();	/* make new node */
		p->addr = x;
		p->left = p->right = p->parent = 0;
//printf("%s: first time -> %p\n", __func__, p);
	}
	else
	{
		if (lcomp(x, p->addr) < 0)
		{
			p->left = stsym(p->left, x);
			(p->left)->parent = p;
//printf("%s: went left -> %p\n", __func__, p);
		}
		else if (lcomp(x, p->addr) > 0)
		{
			p->right = stsym(p->right, x);
			(p->right)->parent = p;
//printf("%s: went right -> %p\n", __func__, p);
		}
	}

	return (p);
}


static TNode *incsym(TNode * p)	/* get next symbol */
{
	if (p->right)
		return (dleft(p->right));
	else
		return (uright(p));
}

static TNode *uright(TNode * p)	/* go up right */
{
	if (!(p->parent))
		return (0);

	if ((p->parent)->right == p)
		return (uright(p->parent));
	else
		return (p->parent);
}


static TNode *dleft(TNode * p)	/* go down left */
{
	if (p->left)
		return (dleft(p->left));
	else
		return (p);
}


static TNode *talloc()
{
	TNode *p;

	if ((p = (TNode *) malloc(sizeof(TNode))) == 0)
		exit(_errmsg(errno, "symbol table full\n"));

	return (p);
}

static int lcomp(u_int x, u_int y)
{
	if (x == y)
		return (0);

	if (x > y)
		return (1);

	return (-1);
}
