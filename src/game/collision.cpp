/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <base/math.h>
#include <base/vmath.h>

#include <math.h>
#include <engine/map.h>
#include <engine/kernel.h>

#include <game/mapitems.h>
#include <game/layers.h>
#include <game/collision.h>

CCollision::CCollision()
{
	m_pTiles = 0;
	m_Width = 0;
	m_Height = 0;
	m_pLayers = 0;
	m_pSwitch = 0;
}

void CCollision::Init(class CLayers *pLayers)
{
	m_pLayers = pLayers;
	m_Width = m_pLayers->GameLayer()->m_Width;
	m_Height = m_pLayers->GameLayer()->m_Height;
	m_pTiles = static_cast<CTile *>(m_pLayers->Map()->GetData(m_pLayers->GameLayer()->m_Data));

	if(m_pLayers->SwitchLayer())
	{
		unsigned int Size = m_pLayers->Map()->GetDataSize(m_pLayers->SwitchLayer()->m_Switch);
		if(Size >= (size_t)m_Width * m_Height * sizeof(CSwitchTile))
			m_pSwitch = static_cast<CSwitchTile *>(m_pLayers->Map()->GetData(m_pLayers->SwitchLayer()->m_Switch));
	}
}

int CCollision::GetTile(int x, int y, bool NoEntities) const
{
	int Nx = clamp(x/32, 0, m_Width-1);
	int Ny = clamp(y/32, 0, m_Height-1);

	if(!NoEntities)
	{
		for(auto& Rect : m_CollisionRects)
		{
			const CCollisionRect *pRect = &Rect.second;
			if(x >= pRect->m_Pos.x - pRect->m_Size.x / 2.0f && x <= pRect->m_Pos.x + pRect->m_Size.x / 2.0f &&
				y >= pRect->m_Pos.y - pRect->m_Size.y / 2.0f && y <= pRect->m_Pos.y + pRect->m_Size.y / 2.0f)
			{
				return pRect->m_Flag;
			}
		}
	}

	int Flag = 0;
	if(m_pTiles[Ny*m_Width+Nx].m_Index == TILE_SOLID)
		Flag |= COLFLAG_SOLID;
	if(m_pTiles[Ny*m_Width+Nx].m_Index == TILE_NOHOOK)
		Flag |= COLFLAG_SOLID | COLFLAG_NOHOOK;
	if(m_pTiles[Ny*m_Width+Nx].m_Index == TILE_DEATH)
		Flag |= COLFLAG_DEATH;

	return Flag;
}

bool CCollision::IsTile(int x, int y, int Flag, bool NoEntities) const
{
	return GetTile(x, y, NoEntities)&Flag;
}

// TODO: rewrite this smarter!
int CCollision::IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision, bool NoEntities) const
{
	const int End = distance(Pos0, Pos1)+1;
	const float InverseEnd = 1.0f/End;
	vec2 Last = Pos0;

	for(int i = 0; i <= End; i++)
	{
		vec2 Pos = mix(Pos0, Pos1, i*InverseEnd);
		if(CheckPoint(Pos.x, Pos.y, COLFLAG_SOLID, NoEntities))
		{
			if(pOutCollision)
				*pOutCollision = Pos;
			if(pOutBeforeCollision)
				*pOutBeforeCollision = Last;
			return GetCollisionAt(Pos.x, Pos.y, NoEntities);
		}
		Last = Pos;
	}
	if(pOutCollision)
		*pOutCollision = Pos1;
	if(pOutBeforeCollision)
		*pOutBeforeCollision = Pos1;
	return 0;
}

// TODO: OPT: rewrite this smarter!
void CCollision::MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces, bool NoEntities) const
{
	if(pBounces)
		*pBounces = 0;

	vec2 Pos = *pInoutPos;
	vec2 Vel = *pInoutVel;
	if(CheckPoint(Pos + Vel, NoEntities))
	{
		int Affected = 0;
		if(CheckPoint(Pos.x + Vel.x, Pos.y, NoEntities))
		{
			pInoutVel->x *= -Elasticity;
			if(pBounces)
				(*pBounces)++;
			Affected++;
		}

		if(CheckPoint(Pos.x, Pos.y + Vel.y, NoEntities))
		{
			pInoutVel->y *= -Elasticity;
			if(pBounces)
				(*pBounces)++;
			Affected++;
		}

		if(Affected == 0)
		{
			pInoutVel->x *= -Elasticity;
			pInoutVel->y *= -Elasticity;
		}
	}
	else
	{
		*pInoutPos = Pos + Vel;
	}
}

bool CCollision::TestBox(vec2 Pos, vec2 Size, int Flag, bool NoEntities) const
{
	Size *= 0.5f;
	if(CheckPoint(Pos.x-Size.x, Pos.y-Size.y, Flag, NoEntities))
		return true;
	if(CheckPoint(Pos.x+Size.x, Pos.y-Size.y, Flag, NoEntities))
		return true;
	if(CheckPoint(Pos.x-Size.x, Pos.y+Size.y, Flag, NoEntities))
		return true;
	if(CheckPoint(Pos.x+Size.x, Pos.y+Size.y, Flag, NoEntities))
		return true;
	return false;
}

void CCollision::MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity, bool *pDeath, bool NoEntities) const
{
	// do the move
	vec2 Pos = *pInoutPos;
	vec2 Vel = *pInoutVel;

	const float Distance = length(Vel);
	const int Max = (int)Distance;

	if(pDeath)
		*pDeath = false;

	if(Distance > 0.00001f)
	{
		const float Fraction = 1.0f/(Max+1);
		for(int i = 0; i <= Max; i++)
		{
			vec2 NewPos = Pos + Vel*Fraction; // TODO: this row is not nice

			//You hit a deathtile, congrats to that :)
			//Deathtiles are a bit smaller
			if(pDeath && TestBox(vec2(NewPos.x, NewPos.y), Size*(2.0f/3.0f), COLFLAG_DEATH))
			{
				*pDeath = true;
			}

			if(TestBox(vec2(NewPos.x, NewPos.y), Size, COLFLAG_SOLID, NoEntities))
			{
				int Hits = 0;

				if(TestBox(vec2(Pos.x, NewPos.y), Size, COLFLAG_SOLID, NoEntities))
				{
					NewPos.y = Pos.y;
					Vel.y *= -Elasticity;
					Hits++;
				}

				if(TestBox(vec2(NewPos.x, Pos.y), Size, COLFLAG_SOLID, NoEntities))
				{
					NewPos.x = Pos.x;
					Vel.x *= -Elasticity;
					Hits++;
				}

				// neither of the tests got a collision.
				// this is a real _corner case_!
				if(Hits == 0)
				{
					NewPos.y = Pos.y;
					Vel.y *= -Elasticity;
					NewPos.x = Pos.x;
					Vel.x *= -Elasticity;
				}
			}

			Pos = NewPos;
		}
	}

	*pInoutPos = Pos;
	*pInoutVel = Vel;
}

int CCollision::AddCollisionRect(CCollisionRect Rect)
{
	m_CollisionRects[m_CollisionRectsIndex ++] = Rect;
	return m_CollisionRectsIndex - 1;
}

void CCollision::RemoveCollisionRect(int Index)
{
	m_CollisionRects.erase(Index);
}

void CCollision::ClearCollisionRects()
{
	m_CollisionRectsIndex = 0;
	m_CollisionRects.clear();
}