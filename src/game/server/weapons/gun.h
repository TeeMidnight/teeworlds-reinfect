#ifndef GAME_SERVER_WEAPONS_GUN_H
#define GAME_SERVER_WEAPONS_GUN_H

#include <game/server/weapon.h>

class CWeaponGun : public IWeapon
{
public:
    CWeaponGun(int Owner);
    const char* GetName() override;
    bool Fire(class CGameWorld *pGameworld, vec2 Pos, vec2 Direction, vec2 Pos2) override;
};

#endif // GAME_SERVER_WEAPONS_GUN_H