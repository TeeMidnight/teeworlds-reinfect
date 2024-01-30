/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_MESSAGE_H
#define ENGINE_MESSAGE_H

#include <engine/shared/packer.h>

class CMsgPacker : public CPacker
{
	int m_Type;
	bool m_System;
	bool m_Convert;
public:
	CMsgPacker(int Type, bool System = false, bool Convert = true)
	{
		Reset();
		if(Type < 0 || Type > 0x3FFFFFFF)
		{
			m_Error = true;
			return;
		}
		
		if(!Convert)
			AddInt((Type << 1) | (System ? 1 : 0));

		m_Type = Type;
		m_System = System;
		m_Convert = Convert;
	}

	int Type() const { return m_Type; }
	bool System() const { return m_System; }
	bool Convert() const { return m_Convert; }
};

class CMsgUnpacker : public CUnpacker
{
	int m_Type;
	bool m_System;

public:
	CMsgUnpacker(const void *pData, int Size)
	{
		Reset(pData, Size);
		const int Msg = GetInt();
		if(Msg < 0)
			m_Error = true;
		if(m_Error)
		{
			m_System = false;
			m_Type = 0;
			return;
		}
		m_System = Msg & 1;
		m_Type = Msg >> 1;
	}

	int Type() const { return m_Type; }
	bool System() const { return m_System; }
};

#endif
