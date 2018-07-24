#ifdef MARK3
#include "stdio.h"
#endif

#include "twcommon.h"
#include "pucktypes.h"

int 
Dump_Puck_Msg (index)
int index;
{

    switch ( msgSelector(index) )
    {
    case CANCEL_ACTION:
	{
	    unActMsg *m;
	    
	    m = (unActMsg *) msgText ( index );

	    tw_printf ( "CANCEL_ACTION MESSAGE \n");
	    tw_printf ( "Sender = %s @ %lf\n",
		     msgSender(index), msgSendTime ( index ) );
	    tw_printf ( "with %s ID = %d\n", m->with_whom,  m->action_id);
	    break;
	}

    case CONFIG_MSG:
    case PUCK_START:
	{
	    int i;
	    char *string;

	    string = (char *) msgText ( index );

	    tw_printf ( "PUCK_START\n");

	    for ( i = 0; ( i <100 && string[i] !='\0'); i++ )
	    {
		if ( i % 40 == 0 )
		    tw_printf (  "--\n--");

		tw_printf (  "%c", string[i] );
	    }
	    break;
	}

    case NEW_TRAJECTORY:
	{
	    infoMsg *m;
	    
	    m = (infoMsg *) msgText ( index );

	    tw_printf ( "NEW_TRAJECTORY \n");
	    tw_printf ( "Sender = %s @ %lf\n",
		     msgSender( index ), msgSendTime ( index ) );
	    tw_printf ( "Puck = %s \n", m->puck_name);
	    printCircle (&m->Puck_State);

	    break;
	}

    case COLLIDE_CUSHION:
	{
	    actionMsg *m;
	    
	    m = (actionMsg *) msgText ( index );

	    tw_printf ( "COLLIDE_CUSHION \n");
	    tw_printf ( "Sender = %s @ %lf\n",
		     msgSender( index ), msgSendTime ( index ) );

	    tw_printf ( "Puck = %s  ID = %d Vtime = %lf \n",
		     m->with_whom, m->action_id, m->action_time.simtime );
	    printCircle (&m->Puck_State);

	    break;
	}

    case COLLIDE_PUCK:
	{
	    actionMsg *m;
	    
	    m = (actionMsg *) msgText ( index );

	    tw_printf ( "COLLIDE_PUCK \n");
	    tw_printf ( "Sender = %s @ %lf\n",
		     msgSender( index ), msgSendTime ( index ) );

	    tw_printf ( "Puck = %s ID = %d  Vtime = %ld \n",
		     m->with_whom, m->action_id, m->action_time );
	    printCircle (&m->Puck_State);

	    break;
	}



    case ENTER_SECTOR:
	{
	    actionMsg *m;
	    
	    m = (actionMsg *) msgText ( index );

	    tw_printf ( "ENTER_SECTOR \n");
	    tw_printf ( "Sender = %s @ %lf\n",
		     msgSender( index ), msgSendTime ( index ) );
	    tw_printf ( "Sector = %s ID = %d Vtime = %lf \n",
		     m->with_whom, m->action_id, m->action_time.simtime );
	    printCircle (&m->Puck_State);

	    break;
	}



    case DEPART_SECTOR:
	{
	    actionMsg *m;
	    
	    m = (actionMsg *) msgText ( index );

	    tw_printf ( "DEPART_SECTOR \n");
	    tw_printf ( "Sender = %s @ %lf\n",
		     msgSender( index ), msgSendTime ( index ) );
	    tw_printf ( "Sector = %s ID = %d Vtime = %lf \n",
		     m->with_whom, m->action_id, m->action_time.simtime );
	    printCircle (&m->Puck_State);

	    break;
	}

    case UPDATE_SELF:
	{
	    tw_printf ( "UPDATE_SELF \n");
	    tw_printf ( "Sender = %s @ %lf\n",
		     msgSender( index ), msgSendTime ( index ) );

	    break;
	}


    case INITIAL_VELOCITY:
	{
	    infoMsg  *m;
	    
	    m = (infoMsg *) msgText ( index );
	    
	    tw_printf ( "INITIAL_VELOCITY \n");
	    tw_printf ( "Sender = %s @ %lf\n",
		     msgSender( index ), msgSendTime ( index ) );
	    tw_printf ( "Puck = %s\n", m->puck_name );
	    printCircle (&m->Puck_State);

	    break;
	}


    case CHANGE_VELOCITY:
	{
	    infoMsg  *m;
	    
	    m = (infoMsg *) msgText ( index );

	    tw_printf ( "CHANGE_VELOCITY \n");
	    tw_printf ( "Sender = %s @ %lf\n",
		     msgSender( index ), msgSendTime ( index ) );
	    tw_printf ( "Puck = %s\n", m->puck_name );
	    printCircle (&m->Puck_State);

	    break;
	}
    default:
	{
	    tw_printf ( "DEFAULT_CASE \n");

	    return FALSE;
	    break;
	}
    }
    return TRUE;

}


/*???PJH
#ifdef SIMULATOR
extern FILE	*m_file;

int Write_Msg_Sel ( index  )
int index;
{
     switch ( msgSelector ( index )  )
     {
	case  CANCEL_ACTION:
	    tw_fprintf ( m_file, "\tC_I_M \n");
	    break;

	case CONFIG_MSG:
	case  PUCK_START:
	    tw_fprintf ( m_file, "\tB_S \n");
	    break;

	case NEW_TRAJECTORY:
	    tw_fprintf ( m_file, "\tE_N_T \n");
	    break;

	case COLLIDE_CUSHION:
	    tw_fprintf ( m_file, "\tC_W_C \n");
	    break;

	case COLLIDE_PUCK:
	    tw_fprintf ( m_file, "\tC_W_B \n");
	    break;

	case ENTER_SECTOR:
	    tw_fprintf ( m_file, "\tE_S \n");
	    break;

	case DEPART_SECTOR:
	    tw_fprintf ( m_file, "\tD_S \n");
	    break;

	case DISPLAY_SELF:
	    tw_fprintf ( m_file, "\tDIS \n");
	    break;

	case INITIAL_VELOCITY:
	    tw_fprintf ( m_file, "\tI_V \n");
	    break;

	case CHANGE_VELOCITY:
	    tw_fprintf ( m_file, "\tC_V \n");
	    break;

	default:
	    tw_fprintf ( m_file, "\n");
	    break;
    }
}
#endif
*/
