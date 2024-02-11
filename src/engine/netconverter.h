#ifndef ENGINE_NETCONVERTER_H
#define ENGINE_NETCONVERTER_H

#include "kernel.h"
#include "message.h"

enum NetProtocolType
{
	NETPROTOCOL_UNKNOWN = -1,
	NETPROTOCOL_SEVEN = 0, // based version
	NETPROTOCOL_SIX,
	NUM_NETPROTOCOLS,
};

class INetConverter : public IInterface
{
	MACRO_INTERFACE("netconverter", 0)
public:
	virtual bool SnapNewItemConvert(void *pItem, void *pSnapClass, int Type, int ID, int Size, int ToClientID) = 0;
	virtual bool PrevConvertClientMsg(CMsgUnpacker *pItem, int& Type, bool System, int FromClientID) = 0;
	virtual int SendMsgConvert(CMsgPacker *pMsg, int Flags, int ToClientID, int Depth = 0) = 0;
	virtual int SendSystemMsgConvert(CMsgPacker *pMsg, int Flags, int ToClientID, int Depth = 0) = 0;
	virtual void Init(class CGameContext *pGameServer) = 0;
	virtual void ResetChatTick() = 0;
	virtual void ResetSnapItemsEx() = 0;
	virtual void ResetEventID() = 0;
	virtual void SnapItemUuid(int ClientID) = 0;
};

extern INetConverter *CreateNetConverter(class IServer *pServer, class CConfig *pConfig);

#endif
