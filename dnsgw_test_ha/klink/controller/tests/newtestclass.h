/*
 * File:   newtestclass.h
 * Author: mfbari
 *
 * Created on Nov 22, 2012, 5:10:29 PM
 */

#ifndef NEWTESTCLASS_H
#define	NEWTESTCLASS_H

#include <cppunit/extensions/HelperMacros.h>

class newtestclass: public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE (newtestclass);

	CPPUNIT_TEST (testDecode);
	CPPUNIT_TEST (testEncode);

	CPPUNIT_TEST_SUITE_END();

public:
	newtestclass();
	virtual ~newtestclass();
	void setUp();
	void tearDown();

private:
	void testDecode();
	void testEncode();

};

#endif	/* NEWTESTCLASS_H */

