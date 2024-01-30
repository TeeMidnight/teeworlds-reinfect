#include <engine/server.h>
#include <engine/message.h>

#include <engine/shared/config.h>
#include <engine/shared/protocol6.h>

#include <game/server/entities/character.h>
#include <game/server/gamecontroller.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include <generated/protocol.h>
#include <generated/protocol6.h>
#include <generated/protocolglue.h>

#include <generated/server_data.h>

#include "netconverter.h"


static int PickupTypeTo6(int Pickup)
{
	switch (Pickup)
	{
	case PICKUP_HEALTH: 
		return protocol6::POWERUP_HEALTH;
	case PICKUP_ARMOR: 
		return protocol6::POWERUP_ARMOR;
	case PICKUP_HAMMER: 
	case PICKUP_GUN: 
	case PICKUP_SHOTGUN: 
	case PICKUP_GRENADE:
	case PICKUP_LASER:  
		return protocol6::POWERUP_WEAPON;
	case PICKUP_NINJA: 
		return protocol6::POWERUP_NINJA;
	}
	return 0;
}

static int PickupSubtypeTo6(int Pickup)
{
	switch (Pickup)
	{
	case PICKUP_HEALTH: 
	case PICKUP_ARMOR: 
		return 0;
	case PICKUP_HAMMER: 
		return WEAPON_HAMMER;
	case PICKUP_GUN: 
		return WEAPON_GUN;
	case PICKUP_SHOTGUN: 
		return WEAPON_SHOTGUN;
	case PICKUP_GRENADE:
		return WEAPON_GRENADE;
	case PICKUP_LASER:  
		return WEAPON_LASER;
	case PICKUP_NINJA: 
		return WEAPON_NINJA;
	}
	return 0;
}

static int PlayerFlags_SevenToSix(int Flags)
{
	int Six = 0;
	if(Flags & PLAYERFLAG_CHATTING)
		Six |= protocol6::PLAYERFLAG_CHATTING;
	if(Flags & PLAYERFLAG_SCOREBOARD)
		Six |= protocol6::PLAYERFLAG_SCOREBOARD;
	return Six;
}

enum
{
	STR_TEAM_GAME,
	STR_TEAM_RED,
	STR_TEAM_BLUE,
	STR_TEAM_SPECTATORS,
};

static int GetStrTeam(int Team, bool Teamplay)
{
	if(Teamplay)
	{
		if(Team == TEAM_RED)
			return STR_TEAM_RED;
		else if(Team == TEAM_BLUE)
			return STR_TEAM_BLUE;
	}
	else if(Team == 0)
		return STR_TEAM_GAME;

	return STR_TEAM_SPECTATORS;
}

CNetConverter::CNetConverter(IServer *pServer, class CConfig *pConfig) :
    m_pServer(pServer),
    m_pConfig(pConfig)
{
    m_GameFlags = 0;
}

