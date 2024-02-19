/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_COLLISION_H
#define GAME_COLLISION_H

#include <base/vmath.h>

#include <map>

struct CCollisionRect
{
	vec2 m_Pos;
	vec2 m_Size;
	int m_Flag;
};

class CCollision
{
	struct CTile *m_pTiles;
	int m_Width;
	int m_Height;
	class CLayers *m_pLayers;
	class CSwitchTile *m_pSwitch;

	std::map<int, CCollisionRect> m_CollisionRects;
	int m_CollisionRectsIndex;

	bool IsTile(int x, int y, int Flag=COLFLAG_SOLID, bool NoEntities=false) const;
	int GetTile(int x, int y, bool NoEntities) const;

public:
	enum
	{
		COLFLAG_SOLID=1,
		COLFLAG_DEATH=2,
		COLFLAG_NOHOOK=4,
	};

	CCollision();
	void Init(class CLayers *pLayers);
	bool CheckPoint(float x, float y, int Flag=COLFLAG_SOLID, bool NoEntities = false) const { return IsTile(round_to_int(x), round_to_int(y), Flag, NoEntities); }
	bool CheckPoint(vec2 Pos, int Flag=COLFLAG_SOLID, bool NoEntities = false) const { return CheckPoint(Pos.x, Pos.y, Flag, NoEntities); }
	int GetCollisionAt(float x, float y, bool NoEntities = false) const { return GetTile(round_to_int(x), round_to_int(y), NoEntities); }
	int GetWidth() const { return m_Width; }
	int GetHeight() const { return m_Height; }
	int IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision, bool NoEntities = false) const;
	void MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces, bool NoEntities = false) const;
	void MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity, bool *pDeath=0, bool NoEntities = false) const;
	bool TestBox(vec2 Pos, vec2 Size, int Flag=COLFLAG_SOLID, bool NoEntities = false) const;

	// return index
	int AddCollisionRect(CCollisionRect Rect);
	void RemoveCollisionRect(int Index);
	void ClearCollisionRects();

	class CSwitchTile *SwitchLayer() { return m_pSwitch; }
	struct CTile *GameLayer() { return m_pTiles; }
};

#endif
