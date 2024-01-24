/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_GRAPHICS_H
#define ENGINE_GRAPHICS_H

#include <base/vmath.h>

#include "kernel.h"


class CImageInfo
{
public:
	enum
	{
		FORMAT_AUTO = -1,
		FORMAT_RGB = 0,
		FORMAT_RGBA = 1,
		FORMAT_ALPHA = 2,
	};

	/* Variable: width
		Contains the width of the image */
	int m_Width;

	/* Variable: height
		Contains the height of the image */
	int m_Height;

	/* Variable: format
		Contains the format of the image. See <Image Formats> for more information. */
	int m_Format;

	/* Variable: data
		Pointer to the image data. */
	void *m_pData;

	static int GetPixelSize(int Format)
	{
		switch(Format)
		{
		case FORMAT_RGB: return 3;
		case FORMAT_RGBA: return 4;
		case FORMAT_ALPHA: return 1;
		}
		return 0;
	}

	int GetPixelSize() const
	{
		return GetPixelSize(m_Format);
	}
};


class IGraphics : public IInterface
{
public:
	class CTextureHandle
	{
		friend class IGraphics;
		int m_Id;
	public:
		CTextureHandle()
		: m_Id(-1)
		{}

		bool IsValid() const { return Id() >= 0; }
		int Id() const { return m_Id; }
		void Invalidate() { m_Id = -1; }
	};
};

#endif
