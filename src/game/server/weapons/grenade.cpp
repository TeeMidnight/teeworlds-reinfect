#include <game/server/entities/projectile.h>
#include <game/server/gamecontext.h>
#include "grenade.h"

CWeaponGrenade::CWeaponGrenade(int Owner) : IWeapon(Owner)
{
    SetFireDelay(1500); // 1.5s
    SetMaxammo(2);
    SetDamage(8);
    SetAmmo(2);
    
    SetType(WEAPON_GRENADE);
}

const char* CWeaponGrenade::GetName()
{
    return "grenade";
}

bool CWeaponGrenade::Fire(CGameWorld *pGameworld, vec2 Pos, vec2 Direction, vec2 Pos2)
{
    CGameContext *pGameServer = pGameworld->GameServer();
    IServer *pServer = pGameworld->Server();

    new CProjectile(pGameworld, WEAPON_GRENADE,
        GetOwner(),
        Pos,
        Direction,
        (int)(pServer->TickSpeed()*pGameServer->Tuning()->m_GrenadeLifetime),
        GetDamage(), true, 0, SOUND_GRENADE_EXPLODE, GetType());

    pGameServer->CreateSound(Pos2, SOUND_GRENADE_FIRE);

    return true;
}
