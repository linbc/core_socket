#include <gtest/gtest.h>
#include "TcpConnection.h"

namespace core_socket
{

TEST(core_socket,OutputBuffer)
{
		TcpConnection::OutputBuffer opbuff;
		int prt[10];
		for (int i = 0;i <10; i++)
		{
			prt[i] = i;
			opbuff.write((char*)&prt[i],sizeof(int));

		}
		ASSERT_TRUE(opbuff.length() == sizeof(int) * 10);
		ASSERT_TRUE(*((int*)opbuff.data()) == prt[0]);
		opbuff.add_read(sizeof(int) * 3);
		ASSERT_TRUE(opbuff.length()== sizeof(int) * (10 - 3));
		ASSERT_TRUE(*((int*)opbuff.data()) == prt[3]);
		//opbuff.add_read(sizeof(int) * 10);
		//ASSERT_TRUE(opbuff.length()==0);
		opbuff.reset();
		ASSERT_TRUE(opbuff.length()==0);

		char ptr2[5] = "abcd";
		opbuff.write(ptr2,sizeof(ptr2));
		ASSERT_TRUE(opbuff.length() == 5);
		ASSERT_TRUE(*opbuff.data() == ptr2[0]);
		opbuff.add_read(2);
		ASSERT_TRUE(opbuff.length() == 5-2);
		ASSERT_TRUE(*opbuff.data() == ptr2[2]);		
}

//解包函数
TEST(core_socket,ComplatePacket)
{
	//这个包正好是一个两个字节
	core_obj::ByteArray pkt;
	pkt.writeShort(3);
	pkt.length(3);
	pkt.position(0);

	TcpConnection conn;
	int last = conn.ComplatePacket(pkt.cur_data(),pkt.length());
	ASSERT_EQ(last,0);

	pkt.clear();
	pkt.writeShort(5);
	pkt.length(6);
	pkt.position(0);
	last = conn.ComplatePacket(pkt.cur_data(),pkt.length());
	ASSERT_EQ(last,1);
}

}
