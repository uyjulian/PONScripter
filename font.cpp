/*
	C++ Freetype rendering code.
	Copyright 2006 Peter Jolly.

	Heavily based on, and much copied verbatim from, SDL_ttf, which is
	copyright (C) 1997-2004 Sam Lantinga.
	
	This library is free software; you can redistribute it and/or modify it 
	under the terms of the GNU General Public License as published by the Free 
	Software Foundation; either version 2 of the License, or (at your option) 
	any later version.
	
	This library is distributed in the hope that it will be useful, but WITHOUT 
	ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
	FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for 
	more details.
	
	You should have received a copy of the GNU General Public License along with 
	this library; if not, write to the Free Foundation, Inc., 59 Temple Place, 
	Suite 330, Boston, MA  02111-1307  USA
*/

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_TRUETYPE_IDS_H
#include <stdio.h>
#include <math.h>
#include "font.h"

HintingMode hinting = NoHinting;
bool lightrender = false;
bool subpixel = false;

inline FT_Int32
load_mode()
{
	return FT_LOAD_NO_BITMAP 
	     | (hinting == NoHinting ? FT_LOAD_NO_HINTING 
	                             : (hinting == LightHinting ? FT_LOAD_TARGET_LIGHT
	                                                        : FT_LOAD_TARGET_NORMAL));
}

inline FT_Render_Mode
render_mode()
{
	return lightrender ? FT_RENDER_MODE_LIGHT : FT_RENDER_MODE_NORMAL;
}

inline FT_Kerning_Mode
kerning_mode()
{
	return FT_KERNING_UNFITTED;//hinting == NoHinting ? FT_KERNING_UNFITTED : FT_KERNING_DEFAULT;
}


#define FT_FLOOR(X)	(((X) & -64) / 64)
#define FT_CEIL(X)	((((X) + 63) & -64) / 64)

static class FT {
	FT_Library ft;
public:
	FT() { FT_Init_FreeType(&ft); }
	~FT() { FT_Done_FreeType(ft); }
	inline FT_Library& operator()() { return ft; }
} freetype;

struct FontInternals {
	FT_Open_Args args, met;
	FT_Face face;
	FT_Error err;

	int currsize;

	FontInternals(const Uint8* data, size_t len, const Uint8* mdat, size_t mlen); // takes ownership of data and mdat
	
	~FontInternals() {
		FT_Done_Face(face);
		delete[] (const Uint8*) args.memory_base;
		if (met.memory_base) delete[] (const Uint8*) met.memory_base;
	}
	
	FT_GlyphSlot load_glyph(Uint16 unicode) {
		err = FT_Load_Glyph(face, FT_Get_Char_Index(face, unicode), load_mode());
		return face->glyph;
	}
};

FontInternals::FontInternals(const Uint8* data, size_t len, const Uint8* mdat, size_t mlen)
: currsize(0)
{
	args.flags = FT_OPEN_MEMORY;
	args.memory_base = (const FT_Byte*) data;
	args.memory_size = len;
	met.flags = FT_OPEN_MEMORY;
	met.memory_base = mdat;
	met.memory_size = mlen;
	FT_Error err = FT_Open_Face(freetype(), &args, 0, &face);
	if (err) throw "Failed to open face.";
	if (mdat) FT_Attach_Stream(face, &met);
}

Font::Font(const char* filename, const char* metrics)
{
	Uint8 *data, *mdat;
	size_t len, mlen;
	FILE* fp = fopen(filename, "rb");
	if (!fp) throw "This should never happen.";
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	data = new Uint8[len];
	fread(data, 1, len, fp);
	fclose(fp);
	if (metrics) {
		fp = fopen(metrics, "rb");
		if (!fp) throw "This should never happen.";
		fseek(fp, 0, SEEK_END);
		mlen = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		mdat = new Uint8[mlen];
		fread(mdat, 1, mlen, fp);
		fclose(fp);
	}
	else {
		mdat = NULL;
		mlen = 0;
	}
	priv = new FontInternals(data, len, mdat, mlen);
}