bool CNetConverter::DeepSnapConvert6(void *pItem, void *pSnapClass, int Type, int ID, int Size, int ToClientID)
{
    // TODO: MOVE THESE TO THEIR FUNCTION
    // Type is 0.7, so we need switch check
    switch(Type)
    {
        case NETOBJTYPE_PROJECTILE:
        case NETOBJTYPE_LASER:
        case NETOBJTYPE_FLAG:
        case NETEVENTTYPE_EXPLOSION:
        case NETEVENTTYPE_SPAWN:
        case NETEVENTTYPE_HAMMERHIT:
        case NETEVENTTYPE_DEATH:
        case NETEVENTTYPE_SOUNDWORLD:
        {
            void *pObj = Server()->SnapNewItem(Obj_SevenToSix(Type), ID, Size);
            if(!pObj)
                return false;
            mem_copy(pObj, pItem, Size);
            return true;
        }
        case NETOBJTYPE_PICKUP:
        {
            CNetObj_Pickup *pObj7 = (CNetObj_Pickup *) pItem;
            protocol6::CNetObj_Pickup *pObj6 = static_cast<protocol6::CNetObj_Pickup *>(Server()->SnapNewItem(protocol6::NETOBJTYPE_PICKUP, ID, sizeof(protocol6::CNetObj_Pickup)));
            
            if(!pObj6)
                return false;
            pObj6->m_Type = PickupTypeTo6(pObj7->m_Type);
            pObj6->m_Subtype = PickupSubtypeTo6(pObj7->m_Type);
            pObj6->m_X = pObj7->m_X;
            pObj6->m_Y = pObj7->m_Y;

            return true;
        }
        case NETOBJTYPE_GAMEDATA:
        {
            CNetObj_GameData *pObj7 = (CNetObj_GameData *) pItem;
            protocol6::CNetObj_GameInfo *pObj6 = static_cast<protocol6::CNetObj_GameInfo *>(Server()->SnapNewItem(protocol6::NETOBJTYPE_GAMEINFO, ID, sizeof(protocol6::CNetObj_GameInfo)));
            
            if(!pObj6)
                return false;
            pObj6->m_GameFlags = ((IGameController *) pSnapClass)->GameFlags();
            pObj6->m_GameStateFlags = 0;
            if(pObj7->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER || pObj7->m_GameStateFlags&GAMESTATEFLAG_ROUNDOVER)
                pObj6->m_GameStateFlags |= protocol6::GAMESTATEFLAG_GAMEOVER;
            if(pObj7->m_GameStateFlags&GAMESTATEFLAG_SUDDENDEATH)
                pObj6->m_GameStateFlags |= protocol6::GAMESTATEFLAG_SUDDENDEATH;
            if(pObj7->m_GameStateFlags&GAMESTATEFLAG_PAUSED && !(pObj7->m_GameStateFlags&GAMESTATEFLAG_STARTCOUNTDOWN)) // don't pause at count down
                pObj6->m_GameStateFlags |= protocol6::GAMESTATEFLAG_PAUSED;
            pObj6->m_RoundStartTick = pObj7->m_GameStartTick;
            
            pObj6->m_WarmupTimer = 0;
            if((pObj7->m_GameStateFlags&GAMESTATEFLAG_WARMUP || pObj7->m_GameStateFlags&GAMESTATEFLAG_STARTCOUNTDOWN) && pObj7->m_GameStateEndTick)
                pObj6->m_WarmupTimer = pObj7->m_GameStateEndTick - Server()->Tick();
            else if((pObj7->m_GameStateFlags&GAMESTATEFLAG_WARMUP || pObj7->m_GameStateFlags&GAMESTATEFLAG_STARTCOUNTDOWN) && (!pObj7->m_GameStateEndTick))
                pObj6->m_WarmupTimer = Server()->TickSpeed() * 999;

            pObj6->m_ScoreLimit = Config()->m_SvScorelimit;
            pObj6->m_TimeLimit = Config()->m_SvTimelimit;

            pObj6->m_RoundNum = (str_length(Config()->m_SvMaprotation) && Config()->m_SvMatchesPerMap) ? Config()->m_SvMatchesPerMap : 0;
            pObj6->m_RoundCurrent = ((IGameController *) pSnapClass)->MatchCount();

            return true;
        };
        case NETOBJTYPE_GAMEDATAFLAG:
        {
            CNetObj_GameDataFlag *pObj7 = (CNetObj_GameDataFlag *) pItem;
            protocol6::CNetObj_GameData *pObj6 = static_cast<protocol6::CNetObj_GameData *>(Server()->SnapNewItem(protocol6::NETOBJTYPE_GAMEDATA, ID, sizeof(protocol6::CNetObj_GameData)));
            
            if(!pObj6)
                return false;
            pObj6->m_FlagCarrierRed = pObj7->m_FlagCarrierRed;
            pObj6->m_FlagCarrierBlue = pObj7->m_FlagCarrierBlue;
            pObj6->m_TeamscoreRed = ((IGameController *) pSnapClass)->TeamScore(TEAM_RED);
            pObj6->m_TeamscoreBlue = ((IGameController *) pSnapClass)->TeamScore(TEAM_BLUE);

            return true;
        };
        case NETOBJTYPE_CHARACTER: // not game core, beacuse we don't need snap game core
        {
            CNetObj_Character *pObj7 = (CNetObj_Character *) pItem;
            protocol6::CNetObj_Character *pObj6 = static_cast<protocol6::CNetObj_Character *>(Server()->SnapNewItem(protocol6::NETOBJTYPE_CHARACTER, ID, sizeof(protocol6::CNetObj_Character)));
            
            if(!pObj6)
                return false;
            pObj6->m_AmmoCount = pObj7->m_AmmoCount;
            pObj6->m_Angle = pObj7->m_Angle;
            pObj6->m_Armor = pObj7->m_Armor;
            pObj6->m_AttackTick = pObj7->m_AttackTick;
            pObj6->m_Direction = pObj7->m_Direction;
            pObj6->m_Emote = pObj7->m_Emote;
            pObj6->m_Health = pObj7->m_Health;

            // hooks
            pObj6->m_HookDx = pObj7->m_HookDx;
            pObj6->m_HookDy = pObj7->m_HookDy;
            pObj6->m_HookedPlayer = pObj7->m_HookedPlayer;
            pObj6->m_HookState = pObj7->m_HookState;
            pObj6->m_HookTick = pObj7->m_HookTick;
            pObj6->m_HookX = pObj7->m_HookX;
            pObj6->m_HookY = pObj7->m_HookY;

            pObj6->m_Jumped = pObj7->m_Jumped;
            pObj6->m_PlayerFlags = PlayerFlags_SevenToSix(((CCharacter *) pSnapClass)->GetPlayer()->m_PlayerFlags);
            pObj6->m_Tick = pObj7->m_Tick;
            pObj6->m_VelX = pObj7->m_VelX;
            pObj6->m_VelY = pObj7->m_VelY;
            pObj6->m_Weapon = pObj7->m_Weapon;
            pObj6->m_X = pObj7->m_X;
            pObj6->m_Y = pObj7->m_Y;

            return true;
        }
        case NETOBJTYPE_PLAYERINFO:
        {
            protocol6::CNetObj_ClientInfo *pClientInfo = static_cast<protocol6::CNetObj_ClientInfo *>(Server()->SnapNewItem(protocol6::NETOBJTYPE_CLIENTINFO, ID, sizeof(protocol6::CNetObj_ClientInfo)));
            
            int ClientID = ((CPlayer *) pSnapClass)->GetCID();
            CTeeInfo TeeInfos = ((CPlayer *) pSnapClass)->m_TeeInfos;

            if(!pClientInfo)
                return false;
            StrToInts(&pClientInfo->m_Name0, 4, Server()->ClientName(ClientID));
            StrToInts(&pClientInfo->m_Clan0, 3, Server()->ClientClan(ClientID));
            pClientInfo->m_Country = Server()->ClientCountry(ClientID);
            StrToInts(&pClientInfo->m_Skin0, 6, TeeInfos.m_aSkinName);
            pClientInfo->m_UseCustomColor = TeeInfos.m_UseCustomColor;
            pClientInfo->m_ColorBody = TeeInfos.m_ColorBody;
            pClientInfo->m_ColorFeet = TeeInfos.m_ColorFeet;

            CNetObj_PlayerInfo *pObj7 = (CNetObj_PlayerInfo *) pItem;
            protocol6::CNetObj_PlayerInfo *pObj6 = static_cast<protocol6::CNetObj_PlayerInfo *>(Server()->SnapNewItem(protocol6::NETOBJTYPE_PLAYERINFO, ID, sizeof(protocol6::CNetObj_PlayerInfo)));
            
            if(!pObj6)
                return false;

            pObj6->m_ClientID = ClientID;
            pObj6->m_Latency = pObj7->m_Latency;
            pObj6->m_Local = (ClientID == ToClientID) ? 1 : 0;
            pObj6->m_Score = pObj7->m_Score;
            pObj6->m_Team = ((CPlayer *) pSnapClass)->m_DeadSpecMode ? protocol6::TEAM_SPECTATORS : ((CPlayer *) pSnapClass)->GetTeam();

            return true;
        }
        case NETOBJTYPE_SPECTATORINFO:
        {
            CNetObj_SpectatorInfo *pObj7 = (CNetObj_SpectatorInfo *) pItem;
            protocol6::CNetObj_SpectatorInfo *pObj6 = static_cast<protocol6::CNetObj_SpectatorInfo *>(Server()->SnapNewItem(protocol6::NETOBJTYPE_SPECTATORINFO, ID, sizeof(protocol6::CNetObj_SpectatorInfo)));
            
            if(!pObj6)
                return false;
            pObj6->m_SpectatorID = pObj7->m_SpecMode == SPEC_FREEVIEW ? protocol6::SPEC_FREEVIEW : pObj7->m_SpectatorID;
            pObj6->m_X = pObj7->m_X;
            pObj6->m_Y = pObj7->m_Y;

            return true;
        }
        // event
        case NETEVENTTYPE_DAMAGE:
        {
            CNetEvent_Damage *pEvent7 = (CNetEvent_Damage*) pItem;
            protocol6::CNetEvent_DamageInd *pEvent6 = static_cast<protocol6::CNetEvent_DamageInd *>(Server()->SnapNewItem(protocol6::NETEVENTTYPE_DAMAGEIND, ID, sizeof(protocol6::CNetEvent_DamageInd)));

            pEvent6->m_Angle = pEvent7->m_Angle;
            pEvent6->m_X = pEvent7->m_X;
            pEvent6->m_Y = pEvent7->m_Y;
        }
    }
    return false;
}

