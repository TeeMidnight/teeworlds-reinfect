#include <game/server/entities/character.h>
#include <game/server/gamemodes/reinfect.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include "hammer.h"

CWeaponHammer::CWeaponHammer(int Owner) : IWeapon(Owner)
{
    SetFireDelay(125); // 0.125s
    SetMaxammo(-1);
    SetDamage(3);
    SetAmmo(-1);
    m_DamageInfect = 12;
    
    SetType(WEAPON_HAMMER);
}

const char* CWeaponHammer::GetName()
{
    return "hammer";
}

bool CWeaponHammer::Fire(CGameWorld *pGameworld, vec2 Pos, vec2 Direction, vec2 Pos2)
{
    CGameContext *pGameServer = pGameworld->GameServer();
    CCharacter *pOwnerChar = pGameServer->GetPlayerChar(GetOwner());
    IServer *pServer = pGameworld->Server();

    CGameControllerReinfect *pController = (CGameControllerReinfect *) pGameServer->m_pController;
    int Damage = pController->IsInfect(GetOwner()) ? m_DamageInfect : GetDamage();

    pGameServer->CreateSound(Pos2, SOUND_HAMMER_FIRE);

    CCharacter *apEnts[MAX_CLIENTS];
    int Hits = 0;
    int Num = pGameworld->FindEntities(Pos, CCharacter::ms_PhysSize*0.5f, (CEntity**)apEnts,
                                                MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

    for(int i = 0; i < Num; ++i)
    {
        CCharacter *pTarget = apEnts[i];

        if((pTarget == pOwnerChar) || pGameServer->Collision()->IntersectLine(Pos, pTarget->GetPos(), NULL, NULL))
            continue;

        // set his velocity to fast upward (for now)
        if(length(pTarget->GetPos()-Pos) > 0.0f)
            pGameServer->CreateHammerHit(pTarget->GetPos()-normalize(pTarget->GetPos()-Pos)*CCharacter::ms_PhysSize*0.5f);
        else
            pGameServer->CreateHammerHit(Pos);

        vec2 Dir;
        if(length(pTarget->GetPos() - Pos2) > 0.0f)
            Dir = normalize(pTarget->GetPos() - Pos2);
        else
            Dir = vec2(0.f, -1.f);

        if(pController->IsInfect(pTarget->GetPlayer()->GetCID()) != pController->IsInfect(GetOwner()) || !pController->IsInfect(pTarget->GetPlayer()->GetCID()))
        {
            pTarget->TakeDamage(vec2(0.f, -1.f) + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f, Dir*-1, 
                Damage, GetOwner(), GetType());
        }else if(pController->IsInfect(GetOwner()))
        {
            if(!pTarget->IncreaseHealth(GetDamage()))
                pTarget->IncreaseArmor(GetDamage());
        }
        Hits++;
    }

    // if we Hit anything, we have to wait for the reload
    if(Hits)
        SetReloadTimer(pServer->TickSpeed()/3);

    return true;
}
