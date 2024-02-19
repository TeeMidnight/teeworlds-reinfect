#include <game/server/entities/laser.h>
#include <game/server/gamecontext.h>
#include "laser.h"

CWeaponLaser::CWeaponLaser(int Owner) : IWeapon(Owner)
{
    SetFireDelay(1500); // 1.5s
    SetMaxammo(2);
    SetDamage(15);
    SetAmmo(2);
    
    SetType(WEAPON_LASER);
}

const char* CWeaponLaser::GetName()
{
    return "laser";
}

bool CWeaponLaser::Fire(CGameWorld *pGameworld, vec2 Pos, vec2 Direction, vec2 Pos2)
{
    CGameContext *pGameServer = pGameworld->GameServer();

    new CLaser(pGameworld, Pos, Direction, pGameServer->Tuning()->m_LaserReach, GetOwner(), GetDamage(), GetType());
    pGameServer->CreateSound(Pos2, SOUND_LASER_FIRE);

    return true;
}