bool CNetConverter::SnapNewItemConvert(void *pItem, void *pSnapClass, int Type, int ID, int Size, int ToClientID)
{
    m_GameFlags = GameServer()->m_pController->GameFlags();
    
    void *pObj = nullptr; // Snap Obj
    if(Server()->ClientProtocol(ToClientID) == NETPROTOCOL_SEVEN) // do nothing
    {
        pObj = Server()->SnapNewItem(Type, ID, Size);
        if(!pObj)
            return false;
        mem_copy(pObj, pItem, Size);
    }

    if(Server()->ClientProtocol(ToClientID) == NETPROTOCOL_SIX)
    {
        return DeepSnapConvert6(pItem, pSnapClass, Type, ID, Size, ToClientID);
    }

    return true;
}

int CNetConverter::DeepMsgConvert6(CMsgPacker *pMsg, int Flags, int ToClientID)
{
    CUnpacker Unpacker;
    Unpacker.Reset(pMsg->Data(), pMsg->Size());

    if(Unpacker.Error())
        return -1;

    switch (pMsg->Type())
    {
        case NETMSGTYPE_SV_MOTD:
        case NETMSGTYPE_SV_BROADCAST:
        case NETMSGTYPE_SV_KILLMSG:
        case NETMSGTYPE_SV_READYTOENTER:
        case NETMSGTYPE_SV_WEAPONPICKUP:
        case NETMSGTYPE_SV_EMOTICON:
        case NETMSGTYPE_SV_VOTECLEAROPTIONS:
        case NETMSGTYPE_SV_VOTEOPTIONLISTADD:
        case NETMSGTYPE_SV_VOTEOPTIONADD:
        case NETMSGTYPE_SV_VOTEOPTIONREMOVE:
        case NETMSGTYPE_SV_VOTESTATUS:
        {
            CMsgPacker Msg6(Msg_SevenToSix(pMsg->Type()), false, false);
            Msg6.AddRaw(pMsg->Data(), pMsg->Size());

            return Server()->SendMsg(&Msg6, Flags, ToClientID);
        }
        case NETMSGTYPE_SV_CHAT:
        {
            CMsgPacker Msg6(protocol6::NETMSGTYPE_SV_CHAT, false, false);

            int Mode = Unpacker.GetInt(); // Mode
            int ChatterClientID = Unpacker.GetInt();
            int TargetID = Unpacker.GetInt();
            const char* pMessage = Unpacker.GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);

            int Team = 0;
            if(ChatterClientID != -1 && Mode == CHAT_WHISPER)
            {
                char aChat[512];
                str_format(aChat, sizeof(aChat), "(%s)%s: %s", "Whisper", TargetID == ToClientID ? "<" : ">", pMessage);

                Msg6.AddInt(Team); // Team
                Msg6.AddInt(ChatterClientID);
                Msg6.AddString(aChat, -1);
            }
            else
            {
                if(Mode == CHAT_TEAM)
                    Team = 1;
                Msg6.AddInt(Team);
                Msg6.AddInt(ChatterClientID);
                Msg6.AddString(pMessage, -1);
            }

            return Server()->SendMsg(&Msg6, Flags, ToClientID);
        }
        case NETMSGTYPE_SV_TEAM:
        {
            int ClientID = Unpacker.GetInt();
            int Team = Unpacker.GetInt();
            int Silent = Unpacker.GetInt();
            Unpacker.GetInt(); // cool down tick, nothing to do

            if(Silent)
                return 0;
            
            // send chat
	        char aBuf[128];
            switch(GetStrTeam(Team, m_GameFlags&GAMEFLAG_TEAMS))
            {
                case STR_TEAM_GAME: str_format(aBuf, sizeof(aBuf), "'%s' joined the game", Server()->ClientName(ClientID)); break;
                case STR_TEAM_RED: str_format(aBuf, sizeof(aBuf), "'%s' joined the red team", Server()->ClientName(ClientID)); break;
                case STR_TEAM_BLUE: str_format(aBuf, sizeof(aBuf), "'%s' joined the blue team", Server()->ClientName(ClientID)); break;
                case STR_TEAM_SPECTATORS: str_format(aBuf, sizeof(aBuf), "'%s' joined the spectators", Server()->ClientName(ClientID)); break;
            }
            protocol6::CNetMsg_Sv_Chat Chat;
            Chat.m_ClientID = -1;
            Chat.m_pMessage = aBuf;
            Chat.m_Team = 0;

            return Server()->SendPackMsg(&Chat, Flags, ToClientID, false);
        }
        case NETMSGTYPE_SV_TUNEPARAMS:
        {  
            CMsgPacker Msg6(protocol6::NETMSGTYPE_SV_TUNEPARAMS, false, false);

            int Pos = -1;
            do
            {
                Pos ++;
                int Int = Unpacker.GetInt();
                if(!Unpacker.Error())
                    Msg6.AddInt(Int);
                if(Pos == 29)
                    Msg6.AddInt(g_pData->m_Weapons.m_aId[WEAPON_LASER].m_Damage);
            }while(Unpacker.Error());

            return Server()->SendMsg(&Msg6, Flags, ToClientID);
        }
        case NETMSGTYPE_SV_VOTESET:
        {
            int ClientID = Unpacker.GetInt(); // ClientID
            int Type = Unpacker.GetInt(); // Type
            int Timeout = Unpacker.GetInt();
            const char* pDesc = Unpacker.GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
            const char* pReason = Unpacker.GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
            
            if(Timeout)
            {
                if(ClientID != -1)
                {
                    char aBuf[128];
                    switch(Type)
                    {
                        case VOTE_START_OP:
                            {
                                str_format(aBuf, sizeof(aBuf), "'%s' called vote to change server option '%s' (%s)", Server()->ClientName(ClientID), pDesc, pReason);
                                break;
                            }
                        case VOTE_START_KICK:
                            {
                                str_format(aBuf, sizeof(aBuf), "'%s' called for vote to kick '%s' (%s)", Server()->ClientName(ClientID), pDesc, pReason);
                                break;
                            }
                        case VOTE_START_SPEC:
                            {
                                str_format(aBuf, sizeof(aBuf), "'%s' called for vote to move '%s' to spectators (%s)", Server()->ClientName(ClientID), pDesc, pReason);
                            }
                    }

                    protocol6::CNetMsg_Sv_Chat Chat;
                    Chat.m_ClientID = -1;
                    Chat.m_pMessage = aBuf;
                    Chat.m_Team = 0;

                    Server()->SendPackMsg(&Chat, Flags, ToClientID, false);
                }
            }
            else
            {
                char aBuf[128];
                switch(Type)
                {
                    case VOTE_START_OP:
                        str_format(aBuf, sizeof(aBuf), "Admin forced server option '%s' (%s)", pDesc, pReason);
                        break;
                    case VOTE_START_SPEC:
                        str_format(aBuf, sizeof(aBuf), "Admin moved '%s' to spectator (%s)", pDesc, pReason);
                        break;
                    case VOTE_END_ABORT:
                        str_copy(aBuf, "Vote aborted", sizeof(aBuf));
                        break;
                    case VOTE_END_PASS:
                        str_copy(aBuf, ClientID == -1 ? "Admin forced vote yes" : "Vote passed", sizeof(aBuf));
                        break;
                    case VOTE_END_FAIL:
                        str_copy(aBuf, ClientID == -1 ? "Admin forced vote no" : "Vote failed", sizeof(aBuf));
                        break;
                }

                protocol6::CNetMsg_Sv_Chat Chat;
                Chat.m_ClientID = -1;
                Chat.m_pMessage = aBuf;
                Chat.m_Team = 0;

                Server()->SendPackMsg(&Chat, Flags, ToClientID, false);
            }

            CMsgPacker Msg6(protocol6::NETMSGTYPE_SV_VOTESET, false, false);
            
            Msg6.AddInt(Timeout); // Timeout
            Msg6.AddString(pDesc); // Desc
            Msg6.AddString(pReason); // Reason

            return Server()->SendMsg(&Msg6, Flags, ToClientID);
        }
        case NETMSGTYPE_SV_CLIENTINFO:
        {
            Unpacker.GetInt(); // ClientID
            Unpacker.GetInt(); // Local
            int Team = Unpacker.GetInt();
            const char* pName = Unpacker.GetString(); // Name
            Unpacker.GetString(); // Clan
            Unpacker.GetInt(); // Country

            for(int i = 0; i < 6; i ++)
                Unpacker.GetString(); // see src/generated/protocol.h or protocol.cpp
            for(int i = 0; i < 12; i ++)
                Unpacker.GetInt(); // see src/generated/protocol.h or protocol.cpp

            int Silent = Unpacker.GetInt();
            if(Silent)
                return 0;

            char aBuf[128];
            switch(GetStrTeam(Team, m_GameFlags&GAMEFLAG_TEAMS))
            {
                case STR_TEAM_GAME: str_format(aBuf, sizeof(aBuf), "'%s' entered and joined the game", pName); break;
                case STR_TEAM_RED: str_format(aBuf, sizeof(aBuf), "'%s' entered and joined the red team", pName); break;
                case STR_TEAM_BLUE: str_format(aBuf, sizeof(aBuf), "'%s' entered and joined the blue team", pName); break;
                case STR_TEAM_SPECTATORS: str_format(aBuf, sizeof(aBuf), "'%s' entered and joined the spectators", pName); break;
            }

            protocol6::CNetMsg_Sv_Chat Chat;
            Chat.m_ClientID = -1;
            Chat.m_pMessage = aBuf;
            Chat.m_Team = 0;

            return Server()->SendPackMsg(&Chat, Flags, ToClientID, false);
        }
        case NETMSGTYPE_SV_CLIENTDROP:
        {
            int ClientID = Unpacker.GetInt();
            const char* pReason = Unpacker.GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
            int Silent = Unpacker.GetInt();

            if(Silent)
                return 0;

            char aBuf[128];
            if(pReason[0])
                str_format(aBuf, sizeof(aBuf), "'%s' has left the game (%s)", Server()->ClientName(ClientID), pReason);
            else
                str_format(aBuf, sizeof(aBuf), "'%s' has left the game", Server()->ClientName(ClientID));
            
            protocol6::CNetMsg_Sv_Chat Chat;
            Chat.m_ClientID = -1;
            Chat.m_pMessage = aBuf;
            Chat.m_Team = 0;

            return Server()->SendPackMsg(&Chat, Flags, ToClientID, false);
        }
        case NETMSGTYPE_SV_GAMEMSG:
        {
            return DeepGameMsgConvert6(pMsg, Flags, ToClientID);
        }
    }
    return -1;
}

