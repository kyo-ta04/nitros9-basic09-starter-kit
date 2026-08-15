/********************************************************************
 * cecbdumpblock.c - Dump CoCo Cassette blocks
 *
 * $Id$
 ********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <cecbpath.h>
#include <cocopath.h>
#include <util.h>

#ifdef _WIN32
#include <io.h>
#define is_stdout_terminal() _isatty(_fileno(stdout))
#else
#include <unistd.h>
#define is_stdout_terminal() isatty(fileno(stdout))
#endif

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

/* Help message */
static char const *const helpMessage[] = {
	"Syntax: dumpblock <imagefile>\n",
	"Usage:  Scan a CAS/WAV image and print a hex dump of each block.\n",
	"Options:\n",
	"     -h, ?       Show this help message\n",
	NULL
};

/* Helper to print a 16-byte aligned hex dump */
static void dump_hex(const unsigned char *data, size_t length)
{
	size_t i, j;
	size_t lines = (length + 15) / 16;
	int use_color = is_stdout_terminal();

	for (i = 0; i < lines; i++)
	{
		size_t offset = i * 16;
		size_t line_len = MIN(16, length - offset);

		printf("  %04x: ", (unsigned int)offset);
		
		for (j = 0; j < 16; j++)
		{
			if (j < line_len)
			{
				unsigned char c = data[offset + j];
				if (use_color && isprint(c))
				{
					printf("\033[32m%02x\033[0m ", c);
				}
				else
				{
					printf("%02x ", c);
				}
			}
			else
			{
				printf("   ");
			}
			if (j == 7) printf(" ");
		}
		
		printf(" |");

		for (j = 0; j < 16; j++)
		{
			if (j < line_len)
			{
				unsigned char c = data[offset + j];
				if (isprint(c))
					printf("%c", c);
				else
					printf("%s", "·");
			}
			else
			{
				printf(" ");
			}
		}

		printf("|\n");
	}
}

int cecbdumpblock(int argc, char *argv[])
{
	error_code ec = 0;
	char *srcfile = NULL;
	cecb_path_id path = NULL;
	int i;
	unsigned char block_type, block_length;
	unsigned char data[256];
	int use_color = is_stdout_terminal();

	/* 1. Walk command line for options and source file. */
	for (i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "?") == 0)
		{
			show_help(helpMessage);
			return 0;
		}
		else if (srcfile == NULL)
		{
			srcfile = argv[i];
		}
		else
		{
			fprintf(stderr, "%s: unexpected argument '%s'\n", argv[0], argv[i]);
			show_help(helpMessage);
			return 1;
		}
	}

	if (srcfile == NULL)
	{
		show_help(helpMessage);
		return 0;
	}

	/* 2. Open the cassette path. */
	ec = _cecb_open(&path, srcfile, FAM_READ);
	if (ec != 0)
	{
		fprintf(stderr, "%s: `%s`: open error %d\n", argv[0], srcfile, ec);
		return 1;
	}

	printf("Scanning blocks in: %s\n", srcfile);
	switch (path->tape_type)
	{
		case WAV:
			printf("Format: WAV\n");
			break;
		case CAS:
		case C10:
			printf("Format: CAS/C10\n");
			break;
		default:
			printf("Format: Unknown\n");
			break;
	}
	printf("------------------------------------------------------------\n");

	/* 3. Read and dump each block. */
	
	do
	{
		ec = _cecb_read_next_block(path, &block_type, &block_length, data);
		
		if (ec == EOS_EOF)
		{
			ec = 0;
			break;
		}
		
		/* Print block header with start position */
		if (path->tape_type == WAV)
		{
			printf("Block: Type=0x%02x, Len=%d",
			       block_type, block_length);
		}
		else
		{
			/* CAS/C10: Calculate absolute bit position from byte/bit state */
			printf("Block: Type=0x%02x, Len=%d",
			       block_type, block_length);
		}

		/* Handle CRC warnings gracefully (non-fatal for dump) */
		if (path->block_cksum != path->block_cksum_calc)
		{
			if (use_color)
			{
				printf(", \033[31m CKSUM Error=0x%02x ≠ 0x%02x\033[0m", path->block_cksum, path->block_cksum_calc);
			}
			else
			{
				printf(", CKSUM Error=0x%02x ≠ 0x%02x", path->block_cksum, path->block_cksum_calc);
			}
		}
		else
			printf(", CKSUM=0x%02x", path->block_cksum);

		if (path->tape_type == WAV)
		{
			printf(", End Sample=%ld\n", path->wav_current_sample);
		}
		else
		{
			printf(", End Byte=%ld, End Bit=%d\n", path->cas_current_byte, path->cas_current_bit);
		}
		
		/* Print hex dump of block data */
		dump_hex(data, block_length);
		printf("\n");
	} while (1);

	/* 4. Close the path. */
	_cecb_close(path);

	return ec;
}
