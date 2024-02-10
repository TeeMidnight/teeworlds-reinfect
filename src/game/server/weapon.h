#ifndef GAME_SERVER_WEAPON_H
#define GAME_SERVER_WEAPON_H

#include <base/vmath.h>

class IWeapon
{
    // in ms
    int m_ReloadTimer;
    int m_FireDelay;
    int m_Maxammo;
    int m_Damage;
    int m_Owner;
    int m_Type;
    int m_Ammo;
protected:
    // in ms
    void SetFireDelay(int FireDelay) { m_FireDelay = FireDelay; } 
    void SetMaxammo(int Maxammo) { m_Maxammo = Maxammo; }
    void SetDamage(int Damage) { m_Damage = Damage; }
    void SetType(int Type) { m_Type = Type; }

public:
    IWeapon(int Owner) 
    { 
        m_Owner = Owner; 
        m_ReloadTimer = 0;
    }
    virtual ~IWeapon() = default;

    // public
    void SetReloadTimer(int ReloadTimer) { m_ReloadTimer = ReloadTimer; } 
    void SetAmmo(int Ammo) { m_Ammo = Ammo; }

    int GetReloadTimer() const { return m_ReloadTimer; }
    // in ms
    int GetFireDelay() const { return m_FireDelay; }
    int GetMaxammo() const { return m_Maxammo; }
    int GetDamage() const { return m_Damage; }
    int GetOwner() const { return m_Owner; }
    int GetType() const { return m_Type; }
    int GetAmmo() const { return m_Ammo; }

    virtual const char* GetName() = 0;
    // return true = fired, Pos2 = m_Pos
    virtual bool Fire(class CGameWorld *pGameworld, vec2 Pos, vec2 Direction, vec2 Pos2) = 0;
    // for tickly weapons like ninja
    virtual void Tick(class CGameWorld *pGameworld) { if(m_ReloadTimer) m_ReloadTimer--; }
};

#endif // GAME_SERVER_WEAPON_H