int CNetConverter::DeepGameMsgConvert6(CMsgPacker *pMsg, int Flags, int ToClientID)
{
    CUnpacker Unpacker;
    Unpacker.Reset(pMsg->Data(), pMsg->Size());

    int GameMsgID = Unpacker.GetInt();
    switch (GameMsgID)
    {
        case GAMEMSG_TEAM_SWAP:
        {
            protocol6::CNetMsg_Sv_Chat Chat;
            Chat.m_ClientID = -1;
            Chat.m_pMessage = "Teams were swapped";
            Chat.m_Team = 0;
            
            return Server()->SendPackMsg(&Chat, Flags, ToClientID, false);
        }
        case GAMEMSG_SPEC_INVALIDID:
        {
            protocol6::CNetMsg_Sv_Chat Chat;
            Chat.m_ClientID = -1;
            Chat.m_pMessage = "Invalid spectator id used";
            Chat.m_Team = 0;
            
            return Server()->SendPackMsg(&Chat, Flags, ToClientID, false);
        }
        case GAMEMSG_TEAM_SHUFFLE:
        {
            protocol6::CNetMsg_Sv_Chat Chat;
            Chat.m_ClientID = -1;
            Chat.m_pMessage = "Teams were shuffled";
            Chat.m_Team = 0;
            
            return Server()->SendPackMsg(&Chat, Flags, ToClientID, false);
        }
        case GAMEMSG_TEAM_BALANCE:
        {
            protocol6::CNetMsg_Sv_Chat Chat;
            Chat.m_ClientID = -1;
            Chat.m_pMessage = "Teams have been balanced";
            Chat.m_Team = 0;
            
            return Server()->SendPackMsg(&Chat, Flags, ToClientID, false);
        }
        case GAMEMSG_CTF_DROP:
        {
            protocol6::CNetMsg_Sv_SoundGlobal Sound;
            Sound.m_SoundID = SOUND_CTF_DROP;
            
            return Server()->SendPackMsg(&Sound, Flags, ToClientID, false);
        }
        case GAMEMSG_CTF_RETURN:
        {
            protocol6::CNetMsg_Sv_SoundGlobal Sound;
            Sound.m_SoundID = SOUND_CTF_RETURN;
            
            return Server()->SendPackMsg(&Sound, Flags, ToClientID, false);
        }
        case GAMEMSG_TEAM_ALL:
        {
            const char *pMsg = "";
            switch(GetStrTeam(Unpacker.GetInt(), m_GameFlags&GAMEFLAG_TEAMS))
            {
                case STR_TEAM_GAME: pMsg = "All players were moved to the game"; break;
                case STR_TEAM_RED: pMsg = "All players were moved to the red team"; break;
                case STR_TEAM_BLUE: pMsg = "All players were moved to the blue team"; break;
                case STR_TEAM_SPECTATORS: pMsg = "All players were moved to the spectators"; break;
            }

            protocol6::CNetMsg_Sv_Chat Chat;
            Chat.m_ClientID = -1;
            Chat.m_pMessage = pMsg;
            Chat.m_Team = 0;
            
            return Server()->SendPackMsg(&Chat, Flags, ToClientID, false);
        }
        case GAMEMSG_TEAM_BALANCE_VICTIM:
        {
            const char *pMsg = "";
            switch(GetStrTeam(Unpacker.GetInt(), m_GameFlags&GAMEFLAG_TEAMS))
            {
                case STR_TEAM_RED: pMsg = "You were moved to the red team due to team balancing"; break;
                case STR_TEAM_BLUE: pMsg = "You were moved to the blue team due to team balancing"; break;
            }

            protocol6::CNetMsg_Sv_Broadcast Broadcast;
            Broadcast.m_pMessage = pMsg;
            
            return Server()->SendPackMsg(&Broadcast, Flags, ToClientID, false);
        }
        case GAMEMSG_CTF_GRAB:
        {
            int Team = Unpacker.GetInt();
            int SoundID = SOUND_CTF_GRAB_PL;
            if((GameServer()->m_apPlayers[ToClientID]->GetTeam() == Team) || 
                (GameServer()->m_apPlayers[ToClientID]->GetTeam() == TEAM_SPECTATORS && 
                    GameServer()->m_apPlayers[ToClientID]->GetSpecMode() != SPEC_FREEVIEW && 
                        GameServer()->m_apPlayers[GameServer()->m_apPlayers[ToClientID]->GetSpectatorID()] && 
                            GameServer()->m_apPlayers[GameServer()->m_apPlayers[ToClientID]->GetSpectatorID()]->GetTeam() == Team))
                SoundID = SOUND_CTF_GRAB_EN;
            protocol6::CNetMsg_Sv_SoundGlobal Sound;
            Sound.m_SoundID = SoundID;
            
            return Server()->SendPackMsg(&Sound, Flags, ToClientID, false);
        }
        case GAMEMSG_GAME_PAUSED:
        {
            int ClientID = clamp(Unpacker.GetInt(), 0, MAX_CLIENTS - 1);
            char aBuf[128];
            str_format(aBuf, sizeof(aBuf), "'%s' initiated a pause", Server()->ClientName(ClientID));

            protocol6::CNetMsg_Sv_Chat Chat;
            Chat.m_ClientID = -1;
            Chat.m_pMessage = aBuf;
            Chat.m_Team = 0;
            
            return Server()->SendPackMsg(&Chat, Flags, ToClientID, false);
        }
        case GAMEMSG_CTF_CAPTURE:
        {
            int Team = Unpacker.GetInt();
            int ClientID = Unpacker.GetInt();
            float Time = Unpacker.GetInt() / (float)Server()->TickSpeed();

            char aBuf[128];
            if(Time <= 60)
            {
                if(Team)
                {
                    str_format(aBuf, sizeof(aBuf), "The blue flag was captured by '%s' (%.2f seconds)", Server()->ClientName(ClientID), Time);
                }
                else
                {
                    str_format(aBuf, sizeof(aBuf), "The red flag was captured by '%s' (%.2f seconds)", Server()->ClientName(ClientID), Time);
                }
            }
            else
            {
                if(Team)
                {
                    str_format(aBuf, sizeof(aBuf), "The blue flag was captured by '%s'", Server()->ClientName(ClientID));
                }
                else
                {
                    str_format(aBuf, sizeof(aBuf), "The red flag was captured by '%s'", Server()->ClientName(ClientID));
                }
            }

            protocol6::CNetMsg_Sv_SoundGlobal Sound;
            Sound.m_SoundID = SOUND_CTF_CAPTURE;

            Server()->SendPackMsg(&Sound, Flags, ToClientID, false);

            protocol6::CNetMsg_Sv_Chat Chat;
            Chat.m_ClientID = -1;
            Chat.m_pMessage = aBuf;
            Chat.m_Team = 0;
            
            return Server()->SendPackMsg(&Chat, Flags, ToClientID, false);
        }
    }
    return -1;
}

