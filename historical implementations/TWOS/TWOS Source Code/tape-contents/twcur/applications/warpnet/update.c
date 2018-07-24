/* update.c -- map updating routines for warpnet */

#include "twcommon.h"
#include "mapconst.h"
#include "msgs.h"
#include "map_defs.h"
#include "warpnet.h"
#include "debug.h"

 
void update_me(ps)
   State *ps;
{
   Int mp,j, set_line_pointer();
 
   ps->Node_Map[ps->mynum].Line_Currency = ps->now1;
   mp = set_line_pointer(ps->mynum, ps);

   for (j=0;j<ps->Node_Map[ps->mynum].Number_Lines;j++)
      if (ps->Line_Map[mp+j].Load < ps->now1)
         ps->Line_Map[mp+j].Load = ps->now1;
 
}   /* end of update_me */

/*********************************************/
  
void update_map(updata, ps)                     
   Update_Info *updata;
   State *ps;
{
  Int node_checking,lp, mp, i, j, set_line_pointer();
  Name_object to_who;

#ifdef Q_DEBUG        
  print_query(ps);
#endif

  lp = 0;
  for (i=0;i<NODES_IN_UPDATE;i++)
     {
     node_checking = updata->Nodes_To_Update[i];

     if (ps->Node_Map[node_checking].Line_Currency <
         updata->Node_Currencies[i])
       { 
       ps->Node_Map[node_checking].Line_Currency = updata->Node_Currencies[i];
       mp = set_line_pointer(node_checking, ps);
       for (j=0;j<ps->Node_Map[node_checking].Number_Lines;j++,lp++)
          ps->Line_Map[mp+j].Load = updata->Line_Loads[lp];
       }  /* end if ps->Node_Map */
     else
        lp += ps->Node_Map[node_checking].Number_Lines;
         
     }  /* end for i */                                 

#ifdef Q_DEBUG          
  print_query(ps);
#endif
}  /* end update_map */
 
/*********************************************/

void check_sender(Packet,ps)
   Message_Packet *Packet;
   State *ps;
{
   Int t1,t2,t3;
   void update_sender();
   extern void shortest_path();
 
   t1 = atoi(Packet->Last_Node);
   t2 = atoi(Packet->Destination_Node);
 
   shortest_path(t1,t2,&t3,ps);
 
#ifdef REASON_UPDATE
   if (ps->mynum != t3) print_reason(ps, t1, t2, t3);
#endif
 
   if (ps->mynum != t3)
      update_sender(ps, Packet->Last_Node);
}

/*********************************************/

void update_sender(ps, node_to_update)
   State *ps;
   Name_object node_to_update;
{
   Update_Output Output_Msg;
   void set_nodes_to_update();

   clear(&Output_Msg, sizeof(Output_Msg));
   Output_Msg.Msg_Type = UPDATE;
   set_nodes_to_update(&Output_Msg.Update, ps);
				 
#ifdef REASON_UPDATE
   print_updates(ps, Output_Msg.Update.Nodes_To_Update);
#endif


	tell(node_to_update, ps->now1+1, UPDATE, sizeof(Output_Msg),
				(char *)(&Output_Msg));
}

/*********************************************/
  
void set_nodes_to_update(updata,ps)
   Update_Info *updata;
   State *ps;
{
   Int mental_ward,i,node_checking, last_node, lp, next_node, in_list(), 
       Nodes_Checked[NUMBER_NODES], line_index, set_line_pointer();
   void add_to_list();
   extern Int senile();
 
   /* init */

   mental_ward = 0;
   node_checking = 0;
   last_node = 1;
   Nodes_Checked[0] = ps->mynum;
   line_index = 0;
 
   /* place yourself in the list of already checked nodes */
 
   add_to_list(ps->mynum, &line_index, &mental_ward, updata,ps);
 
   /* loop 'til node_checking=last_node or 'til mental ward full */
   /* put non-senile nodes into the list */
 
   while ( (node_checking != last_node)&&(mental_ward < NODES_IN_UPDATE) )
      {  
      lp = set_line_pointer(Nodes_Checked[node_checking], ps);
 
      for (i=0;i<ps->Node_Map[Nodes_Checked[node_checking]].Number_Lines;i++)
          {
          if (mental_ward>=NODES_IN_UPDATE) break;
          next_node = ps->Line_Map[lp+i].Node_To;

          if (in_list(next_node,last_node,Nodes_Checked)==0)
             /* next_node not in Nodes_Checked */
             {
             Nodes_Checked[last_node++] = next_node;
             if ( senile(next_node, ps)==0 )
                add_to_list(next_node, &line_index, &mental_ward, updata,ps);
             } /* end if in_list */
 
          } /* end for i */
      node_checking++;
      } /* end while */
   
   /* now it is time to fill in the left over spaces in the list */
   /* with the closest(physically) senile nodes */
 
   next_node = 1;      /* skip over yourself (no 0 element check) */
   while (mental_ward<NODES_IN_UPDATE) /* fill up mental ward */
      {
      while ( senile(Nodes_Checked[next_node], ps)==0 )
         next_node++;    /* This won't overflow since Q_NUM < NUM_NODES */
      add_to_list(Nodes_Checked[next_node], &line_index, &mental_ward,
                                                                 updata,ps);
      next_node++;
      } /* end while mental_ward */
 
} /* set_nodes_to_update */

/*********************************************/

void add_to_list(node, line_index, mental_ward, updata, ps)
   Int node, *line_index, *mental_ward;
   Update_Info *updata;
   State *ps;
{
   Int i, lp, set_line_pointer();

   updata->Nodes_To_Update[*mental_ward] = (short)node;

   updata->Node_Currencies[(*mental_ward)++] =
                           (short)ps->Node_Map[node].Line_Currency;

   lp = set_line_pointer(node, ps);

   for (i=0;i<ps->Node_Map[node].Number_Lines; i++, (*line_index)++)
      updata->Line_Loads[*line_index] = (short)ps->Line_Map[lp+i].Load;
}

/*********************************************/
Int in_list(node_to_check, number_in_list, list)
   Int node_to_check, number_in_list, list[];
{
   Int i;

   for (i=0;i<number_in_list;i++)
      if (node_to_check == list[i])
         return 1;

   return 0;
}
