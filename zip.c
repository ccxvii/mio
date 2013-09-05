#include "mio.h"

// to get zlib decompressor prototypes
#define STBI_NO_WRITE
#define STBI_NO_HDR
#define STBI_HEADER_FILE_ONLY
#include "stb_image.c"

#define ZIP_LOCAL_FILE_SIG 0x04034b50
#define ZIP_CENTRAL_DIRECTORY_SIG 0x02014b50
#define ZIP_END_OF_CENTRAL_DIRECTORY_SIG 0x06054b50

struct entry {
	char *name;
	int offset;
};

struct archive {
	FILE *file;
	int count;
	struct entry *table;
	struct archive *next;
};

static inline int getshort(FILE *file)
{
	int a = getc(file);
	int b = getc(file);
	return a | b << 8;
}

static inline int getlong(FILE *file)
{
	int a = getc(file);
	int b = getc(file);
	int c = getc(file);
	int d = getc(file);
	return a | b << 8 | c << 16 | d << 24;
}

static int cmpentry(const void *a_, const void *b_)
{
	const struct entry *a = a_;
	const struct entry *b = b_;
	return strcmp(a->name, b->name);
}

static unsigned char *read_zip_file(FILE *file, int offset, int *sizep)
{
	int sig, method, csize, usize;
	int namelength, extralength;
	char *cdata, *udata;

	fseek(file, offset, 0);

	sig = getlong(file);
	if (sig != ZIP_LOCAL_FILE_SIG) {
		warn("zip: wrong signature for local file");
		return NULL;
	}

	(void) getshort(file); /* version */
	(void) getshort(file); /* general */
	method = getshort(file);
	(void) getshort(file); /* file time */
	(void) getshort(file); /* file date */
	(void) getlong(file); /* crc-32 */
	csize = getlong(file); /* csize */
	usize = getlong(file); /* usize */
	namelength = getshort(file);
	extralength = getshort(file);

	fseek(file, namelength + extralength, 1);

	if (method == 0 && csize == usize) {
		cdata = malloc(csize);
		fread(cdata, 1, csize, file);
		*sizep = csize;
		return (unsigned char*) cdata;
	}

	if (method == 8) {
		cdata = malloc(csize);
		fread(cdata, 1, csize, file);
		udata = malloc(usize);
		usize = stbi_zlib_decode_noheader_buffer(udata, usize, cdata, csize);
		free(cdata);
		if (usize < 0) {
			warn("zip: %s", stbi_failure_reason());
			return NULL;
		}
		*sizep = usize;
		return (unsigned char*) udata;
	}

	warn("zip: unknown compression method");
	return NULL;
}

static int read_zip_dir_imp(struct archive *zip, int startoffset)
{
	FILE *file = zip->file;
	int sig, offset, count;
	int namesize, metasize, commentsize;
	int i, k;

	fseek(file, startoffset, 0);

	sig = getlong(file);
	if (sig != ZIP_END_OF_CENTRAL_DIRECTORY_SIG) {
		warn("zip: wrong signature for end of central directory");
		return -1;
	}

	(void) getshort(file); /* this disk */
	(void) getshort(file); /* start disk */
	(void) getshort(file); /* entries in this disk */
	count = getshort(file); /* entries in central directory disk */
	(void) getlong(file); /* size of central directory */
	offset = getlong(file); /* offset to central directory */

	zip->count = count;
	zip->table = calloc(count, sizeof(struct entry));

	fseek(file, offset, 0);

	for (i = 0; i < count; i++)
	{
		struct entry *entry = zip->table + i;

		sig = getlong(file);
		if (sig != ZIP_CENTRAL_DIRECTORY_SIG) {
			for (k = 0; k < i; k++)
				free(zip->table[k].name);
			free(zip->table);
			warn("zip: wrong signature for central directory");
			return -1;
		}

		(void) getshort(file); /* version made by */
		(void) getshort(file); /* version to extract */
		(void) getshort(file); /* general */
		(void) getshort(file); /* method */
		(void) getshort(file); /* last mod file time */
		(void) getshort(file); /* last mod file date */
		(void) getlong(file); /* crc-32 */
		(void) getlong(file); /* csize */
		(void) getlong(file); /* usize */
		namesize = getshort(file);
		metasize = getshort(file);
		commentsize = getshort(file);
		(void) getshort(file); /* disk number start */
		(void) getshort(file); /* int file atts */
		(void) getlong(file); /* ext file atts */
		entry->offset = getlong(file);

		entry->name = malloc(namesize + 1);
		fread(entry->name, 1, namesize, file);
		entry->name[namesize] = 0;

		fseek(file, metasize, 1);
		fseek(file, commentsize, 1);
	}

	qsort(zip->table, count, sizeof(struct entry), cmpentry);

	return 0;
}

