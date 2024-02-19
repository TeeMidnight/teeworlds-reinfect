#ifndef GAME_SERVER_GAMEMODES_REINFECT_H
#define GAME_SERVER_GAMEMODES_REINFECT_H

#include <game/server/gamecontroller.h>

#include <vector>
#include <map>

#define LASTDOOR_INDEX -1

class CGameControllerReinfect : public IGameController
{
    struct CDoorPos
    {
        vec2 m_Pos;
        int m_Index;
        int m_SnapID;
    };
    bool m_Infects[MAX_CLIENTS];
    bool m_Finishes[MAX_CLIENTS];
    int m_NumNeedInfects;

    std::map<int, std::vector<vec2>> m_DoorsPoses;
    std::map<int, std::vector<CDoorPos>> m_DoorsPosesInfo;
    std::map<int, int> m_DoorsOpenTimer;

    void DoPlayerFinish(int ClientID);

    void DoDoorSwitch(const CSwitchTile *pDoorSwitch, bool IsEnd);
    void InitDoors();
    void OnTiles();
public:
    CGameControllerReinfect(CGameContext *pGameServer);
    ~CGameControllerReinfect();
    
    bool IsFinish(int ClientID) const;
    bool IsInfect(int ClientID) const;
    bool IsInfection() const;

    void InfectPlayer(int ClientID, bool Chat = false);
    void ChooseInfects();

    bool DoWincheckMatch() override;
    bool OnEntity(int Index, vec2 Pos) override;
    bool PlayerPickable(int ClientID) override;
    bool IsFriendlyFire(int ClientID1, int ClientID2) const override;
    int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);

    void OnPlayerInfoChange(class CPlayer *pPlayer) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
	void OnPlayerConnect(class CPlayer *pPlayer) override;
    void ResetGame() override;
    void Snap(int SnappingClient) override;
    void Tick() override;

    void OnMapInit(int Width, int Height) override;
};

#endif // GAME_SERVER_GAMEMODES_REINFECT_H