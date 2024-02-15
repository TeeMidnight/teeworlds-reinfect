#include <game/server/entities/projectile.h>
#include <game/server/gamecontext.h>
#include "gun.h"

CWeaponGun::CWeaponGun(int Owner) : IWeapon(Owner)
{
    SetFireDelay(125); // 0.125s
    SetMaxammo(5);
    SetDamage(2);
    SetAmmo(5);
    
    SetType(WEAPON_GUN);
}

const char* CWeaponGun::GetName()
{
    return "gun";
}

bool CWeaponGun::Fire(CGameWorld *pGameworld, vec2 Pos, vec2 Direction, vec2 Pos2)
{
    CGameContext *pGameServer = pGameworld->GameServer();
    IServer *pServer = pGameworld->Server();

    new CProjectile(pGameworld, WEAPON_GUN,
        GetOwner(),
        Pos,
        Direction,
        (int)(pServer->TickSpeed()*pGameServer->Tuning()->m_GunLifetime),
        GetDamage(), false, 0, -1, GetType());

    pGameServer->CreateSound(Pos2, SOUND_GUN_FIRE);

    return true;
}