int CNetConverter::SendMsgConvert(CMsgPacker *pMsg, int Flags, int ToClientID, int Depth)
{
    // send a msg for record
    if(Depth == 0)
    {
        CMsgPacker Msg7(pMsg->Type(), false, false);
        Msg7.AddRaw(pMsg->Data(), pMsg->Size());

        Server()->SendMsg(&Msg7, MSGFLAG_NOSEND | Flags, -1);
    }

    if(ToClientID == -1)
    {
        for(int i = 0 ; i < MAX_CLIENTS; i ++)
        {
            SendMsgConvert(pMsg, Flags, i, Depth + 1);
        }
        return 0;
    }
    else if(Server()->ClientProtocol(ToClientID) == NETPROTOCOL_SEVEN)
    {
        CMsgPacker Msg7(pMsg->Type(), false, false);
        Msg7.AddRaw(pMsg->Data(), pMsg->Size());

        return Server()->SendMsg(&Msg7, Flags | MSGFLAG_NORECORD, ToClientID); // no record
    }
    else if(Server()->ClientProtocol(ToClientID) == NETPROTOCOL_SIX)
    {
        return DeepMsgConvert6(pMsg, Flags | MSGFLAG_NORECORD, ToClientID); // no record
    }
    return -1;
}

static int SystemMsg7To6(int Msg)
{
    switch (Msg)
    {
        case NETMSG_MAP_DATA: return protocol6::NETMSG_MAP_DATA;
        case NETMSG_CON_READY: return protocol6::NETMSG_CON_READY;
        case NETMSG_SNAP: return protocol6::NETMSG_SNAP;
        case NETMSG_SNAPEMPTY: return protocol6::NETMSG_SNAPEMPTY;
        case NETMSG_SNAPSINGLE: return protocol6::NETMSG_SNAPSINGLE;
        case NETMSG_SNAPSMALL: return protocol6::NETMSG_SNAPSMALL;
        case NETMSG_INPUTTIMING: return protocol6::NETMSG_INPUTTIMING;
        case NETMSG_RCON_LINE: return protocol6::NETMSG_RCON_LINE;
        case NETMSG_RCON_CMD_ADD: return protocol6::NETMSG_RCON_CMD_ADD;
        case NETMSG_RCON_CMD_REM: return protocol6::NETMSG_RCON_CMD_REM;
        case NETMSG_PING: return protocol6::NETMSG_PING;
        case NETMSG_PING_REPLY: return protocol6::NETMSG_PING_REPLY;
    }
    return -1;
}

