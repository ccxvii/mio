/* Simple DDS (Direct Draw Surface) image loader for DXT[135] */

#include "mio.h"

/* header.flags */
#define DDSD_CAPS			0x00000001
#define DDSD_HEIGHT			0x00000002
#define DDSD_WIDTH			0x00000004
#define DDSD_PITCH			0x00000008
#define DDSD_PIXELFORMAT		0x00001000
#define DDSD_MIPMAPCOUNT		0x00020000
#define DDSD_LINEARSIZE			0x00080000
#define DDSD_DEPTH			0x00800000

/* header.pixelFormat.flags */
#define DDPF_ALPHAPIXELS		0x00000001
#define DDPF_FOURCC			0x00000004
#define DDPF_INDEXED			0x00000020
#define DDPF_RGB			0x00000040

/* header.caps.dwCaps1 */
#define DDSCAPS_COMPLEX			0x00000008
#define DDSCAPS_TEXTURE			0x00001000
#define DDSCAPS_MIPMAP			0x00400000

/* header.caps.caps2 */
#define DDSCAPS2_CUBEMAP		0x00000200
#define DDSCAPS2_CUBEMAP_POSITIVEX	0x00000400
#define DDSCAPS2_CUBEMAP_NEGATIVEX	0x00000800
#define DDSCAPS2_CUBEMAP_POSITIVEY	0x00001000
#define DDSCAPS2_CUBEMAP_NEGATIVEY	0x00002000
#define DDSCAPS2_CUBEMAP_POSITIVEZ	0x00004000
#define DDSCAPS2_CUBEMAP_NEGATIVEZ	0x00008000
#define DDSCAPS2_VOLUME			0x00200000

static void dds_warn(char *msg)
{
	fprintf(stderr, "dds error: %s\n", msg);
}

static inline int getint(FILE *file)
{
	int a = getc(file);
	int b = getc(file);
	int c = getc(file);
	int d = getc(file);
	return (d << 24) | (c << 16) | (b << 8) | a;
}

struct dds_info_t
{
	int div_size;
	int block_bytes;
	int int_fmt;
};

static struct dds_info_t dds_info_dxt1 = {
	4, 8, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
};
static struct dds_info_t dds_info_dxt3 = {
	4, 16, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
};
static struct dds_info_t dds_info_dxt5 = {
	4, 16, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
};

#define MAX(a,b) (a>b?a:b)

int load_texture_dds(char *filename)
{
	int format;
	FILE *file;
	unsigned int texture;
	unsigned char *data;
	char pf_fourcc[5];
	char magic[4];
	int i, w, h, size, blocksize;

	file = fopen(filename, "rb");
	if (!file) {
		dds_warn("cannot open dds file");
		return -1;
	}

	fread(magic, 1, 4, file);
	if (memcmp(magic, "DDS ", 4)) {
		dds_warn("not a dds file");
		return -1;
	}

	unsigned int hdr_size = getint(file); /* must be 124 */
	unsigned int hdr_flags = getint(file);
	unsigned int hdr_height = getint(file);
	unsigned int hdr_width = getint(file);
	unsigned int hdr_pitch_or_linear_size = getint(file);
	unsigned int hdr_depth = getint(file);
	unsigned int hdr_mipmap_count = getint(file);
	for (i = 0; i < 11; i++)
		(void) getint(file); /* reserved 1 */

	unsigned int pf_size = getint(file); /* must be 32 */
	unsigned int pf_flags = getint(file);
	fread(pf_fourcc, 1, 4, file); pf_fourcc[4] = 0;
	unsigned int pf_bits = getint(file);
	unsigned int pf_rmask = getint(file);
	unsigned int pf_gmask = getint(file);
	unsigned int pf_bmask = getint(file);
	unsigned int pf_amask = getint(file);

	unsigned int caps1 = getint(file);
	unsigned int caps2 = getint(file);
	(void) getint(file);
	(void) getint(file);

	(void) getint(file); /* reserved 2 */

	/*
	 * Select a recognized image format.
	 */

	format = 0;
	if (pf_flags & DDPF_FOURCC) {
		if (!memcmp(pf_fourcc, "DXT1", 4))
			format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		if (!memcmp(pf_fourcc, "DXT3", 4))
			format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		if (!memcmp(pf_fourcc, "DXT5", 4))
			format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
	}
	if (!format) {
		dds_warn("unknown dds pixel format");
		return -1;
	}

	if (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT)
		blocksize = 8;
	else
		blocksize = 16;

	printf("loading dds %dx%d fourcc=%s mipmaps=%d\n", hdr_width, hdr_height, pf_fourcc, hdr_mipmap_count);

	/* TODO: cube maps */

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	w = hdr_width;
	h = hdr_height;
	size = MAX(4, w) / 4 * MAX(4, h) / 4 * blocksize;
	data = malloc(size);
	for (i = 0; i < hdr_mipmap_count; i++) {
		fread(data, 1, size, file);
		glCompressedTexImage2D(GL_TEXTURE_2D, i, format, w, h, 0, size, data);
		w = (w + 1) >> 1;
		h = (h + 1) >> 1;
		size = MAX(4, w) / 4 * MAX(4, h) / 4 * blocksize;
	}
	free(data);

	return texture;
}

#ifdef TEST
int main(int argc, char **argv)
{
	int i;
	for (i = 1; i < argc; i++) {
		load_dds_texture(argv[i]);
	}
}
#endif