static int read_zip_dir(struct archive *zip)
{
	FILE *file = zip->file;
	unsigned char buf[512];
	int filesize, back, maxback;
	int i, n;

	fseek(file, 0, 2);
	filesize = ftell(file);

	maxback = MIN(filesize, 0xFFFF + sizeof buf);
	back = MIN(maxback, sizeof buf);

	while (back < maxback) {
		fseek(file, filesize - back, 0);
		n = fread(buf, 1, sizeof buf, file);
		if (n < 0) {
			warn("zip: cannot read end of central directory");
			return -1;
		}

		for (i = n - 4; i > 0; i--)
			if (!memcmp(buf + i, "PK\5\6", 4))
				return read_zip_dir_imp(zip, filesize - back + i);

		back += sizeof buf - 4;
	}

	warn("zip: cannot find end of central directory");
	return -1;
}

struct archive *open_archive(const char *filename)
{
	struct archive *zip;
	FILE *file;

	file = fopen(filename, "rb");
	if (!file) {
		warn("cannot open archive: '%s'", filename);
		return NULL;
	}

	zip = malloc(sizeof(struct archive));
	zip->file = file;
	zip->count = 0;
	zip->table = NULL;
	zip->next = NULL;

	if (read_zip_dir(zip) < 0) {
		free(zip);
		fclose(file);
		return NULL;
	}

	return zip;
}

void close_archive(struct archive *zip)
{
	int i;
	fclose(zip->file);
	for (i = 0; i < zip->count; i++)
		free(zip->table[i].name);
	free(zip->table);
	free(zip);
}

unsigned char *read_archive(struct archive *zip, const char *filename, int *sizep)
{
	int l = 0;
	int r = zip->count - 1;
	while (l <= r) {
		int m = (l + r) >> 1;
		int c = strcmp(filename, zip->table[m].name);
		if (c < 0)
			r = m - 1;
		else if (c > 0)
			l = m + 1;
		else
			return read_zip_file(zip->file, zip->table[m].offset, sizep);
	}
	return NULL;
}

unsigned char *read_file(const char *filename, int *lenp)
{
	unsigned char *data;
	int len;
	FILE *file = fopen(filename, "rb");
	if (!file) {
		return NULL;
	}
	fseek(file, 0, 2);
	len = ftell(file);
	fseek(file, 0, 0);
	data = malloc(len);
	fread(data, 1, len, file);
	fclose(file);
	if (lenp) *lenp = len;
	return data;
}

/*
 * Virtual filesystem -- look for files in registered directories and archives.
 */

struct directory
{
	char *name;
	struct directory *next;
};

static struct directory *dir_head = NULL;
static struct archive *zip_head = NULL;

void register_directory(const char *dirname)
{
	char buf[512];
	int n = strlen(dirname);
	struct directory *dir = malloc(sizeof(struct directory));
	strlcpy(buf, dirname, sizeof buf);
	if (n > 0 && buf[n-1] != '/')
		strlcat(buf, "/", sizeof buf);
	dir->name = strdup(buf);
	dir->next = dir_head;
	dir_head = dir;
}

void register_archive(const char *zipname)
{
	struct archive *zip = open_archive(zipname);
	if (zip) {
		zip->next = zip_head;
		zip_head = zip;
	}
}

unsigned char *load_file(const char *filename, int *lenp)
{
	struct directory *dir = dir_head;
	struct archive *zip = zip_head;
	unsigned char *data;
	char buf[512];

	data = read_file(filename, lenp);

	while (!data && dir) {
		strlcpy(buf, dir->name, sizeof buf);
		strlcat(buf, filename, sizeof buf);
		data = read_file(buf, lenp);
		dir = dir->next;
	}

	while (!data && zip) {
		data = read_archive(zip, filename, lenp);
		zip = zip->next;
	}

	return data;
}