int CNetConverter::DeepSystemMsgConvert6(CMsgPacker *pMsg, int Flags, int ToClientID)
{
    CUnpacker Unpacker;
    Unpacker.Reset(pMsg->Data(), pMsg->Size());

    if(Unpacker.Error())
        return -1;

    switch (pMsg->Type())
    {
        case NETMSG_MAP_DATA: // see server.cpp : NETMSG_REQUEST_MAP_DATA
        case NETMSG_CON_READY:
        case NETMSG_SNAP:
        case NETMSG_SNAPEMPTY:
        case NETMSG_SNAPSINGLE:
        case NETMSG_SNAPSMALL:
        case NETMSG_INPUTTIMING:
        case NETMSG_RCON_LINE:
        case NETMSG_RCON_CMD_ADD:
        case NETMSG_RCON_CMD_REM:
        case NETMSG_PING:
        case NETMSG_PING_REPLY:
        {
            CMsgPacker Msg6(SystemMsg7To6(pMsg->Type()), true, false);
            Msg6.AddRaw(pMsg->Data(), pMsg->Size());

            return Server()->SendMsg(&Msg6, Flags | MSGFLAG_NORECORD, ToClientID); // no record
        }
        case NETMSG_MAP_CHANGE:
        {
            CMsgPacker Msg6(protocol6::NETMSG_MAP_CHANGE, true, false);
            Msg6.AddString(Unpacker.GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES)); // Map name
            Msg6.AddInt(Unpacker.GetInt()); // Crc
            Msg6.AddInt(Unpacker.GetInt()); // Size

            return Server()->SendMsg(&Msg6, Flags, ToClientID); // no record
        }
        case NETMSG_SERVERINFO:
        {
            Server()->SendServerInfo(ToClientID);
            return 0;
        }
        case NETMSG_RCON_AUTH_ON:
        {
            CMsgPacker Msg6(protocol6::NETMSG_RCON_AUTH_STATUS, true, false);
            Msg6.AddInt(1); // Authed
            Msg6.AddInt(1); // Cmdlist

            return Server()->SendMsg(&Msg6, Flags, ToClientID); // no record
        }
        case NETMSG_RCON_AUTH_OFF:
        {
            CMsgPacker Msg6(protocol6::NETMSG_RCON_AUTH_STATUS, true, false);
            Msg6.AddInt(0); // Authed
            Msg6.AddInt(0); // Cmdlist

            return Server()->SendMsg(&Msg6, Flags, ToClientID); // no record
        }
    }
    return -1;
}

