#ifndef SR_RDT_SENDER_H
#define SR_RDT_SENDER_H
#include "RdtSender.h"
#include <list>

using namespace std;

class SRRdtSender :public RdtSender
{
private:
	int size;                       //���ڴ�С
	int base;                       //�����
	int expectSequenceNumberSend;	// ��һ��������� 
	bool waitingState;				// �Ƿ��ڵȴ�Ack��״̬
	Packet packetWaitingAck;		//�ѷ��Ͳ��ȴ�Ack
	std::list<Packet> swindows_packets;

public:

	bool getWaitingState();
	bool send(Message &message);						//����Ӧ�ò�������Message����NetworkServiceSimulator����,������ͷ��ɹ��ؽ�Message���͵�����㣬����true;�����Ϊ���ͷ����ڵȴ���ȷȷ��״̬���ܾ�����Message���򷵻�false
	void receive(Packet &ackPkt);						//����ȷ��Ack������NetworkServiceSimulator����	
	void timeoutHandler(int seqNum);					//Timeout handler������NetworkServiceSimulator����

public:
	SRRdtSender();
	virtual ~SRRdtSender();
};

#endif