// stb_truetype_ext.h -- include after stb_truetype.h

// Extend stb_truetype to render with sub-pixel offsets.
// shift_x and shift_y must be <= 0 and < 1

static void stbtt__rasterize_ext(stbtt__bitmap *result, stbtt__point *pts,
	int *wcount, int windings,
	float scale_x, float scale_y, float shift_x, float shift_y,
	int off_x, int off_y, int invert, void *userdata)
{
	float y_scale_inv = invert ? -scale_y : scale_y;
	stbtt__edge *e;
	int n,i,j,k,m;
	int vsubsample = result->h < 8 ? 15 : 5;
	// vsubsample should divide 255 evenly; otherwise we won't reach full opacity

	// now we have to blow out the windings into explicit edge lists
	n = 0;
	for (i=0; i < windings; ++i)
		n += wcount[i];

	e = (stbtt__edge *) STBTT_malloc(sizeof(*e) * (n+1), userdata); // add an extra one as a sentinel
	if (e == 0) return;
	n = 0;

	m=0;
	for (i=0; i < windings; ++i) {
		stbtt__point *p = pts + m;
		m += wcount[i];
		j = wcount[i]-1;
		for (k=0; k < wcount[i]; j=k++) {
			int a=k,b=j;
			// skip the edge if horizontal
			if (p[j].y == p[k].y)
				continue;
			// add edge from j to k to the list
			e[n].invert = 0;
			if (invert ? p[j].y > p[k].y : p[j].y < p[k].y) {
				e[n].invert = 1;
				a=j,b=k;
			}
			e[n].x0 = p[a].x * scale_x + shift_x;
			e[n].y0 = p[a].y * y_scale_inv * vsubsample + shift_y;
			e[n].x1 = p[b].x * scale_x + shift_x;
			e[n].y1 = p[b].y * y_scale_inv * vsubsample + shift_y;
			++n;
		}
	}

	// now sort the edges by their highest point (should snap to integer, and then by x)
	STBTT_sort(e, n, sizeof(e[0]), stbtt__edge_compare);

	// now, traverse the scanlines and find the intersections on each scanline, use xor winding rule
	stbtt__rasterize_sorted_edges(result, e, n, vsubsample, off_x, off_y, userdata);

	STBTT_free(e, userdata);
}

void stbtt_RasterizeExt(stbtt__bitmap *result, float flatness_in_pixels,
	stbtt_vertex *vertices, int num_verts,
	float scale_x, float scale_y, float shift_x, float shift_y,
	int x_off, int y_off,
	int invert, void *userdata)
{
	float scale = scale_x > scale_y ? scale_y : scale_x;
	int winding_count, *winding_lengths;
	stbtt__point *windings = stbtt_FlattenCurves(vertices, num_verts, flatness_in_pixels / scale, &winding_lengths, &winding_count, userdata);
	if (windings) {
		stbtt__rasterize_ext(result, windings, winding_lengths, winding_count,
			scale_x, scale_y, shift_x, shift_y, x_off, y_off, invert, userdata);
		STBTT_free(winding_lengths, userdata);
		STBTT_free(windings, userdata);
	}
}

unsigned char *stbtt_GetGlyphBitmapExt(const stbtt_fontinfo *info,
	float scale_x, float scale_y, float shift_x, float shift_y,
	int glyph, int *width, int *height, int *xoff, int *yoff)
{
	int ix0,iy0,ix1,iy1;
	stbtt__bitmap gbm;
	stbtt_vertex *vertices;
	int num_verts = stbtt_GetGlyphShape(info, glyph, &vertices);

	if (scale_x == 0) scale_x = scale_y;
	if (scale_y == 0) {
		if (scale_x == 0) return NULL;
		scale_y = scale_x;
	}

	stbtt_GetGlyphBitmapBox(info, glyph, scale_x, scale_y, &ix0,&iy0,&ix1,&iy1);

	// now we get the size
	gbm.w = (ix1 - ix0);
	gbm.h = (iy1 - iy0);
	gbm.pixels = NULL; // in case we error

	// adjust for possible shift_x and shift_y
	if (gbm.w && gbm.h) {
		gbm.w++;
		gbm.h++;
	}

	if (width ) *width = gbm.w;
	if (height) *height = gbm.h;
	if (xoff) *xoff = ix0;
	if (yoff) *yoff = iy0;

	if (gbm.w && gbm.h) {
		gbm.pixels = (unsigned char *) STBTT_malloc(gbm.w * gbm.h, info->userdata);
		if (gbm.pixels) {
			gbm.stride = gbm.w;
			stbtt_RasterizeExt(&gbm, 0.35f, vertices, num_verts,
				scale_x, scale_y, shift_x, shift_y, ix0, iy0, 1, info->userdata);
		}
	}
	STBTT_free(vertices, info->userdata);
	return gbm.pixels;
}
