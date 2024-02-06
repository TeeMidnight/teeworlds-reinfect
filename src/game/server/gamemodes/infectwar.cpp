#include <engine/shared/protocol.h>

#include <game/localization.h>

#include <game/server/entities/character.h>

#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/teeinfo.h>

#include <vector>

#include "infectwar.h"

static CTeeInfo s_InfectInfo;

static int HSLA_to_int(int H, int S, int L, int Alpha = 255)
{
	int color = 0;
	color = (color & 0xFF00FFFF) | (H << 16);
	color = (color & 0xFFFF00FF) | (S << 8);
	color = (color & 0xFFFFFF00) | L;
	color = (color & 0x00FFFFFF) | (Alpha << 24);
	return color;
}

CGameControllerInfectWar::CGameControllerInfectWar(CGameContext *pGameServer) :
    IGameController(pGameServer)
{
    m_pGameType = "InfectWar";
    m_GameFlags = 0;
    
    str_copy(s_InfectInfo.m_apSkinPartNames[SKINPART_BODY], "standard", sizeof(s_InfectInfo.m_apSkinPartNames[SKINPART_BODY]));
	str_copy(s_InfectInfo.m_apSkinPartNames[SKINPART_MARKING], "cammostripes", sizeof(s_InfectInfo.m_apSkinPartNames[SKINPART_MARKING]));
	s_InfectInfo.m_apSkinPartNames[SKINPART_DECORATION][0] = 0;
	str_copy(s_InfectInfo.m_apSkinPartNames[SKINPART_HANDS], "standard", sizeof(s_InfectInfo.m_apSkinPartNames[SKINPART_HANDS]));
	str_copy(s_InfectInfo.m_apSkinPartNames[SKINPART_FEET], "standard", sizeof(s_InfectInfo.m_apSkinPartNames[SKINPART_FEET]));
	str_copy(s_InfectInfo.m_apSkinPartNames[SKINPART_EYES], "standard", sizeof(s_InfectInfo.m_apSkinPartNames[SKINPART_EYES]));

	s_InfectInfo.m_aUseCustomColors[SKINPART_BODY] = true;
	s_InfectInfo.m_aUseCustomColors[SKINPART_MARKING] = true;
	s_InfectInfo.m_aUseCustomColors[SKINPART_DECORATION] = false;
	s_InfectInfo.m_aUseCustomColors[SKINPART_HANDS] = true;
	s_InfectInfo.m_aUseCustomColors[SKINPART_FEET] = true;
	s_InfectInfo.m_aUseCustomColors[SKINPART_EYES] = false;

	s_InfectInfo.m_aSkinPartColors[SKINPART_BODY] = HSLA_to_int(58, 255, 40);
	s_InfectInfo.m_aSkinPartColors[SKINPART_MARKING] = HSLA_to_int(58, 255, 10);
	s_InfectInfo.m_aSkinPartColors[SKINPART_DECORATION] = HSLA_to_int(0, 0, 0);
	s_InfectInfo.m_aSkinPartColors[SKINPART_HANDS] = HSLA_to_int(58, 255, 40);
	s_InfectInfo.m_aSkinPartColors[SKINPART_FEET] = HSLA_to_int(0, 60, 0);
	s_InfectInfo.m_aSkinPartColors[SKINPART_EYES] = HSLA_to_int(0, 0, 0);

	str_copy(s_InfectInfo.m_aSkinName, "cammostripes", sizeof(s_InfectInfo.m_aSkinName));
	s_InfectInfo.m_UseCustomColor = true;
	s_InfectInfo.m_ColorBody = HSLA_to_int(58, 255, 0);
	s_InfectInfo.m_ColorFeet = HSLA_to_int(0, 255, 134);
}

