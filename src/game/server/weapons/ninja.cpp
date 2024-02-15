#include <generated/protocol.h>
#include "ninja.h"

CWeaponNinja::CWeaponNinja(int Owner) : IWeapon(Owner)
{
    SetFireDelay(800);
    SetMaxammo(-1);
    SetDamage(-1);
    SetAmmo(-1);
    
    SetType(WEAPON_NINJA);
}

const char* CWeaponNinja::GetName()
{
    return "ninja";
}