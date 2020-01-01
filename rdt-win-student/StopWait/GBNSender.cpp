#include "stdafx.h"
#include "Global.h"
#include "GBNSender.h"


GBNSender::GBNSender() :size(3),base(0),expectSequenceNumberSend(0), waitingState(false)
{
}


GBNSender::~GBNSender()
{
}

bool GBNSender::getWaitingState() {
	if (this->windows_packets.size()==3)
	{
		this->waitingState = true;
	}
	else
	{
		this->waitingState = false;
	}
	return waitingState;
}

bool GBNSender::send(Message &message) {
	if (this->getWaitingState()) { //���ͷ����ڵȴ�ȷ��״̬
		return false;
	}

	this->packetWaitingAck.acknum = -1; //���Ը��ֶ�
	this->packetWaitingAck.seqnum = this->expectSequenceNumberSend;
	this->packetWaitingAck.checksum = 0;
	memcpy(this->packetWaitingAck.payload, message.data, sizeof(message.data));
	this->packetWaitingAck.checksum = pUtils->calculateCheckSum(this->packetWaitingAck);
	pUtils->printPacket("���ͷ����ͱ���", this->packetWaitingAck);
	if (this->base == this->expectSequenceNumberSend)
	{
		pns->startTimer(SENDER, Configuration::TIME_OUT, 0);			//�������ͷ���ʱ��
	}

	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck);								//����ģ�����绷����sendToNetworkLayer��ͨ������㷢�͵��Է�
	this->windows_packets.push_back(this->packetWaitingAck);
	//this->expectSequenceNumberSend++;
	this->expectSequenceNumberSend = ((this->expectSequenceNumberSend + 1) % 4);
	//this->waitingState = true;
	return true;
}

void GBNSender::receive(Packet &ackPkt) {
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	//���У�����ȷ
	if (checkSum == ackPkt.checksum)
	{
		//int flag = 0;
		//if (this->base <= ((ackPkt.acknum + 1)%4))
		while (this->base != ((ackPkt.acknum + 1) % 4))
		{
			this->windows_packets.pop_front(); //��������
			this->base = (this->base + 1) % 4;
		}
		this->base = ((ackPkt.acknum + 1)%4);
		list<Packet>::iterator it;                                          //�������ݱ������windows_packets�д洢�ı���
		cout << "��ʱ���ڵ�baseֵΪ��" << this->base << endl<<"�������б����У�"<<endl;
		for (it = this->windows_packets.begin(); it != this->windows_packets.end(); it++)
		{
			pUtils->printPacket("    ",*it);
		}
		if (this->base == this->expectSequenceNumberSend)
		{
			pUtils->printPacket("���ͷ���ȷ�յ�ȷ��", ackPkt);
			pns->stopTimer(SENDER, 0);		//�رն�ʱ��
		}
		else
		{
			pns->stopTimer(SENDER, 0);									//���ȹرն�ʱ��
			pns->startTimer(SENDER, Configuration::TIME_OUT, 0);			//�����������ͷ���ʱ��
		}
	}
}

void GBNSender::timeoutHandler(int seqNum) {
	//Ψһһ����ʱ��,���迼��seqNum
	pUtils->printPacket("���ͷ���ʱ��ʱ�䵽�����·��ʹ˱���֮��ı���", this->packetWaitingAck);
	pns->stopTimer(SENDER, seqNum);										//���ȹرն�ʱ��
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//�����������ͷ���ʱ��
	list<Packet>::iterator it;                                          //�������ݱ������windows_packets�д洢�ı���
	for (it = this->windows_packets.begin(); it != this->windows_packets.end(); it++)
	{
		pns->sendToNetworkLayer(RECEIVER, *it);			//���·������ݰ�
	}
}
