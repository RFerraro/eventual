// test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

int main(int argCount, char* args[])
{
   testing::InitGoogleTest(&argCount, args);
   
   return RUN_ALL_TESTS();
}
