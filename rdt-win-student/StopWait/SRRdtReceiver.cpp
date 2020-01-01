#include "stdafx.h"
#include "Global.h"
#include <cmath>
#include "SRRdtReceiver.h"

using namespace std;

SRRdtReceiver::SRRdtReceiver() :rsize(4),rbase(0),expectSequenceNumberRcvd(0)
{
	lastAckPkt.acknum = -1; //初始状态下，上次发送的确认包的确认序号为-1，使得当第一个接受的数据包出错时该确认报文的确认号为-1
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;	//忽略该字段
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
	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(packet);

	//如果校验和正确
	if (checkSum == packet.checksum ) {
		pUtils->printPacket("接收方正确收到发送方的报文", packet);
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
				for (it = this->rwindows_packets.begin(); it != this->rwindows_packets.end(); it++)//定义数据便于输出windows_packets中存储的报文
				{
					if (it->seqnum == packet.seqnum)
					{
						flag1 = 1;    //接受的报文为重复报文
						break;
					}
				}
				if (!flag1)
				{
					for (it = this->rwindows_packets.begin(); it != this->rwindows_packets.end(); it++)//定义数据便于输出windows_packets中存储的报文
					{
						int num1 = (it->seqnum - this->rbase + 8) % 8;
						int num2 = (packet.seqnum - this->rbase + 8) % 8;
						if (num1 > num2)
						{
							flag2 = 1;    //找到合适位置，插入缓存区
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
			lastAckPkt.acknum = packet.seqnum; //确认序号等于收到的报文序号
			lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
			pUtils->printPacket("接收方发送确认报文", lastAckPkt);
			pns->sendToNetworkLayer(SENDER, lastAckPkt);	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送确认报文到对方
		}

		cout << "接收方期望：" << this->expectSequenceNumberRcvd << endl;
		cout << "此时接收方窗口的base值为：" << this->rbase << endl << "接收方缓存区中报文有：" << endl;
		list<Packet>::iterator it;
		for (it = this->rwindows_packets.begin(); it != this->rwindows_packets.end(); it++)
		{
			pUtils->printPacket("    ", *it);
		}

		//从接收方缓存区取出数据并处理
		if (this->rwindows_packets.size())
		{
			if (this->rwindows_packets.front().seqnum == this->expectSequenceNumberRcvd)
			{
				//取出Message，向上递交给应用层
				Message msg;
				memcpy(msg.data, this->rwindows_packets.front().payload, sizeof(packet.payload));
				pns->delivertoAppLayer(RECEIVER, msg);

				lastAckPkt.acknum = this->rwindows_packets.front().seqnum; //确认序号等于收到的报文序号
				lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
				pUtils->printPacket("接收方发送确认报文", lastAckPkt);
				pns->sendToNetworkLayer(SENDER, lastAckPkt);	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送确认报文到对方

				//弹出缓存区，序号自增
				this->rwindows_packets.pop_front();
				this->expectSequenceNumberRcvd = ((this->expectSequenceNumberRcvd + 1) % 8);
				this->rbase = this->expectSequenceNumberRcvd;
			}
		}
	}
	else {
		if (checkSum != packet.checksum) {
			pUtils->printPacket("接收方没有正确收到发送方的报文,数据校验错误", packet);
		}
		else {
			pUtils->printPacket("接收方没有正确收到发送方的报文,报文序号不对", packet);
		}
	}
}