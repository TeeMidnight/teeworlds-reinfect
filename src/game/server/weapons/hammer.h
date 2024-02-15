#ifndef GAME_SERVER_WEAPONS_HAMMER_H
#define GAME_SERVER_WEAPONS_HAMMER_H

#include <game/server/weapon.h>

class CWeaponHammer : public IWeapon
{
    int m_DamageInfect;
public:
    CWeaponHammer(int Owner);
    const char* GetName() override;
    bool Fire(class CGameWorld *pGameworld, vec2 Pos, vec2 Direction, vec2 Pos2) override;
};

#endif // GAME_SERVER_WEAPONS_HAMMER_H