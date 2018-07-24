/*  	Copyright (C) 1989, 1991, California Institute of Technology.
		U.S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

#include "twcommon.h"
#include "twusrlib.h"
#include "outList.h"
#include "twpackages.h"




int multi_packetSoe(), multi_packetTell();



#define MULTI_SELECTOR -1
#define MULTI_SIZE ( MAXPKTL - sizeof(multiHeader) )
#define MAX_MSG_LEN MAXPKTL


typedef struct
   {
   int count;
   }  multiPacketState;

packageType multiPacketPackage =
   {
   NULL, multi_packetSoe, 2, NULL, 0, multi_packetTell, 5, 
                              NULL, sizeof(multiPacketState)
   };



/* This is the header placed at the start of each packet.  It is the
 * first item in a packet's message text area (msgText).
 */
typedef struct
   {
   Name sender;
	VTime sendTime;
   int count, packetNum, size, numPackets;
   long selector;
   } multiHeader;


/***************************************************************************
 * These functions put the packets together into a single message.
 ***************************************************************************/
multi_packetSoe()
{
   int i, j, numPackets;
   multiHeader * firstPkt, * tmpPkt;
   Pointer msgBlock;
   char * msgPtr;
   void put_textInMsg();
   multiPacketState * ps;
	

   /* We must set the multipacket counter to zero.
    */
   ps = myPState(&multiPacketPackage);
   ps->count = 0;


   /* Loop through the messages looking for the 
    * packets of a multipacket message.
    */
   for (i=0; i<LIBnumMsgs; i++)
      {
      if (LIBmsgSelector(i) != MULTI_SELECTOR)
         continue;

      numPackets = 1;

      /* We've found a packet so we must search
       * for the rest.  Place the text of the messages
       * into a dynamically allocated block of memory.
       */
      firstPkt = LIBmsgText(i);

      msgBlock = newBlockPtr(firstPkt->size);
      msgPtr = pointerPtr(msgBlock);

      put_textInMsg(msgPtr, firstPkt);

      for (j=i+1; j<LIBnumMsgs; )
         {
         if (LIBmsgSelector(j) != MULTI_SELECTOR)
            {
            j++;
            continue;
            }
         
         tmpPkt = LIBmsgText(j);
         
         if ( (tmpPkt->count != firstPkt->count) ||
                     (strcmp(tmpPkt->sender, firstPkt->sender) != 0) )
            {
            j++;
            continue;
            }

         put_textInMsg(msgPtr, tmpPkt);
         LIBdelete_message(j); 
			numPackets++;
         } /* end for j=i ... */

      /* If not all of the packets have arrived then 
       * just dealloc the previously allocated msg
       * space and error.  Instead of error, we might
       * want to continue the event if there is another
       * message and then error in the end of evt section.
       *
       * Otherwise, insert the message into the message list.
       */
      if (numPackets != firstPkt->numPackets)
			{
         disposeBlockPtr(msgBlock);
			userError("Missing packet of multipacket message.");
			}
      else
         LIBinsert_message(firstPkt->selector, firstPkt->sender, 
								firstPkt->sendTime, msgPtr, msgBlock, firstPkt->size);

      LIBdelete_message(i);

		/* Note that we've just rearranged the msgList.
       * Thus, we should start counting packets over.
       * To do this we reset i to be -1 (it will be
       * incremented to zero).
       */
		i = -1;

      } /* end for i */      
}
   
         
void put_textInMsg(dest, packet)
   char * dest;
   multiHeader * packet;
{
   int numBytes;

   /* Based of the current packet size definition and the 
    * packet number of this packet, we can determine where   
    * to put the text of the packet.
    */
   dest += packet->packetNum * MULTI_SIZE;

   if (packet->packetNum < packet->numPackets-1)
      numBytes = MULTI_SIZE;
   else
      numBytes = (packet->size-1) % MULTI_SIZE + 1;

   TWULfastcpy(dest, (char *)(packet+1), numBytes);
}



