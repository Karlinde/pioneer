﻿#ifndef _TEXT_DISTANCEFIELDFONT_H
#define _TEXT_DISTANCEFIELDFONT_H
/*
 * Font rendering using a pregenerated signed distance field texture
 * to produce highly zoomable text.
 * Reads texture+definition generated by:
 * http://www.angelcode.com/products/bmfont/ combined with
 * http://bitsquid.blogspot.ca/2010/04/distance-field-based-rendering-of.html
 */
#include "RefCounted.h"

namespace Graphics {
	class Texture;
	class VertexArray;
}

namespace Text {

class DistanceFieldFont : public RefCounted {
public:
	DistanceFieldFont(const std::string &definitionFileName, Graphics::Texture*);
	void GetGeometry(Graphics::VertexArray&, const std::string&, const vector2f &offset);
	Graphics::Texture *GetTexture() const { return m_texture; }
	Graphics::VertexArray *CreateVertexArray() const;

private:
	struct Glyph {
		vector2f uv;
		vector2f uvSize;
		vector2f size; //geometry size
	};
	Graphics::Texture *m_texture;
	std::map<Uint32, Glyph> m_glyphs;

	void ParseLine(const StringRange&);
	void AddGlyph(Graphics::VertexArray &va, const vector2f &pos, const Glyph&);
};

}

#endif