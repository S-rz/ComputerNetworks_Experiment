#ifndef SR_RECEIVER_H
#define SR_RECEIVER_H
#include "RdtReceiver.h"
#include <list>

class SRRdtReceiver :public RdtReceiver
{
private:
	int rsize;
	int rbase;
	int expectSequenceNumberRcvd;	// �ڴ��յ�����һ���������
	bool FullState;				// �������Ƿ�����״̬
	Packet lastAckPkt;				//�ϴη��͵�ȷ�ϱ���
	std::list<Packet> rwindows_packets;

public:
	SRRdtReceiver();
	virtual ~SRRdtReceiver();

public:

	bool getFullState();
	void receive(Packet &packet);	//���ձ��ģ�����NetworkService����
};

#endif
