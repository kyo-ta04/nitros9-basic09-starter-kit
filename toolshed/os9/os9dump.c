/********************************************************************
 * os9dump.c - OS-9 dump utility
 *
 * $Id$
 ********************************************************************/

#include "util.h"
#include "cocopath.h"
#include "cocotypes.h"
#include "cococonv.h"

#define BUFFSIZ	256

enum dumpformat
{
	FMT_STANDARD,
	FMT_ASMHEX,
	FMT_ASMBIN,
	FMT_C,
};

static void dump(u_char * buffer, u_int offset, size_t num_bytes,
		 enum dumpformat format);
static void dumpDECB(u_char * buffer, size_t num_bytes,
		     enum dumpformat format);
static void dump_line(u_char * buffer, u_int count, enum dumpformat format);
static int do_dump(char **argv, char *file, enum dumpformat format);
static void dump_header(enum dumpformat format);
static char *binary(char s);

/* Help message */
static char const *const helpMessage[] = {
	"Syntax: dump {[<opts>]} {<file> [<...>]} {[<opts>]}\n",
	"Usage:  Display the contents of a file in hexadecimal.\n",
	"Options:\n",
	"     -a    dump output in assembler format (hex)\n",
	"     -b    dump output in assembler format (binary)\n",
	"     -c    don't display ASCII character data\n",
	"     -e    dump output in C format\n",
	"     -t    don't display header\n",
	"     -h    don't display header (DEPRECATED)\n",
	"     -l    don't display line label/count\n",
	"     -z    decode DECB binary\n",
	NULL
};

static int displayASCII;
static int displayHeader;
static int displayLabel;
static int decbBinary;
static u_int dumpchunk;

int os9dump(int argc, char **argv)
{
	static enum dumpformat format;
	error_code ec = 0;
	char *p = NULL;
	int i;

	format = FMT_STANDARD;
	displayASCII = 1;
	displayHeader = 1;
	displayLabel = 1;
	dumpchunk = 16;
	decbBinary = 0;

	if (argv[1] == NULL)
	{
		show_help(helpMessage);
		return (0);
	}
	/* walk command line for options */
	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			for (p = &argv[i][1]; *p != '\0'; p++)
			{
				switch (*p)
				{
				case 'a':
					format = FMT_ASMHEX;
					dumpchunk = 8;
					break;

				case 'b':
					format = FMT_ASMBIN;
					dumpchunk = 1;
					break;

				case 'c':
					displayASCII = 0;
					break;

				case 'e':
					format = FMT_C;
					dumpchunk = 8;
					break;

				case 'h':
					fprintf(stderr,
						"%s: warning: option -h is deprecated, use -t\n",
						argv[0]);
					/* fallthrough */
				case 't':
					displayHeader = 0;
					break;

				case 'l':
					displayLabel = 0;
					break;

				case 'z':
					decbBinary = 1;
					break;

				case '?':
					show_help(helpMessage);
					return (0);

				default:
					fprintf(stderr,
						"%s: unknown option '%c'\n",
						argv[0], *p);
					return (0);
				}
			}
		}
	}

	/* walk command line for pathnames */
	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			continue;
		}
		else
		{
			p = argv[i];
		}

		ec = do_dump(argv, p, format);

		if (ec != 0)
		{
			fprintf(stderr, "%s: error %d opening '%s'\n",
				argv[0], ec, p);
			return (ec);
		}
	}

	return (0);
}

static int do_dump(char **argv, char *file, enum dumpformat format)
{
	error_code ec = 0;
	u_char buffer[BUFFSIZ], *tot_buffer;
	u_int tot_bytes = 0;
	coco_path_id path;

	/* 1. Open a path to the file. */
	ec = _coco_open(&path, file, FAM_READ);

	if (ec != 0)
	{
		ec = _coco_open(&path, file, FAM_DIR | FAM_READ);

		if (ec != 0)
		{
			fprintf(stderr, "%s: cannot open file\n", argv[0]);
			return (ec);
		}
	}

	if (format == FMT_C)
	{
		printf("{");
	}

	/* 2. Determine file size by reading until error */

	while (1)
	{
		u_int num_bytes;

		num_bytes = BUFFSIZ;
		ec = _coco_read(path, buffer, &num_bytes);
		tot_bytes += num_bytes;
		if (ec != 0)
		{
			break;
		}
	}

	/* 2. Rewind and dump the file */

	_coco_seek(path, 0, SEEK_SET);

	tot_buffer = malloc(tot_bytes);

	if (tot_buffer != 0)
	{

		ec = _coco_read(path, tot_buffer, &tot_bytes);

		if (decbBinary)
		{
			dumpDECB(tot_buffer, tot_bytes, format);
		}
		else
		{
			dump_header(format);
			dump(tot_buffer, 0, tot_bytes, format);
		}

		free(tot_buffer);
	}
	else
	{
		fprintf(stderr, "No memory to open: %s.\n", file);
	}

	if (format == FMT_C)
	{
		printf("\n}");
	}

	printf("\n");

	ec = _coco_close(path);

	return ec;
}

static void dumpDECB(u_char * buffer, size_t num_bytes,
		     enum dumpformat format)
{
	int length;
	u_int address;
	int count = 0;

