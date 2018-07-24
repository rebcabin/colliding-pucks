/*  	Copyright (C) 1989, 1991, California Institute of Technology.
		U.S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */


#include "twcommon.h"
#include "twusrlib.h"

extern packageType userMsgListPackage;
extern packageType multiPacketPackage;
extern packageType evtListPackage;


packageType * libPackageTable[] =
	{
	&userMsgListPackage, &multiPacketPackage, &evtListPackage, 0
	};




/*******************************************************

List of current priorities:


Package					Soe	Eoe	Tell
---------------------------------------
userMsgList				1		99		XX
multiPacket				2		XX		5
evtList					5		50		1	


List of negative msg selectors in use:

Package					selector(s)
---------------------------------------
multiPacket				-1
evtList					-2

*******************************************************/