void CGameControllerInfectWar::InfectPlayer(int ClientID, bool Chat)
{
	if(m_Infects[ClientID])
		return;
	if(!Server()->ClientIngame(ClientID))
		return;
	if(!GameServer()->m_apPlayers[ClientID])
		return;
	if(GameServer()->m_apPlayers[ClientID]->GetTeam() == TEAM_SPECTATORS)
		return;

	m_Infects[ClientID] = true;

	if(GameServer()->GetPlayerChar(ClientID))
		GameServer()->m_apPlayers[ClientID]->KillCharacter();

	bool Comp = GameServer()->m_apPlayers[ClientID]->m_TeeInfos == s_InfectInfo;
	if(!Comp)
	{
		GameServer()->m_apPlayers[ClientID]->m_TeeInfos = s_InfectInfo;
		// update all clients
		for(int j = 0; j < MAX_CLIENTS; ++ j)
		{
			if(!GameServer()->m_apPlayers[j] || 
				(!Server()->ClientIngame(j) && !GameServer()->m_apPlayers[j]->IsDummy()) || 
					Server()->GetClientVersion(j) < CGameContext::MIN_SKINCHANGE_CLIENTVERSION)
				continue;

			GameServer()->SendSkinChange(ClientID, j);
		}
	}

	if(Chat)
	{
		for(int i = 0; i < MAX_CLIENTS; i ++)
		{
			if(!GameServer()->m_apPlayers[i])
				continue;
			char aChat[128];
			str_format(aChat, sizeof(aChat), Localize(Server()->GetClientLanguage(i), "'%s' has been a infect!"), Server()->ClientName(ClientID));
			GameServer()->SendChat(-1, CHAT_WHISPER, i, aChat);
		}
	}
}

void CGameControllerInfectWar::ChooseInfects()
{
	int Infects = 0, Players = 0;
	std::vector<int> vHumansID;
	for(int i = 0; i < MAX_CLIENTS; i ++)
	{
		if(!GameServer()->m_apPlayers[i])
			continue;
		if(GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS)
			continue;

		if(m_Infects[i])
			Infects ++;
		else
			vHumansID.push_back(i);
		Players ++;
	}
	int NeedInfects = m_NumNeedInfects;
	if(m_NumNeedInfects == 0)
	{
		if(Players < 10)
			NeedInfects = maximum(1, Players / 4);
		else if(Players < 20)
			NeedInfects = Players / 5;
		else if(Players < 30)
			NeedInfects = Players / 6;
		else if(Players < 40)
			NeedInfects = Players / 7;
		else if(Players < 50)
			NeedInfects = Players / 8;
		else
			NeedInfects = Players / 9;
		m_NumNeedInfects = NeedInfects;
	}
	
	NeedInfects -= Infects;

	for(int i = 0; i < NeedInfects; i ++)
	{
		int ID = random_int() % (int) vHumansID.size();
		InfectPlayer(vHumansID[ID], true);
		vHumansID.erase(vHumansID.begin() + ID);
	}
}

bool CGameControllerInfectWar::DoWincheckMatch()
{
	// check for time based win
	if((m_GameInfo.m_TimeLimit > 0 && (Server()->Tick()-m_GameStartTick) >= m_GameInfo.m_TimeLimit*Server()->TickSpeed()*60))
	{
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
			{
                if(!m_Infects[i])
                    GameServer()->m_apPlayers[i]->m_Score += 5;
                
                GameServer()->SendChat(-1, CHAT_WHISPER, i, Localize(Server()->GetClientLanguage(i), "We're safe... Temporarily"));
            }
		}

		EndMatch();
        return true;
	}
	else
	{
		// check for infect win
		int AlivePlayerCount = 0;
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS && !m_Infects[i])
			{
				++AlivePlayerCount;
			}
		}

		if(AlivePlayerCount == 0)		// infect win
        {
            for(int i = 0; i < MAX_CLIENTS; i ++)
            {
                if(GameServer()->m_apPlayers[i])
                {
                    GameServer()->SendChat(-1, CHAT_WHISPER, i, Localize(Server()->GetClientLanguage(i), "Anytime, it's zombies time..."));
                }
            }
			EndMatch();
            return true;
        }
    }

	return false;
}

bool CGameControllerInfectWar::IsFriendlyFire(int ClientID1, int ClientID2) const
{
	if(ClientID1 < 0 || ClientID1 >= MAX_CLIENTS)
		return false;
	if(ClientID2 < 0 || ClientID2 >= MAX_CLIENTS)
		return false;
	if(m_Infects[ClientID1] != m_Infects[ClientID2])
		return false;

    return true;
}

