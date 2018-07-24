/***************************************************************/
/*	stb_debug.c	- Philip Hontalas, Jet Propulsion Laboratory		*/
/*											 January 1989						*/
/*																					*/
/*  This file contains a set of routines which prints formatted*/
/*  output ( to stdout or file ) of application dependent data.*/
/*  																				*/
/***************************************************************/


#ifdef MARK3
#include "/usr/local/src/nncp/include/cubix/stdio.h"
#else
#include <stdio.h>
#endif

#include "twcommon.h"
#include "motion.h"   /* motion ADT */
#include "stb88.h"   /* constants and typedefs common to all objects */
#include "sysdata.h"  /* initial data for each cs and css system */
#include "divconst.h" /* constants (#define's) for division */
#include "divmsg.h"   /* message data structure definitions */
#include "divdefs.h" /* must follow divmsg.h */
#include "gridmsg.h" /* grid message data structure defs */
#include "statmsg.h" /* stat object input message definitions */
#include "corpsmsg.h" /* corps object input message defs */



/*************************************************************************
 *
 *       M E S S A G E    S T R U C T U R E    D E F I N I T I O N S
 *
 *************************************************************************/
/*
 * Structures found in the input and output message lists are defined
 * in one of the *msg.h files (i.e. divmsg.h, corpsmsg.h, etc.)
 */



typedef union Output_message
        {
        Int behavior_selector;	  
        Move_order mo;					/* Div. message types	*/
        Change_velocity cv;
        Combat_assess ca;
        Initiate_battle ib;
        Suffer_attrition sa;
        Update_stats us;
        Spot_report sr;
        Send_Unit_parms sup;
        Unit_parameters up;
        Del_unit du;						/* Corps message types  */
        Init_div id;
        Add_unit au;
        Op_order oo;
        Evaluate_objectives eo;
        Move_order sm;
        Perception p;					/* Grid message types	*/
        Evaluate eval;
        Change_location change;
        Draw_graphics dg;

        } Output_message;

int	i;
	

