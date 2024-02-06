#ifndef GAME_SERVER_GAMEMODES_INFECTWAR_H
#define GAME_SERVER_GAMEMODES_INFECTWAR_H

#include <game/server/gamecontroller.h>

class CGameControllerInfectWar : public IGameController
{
    bool m_Infects[MAX_CLIENTS];
    int m_NumNeedInfects;
public:
    CGameControllerInfectWar(CGameContext *pGameServer);

    void InfectPlayer(int ClientID, bool Chat = false);
    void ChooseInfects();

    bool DoWincheckMatch() override;
    bool IsFriendlyFire(int ClientID1, int ClientID2) const override;
    int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);

    void OnPlayerInfoChange(CPlayer *pPlayer) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
	void OnPlayerConnect(class CPlayer *pPlayer) override;
    void Snap(int SnappingClient) override;
    void ResetGame() override;
    void Tick() override;
};

#endif // GAME_SERVER_GAMEMODES_INFECTWAR_H