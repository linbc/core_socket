#include <gtest/gtest.h>
#include "TcpConnection.h"

using namespace core_socket;

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
		system("pause");


		char ptr2[5] = "abcd";
		opbuff.write(ptr2,sizeof(ptr2));
		ASSERT_TRUE(opbuff.length() == 5);
		ASSERT_TRUE(*opbuff.data() == ptr2[0]);
		opbuff.add_read(2);
		ASSERT_TRUE(opbuff.length() == 5-2);
		ASSERT_TRUE(*opbuff.data() == ptr2[2]);
		system("pause");
		
}