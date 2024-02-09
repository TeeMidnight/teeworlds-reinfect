#ifndef GAME_SERVER_GAMEMODES_REINFECT_H
#define GAME_SERVER_GAMEMODES_REINFECT_H

#include <game/server/gamecontroller.h>

class CGameControllerReinfect : public IGameController
{
    bool m_Infects[MAX_CLIENTS];
    int m_NumNeedInfects;
public:
    CGameControllerReinfect(CGameContext *pGameServer);
    
    bool IsInfect(int ClientID) const;
    bool IsInfection() const;

    void InfectPlayer(int ClientID, bool Chat = false);
    void ChooseInfects();

    bool DoWincheckMatch() override;
    bool OnEntity(int Index, vec2 Pos) override;
    bool PlayerPickable(int ClientID) override;
    bool IsFriendlyFire(int ClientID1, int ClientID2) const override;
    int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);

	void OnCharacterSpawn(class CCharacter *pChr) override;
	void OnPlayerConnect(class CPlayer *pPlayer) override;
    void Snap(int SnappingClient) override;
    void ResetGame() override;
    void Tick() override;
};

#endif // GAME_SERVER_GAMEMODES_REINFECT_H