/**********************************************************************
 * These functions are for the multiPacket tell that breaks long msgs
 * into small ones.
 **********************************************************************/
multi_packetTell(outListPtr)
   OutListType * outListPtr;
{
   multiPacketState * ps;
   int listIndex, numMsgsToCheck;
   OutMsgType * msgPtr;
	void packetize_message();

   ps = myPState(&multiPacketPackage);

	numMsgsToCheck = out_listSize(outListPtr);
   for (listIndex=0; numMsgsToCheck>0; numMsgsToCheck--)
      {
      /* The msgPtr points to the message we are considering.  If the 
       * length of the message is under 1 packet, then go to 
       * next message.  Otherwise, packetize the message.  
       * This involves removing the creating new tell messages
       * (these are the packets) and then removing the old one.
       */
      msgPtr = get_outListMsg(outListPtr, listIndex);

      if (msgPtr->length > MAX_MSG_LEN)
         {
         packetize_message(msgPtr, outListPtr, ps);
         remove_outListMsg(outListPtr, listIndex);
         }
	
		/* We only increment listIndex if we haven't removed a msg from 
       * the outlist.  If we remove a msg then the list is collapsed,
       * which means that we need not increase the index.
       */
		else listIndex++;

		} /* end for numMsgsToCheck */
}


void packetize_message(msgPtr, outListPtr, ps)
   OutMsgType * msgPtr;
   OutListType * outListPtr;
   multiPacketState * ps;
{
   int msgSize;
   char * txtPtr;
   multiHeader head;
      

   /* Set up the information about the multipacket header.
    * Only the packetNum field will change between the packets.
    */
   myName(head.sender);
	head.sendTime = now;
   head.size = msgPtr->length;
   head.packetNum = 0;
   head.count = (ps->count)++;
   head.selector = msgPtr->selector;
   head.numPackets = head.size / MULTI_SIZE;
   if (head.size % MULTI_SIZE > 0)
      head.numPackets++;
      

   /* Set the textPtr to the text of the message we are 
    * considering.  This is the text of the message that 
    * msgPtr points at.
    */
	txtPtr = msgPtr->text;
            

   /* Make as many packets as needed.
    */
   for (msgSize=head.size; msgSize > 0; msgSize -= MULTI_SIZE)
      {
      make_packet(&head, msgPtr, txtPtr, msgSize, outListPtr);
      head.packetNum++;
      txtPtr += MULTI_SIZE;
      }
}


make_packet(headPtr, msgPtr, textPtr, bytesLeft, outListPtr)
   multiHeader * headPtr;
   OutMsgType * msgPtr;
   char * textPtr;
   int bytesLeft;
   OutListType * outListPtr;
{
	Pointer newText;
	char * text;
   int numBytes, length;
   multiHeader * tmpHeadPtr;
   char * packetText;


	/* Determine the number of bytes to copy and the space needed.
    * Allocate the space and set the text pointer.
    */
   numBytes = ( (bytesLeft > MULTI_SIZE) ? MULTI_SIZE : bytesLeft );
   length =  numBytes + sizeof(multiHeader);

   newText = newBlockPtr(length);
   text = pointerPtr(newText);
   

   /* Now copy the multipacket header into the packet
    * data spacce.
    */
   tmpHeadPtr = (multiHeader *)text;
   *tmpHeadPtr = *headPtr;

   
   /* Now copy in MULTI_SIZE bytes from the message to the
    * packet text space (after the multipacket head).
    */
   packetText = text + sizeof(multiHeader);   
   TWULfastcpy(packetText, textPtr, numBytes);

   
   /* Now add the packet to the tell message list.
    */
	add_outListMsg(outListPtr, msgPtr->receiver, msgPtr->recTime, 
												MULTI_SELECTOR, length, text, newText);
}

      
