/*
 * File:   newtestclass.cpp
 * Author: mfbari
 *
 * Created on Nov 22, 2012, 5:10:30 PM
 */

#include "newtestclass.h"

CPPUNIT_TEST_SUITE_REGISTRATION (newtestclass);

newtestclass::newtestclass()
{
}

newtestclass::~newtestclass()
{
}

void newtestclass::setUp()
{
}

void newtestclass::tearDown()
{
}

int* ReedMuller::decode(int* codeword);

void newtestclass::testDecode()
{
	int* codeword;
	ReedMuller reedMuller;
	int* result = reedMuller.decode(codeword);
	if (true /*check result*/)
	{
		CPPUNIT_ASSERT(false);
	}
}

int* ReedMuller::encode(int* message);

void newtestclass::testEncode()
{
	int* message;
	ReedMuller reedMuller;
	int* result = reedMuller.encode(message);
	if (true /*check result*/)
	{
		CPPUNIT_ASSERT(false);
	}
}

