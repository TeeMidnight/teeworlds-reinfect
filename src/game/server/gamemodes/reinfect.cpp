#include <engine/shared/config.h>
#include <engine/shared/protocol.h>

#include <game/server/localization.h>

#include <game/server/entities/character.h>
#include <game/server/entities/pickup.h>

#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/teeinfo.h>

#include <game/server/weapons/vanilla.h>

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

static inline void AppendDecimals(char *pBuf, int Size, int Time, int Precision)
{
	if(Precision > 0)
	{
		char aInvalid[] = ".---";
		char aMSec[] = {
			'.',
			(char)('0' + (Time / 100) % 10),
			(char)('0' + (Time / 10) % 10),
			(char)('0' + Time % 10),
			0
		};
		char *pDecimals = Time < 0 ? aInvalid : aMSec;
		pDecimals[minimum(Precision, 3)+1] = 0;
		str_append(pBuf, pDecimals, Size);
	}
}

static void FormatTime(char *pBuf, int Size, int Time, int Precision)
{
	if(Time < 0)
		str_copy(pBuf, "-:--", Size);
	else
		str_format(pBuf, Size, "%02d:%02d", Time / (60 * 1000), (Time / 1000) % 60);
	AppendDecimals(pBuf, Size, Time, Precision);
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
	mem_zero(m_Finishes, sizeof(m_Finishes));

	m_DoorsPoses.clear();
	m_DoorsPosesInfo.clear();
	m_DoorsOpenTimer.clear();
}

CGameControllerReinfect::~CGameControllerReinfect()
{
	for(auto& DoorPoses : m_DoorsPosesInfo)
	{
		for(auto& DoorPos : DoorPoses.second)
		{
			Server()->SnapFreeID(DoorPos.m_SnapID); // free id
		}
	}
	m_DoorsPosesInfo.clear();
	m_DoorsOpenTimer.clear();
}

bool CGameControllerReinfect::IsFinish(int ClientID) const
{
	if(ClientID < 0 || ClientID > MAX_CLIENTS)
		return false;
	return m_Finishes[ClientID];
}

bool CGameControllerReinfect::IsInfect(int ClientID) const
{
	if(ClientID < 0 || ClientID > MAX_CLIENTS)
		return false;
	return m_Infects[ClientID];
}