// event
int CGameControllerInfectWar::OnCharacterDeath(CCharacter *pVictim, CPlayer *pKiller, int Weapon)
{
	// do scoreing
	if(!pKiller || Weapon == WEAPON_GAME)
		return 0;
	if(pKiller == pVictim->GetPlayer())
		pVictim->GetPlayer()->m_Score--; // suicide or world
	else
	{
		if(m_Infects[pVictim->GetPlayer()->GetCID()] == m_Infects[pKiller->GetCID()])
			pKiller->m_Score--; // teamkill
		else
			pKiller->m_Score += m_Infects[pKiller->GetCID()] ? 3 : 1; // normal kill
	}

	if(Weapon == WEAPON_SELF)
		pVictim->GetPlayer()->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()*3.0f;

	if(!m_Infects[pVictim->GetPlayer()->GetCID()])
	{
		InfectPlayer(pVictim->GetPlayer()->GetCID());
		return 1; // infect
	}

	return 0;
}

void CGameControllerInfectWar::OnPlayerInfoChange(CPlayer *pPlayer)
{
	const int aTeamColors[2] = {65387, 10223467}; // Only for 0.6, We don't need care 0.7
	if(IsTeamplay())
	{
		pPlayer->m_TeeInfos.m_UseCustomColor = 1;
		if(pPlayer->GetTeam() >= TEAM_RED && pPlayer->GetTeam() <= TEAM_BLUE)
		{
			pPlayer->m_TeeInfos.m_ColorBody = aTeamColors[pPlayer->GetTeam()];
			pPlayer->m_TeeInfos.m_ColorFeet = aTeamColors[pPlayer->GetTeam()];
		}
		else
		{
			pPlayer->m_TeeInfos.m_ColorBody = 12895054;
			pPlayer->m_TeeInfos.m_ColorFeet = 12895054;
		}
	}
}

void CGameControllerInfectWar::OnCharacterSpawn(CCharacter *pChr)
{
	// default health
	pChr->IncreaseHealth(10);

	// give default weapons
	if(!m_Infects[pChr->GetPlayer()->GetCID()])
	{
		pChr->GiveWeapon(WEAPON_GUN, 5); // give a full gun
		pChr->GiveWeapon(WEAPON_SHOTGUN, 1); // give a shotgun, but only 1 fire
		pChr->SetWeapon(WEAPON_SHOTGUN);
	}else
	{
		pChr->GiveWeapon(WEAPON_HAMMER, -1);
		pChr->SetWeapon(WEAPON_HAMMER);
	}
}

void CGameControllerInfectWar::OnPlayerConnect(CPlayer *pPlayer)
{
	pPlayer->m_TempInfos = pPlayer->m_TeeInfos;

	if(m_GameStartTick + Server()->TickSpeed() * TIMER_END < Server()->Tick())
		InfectPlayer(pPlayer->GetCID());
	
	IGameController::OnPlayerConnect(pPlayer);
}

