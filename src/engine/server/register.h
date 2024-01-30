/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SERVER_REGISTER_H
#define ENGINE_SERVER_REGISTER_H

#include <engine/shared/network.h>

enum
{
	REGISTERTYPE_SEVEN=0,
	REGISTERTYPE_SIX=1,

	NUM_REGISTERTYPES,
};

class CRegister
{
	enum
	{
		REGISTERSTATE_START=0,
		REGISTERSTATE_UPDATE_ADDRS,
		REGISTERSTATE_QUERY_COUNT,
		REGISTERSTATE_HEARTBEAT,
		REGISTERSTATE_REGISTERED,
		REGISTERSTATE_ERROR
	};

	struct CMasterserverInfo
	{
		NETADDR m_Addr;
		int m_Count;
		int m_Valid;
		int64 m_LastSend;
	};

	class CNetServer *m_pNetServer;
	class IEngineMasterServer *m_pMasterServer;
	class CConfig *m_pConfig;
	class IConsole *m_pConsole;

	int m_RegisterState;
	int64 m_RegisterStateStart;
	int m_RegisterFirst;
	int m_RegisterCount;
	int m_RegisterProtocol;

	CMasterserverInfo m_aMasterserverInfo[IMasterServer::MAX_MASTERSERVERS];
	int m_RegisterRegisteredServer;
	
	const char* RegisterName();

	void RegisterNewState(int State);
	void RegisterSendFwcheckresponse(NETADDR *pAddr, TOKEN Token);
	void RegisterSendHeartbeat(NETADDR Addr);
	void RegisterSendCountRequest(NETADDR Addr);
	void RegisterGotCount(struct CNetChunk *pChunk);

public:
	CRegister();
	void Init(int ProtocolType, class CNetServer *pNetServer, class IEngineMasterServer *pMasterServer, class CConfig *pConfig, class IConsole *pConsole);
	void RegisterUpdate(int Nettype);
	int RegisterProcessPacket(struct CNetChunk *pPacket, TOKEN Token);
};

struct CUuid
{
	unsigned char m_aData[16];

	bool operator==(const CUuid &Other) const
    {
        return mem_comp(this, &Other, sizeof(*this)) == 0;
    }

	bool operator!=(const CUuid &Other) const
    {
        return !(*this == Other);
    }

	bool operator<(const CUuid &Other) const
    {
        return mem_comp(this, &Other, sizeof(*this)) < 0;
    }
};

#ifdef DDNET_MASTER

class IRegisterDDNet
{
public:
	virtual ~IRegisterDDNet() {}

	virtual void Update() = 0;
	// Call `OnConfigChange` if you change relevant config variables
	// without going through the console.
	virtual void OnConfigChange() = 0;
	// Returns `true` if the packet was a packet related to registering
	// code and doesn't have to processed furtherly.
	virtual bool OnPacket(const CNetChunk *pPacket) = 0;
	// `pInfo` must be an encoded JSON object.
	virtual void OnNewInfo(const char *pInfo) = 0;
	virtual void OnShutdown() = 0;
};

IRegisterDDNet *CreateRegister(CConfig *pConfig, IConsole *pConsole, IEngine *pEngine, int ServerPort, unsigned SevenSecurityToken);

#endif

#endif
