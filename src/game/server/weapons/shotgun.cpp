#include <game/server/entities/projectile.h>
#include <game/server/gamecontext.h>
#include "shotgun.h"

CWeaponShotgun::CWeaponShotgun(int Owner) : IWeapon(Owner)
{
    SetFireDelay(1000); // 1s
    SetMaxammo(3);
    SetDamage(2);
    SetAmmo(3);
    
    SetType(WEAPON_SHOTGUN);
}

const char* CWeaponShotgun::GetName()
{
    return "shotgun";
}

bool CWeaponShotgun::Fire(CGameWorld *pGameworld, vec2 Pos, vec2 Direction, vec2 Pos2)
{
    CGameContext *pGameServer = pGameworld->GameServer();
    IServer *pServer = pGameworld->Server();

    int ShotSpread = 2;

    for(int i = -ShotSpread; i <= ShotSpread; ++i)
    {
        float Spreading[] = {-0.185f, -0.070f, 0, 0.070f, 0.185f};
        float a = angle(Direction);
        a += Spreading[i+2];
        float v = 1-(absolute(i)/(float)ShotSpread);
        float Speed = mix((float)pGameServer->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);
        new CProjectile(pGameworld, WEAPON_SHOTGUN,
            GetOwner(),
            Pos,
            vec2(cosf(a), sinf(a))*Speed,
            (int)(pServer->TickSpeed()*pGameServer->Tuning()->m_ShotgunLifetime),
            GetDamage(), false, 0, -1, GetType());
    }

    pGameServer->CreateSound(Pos2, SOUND_SHOTGUN_FIRE);

    return true;
}