void CGameControllerInfectWar::Snap(int SnappingClient)
{
	CNetObj_GameData GameData;

	GameData.m_GameStartTick = m_GameStartTick;
	GameData.m_GameStateFlags = 0;
	GameData.m_GameStateEndTick = 0; // no timer/infinite = 0, on end = GameEndTick, otherwise = GameStateEndTick
	switch(m_GameState)
	{
	case IGS_WARMUP_GAME:
	case IGS_WARMUP_USER:
		GameData.m_GameStateFlags |= GAMESTATEFLAG_WARMUP;
		if(m_GameStateTimer != TIMER_INFINITE)
			GameData.m_GameStateEndTick = Server()->Tick()+m_GameStateTimer;
		break;
	case IGS_START_COUNTDOWN:
		GameData.m_GameStateFlags |= GAMESTATEFLAG_STARTCOUNTDOWN|GAMESTATEFLAG_PAUSED;
		if(m_GameStateTimer != TIMER_INFINITE)
			GameData.m_GameStateEndTick = Server()->Tick()+m_GameStateTimer;
		break;
	case IGS_GAME_PAUSED:
		GameData.m_GameStateFlags |= GAMESTATEFLAG_PAUSED;
		if(m_GameStateTimer != TIMER_INFINITE)
			GameData.m_GameStateEndTick = Server()->Tick()+m_GameStateTimer;
		break;
	case IGS_END_ROUND:
		GameData.m_GameStateFlags |= GAMESTATEFLAG_ROUNDOVER;
		GameData.m_GameStateEndTick = Server()->Tick()-m_GameStartTick-TIMER_END/2*Server()->TickSpeed()+m_GameStateTimer;
		break;
	case IGS_END_MATCH:
		GameData.m_GameStateFlags |= GAMESTATEFLAG_GAMEOVER;
		GameData.m_GameStateEndTick = Server()->Tick()-m_GameStartTick-TIMER_END*Server()->TickSpeed()+m_GameStateTimer;
		break;
	case IGS_GAME_RUNNING:
		// not effected
		break;
	}
	if(m_SuddenDeath)
		GameData.m_GameStateFlags |= GAMESTATEFLAG_SUDDENDEATH;

	int TempFlag = m_GameFlags;
	if(Server()->ClientProtocol(SnappingClient) == NETPROTOCOL_SIX)
	{
		m_GameFlags = 0;
	}

	if(!NetConverter()->SnapNewItemConvert(&GameData, this, NETOBJTYPE_GAMEDATA, 0, sizeof(CNetObj_GameData), SnappingClient))
		return;

	m_GameFlags = TempFlag;

	// demo recording
	if(SnappingClient == -1)
	{
		CNetObj_De_GameInfo *pGameInfo = static_cast<CNetObj_De_GameInfo *>(Server()->SnapNewItem(NETOBJTYPE_DE_GAMEINFO, 0, sizeof(CNetObj_De_GameInfo)));
		if(!pGameInfo)
			return;

		pGameInfo->m_GameFlags = m_GameFlags;
		pGameInfo->m_ScoreLimit = m_GameInfo.m_ScoreLimit;
		pGameInfo->m_TimeLimit = m_GameInfo.m_TimeLimit;
		pGameInfo->m_MatchNum = m_GameInfo.m_MatchNum;
		pGameInfo->m_MatchCurrent = m_GameInfo.m_MatchCurrent;
	}
}

void CGameControllerInfectWar::ResetGame()
{
	// reset the game
	GameServer()->m_World.m_ResetRequested = true;

	SetGameState(IGS_GAME_RUNNING);
	m_GameStartTick = Server()->Tick();
	m_SuddenDeath = 0;
    m_NumNeedInfects = 0;

	CheckGameInfo();

	for(int i = 0; i < MAX_CLIENTS; i ++)
	{
		m_Infects[i] = false;
	}

	for(int i = 0; i < MAX_CLIENTS; i ++)
	{
		if(GameServer()->m_apPlayers[i])
		{
			if(GameServer()->m_apPlayers[i]->m_TeeInfos == GameServer()->m_apPlayers[i]->m_TempInfos)
				continue;

			GameServer()->m_apPlayers[i]->m_TeeInfos = GameServer()->m_apPlayers[i]->m_TempInfos;
			// update all clients
			for(int j = 0; j < MAX_CLIENTS; ++ j)
			{
				if(!GameServer()->m_apPlayers[j] || 
					(!Server()->ClientIngame(j) && !GameServer()->m_apPlayers[j]->IsDummy()) || 
						Server()->GetClientVersion(j) < CGameContext::MIN_SKINCHANGE_CLIENTVERSION)
					continue;

				GameServer()->SendSkinChange(i, j);
			}
		}
	}
}

void CGameControllerInfectWar::Tick()
{
    if(IsGameRunning())
    {
		if(m_GameStartTick + Server()->TickSpeed() * TIMER_END < Server()->Tick())
		{
			ChooseInfects();
		}
	}

    IGameController::Tick();
}