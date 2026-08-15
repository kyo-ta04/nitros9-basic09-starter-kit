/********************************************************************
 * libdecbdskini.c - Disk BASIC Format Routine
 *
 * $Id$
 ********************************************************************/
#include <util.h>
#include <string.h>
#include <decbpath.h>
#include <nativepath.h>
#include <cocotypes.h>


#define MAX_BPS 			256
#define MAX_DISKNAME_LEN	9	// the maximum number of characters an HDB-DOS disk name can be

error_code _decb_dskini(char *vdisk, int tracks, char *diskName,
			int hdbdrives, int bps, int skitzo)
{
	error_code ec = 0;
	native_path_id nativepath;
	int max_s, i;
	char sector[MAX_BPS];

	if (diskName && strlen(diskName) > MAX_DISKNAME_LEN)
	{
		return EOS_BPNAM;
	}

	/* 1. Open a path to the virtual disk. */

	ec = _native_open(&nativepath, vdisk, FAM_WRITE);

	if (ec != 0)
	{
		ec = _native_create(&nativepath, vdisk, FAM_READ | FAM_WRITE,
				    FAP_READ | FAP_WRITE);

		if (ec != 0)
		{
			return (ec);
		}
	}

	_native_seek(nativepath, 0, SEEK_SET);


	for (i = 0; i < hdbdrives; i++)
	{
		/* 2. Write 17 tracks of $FF */

		memset(sector, 0xFF, bps);

		{
			int t, s;

			for (t = 0; t < 17; t++)
			{
				for (s = 1; s < 19; s++)
				{
					u_int size = bps;

					_native_write(nativepath, sector,
						      &size);
				}
			}
		}


		/* 3. Write directory track. */

		memset(sector, 0x00, bps);

		{
			int s;
			u_int size, min_s = 0;


			/* 1. Write sector of track 17 (all 0s..). */

			size = bps;

			_native_write(nativepath, sector, &size);


			/* 2. Write FAT sector. */

			switch (tracks)
			{
			case 40:
				max_s = 78;
				break;

			case 80:
				max_s = 156;
				break;

			case 35:
			default:
				max_s = 68;
				break;
			}

			/* Process skitzo here -- we set the first 34 granules as allocated. */

			if (skitzo == 1)
			{
				min_s = 34;
			}


			for (s = min_s; s < max_s; s++)
			{
				sector[s] = 0xFF;
			}


			size = bps;

			_native_write(nativepath, sector, &size);


			/* 3. Write 14 sectors of 0xFF. */

			memset(sector, 0xFF, bps);

			for (s = 0; s < 14; s++)
			{
				size = bps;

				_native_write(nativepath, sector, &size);
			}


			/* 4. If disk name was provided, copy it to sector. */

			if (diskName != NULL)
			{
				strcpy(sector, diskName);
			}


			/* 5. Write 17th sector. */

			size = bps;

			_native_write(nativepath, sector, &size);


			memset(sector, 0xFF, bps);


			/* 6. Write 18th sector. */

			size = bps;

			_native_write(nativepath, sector, &size);
		}


		/* 5. Write remaining 17, 22 or 62 tracks of $FF. */

		memset(sector, 0xFF, bps);

		{
			int t, s;

			for (t = 0; t < tracks - 18; t++)
			{
				for (s = 1; s < 19; s++)
				{
					u_int size = bps;

					_native_write(nativepath, sector,
						      &size);
				}
			}
		}
	}


	_native_close(nativepath);


	return (ec);
}
