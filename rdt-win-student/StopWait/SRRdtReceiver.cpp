#include "stdafx.h"
#include "Global.h"
#include <cmath>
#include "SRRdtReceiver.h"

using namespace std;

SRRdtReceiver::SRRdtReceiver() :rsize(4),rbase(0),expectSequenceNumberRcvd(0)
{
	lastAckPkt.acknum = -1; //��ʼ״̬�£��ϴη��͵�ȷ�ϰ���ȷ�����Ϊ-1��ʹ�õ���һ�����ܵ����ݰ�����ʱ��ȷ�ϱ��ĵ�ȷ�Ϻ�Ϊ-1
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;	//���Ը��ֶ�
	for (int i = 0; i < Configuration::PAYLOAD_SIZE; i++) {
		lastAckPkt.payload[i] = '.';
	}
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
}


SRRdtReceiver::~SRRdtReceiver()
{
}

bool SRRdtReceiver::getFullState()
{
	if (this->rwindows_packets.size() == 4)
	{
		this->FullState = true;
	}
	else
	{
		this->FullState = false;
	}
	return this->FullState;
}

void SRRdtReceiver::receive(Packet &packet) {
	if (this->getFullState()) { 
		return ;
	}
	//���У����Ƿ���ȷ
	int checkSum = pUtils->calculateCheckSum(packet);

	//���У�����ȷ
	if (checkSum == packet.checksum ) {
		pUtils->printPacket("���շ���ȷ�յ����ͷ��ı���", packet);
		int num= (packet.seqnum + 8 - this->rbase) % 8;
		if (0 <= num && num<this->rsize)
		{
			if (this->rwindows_packets.size()==0)
			{
				this->rwindows_packets.push_front(packet);
			}
			else
			{
				int flag1, flag2;
				flag1 = 0; flag2 = 0;
				list<Packet>::iterator it; 
				for (it = this->rwindows_packets.begin(); it != this->rwindows_packets.end(); it++)//�������ݱ������windows_packets�д洢�ı���
				{
					if (it->seqnum == packet.seqnum)
					{
						flag1 = 1;    //���ܵı���Ϊ�ظ�����
						break;
					}
				}
				if (!flag1)
				{
					for (it = this->rwindows_packets.begin(); it != this->rwindows_packets.end(); it++)//�������ݱ������windows_packets�д洢�ı���
					{
						int num1 = (it->seqnum - this->rbase + 8) % 8;
						int num2 = (packet.seqnum - this->rbase + 8) % 8;
						if (num1 > num2)
						{
							flag2 = 1;    //�ҵ�����λ�ã����뻺����
							this->rwindows_packets.insert(it, packet);
							break;
						}
					}
				}
				if (flag1 == 0 && flag2 == 0)
				{
					this->rwindows_packets.push_back(packet);
				}
			}
		}
		num = (this->rbase + 8 - packet.seqnum) % 8;
		if (1 <= num && num <= this->rsize)
		{
			lastAckPkt.acknum = packet.seqnum; //ȷ����ŵ����յ��ı������
			lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
			pUtils->printPacket("���շ�����ȷ�ϱ���", lastAckPkt);
			pns->sendToNetworkLayer(SENDER, lastAckPkt);	//����ģ�����绷����sendToNetworkLayer��ͨ������㷢��ȷ�ϱ��ĵ��Է�
		}

		cout << "���շ�������" << this->expectSequenceNumberRcvd << endl;
		cout << "��ʱ���շ����ڵ�baseֵΪ��" << this->rbase << endl << "���շ��������б����У�" << endl;
		list<Packet>::iterator it;
		for (it = this->rwindows_packets.begin(); it != this->rwindows_packets.end(); it++)
		{
			pUtils->printPacket("    ", *it);
		}

		//�ӽ��շ�������ȡ�����ݲ�����
		if (this->rwindows_packets.size())
		{
			if (this->rwindows_packets.front().seqnum == this->expectSequenceNumberRcvd)
			{
				//ȡ��Message�����ϵݽ���Ӧ�ò�
				Message msg;
				memcpy(msg.data, this->rwindows_packets.front().payload, sizeof(packet.payload));
				pns->delivertoAppLayer(RECEIVER, msg);

				lastAckPkt.acknum = this->rwindows_packets.front().seqnum; //ȷ����ŵ����յ��ı������
				lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
				pUtils->printPacket("���շ�����ȷ�ϱ���", lastAckPkt);
				pns->sendToNetworkLayer(SENDER, lastAckPkt);	//����ģ�����绷����sendToNetworkLayer��ͨ������㷢��ȷ�ϱ��ĵ��Է�

				//�������������������
				this->rwindows_packets.pop_front();
				this->expectSequenceNumberRcvd = ((this->expectSequenceNumberRcvd + 1) % 8);
				this->rbase = this->expectSequenceNumberRcvd;
			}
		}
	}
	else {
		if (checkSum != packet.checksum) {
			pUtils->printPacket("���շ�û����ȷ�յ����ͷ��ı���,����У�����", packet);
		}
		else {
			pUtils->printPacket("���շ�û����ȷ�յ����ͷ��ı���,������Ų���", packet);
		}
	}
}