	while (count < num_bytes)
	{
		if (buffer[count] == 0)
		{
			count++;
			length = buffer[count++] << 8;
			length += buffer[count++];
			address = buffer[count++] << 8;
			address += buffer[count++];

			dump_header(format);
			dump(&(buffer[count]), address, length, format);
			printf("\n");
			count += length;
		}
		else if (buffer[count] == 0xff)
		{
			count++;
			length = buffer[count++] << 8;
			length += buffer[count++];
			address = buffer[count++] << 8;
			address += buffer[count++];

			printf("\nExecution address: $%04X\n", address);

			int remain = num_bytes - count;

			if (remain > 0)
			{
				printf("%d bytes extra at end of file.\n",
				       remain);
			}
			break;
		}
		else
		{
			printf("Aborting: expected 0 or 255 for block header\n");
			break;
		}
	}
}

static void dump(u_char * buffer, u_int offset, size_t num_bytes,
		 enum dumpformat format)
{
	u_int i, j;

	for (i = 0, j = 0; i < num_bytes; i += dumpchunk)
	{
		if (j > 255)
		{
			dump_header(format);
			j = 0;
		}
		if (format == FMT_STANDARD)
		{
			if (displayLabel == 1)
			{
				printf("\n%08x  ", offset);
			}
			else
			{
				printf("\n");
			}
		}
		else if (format == FMT_ASMHEX || format == FMT_ASMBIN)
		{
			if (displayLabel == 1)
			{
				printf("\nL%04X    fcb   ", offset);
			}
			else
			{
				printf("\n         fcb   ");
			}
		}
		else if (format == FMT_C)
		{
			if (displayLabel == 1)
			{
				printf("\n   /* offset = %08X */ ", offset);
			}
			else
			{
				printf("\n   ");
			}
		}

		if (num_bytes - i > dumpchunk)
		{
			dump_line(&buffer[i], dumpchunk, format);
		}
		else
		{
			dump_line(&buffer[i], num_bytes - i, format);
		}

		offset += dumpchunk;

		j += dumpchunk;

	}

	return;
}

static void dump_line(u_char * buffer, u_int count, enum dumpformat format)
{
	int i;
	int carry = 0;

	if (count % 2 != 0)
	{
		count--;
		carry = 1;
	}
	for (i = 0; i < count; i += 2)
	{
		switch (format)
		{
		case FMT_STANDARD:
			printf("%02x%02x ", buffer[i], buffer[i + 1]);
			break;

		case FMT_ASMHEX:
			if (i == count - 2 && carry == 0)
			{
				printf("$%02X,$%02X", buffer[i],
				       buffer[i + 1]);
			}
			else
			{
				printf("$%02X,$%02X,", buffer[i],
				       buffer[i + 1]);
			}
			break;

		case FMT_ASMBIN:
			if (i == count - 2 && carry == 0)
			{
				printf("%%%s,%%%s", binary(buffer[i]),
				       binary(buffer[i + 1]));
			}
			else
			{
				printf("%%%s,%%%s,", binary(buffer[i]),
				       binary(buffer[i + 1]));
			}
			break;

		case FMT_C:
			{
				printf("0x%02X,0x%02X,", buffer[i],
				       buffer[i + 1]);
			}
			break;
		}
	}

	if (carry == 1)
	{
		switch (format)
		{
		case FMT_STANDARD:
			printf("%02x", buffer[i]);
			break;

		case FMT_ASMHEX:
			printf("$%02X", buffer[i]);
			break;

		case FMT_ASMBIN:
			printf("%%%s", binary(buffer[i]));
			break;

		case FMT_C:
			printf("0x%02X,", buffer[i]);
			break;
		}
		count++;
	}
	if (displayASCII == 1)
	{
		/* make spaces available if last line is not full */
		i = (dumpchunk - count);

		if (format == FMT_ASMHEX)
		{
			printf("   ");
		}
		if (i % 2 != 0)
		{
			switch (format)
			{
			case FMT_ASMHEX:
				printf("     ");
				break;

			case FMT_C:
				printf("     ");
				break;

			default:
				printf("   ");
				break;
			}
		}
		i /= 2;

		while (i--)
		{
			if (format == FMT_ASMHEX)
			{
				printf("        ");
			}
			else if (format == FMT_C)
			{
				printf("          ");
			}
			else
			{
				printf("     ");
			}
		}

		if (format == FMT_C)
		{
			printf("  // ");
		}

		/* print character dump on right side */
		for (i = 0; i < count; i++)
		{
			if (buffer[i] >= 32 && buffer[i] < 127)
			{
				printf("%c", buffer[i]);
			}
			else if (buffer[i] >= 128 + 32
				 && buffer[i] <= 128 + 'z')
			{
				printf("%c", buffer[i] - 128);
			}
			else
			{
				printf(".");
			}
		}
	}
	return;
}

static void dump_header(enum dumpformat format)
{
	if (format == FMT_STANDARD && displayHeader == 1)
	{
		printf("\n\n");
		if (displayLabel)
		{
			printf("  Addr    ");
		}
		printf(" 0 1  2 3  4 5  6 7  8 9  A B  C D  E F");
		if (displayASCII == 1)
		{
			printf(" 0 2 4 6 8 A C E");
		}
		printf("\n");

		if (displayLabel)
		{
			printf("--------  ");
		}
		printf("---- ---- ---- ---- ---- ---- ---- ----");
		if (displayASCII == 1)
		{
			printf(" ----------------");
		}
	}
	return;
}

static char *binary(char s)
{
	static char buffer[9] =
		{ '0', '0', '0', '0', '0', '0', '0', '0', '\0' };
	int i;

	for (i = 0; i < 8; i++)
	{
		int x = s & (1 << (7 - i));

		if (x != 0)
		{
			buffer[i] = '1';
		}
		else
		{
			buffer[i] = '0';
		}
	}

	return (buffer);
}
