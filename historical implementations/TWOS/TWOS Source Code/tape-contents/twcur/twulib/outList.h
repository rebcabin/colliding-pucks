/*  	Copyright (C) 1989, 1991, California Institute of Technology.
		U.S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/********************************************************************
* outList.h
*
* This is the header file for the output message list ADT.  This
* ADT is used by the TWULIB packages' Tell functions to coordinate
* the acutal message or messages that will be sent.  After the
* last package runs its Tell function, the system calls the 
* send_outList function to send all messages in the outList.
********************************************************************/


/* This is the outList type.  A pointer to an outList is passed to
 * each package tell function.  The tell function should used the
 * provided functions to delete and insert messages into the list.
 */
typedef TWULarrayType OutListType;


/* This is an element of the outList.  A pointer to any element
 * can be acquired through provided functions.  The fields represent
 * those fields required for the tell function.  The 'text' field
 * is the pointer to the text to send.  If the text exists in a
 * dynamically allocated memory block, then newText should indicate
 * that block ('text' should still point to the text).  Both the
 * dispose_outList and remove_outListMsg functions will attempt
 * to free non-null newText fields.
 * 
 * REMINDER:  THE 'TEXT' FIELD MUST POINT TO THE START OF TEXT
 *            EVEN IF NEWTEXT IS NON-NULL.
 */
typedef struct
   {
   Name receiver;
   VTime recTime;
   long selector;
   int length;
   char * text;
   Pointer newText;
   } OutMsgType;



/* This function is called by the system before the packages are
 * invoked.  The packages should not call this function.
 */
void init_outList();
/* OutListType * outListPtr;
 */


/* This function will add a message to the outList.  It requires the
 * parameters of the message and, of course, a pointer to the outList.
 */
void add_outListMsg();
/* OutListType * outListPtr;
 * char * rec;
 * VTime recTime; 
 * long sel; 
 * int len;
 * char * txt;
 * Pointer newText;
 */


/* This will return the size of the outList.  Note that the outList
 * could have zero length.
 */
int out_listSize();
/* OutListType * outListPtr;
 */


/* This function will return a pointer to element i (0<=i<out_listSize())
 * of the outList.
 */
OutMsgType * get_outListMsg();
/* OutListType * outListPtr;
 * int i;
 */


/* This function will send the entire outList.  Each element in the
 * outList is considered a single message and is sent to the indicated
 * destination.  Note that the system calls this function after all
 * packages have been invoked for the tell function.  The packages
 * should not call this function.
 */
void send_outList();
/* OutListType * outListPtr;
 */


/* This function destroys the entire outList.  The system calls this
 * function just before exited the tell function.  This should not
 * be called by a package.
 */
void dispose_outList();
/* OutListType * outListPtr;
 */


/* This function will remove an element from the outList.  Note that
 * the list is collapsed down after the element is removed (this could
 * affect looping through elements).  If the element has a non-null
 * newText field, then disposeBlockPtr is called.
 */
void remove_outListMsg();
/* OutListType * outListPtr;
 * int i;
 */