bool CGameControllerReinfect::IsInfection() const 
{
	return (m_GameStartTick + Server()->TickSpeed() * TIMER_END * 1.5f <= Server()->Tick()) && IsGameRunning();
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

// Must put this in CGameControllerReinfect::Tick!!!!
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

	if(vHumansID.size() == 0)
	{
		// do a return if there isn't any humans
		return;
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
	// check for infect win
	int AlivePlayerCount = 0;
	int FinishPlayerCount = 0;
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS && !IsInfect(i))
		{
			++AlivePlayerCount;
			if(IsFinish(i))
				++FinishPlayerCount;
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
	else if(AlivePlayerCount == FinishPlayerCount)
	{
		for(int i = 0; i < MAX_CLIENTS; i ++)
		{
			if(GameServer()->m_apPlayers[i])
			{
				GameServer()->SendChat(-1, CHAT_WHISPER, i, Localize(Server()->GetClientLanguage(i), "We're safe... Temporarily"));

				char aBuf[128];
				char aTime[32];
				int Time = (int)(((Server()->Tick() - m_GameStartTick) / (float)(Server()->TickSpeed())) * 1000.f);
           		FormatTime(aTime, sizeof(aTime), Time, 3);
				str_format(aBuf, sizeof(aBuf), Localize(Server()->GetClientLanguage(i), "This round finishes in: %s"), aTime);
				GameServer()->SendChat(-1, CHAT_WHISPER, i, aBuf);
			}
		}
		EndMatch();
		return true;
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
	if(IsInfect(pChr->GetPlayer()->GetCID()))
		pChr->SetMaxHealth(10);
	// default health
	pChr->IncreaseHealth(pChr->GetMaxHealth());

	// give default weapons
	if(!IsInfect(pChr->GetPlayer()->GetCID()))
	{
		pChr->GiveWeapon(WEAPON_GUN, new CWeaponGun(pChr->GetPlayer()->GetCID())); // give a full gun
		IWeapon *pShotgun = new CWeaponShotgun(pChr->GetPlayer()->GetCID());
		pShotgun->SetAmmo(1);
		pChr->GiveWeapon(WEAPON_SHOTGUN, pShotgun); // give a shotgun, but only 1 fire
	}
	
	pChr->GiveWeapon(WEAPON_HAMMER, new CWeaponHammer(pChr->GetPlayer()->GetCID())); // give a hammer
	pChr->SetWeapon(WEAPON_HAMMER);
}

void CGameControllerReinfect::OnPlayerConnect(CPlayer *pPlayer)
{
	pPlayer->m_TempInfos = pPlayer->m_TeeInfos;

	if(IsInfection())
		InfectPlayer(pPlayer->GetCID());
	
	IGameController::OnPlayerConnect(pPlayer);
}

void CGameControllerReinfect::InitDoors()
{
	for(auto& DoorPoses : m_DoorsPosesInfo)
	{
		for(auto& DoorPos : DoorPoses.second)
		{
			Server()->SnapFreeID(DoorPos.m_SnapID); // free id
		}
	}
	m_DoorsPosesInfo.clear();
	m_DoorsOpenTimer.clear();

	for(auto& DoorPoses : m_DoorsPoses)
	{
		for(auto& Pos : DoorPoses.second)
		{
			if(!m_DoorsPosesInfo.count(DoorPoses.first))
				m_DoorsPosesInfo[DoorPoses.first] = std::vector<CDoorPos>();
			int Index = GameServer()->Collision()->AddCollisionRect(CCollisionRect{Pos, 
				vec2(32.0f, 32.0f), CCollision::COLFLAG_SOLID|CCollision::COLFLAG_NOHOOK});
			m_DoorsPosesInfo[DoorPoses.first].push_back(CDoorPos{Pos, Index, Server()->SnapNewID()}); // get snap id
		}
	}
}

void CGameControllerReinfect::ResetGame()
{
	// reset the game
	GameServer()->m_World.m_ResetRequested = true;
	GameServer()->Collision()->ClearCollisionRects();

	InitDoors();

	SetGameState(IGS_GAME_RUNNING);
	m_GameStartTick = Server()->Tick();
	m_SuddenDeath = 0;
    m_NumNeedInfects = 0;

	CheckGameInfo();

	mem_zero(m_Infects, sizeof(m_Infects));
	mem_zero(m_Finishes, sizeof(m_Finishes));

	for(int i = 0; i < MAX_CLIENTS; i ++)
	{
		if(GameServer()->m_apPlayers[i])
		{
			GameServer()->UpdatePlayerSkin(i, GameServer()->m_apPlayers[i]->m_TempInfos);
		}
	}
}

void CGameControllerReinfect::DoPlayerFinish(int ClientID)
{
	if(!GameServer()->GetPlayerChar(ClientID))
		return;

	if(IsInfect(ClientID))
		return;
	
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientID];

	pPlayer->m_RespawnDisabled = true;
	pPlayer->m_DeadSpecMode = true;

	pPlayer->KillCharacter();

	CNetMsg_Sv_RaceFinish FinishMsg;
	FinishMsg.m_ClientID = ClientID;
	FinishMsg.m_Time = (int)(((Server()->Tick() - m_GameStartTick) / (float)(Server()->TickSpeed())) * 1000.f);
	FinishMsg.m_Diff = 0;
	FinishMsg.m_RecordPersonal = 0;
	FinishMsg.m_RecordServer = 0;

	Server()->SendPackMsg(&FinishMsg, MSGFLAG_VITAL, -1);
	m_Finishes[ClientID] = true;
}

static const CSwitchTile *GetSwitchTile(const CSwitchTile *pSwitch, vec2 Pos, int Width, int Height)
{
	int Nx = clamp(round_to_int(Pos.x) / 32, 0, Width - 1);
	int Ny = clamp(round_to_int(Pos.y) / 32, 0, Height - 1);
	
	return &pSwitch[Ny * Width + Nx];
}

static const CTile *GetTile(const CTile *pTiles, vec2 Pos, int Width, int Height)
{
	int Nx = clamp(round_to_int(Pos.x) / 32, 0, Width - 1);
	int Ny = clamp(round_to_int(Pos.y) / 32, 0, Height - 1);
	
	return &pTiles[Ny * Width + Nx];
}

static bool IsDoorSwitch(int Type)
{
	return Type == TILE_DOORSWITCH || Type == TILE_LASTSWITCH;
}

void CGameControllerReinfect::DoDoorSwitch(const CSwitchTile *pDoorSwitch, bool IsEnd)
{
	if(!IsDoorSwitch(pDoorSwitch->m_Type))
		return;
	if(!m_DoorsPosesInfo.count(IsEnd ? LASTDOOR_INDEX : pDoorSwitch->m_Number)) // check door is open
		return;
	if(m_DoorsOpenTimer.count(IsEnd ? LASTDOOR_INDEX : pDoorSwitch->m_Number)) // check door is opening
		return;
	
	m_DoorsOpenTimer[IsEnd ? LASTDOOR_INDEX : pDoorSwitch->m_Number] = maximum(0, (int) pDoorSwitch->m_Delay) * Server()->TickSpeed();
	
	if(maximum(0, (int) pDoorSwitch->m_Delay) == 0)
		return; // open with no send

	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
		{
			char aBuf[128];
			if(IsEnd)
				str_format(aBuf, sizeof(aBuf), Localize(Server()->GetClientLanguage(i), "The Last Door will open in %d seconds!"), pDoorSwitch->m_Delay);
			else
				str_format(aBuf, sizeof(aBuf), Localize(Server()->GetClientLanguage(i), "Door %d will open in %d seconds!"), pDoorSwitch->m_Number, pDoorSwitch->m_Delay);
			GameServer()->SendChat(-1, CHAT_WHISPER, i, aBuf);
		}
	}
}

