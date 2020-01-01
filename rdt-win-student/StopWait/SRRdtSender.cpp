#include "stdafx.h"
#include "Global.h"
#include "SRRdtSender.h"

SRRdtSender::SRRdtSender() :size(4), base(0), expectSequenceNumberSend(0), waitingState(false)
{
}

SRRdtSender::~SRRdtSender()
{
}

bool SRRdtSender::getWaitingState() {
	if (this->swindows_packets.size() == 4)
	{
		this->waitingState = true;
	}
	else
	{
		this->waitingState = false;
	}
	return waitingState;
}

bool SRRdtSender::send(Message &message) {
	if (this->getWaitingState()) { //���ͷ����ڵȴ�ȷ��״̬
		return false;
	}

	this->packetWaitingAck.acknum = -1; //���Ը��ֶ�
	this->packetWaitingAck.seqnum = this->expectSequenceNumberSend;
	this->packetWaitingAck.checksum = 0;
	memcpy(this->packetWaitingAck.payload, message.data, sizeof(message.data));
	this->packetWaitingAck.checksum = pUtils->calculateCheckSum(this->packetWaitingAck);
	pUtils->printPacket("���ͷ����ͱ���", this->packetWaitingAck);

	pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck.seqnum);			//�������ͷ���ʱ��

	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck);								//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�
	this->swindows_packets.push_back(this->packetWaitingAck);
	//this->expectSequenceNumberSend++;
	this->expectSequenceNumberSend = ((this->expectSequenceNumberSend + 1) % 8);
	//this->waitingState = true;
	return true;
}

void SRRdtSender::receive(Packet &ackPkt) {
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	//���У�����ȷ
	if (checkSum == ackPkt.checksum)
	{
		//int flag = 0;
		pUtils->printPacket("���ͷ��յ���Ӧ���ĵ�ȷ�ϣ�", ackPkt);
		pns->stopTimer(SENDER,ackPkt.acknum);
		list<Packet>::iterator it;                                          //�������ݱ���windows_packets�д洢�ı���
		for (it = this->swindows_packets.begin(); it != this->swindows_packets.end(); it++)
		{
			if (it->seqnum == ackPkt.acknum)
			{
				this->swindows_packets.erase(it);
				break;
			}
		}
		
		cout << "��ʱ���ͷ����ڵ�baseֵΪ��" << this->base << endl << "���ͷ��������б����У�" << endl;
		for (it = this->swindows_packets.begin(); it != this->swindows_packets.end(); it++)
		{
			pUtils->printPacket("    ", *it);
		}

		//��������
		if (!this->swindows_packets.size())
		{
			this->base = this->expectSequenceNumberSend;
		}
		else
		{
			this->base = this->swindows_packets.front().seqnum;
		}
	}
}

void SRRdtSender::timeoutHandler(int seqNum) {
	//Ψһһ����ʱ��,���迼��seqNum
	pns->stopTimer(SENDER, seqNum);										//���ȹرն�ʱ��
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//�����������ͷ���ʱ��
	list<Packet>::iterator it;                                          //�������ݱ������windows_packets�д洢�ı���
	for (it = this->swindows_packets.begin(); it != this->swindows_packets.end(); it++)
	{
		if (it->seqnum == seqNum)
		{
			pUtils->printPacket("���ͷ���ʱ��ʱ�䵽�����·��ʹ˱���", *it);
			pns->sendToNetworkLayer(RECEIVER, *it);			//���·������ݰ�
			break;
		}
	}
}