Font::Font(const Uint8* data, size_t len, const Uint8* mdat, size_t mlen)
{
	priv = new FontInternals(data, len, mdat, mlen);
}

Font::~Font()
{
	delete priv;
}

void Font::get_metrics(Uint16 ch, float* minx, float* maxx, float* miny, float* maxy)
{
	FT_Glyph_Metrics& metrics = priv->load_glyph(ch)->metrics;
	float hbx = float(metrics.horiBearingX) / 64.0;
	float hby = float(metrics.horiBearingY) / 64.0;
	if (!subpixel) {
		hbx = floor(hbx);
		hby = floor(hby);
	}
	if (minx) *minx = hbx;
	if (maxx) {
		*maxx = hbx + float(metrics.width) / 64.0;
		if (!subpixel) *maxx = ceil(*maxx);
	}
	if (miny) {
		*miny = hby + float(metrics.height) / 64.0;
		if (!subpixel) *miny = ceil(*miny);
	}
	if (maxy) *maxy = hby;
}

float Font::advance(Uint16 ch)
{
	FT_Glyph_Metrics& metrics = priv->load_glyph(ch)->metrics;
	float rv = float(metrics.horiAdvance) / 64.0;
	return subpixel ? rv : floor(rv);
}

float Font::kerning(Uint16 left, Uint16 right)
{
	FT_Face& face = priv->face;
	FT_Vector kern;
	FT_Error err = FT_Get_Kerning(face, FT_Get_Char_Index(face, left), FT_Get_Char_Index(face, right), 
	                              kerning_mode(), &kern);
	if (err) return 0.0;
	float rv = float(kern.x) / 64.0;
	return subpixel ? rv : floor(rv);
}

int Font::ascent()
{
	return FT_CEIL(FT_MulFix(priv->face->ascender, priv->face->size->metrics.y_scale));
}

int Font::lineskip()
{
	return FT_CEIL(FT_MulFix(priv->face->height, priv->face->size->metrics.y_scale));
}

void Font::set_size(int val)
{
	if (val != priv->currsize) {
		priv->currsize = val;
		FT_Set_Char_Size(priv->face, 0, val * 64, 0, 0);
	}
}

SDL_Surface* Font::render_glyph(Uint16 ch, SDL_Color fg, SDL_Color bg, float x_fractional_part)
{
    FT_Vector v;
	v.x = subpixel ? FT_Pos(x_fractional_part * 64.0) : 0;
	v.y = 0;
	FT_Set_Transform(priv->face, 0, &v);

	FT_GlyphSlot glyph = priv->load_glyph(ch);
	if (priv->err) return NULL;
	
	FT_Error err = FT_Render_Glyph(glyph, render_mode());
	if (err) return NULL;
	
	SDL_Surface* rv = SDL_CreateRGBSurface(SDL_SWSURFACE, glyph->bitmap.width, glyph->bitmap.rows, 8, 0, 0, 0, 0);
	if (!rv) return NULL;
	
	// Fill palette with 256 shades interpolating between foreground and background colours.
	SDL_Palette* pal = rv->format->palette;
	int dr = fg.r - bg.r;
	int dg = fg.g - bg.g;
	int db = fg.b - bg.b;
	for (int i = 0; i < 256; ++i) {
		pal->colors[i].r = bg.r + i * dr / 255;
		pal->colors[i].g = bg.g + i * dg / 255;
		pal->colors[i].b = bg.b + i * db / 255;
	}

	// Copy the character from the pixmap
	Uint8* src = (Uint8*) glyph->bitmap.buffer;
	SDL_LockSurface(rv);
	Uint8* dst = (Uint8*) rv->pixels;
	for (int row = 0; row < rv->h; ++row) {
		memcpy(dst, src, glyph->bitmap.pitch);
		src += glyph->bitmap.pitch;
		dst += rv->pitch;
	}
	SDL_UnlockSurface(rv);
	
	return rv;
}

bool
Font::has_char(Uint16 ch)
{
	return FT_Get_Char_Index(priv->face, ch);
}
