#ifndef GAME_SERVER_WEAPONS_NINJA_H
#define GAME_SERVER_WEAPONS_NINJA_H

#include <game/server/weapon.h>

class CWeaponNinja : public IWeapon
{
public:
    CWeaponNinja(int Owner);
    const char* GetName() override;
    bool Fire(class CGameWorld *pGameworld, vec2 Pos, vec2 Direction, vec2 Pos2) { return true; }
};

#endif // GAME_SERVER_WEAPONS_NINJA_H