int CNetConverter::SendSystemMsgConvert(CMsgPacker *pMsg, int Flags, int ToClientID, int Depth)
{
    dbg_assert(((CMsgPacker *)pMsg)->Convert(), "SendSystemMsgConvert::Msg needn't convert");
    dbg_assert(((CMsgPacker *)pMsg)->System(), "SendSystemMsgConvert::Msg isn't system msg");

    // send a msg for record
    if(Depth == 0)
    {
        CMsgPacker Msg7(pMsg->Type(), false, false);
        Msg7.AddRaw(pMsg->Data(), pMsg->Size());

        Server()->SendMsg(&Msg7, MSGFLAG_NOSEND | Flags, -1);
    }

    if(ToClientID == -1)
    {
        for(int i = 0 ; i < MAX_CLIENTS; i ++)
        {
            SendSystemMsgConvert(pMsg, Flags, i, Depth + 1);
        }
        return 0;
    }
    else if(Server()->ClientProtocol(ToClientID) == NETPROTOCOL_SEVEN)
    {
        CMsgPacker Msg7(pMsg->Type(), true, false);
        Msg7.AddRaw(pMsg->Data(), pMsg->Size());

        return Server()->SendMsg(&Msg7, Flags | MSGFLAG_NORECORD, ToClientID); // no record
    }
    else if(Server()->ClientProtocol(ToClientID) == NETPROTOCOL_SIX)
    {
        return DeepSystemMsgConvert6(pMsg, Flags | MSGFLAG_NORECORD, ToClientID); // no record
    }

    return -1;
}

INetConverter *CreateNetConverter(IServer *pServer, class CConfig *pConfig) { return new CNetConverter(pServer, pConfig); }