int Print_Stb_Msg (  m  )
  Output_message	*m;

 {
	 switch ( m -> behavior_selector )
     {

/******************************************************************/
/*  This  group has  the message structures which the div			*/
/*  object type is likely to receive.										*/
/******************************************************************/


		 case  INIT_DIV :
		 {
			printf("INIT_DIV  Name= %s Posture= %d\n",
						 m->id.div_name, m->id.posture );
			printf("objective.x=  %f objective.y= %f\n",
					    m->id.objective.x, m->id.objective.y );
			printf("init_pos.x=  %f init_pos.y= %f\n",
					    m->id.init_pos.x, m->id.init_pos.y );
			printf("side= %d corps_sup= %s\n",
					    m->id.side, m->id.corps_superior );
			printf("sect_w= %d pradius= %d role= %d\n",
					    m->id.sector_width, 
						 m->id.pradius, 
						 m->id.role 
					);

	   	break;
		  }

		 case  START_MOVE :
		 {
			printf ("START_MOVE  key = %d\n",m->mo.movement_key );
		   for ( i =0; i<MAX_NUM_LEGS; i++ )
			  {
					printf ("route# %d = %f %f %f %f\n",
								i,
								m->mo.route_list[i].pos.x,
								m->mo.route_list[i].pos.y,
								m->mo.route_list[i].vel.x,
								m->mo.route_list[i].vel.y
							 );
			  }
		   printf ("legs= %d mode= %d\n",
						m->mo.num_legs_in_route,
						m->mo.move_mode
					 );
			break;
		  }

		 case  CONTINUE_MOVE :
		 {
			printf ("CONTINUE_MOVE  key = %d\n",m->mo.movement_key );
		   for ( i =0; i<MAX_NUM_LEGS; i++ )
			  {
					printf ("route# %d = %f %f %f %f\n",
								i,
								m->mo.route_list[i].pos.x,
								m->mo.route_list[i].pos.y,
								m->mo.route_list[i].vel.x,
								m->mo.route_list[i].vel.y
							 );
			  }
		   printf ("legs= %d mode= %d\n",
						m->mo.num_legs_in_route,
						m->mo.move_mode
					 );
			break;
		  }
		
		 case  HALT :
		 {
			printf ("HALT\n");
			break;
		 }

		 case  CHANGE_LOCATION :
		 {
			printf ("CHANGE_LOCATION  Unit= %s\n",m->change.whereto );
			break;
		 }

		case  INITIATE_BATTLE :
		{
			printf ("INITIATE_BATTLE Unit= %s flag= %d\n",
						m->ib.UnitName,
						m->ib.enemy_sighted_flag
					 );
			break;
		}

		case  ASSESS_COMBAT :
		{
			printf ("ASSESS_COMBAT Enemy= %s\n",
						m->ca.EnemyName
					 );
			printf ("pct_sys= %f source= %d key %d\n",
						m->ca.pct_sys_to_alloc,
						m->ca.source_is,
						m->ca.combat_key
					 );
			break;
		}

		case SUFFER_LOSSES :
		{
			printf ("SUFFER_LOSSES Unit= %s\n",
						m->sa.UnitName
					 );
			/*PJH Left out Combat_systems	*/
			printf ("nb= %f %f %f %f\n",
						m->sa.north_boundary.e1.x,
						m->sa.north_boundary.e1.y,
						m->sa.north_boundary.e2.x,
						m->sa.north_boundary.e2.y
					 );
			printf ("sb= %f %f %f %f\n",
						m->sa.south_boundary.e1.x,
						m->sa.south_boundary.e1.y,
						m->sa.south_boundary.e2.x,
						m->sa.south_boundary.e2.y
					 );

			printf ("flot_xloc %f rear_pos= %f\n",
						m->sa.flot_xloc, m->sa.rear_position
					 );

			printf ("p= %f %f %f %f\n",
						m->sa.p.pos.x, m->sa.p.pos.y,
						m->sa.p.vel.x, m->sa.p.vel.y
					 );

			printf ("Posture= %d prev_strength= %f\n",
						m->sa.posture,
						m->sa.prev_strength_ratio
					 );
			break;
		}
		case PERCEIVE :
		{
			printf ("PERCEIVE Unit= %s \n",
						m->p.UnitName
					 );
			printf ("Where= %f %f %f %f time= %ld\n",
						m->p.where.pos.x,
						m->p.where.pos.y,
						m->p.where.vel.x,
						m->p.where.vel.y,
						m->p.time
					 );

			break;
	   }

		case OP_ORDER :
		{
			printf ("OP_ORDER pos= %d obj= %f\n",
						m->oo.posture,
						m->oo.objective
					 );
			for ( i=0; i<MAX_NUM_LEGS; i++ )
			{
				printf ("leg#%d %f %f %f %f\n",
							m->oo.movement_path[i].pos.x,
							m->oo.movement_path[i].pos.y,
							m->oo.movement_path[i].vel.x,
							m->oo.movement_path[i].vel.y
						 );
			}
		break;
		}

		case SPOT_REPORT :
		{
			printf ("SPOT_REPORT div= %s flot= %f\n",
						m->sr.div, m->sr.flot
					 );
			printf ("cur_str= %f full_str= %f\n",
						m->sr.current_str,
						m->sr.full_up_str
					 );
 			printf ("p= %f %f %f %f\n",
						m->sr.p.pos.x,
						m->sr.p.pos.y,
						m->sr.p.vel.x,
						m->sr.p.vel.y
					 );

			/*PJH Left out enemey names for now	*/

		break;
		}

/******************************************************************/
/*  This  group has the message structures which the corps			*/
/*  object type is likely to receive.										*/
/******************************************************************/
		case EVALUATE_OBJECTIVES :
		{
			printf ("EVALUATE_OBJECTIVES\n");
		   break;
		}

		case CORPS_INTEL :
		{
			printf ("CORPS_INTEL\n");
		   break;
		}
 
		case DIV_SPOT_REPORT:
		{
			printf ("DIV_SPOT_REPORT Unit= %s flot= %f cur_str= %f f_str= %f\n",
						m->sr.div,
						m->sr.flot,
						m->sr.current_str,
						m->sr.full_up_str
					 );
			printf ("P= %f %f %f %f Posture= %d\n",
						m->sr.p.pos.x,
						m->sr.p.pos.y,
						m->sr.p.vel.x,
						m->sr.p.vel.y,
						m->sr.posture
					 );
			break;

		}

/*PJH Left out the enemies list	*/


		 default :
		  	{
				return FALSE;
			}	
	  }
	return TRUE;
  }


/*???PJH
#ifdef SIMULATOR

int Write_Msg_Sel ( m  )

  MessageType	*m;

 {


     switch ( m -> selector )
     {

	case  CANCEL_INTERACTION :
	 {

	    fprintf (
		       m_file, 
		       "\tC_I_M \n"
		    );

	    break;

	 } 

	case  PUCK_START :
	 {

	    fprintf (
		       m_file, 
		       "\tB_S \n"
		    );

	    break;
	    
	 }

	case EXAMINE_NEW_TRAJECTORY :
	 {
	   
	    fprintf (
		       m_file, 
		       "\tE_N_T \n"
		    );

	    break;
	 }

	case COLLIDE_WITH_CUSHION :
	 {
	   
	    fprintf (
		       m_file, 
		       "\tC_W_C \n"
		    );

	    break;
	 }

	case COLLIDE_WITH_PUCK :
	 {
	    fprintf (
		       m_file, 
		       "\tC_W_B \n"
		    );

	    break;
	 }

				  

	case ENTER_SECTOR :
	 {
	    fprintf (
		       m_file, 
		       "\tE_S \n"
		    );

	    break;
	 }

	

	case DEPART_SECTOR :
	 {
	    fprintf (
		       m_file, 
		       "\tD_S \n"
		    );

	    break;
	 }

	case DISPLAY_SELF :
	 {
	    fprintf (
		       m_file, 
		       "\tDIS \n"
		    );

	    break;
	 }


	case INITIALIZE_VELOCITY :
	 {
	    fprintf (
		       m_file, 
		       "\tI_V \n"
		    );

	    break;
	 }


	case CHANGE_VELOCITY :
	 {
	    fprintf (
		       m_file, 
		       "\tC_V \n"
		    );

	    break;
	 }
	default :
	 {

	    fprintf ( m_file, "\n");
 
	    break;
	 }
      }

  }

#endif
	*/

