#include <engine/shared/config.h>
#include <engine/shared/protocol.h>

#include <game/localization.h>

#include <game/server/entities/character.h>
#include <game/server/entities/pickup.h>

#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/teeinfo.h>

#include <vector>

#include "reinfect.h"

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

CGameControllerReinfect::CGameControllerReinfect(CGameContext *pGameServer) :
    IGameController(pGameServer)
{
    m_pGameType = "Reinfect";
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

	mem_zero(m_Infects, sizeof(m_Infects));
}

bool CGameControllerReinfect::IsInfect(int ClientID) const
{
	if(ClientID < 0 || ClientID > MAX_CLIENTS)
		return false;
	return m_Infects[ClientID];
}

bool CGameControllerReinfect::IsInfection() const 
{
	return m_GameStartTick + Server()->TickSpeed() * TIMER_END * 1.5f <= Server()->Tick();
}

void CGameControllerReinfect::InfectPlayer(int ClientID, bool Chat)
{
	if(!Server()->ClientIngame(ClientID))
		return;
	if(!GameServer()->m_apPlayers[ClientID])
		return;
	if(GameServer()->m_apPlayers[ClientID]->GetTeam() == TEAM_SPECTATORS)
		return;
	if(IsInfect(ClientID))
		return;

	m_Infects[ClientID] = true;

	if(GameServer()->GetPlayerChar(ClientID))
		GameServer()->m_apPlayers[ClientID]->KillCharacter();

	bool Comp = GameServer()->m_apPlayers[ClientID]->m_TeeInfos == s_InfectInfo;
	if(!Comp)
	{
		GameServer()->UpdatePlayerSkin(ClientID, s_InfectInfo);
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

void CGameControllerReinfect::ChooseInfects()
{
	int Infects = 0, Players = 0;
	std::vector<int> vHumansID;
	for(int i = 0; i < MAX_CLIENTS; i ++)
	{
		if(!GameServer()->m_apPlayers[i])
			continue;
		if(GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS)
			continue;

		if(IsInfect(i))
		{
			Infects ++; // infect
		}
		else if(!GameServer()->m_apPlayers[i]->GetCharacter())
		{
			InfectPlayer(i); // not spawn, infect
			Infects ++;
		}
		else 
		{
			vHumansID.push_back(i); // humans
		}
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

bool CGameControllerReinfect::DoWincheckMatch()
{
	// check for time based win
	if((m_GameInfo.m_TimeLimit > 0 && (Server()->Tick()-m_GameStartTick) >= m_GameInfo.m_TimeLimit*Server()->TickSpeed()*60))
	{
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
			{
                if(!IsInfect(i))
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
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS && !IsInfect(i))
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

bool CGameControllerReinfect::OnEntity(int Index, vec2 Pos)
{
	int Type = -1;
	switch(Index)
	{
	case ENTITY_SPAWN:
		m_aaSpawnPoints[0][m_aNumSpawnPoints[0]++] = Pos;
		break;
	case ENTITY_SPAWN_RED:
		m_aaSpawnPoints[1][m_aNumSpawnPoints[1]++] = Pos;
		break;
	case ENTITY_SPAWN_BLUE:
		m_aaSpawnPoints[2][m_aNumSpawnPoints[2]++] = Pos;
		break;
	case ENTITY_ARMOR_1:
		Type = PICKUP_ARMOR;
		break;
	case ENTITY_HEALTH_1:
		Type = PICKUP_HEALTH;
		break;
	case ENTITY_WEAPON_SHOTGUN:
		Type = PICKUP_SHOTGUN;
		break;
	case ENTITY_WEAPON_GRENADE:
		Type = PICKUP_GRENADE;
		break;
	case ENTITY_WEAPON_LASER:
		Type = PICKUP_LASER;
		break;
	case ENTITY_WEAPON_GUN:
		Type = PICKUP_GUN;
		break;
	case ENTITY_POWERUP_NINJA:
		if(Config()->m_SvPowerups)
			Type = PICKUP_NINJA;
	}

	if(Type != -1)
	{
		CPickup *pPickup = new CPickup(&GameServer()->m_World, Type, Pos);
		pPickup->SetOnetime(true);
		return true;
	}

	return false;
}

bool CGameControllerReinfect::PlayerPickable(int ClientID)
{
	return !m_Infects[ClientID];
}

bool CGameControllerReinfect::IsFriendlyFire(int ClientID1, int ClientID2) const
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
int CGameControllerReinfect::OnCharacterDeath(CCharacter *pVictim, CPlayer *pKiller, int Weapon)
{
	// do scoreing
	if(!pKiller || Weapon == WEAPON_GAME)
		return 0;
	if(pKiller != pVictim->GetPlayer())
	{
		if(IsInfect(pVictim->GetPlayer()->GetCID()) == IsInfect(pKiller->GetCID()))
			pKiller->m_Score--; // teamkill
		else
			pKiller->m_Score += IsInfect(pKiller->GetCID()) ? 3 : 1; // normal kill
	}

	if(Weapon == WEAPON_SELF)
		pVictim->GetPlayer()->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()*3.0f;

	if(!IsInfect(pVictim->GetPlayer()->GetCID()) && IsInfection())
	{
		InfectPlayer(pVictim->GetPlayer()->GetCID());
		return 1; // infect
	}

	return 0;
}

void CGameControllerReinfect::OnPlayerInfoChange(CPlayer *pPlayer)
{
	if(!IsInfect(pPlayer->GetCID()) && pPlayer->m_IsReadyToEnter)
	{
		GameServer()->UpdatePlayerSkin(pPlayer->GetCID(), pPlayer->m_TempInfos);
	}
}

void CGameControllerReinfect::OnCharacterSpawn(CCharacter *pChr)
{
	// default health
	pChr->IncreaseHealth(10);

	// give default weapons
	if(!IsInfect(pChr->GetPlayer()->GetCID()))
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

void CGameControllerReinfect::OnPlayerConnect(CPlayer *pPlayer)
{
	pPlayer->m_TempInfos = pPlayer->m_TeeInfos;

	if(IsInfection())
		InfectPlayer(pPlayer->GetCID());
	
	IGameController::OnPlayerConnect(pPlayer);
}

void CGameControllerReinfect::ResetGame()
{
	// reset the game
	GameServer()->m_World.m_ResetRequested = true;

	SetGameState(IGS_GAME_RUNNING);
	m_GameStartTick = Server()->Tick();
	m_SuddenDeath = 0;
    m_NumNeedInfects = 0;

	CheckGameInfo();

	mem_zero(m_Infects, sizeof(m_Infects));

	for(int i = 0; i < MAX_CLIENTS; i ++)
	{
		if(GameServer()->m_apPlayers[i])
		{
			GameServer()->UpdatePlayerSkin(i, GameServer()->m_apPlayers[i]->m_TempInfos);
		}
	}
}

void CGameControllerReinfect::Tick()
{
    if(IsGameRunning())
    {
		if(IsInfection())
		{
			ChooseInfects();
		}
	}

    IGameController::Tick();
}