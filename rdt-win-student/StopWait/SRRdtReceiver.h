#ifndef SR_RECEIVER_H
#define SR_RECEIVER_H
#include "RdtReceiver.h"
#include <list>

class SRRdtReceiver :public RdtReceiver
{
private:
	int rsize;
	int rbase;
	int expectSequenceNumberRcvd;	// 期待收到的下一个报文序号
	bool FullState;				// 缓存区是否处于满状态
	Packet lastAckPkt;				//上次发送的确认报文
	std::list<Packet> rwindows_packets;

public:
	SRRdtReceiver();
	virtual ~SRRdtReceiver();

public:

	bool getFullState();
	void receive(Packet &packet);	//接收报文，将被NetworkService调用
};

#endif