void CGameControllerReinfect::OnTiles()
{
	if(!GameServer()->Collision()->GameLayer())
		return;
	const CTile *pGameTiles = GameServer()->Collision()->GameLayer();
	for(int i = 0; i < MAX_CLIENTS; i ++)
	{
		if(IsFinish(i))
			continue;
		if(IsInfect(i))
			continue;
		if(!GameServer()->GetPlayerChar(i))
			continue; // skip no character player

		CCharacter *pChr = GameServer()->GetPlayerChar(i);
		vec2 CheckPos = pChr->GetPos();
		vec2 Size = vec2(pChr->GetProximityRadius(), pChr->GetProximityRadius());
		Size *= 0.5f;

		int Width = GameServer()->Collision()->GetWidth();
		int Height = GameServer()->Collision()->GetHeight();

		const CTile *pTile = GetTile(pGameTiles, vec2(CheckPos.x - Size.x, CheckPos.y - Size.y), Width, Height);
		if(pTile->m_Index == TILE_FINISH)
		{
			DoPlayerFinish(i);
			continue;
		}

		pTile = GetTile(pGameTiles, vec2(CheckPos.x + Size.x, CheckPos.y - Size.y), Width, Height);
		if(pTile->m_Index == TILE_FINISH)
		{
			DoPlayerFinish(i);
			continue;
		}

		pTile = GetTile(pGameTiles, vec2(CheckPos.x + Size.x, CheckPos.y + Size.y), Width, Height);
		if(pTile->m_Index == TILE_FINISH)
		{
			DoPlayerFinish(i);
			continue;
		}

		pTile = GetTile(pGameTiles, vec2(CheckPos.x - Size.x, CheckPos.y + Size.y), Width, Height);
		if(pTile->m_Index == TILE_FINISH)
		{
			DoPlayerFinish(i);
			continue;
		}
	}
	

	if(m_DoorsPosesInfo.size() == 0)
		return; // all of the doors were opened

	const CSwitchTile *pSwitch = GameServer()->Collision()->SwitchLayer();
	if(!pSwitch)
		return; // some wrong

	for(int i = 0; i < MAX_CLIENTS; i ++)
	{
		if(!GameServer()->GetPlayerChar(i))
			continue; // skip no character player

		CCharacter *pChr = GameServer()->GetPlayerChar(i);
		vec2 CheckPos = pChr->GetPos();
		vec2 Size = vec2(pChr->GetProximityRadius(), pChr->GetProximityRadius());
		Size *= 0.5f;

		int Width = GameServer()->Collision()->GetWidth();
		int Height = GameServer()->Collision()->GetHeight();

		const CSwitchTile *pTile = GetSwitchTile(pSwitch, vec2(CheckPos.x - Size.x, CheckPos.y - Size.y), Width, Height);
		if(IsDoorSwitch(pTile->m_Type))
		{
			DoDoorSwitch(pTile, pTile->m_Type == TILE_LASTSWITCH);
		}

		pTile = GetSwitchTile(pSwitch, vec2(CheckPos.x + Size.x, CheckPos.y - Size.y), Width, Height);
		if(IsDoorSwitch(pTile->m_Type))
		{
			DoDoorSwitch(pTile, pTile->m_Type == TILE_LASTSWITCH);
		}

		pTile = GetSwitchTile(pSwitch, vec2(CheckPos.x + Size.x, CheckPos.y + Size.y), Width, Height);
		if(IsDoorSwitch(pTile->m_Type))
		{
			DoDoorSwitch(pTile, pTile->m_Type == TILE_LASTSWITCH);
		}

		pTile = GetSwitchTile(pSwitch, vec2(CheckPos.x - Size.x, CheckPos.y + Size.y), Width, Height);
		if(IsDoorSwitch(pTile->m_Type))
		{
			DoDoorSwitch(pTile, pTile->m_Type == TILE_LASTSWITCH);
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
		
		OnTiles();
		// doors
		for(auto& OpeningDoor : m_DoorsOpenTimer)
		{
			if(!m_DoorsPosesInfo.count(OpeningDoor.first)) // the door was opened
			{
				continue;
			}

			OpeningDoor.second --; // Timer
			if(OpeningDoor.second <= 0)
			{
				for(auto& PosInfo : m_DoorsPosesInfo[OpeningDoor.first])
				{
					GameServer()->Collision()->RemoveCollisionRect(PosInfo.m_Index);
					Server()->SnapFreeID(PosInfo.m_SnapID); // free id
				}
				m_DoorsPosesInfo.erase(OpeningDoor.first); // remove door
			}
		}
	}

    IGameController::Tick();
}

void CGameControllerReinfect::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);

	if(SnappingClient != -1 && !GameServer()->m_apPlayers[SnappingClient])
	{
		return;
	}
	
	for(auto& DoorPoses : m_DoorsPosesInfo)
	{
		for(auto& DoorPos : DoorPoses.second)
		{
			if(SnappingClient != -1)
			{
				float dx = GameServer()->m_apPlayers[SnappingClient]->m_ViewPos.x-DoorPos.m_Pos.x;
				float dy = GameServer()->m_apPlayers[SnappingClient]->m_ViewPos.y-DoorPos.m_Pos.y;
				if(absolute(dx) > 1000.0f || absolute(dy) > 800.0f)
					continue;
				if(distance(GameServer()->m_apPlayers[SnappingClient]->m_ViewPos, DoorPos.m_Pos) > 1100.0f)
					continue;
			}
			CNetObj_Pickup Pickup;

			Pickup.m_X = round_to_int(DoorPos.m_Pos.x);
			Pickup.m_Y = round_to_int(DoorPos.m_Pos.y);
			Pickup.m_Type = PICKUP_ARMOR;

			if(!NetConverter()->SnapNewItemConvert(&Pickup, this, NETOBJTYPE_PICKUP, DoorPos.m_SnapID, sizeof(CNetObj_Pickup), SnappingClient))
				return;
		}
	}
}

void CGameControllerReinfect::OnMapInit(int Width, int Height)
{
	const CSwitchTile *pSwitch = GameServer()->Collision()->SwitchLayer();
	if(!pSwitch)
		return;
	for(int y = 0; y < Height; y ++)
	{
		for(int x = 0; x < Width; x ++)
		{
			const int Index = y * Width + x;
			const int SwitchType = pSwitch[Index].m_Type;
			if(SwitchType == TILE_DOOR)
			{
				m_DoorsPoses[pSwitch[Index].m_Number].push_back(vec2(x*32.0f+16.0f, y*32.0f+16.0f));
			}
			if(SwitchType == TILE_LASTDOOR)
			{
				m_DoorsPoses[LASTDOOR_INDEX].push_back(vec2(x*32.0f+16.0f, y*32.0f+16.0f));
			}
		}
	}

	for(auto& DoorPoses : m_DoorsPoses)
	{
		for(auto& Pos : DoorPoses.second)
		{
			GameServer()->Collision()->AddCollisionRect(CCollisionRect{Pos, vec2(32.0f, 32.0f), CCollision::COLFLAG_SOLID|CCollision::COLFLAG_NOHOOK});
		}
	}

	InitDoors();
}