/* ----------------------------------------------------
 * Name			: Colin Dolan
 * e-mail		: dolan.59@osu.edu
 * CSE account	: dolan.59
 * ---------------------------------------------------- */

/* --- DO NOT REMOVE OR MODIFY  #inclode STATEMENTS BELOW --- */

#include "cisePort.h"
#include "sim.h"
#include "component.h"
#include "comptypes.h"
#include "list.h"
#include "eventdefs.h"
#include "main.h"
#include "route_activity.h"
#include "sim_tk.h"
#include "dlc_layer.h"

/****************************************************/
/* --- YOU DO NOT HAVE TO HAVE THIS FUNCTION --- */
static int
window_open(DLC_Conn_Info_TYPE *dci)
{
      int result;
      /* Based on a number of a_pdu's in the transmission buffer
      and values for snd_nxt, snd_una and window_size
      determine if there is a new a_pdu ready to be sent */
      int numOfPDUs = DataInPDUBuffer(dci);

      printf("Window Open: ");
      printf("%d\n", numOfPDUs);

      if (dci->snd_nxt - dci->snd_una < dci->window_size && numOfPDUs > 0) {
            printf("PDU ready to send\n");
            result = 1;
        } else {
            printf("No PDU in buffer\n");
            result = 0;
        }

 return result; // result = 1, there is an a_pdu ready
                // result = 0, there is not
}

/**************************************************************/
/* --- DO NOT REMOVE OR MODIFY THIS FUNCTION --- */
static
dlc_layer_receive(DLC_LAYER_ENTITY_TYPE *dlc_layer_entity,
		  GENERIC_LAYER_ENTITY_TYPE *generic_layer_entity,
		  PDU_TYPE *pdu)
{
	/* Gets the appropriate  DLC_Conn_Info_TYPE structure */
	DLC_Conn_Info_TYPE *dci;
	dci = Datalink_Get_Conn_Info(dlc_layer_entity,pdu);

	if (DatalinkFromApplication(generic_layer_entity)) {
	    FromApplicationToDatalink(dlc_layer_entity, pdu, dci);
	} else if (DatalinkFromPhysical(generic_layer_entity)) {
	    FromPhysicalToDatalink(dlc_layer_entity, pdu, dci);
	}
	return 0;
}

/**************************************************************/
/* --- YOU MUST HAVE THIS FUNCTION --- */
static
FromApplicationToDatalink(DLC_LAYER_ENTITY_TYPE *dlc_layer_entity, PDU_TYPE *pdu_from_application,
				   DLC_Conn_Info_TYPE *dci)
{
    printf("Application to datalink\n");
  /* Insert the pdu from the application layer to the
     transmission buffer. */
     InsertPDUIntoBuffer(dlc_layer_entity, pdu_from_application, dci);

     // If possible send info frame.
     if (window_open(dci) > 0) {
       SendInfo(dlc_layer_entity, dci);
     }
}

/**************************************************************/
/* --- YOU MUST HAVE THIS FUNCTION --- */
static
FromPhysicalToDatalink(DLC_LAYER_ENTITY_TYPE *dlc_layer_entity,
					  PDU_TYPE *pdu_from_physical,
					  DLC_Conn_Info_TYPE *dci)
{
    printf("Physical to data link\n");

    int node_address = GetNodeID( dlc_layer_entity );
	     /* Check and discard the pdu when error is detected or P/F
        bit is set */
        if (pdu_from_physical->u.d_pdu.error == NO && pdu_from_physical->u.d_pdu.address == node_address) {
          /* If not discarded, check d_pdu.type and call an
                appropriate function: */
            if (pdu_from_physical->u.d_pdu.type == D_INFO) {
              //if INFO you may use and call DataLinkProcessINFO()
              DatalinkProcessInfo(dlc_layer_entity, pdu_from_physical, dci);

            } else if (pdu_from_physical->u.d_pdu.type == D_RR) {
              //if RR you may use and call DataLinkProcessRR()
              DatalinkProcessRR(dlc_layer_entity, pdu_from_physical, dci);

            } else if (pdu_from_physical->u.d_pdu.type == D_REJ) {
              //if REJ you may use and call DataLinkProcessREJ()
              DatalinkProcessREJ(dlc_layer_entity, pdu_from_physical, dci);

            } else {
              printf("Error: Unexpected PDU Type\n");
            }

        } else {
            printf("Error in transmission: ");
            printf("%d", &pdu_from_physical->u.d_pdu.error);
            printf( "%d\n", node_address);
            pdu_free(pdu_from_physical);
        }
}

/**************************************************************/
/* --- YOU DO NOT HAVE TO HAVE THIS FUNCTION --- */
static
DatalinkProcessRR(DLC_LAYER_ENTITY_TYPE *dlc_layer_entity,
				  PDU_TYPE *pdu,
				  DLC_Conn_Info_TYPE *dci)
{
     printf("Process RR\n");
     /* Check the address and if not correct discard frame and
         return 0;   */
         int node_address = GetNodeID( dlc_layer_entity );

         if(pdu->u.d_pdu.address == node_address) {

           //If this is a response RR with P/F_bit=0:
           if(pdu->u.d_pdu.p_bit == 0 && dci->rr_pbit_sent == 0) {
               printf("Response RR with Pbit = 0\n");
               //Free up space in the retransmission buffer.
               UpdatePDUBuffer(dlc_layer_entity, pdu, dci);

               /* update snd_una  */
               dci->snd_una = pdu->u.d_pdu.number;

               // Send as many info pdu's as allowed by window
               if(window_open(dci) > 0) {
                   SendInfo(dlc_layer_entity, dci);
                 }

           // If this is a command RR with P/F_bit=1:
         } else if(pdu->u.d_pdu.p_bit == 1 && dci->rr_pbit_sent == 0){
                printf("Command RR with Pbit = 1\n");
                //Free up space in the retransmission buffer.
                UpdatePDUBuffer(dlc_layer_entity, pdu, dci);

                //Clear rej_already_sent
                dci->rej_already_sent = 0;

                //Update snd_una
                dci->snd_una = pdu->u.d_pdu.number;

                //Send as many info pdu's as allowed by window.
                if(window_open(dci) > 0) {
                  SendInfo(dlc_layer_entity, dci);
                }

                //Create and send to physical layer a response RR with P/F_=1
                PDU_TYPE *pdu_to_physical;
                pdu_to_physical = pdu_alloc();
                pdu_to_physical->type = TYPE_IS_D_PDU;
                pdu_to_physical->u.d_pdu.type = D_RR;
                pdu_to_physical->u.d_pdu.number = dci->rcv_nxt;

                int receive_address = GetReceiverID( dlc_layer_entity );
                pdu_to_physical->u.d_pdu.address = receive_address;
                pdu_to_physical->u.d_pdu.p_bit = 1;
                pdu_to_physical->u.d_pdu.error = NO;

                //Set rr_pbit_sent=0
                dci->rr_pbit_sent = 0;

                //Send to d_pdu to physical layer
                send_pdu_to_physical_layer(dlc_layer_entity, pdu_to_physical);

        //If this is response RR with P/F_bit=1 and rr_pbit=1:
      } else if(pdu->u.d_pdu.p_bit == 1 && dci->rr_pbit_sent == 1){
                printf("Response RR with Pbit = 1\n");
                //Free up space in the retransmission buffer.
                UpdatePDUBuffer(dlc_layer_entity, pdu, dci);

                //Clear rr_pbit_sent
                dci->rr_pbit_sent = 0;

                //Update snd_una and snd_nxt
                dci->snd_una = pdu->u.d_pdu.number;
                dci->snd_nxt = dci->snd_una;

                //Send as many info pdu's as allowed by window.
                if(window_open(dci) > 0) {
                  SendInfo(dlc_layer_entity, dci);
                }

         } else {
           printf("Error in processing RR.\n");
         }
       }
	/* Free pdu */
  pdu_free(pdu);
	return 0;
}

/*************************************************************/
/* --- YOU DO NOT HAVE TO HAVE THIS FUNCTION --- */
static
DatalinkProcessREJ(DLC_LAYER_ENTITY_TYPE *dlc_layer_entity,
				   PDU_TYPE *pdu,
				   DLC_Conn_Info_TYPE *dci)
{
         printf("Process REJ\n");
         /* Check the address and if not correct discard frame and
         return 0;   */
         int node_address = GetNodeID( dlc_layer_entity );

         if(pdu->u.d_pdu.address == node_address && pdu->u.d_pdu.p_bit == 0) {
             /* Otherwise, free up space in the retransmission buffer */
             UpdatePDUBuffer(dlc_layer_entity,pdu,dci);

             /* update snd_una and snd_nxt */
             dci->snd_una = pdu->u.d_pdu.number;
             dci->snd_nxt = pdu->u.d_pdu.number;

             /* Send as many pdu's as allowed by window */
             if(window_open(dci) > 0) {
               SendInfo(dlc_layer_entity, dci);
             }
         }

	  /* Free pdu */
    pdu_free(pdu);
    return 0;
}

/**************************************************************/
/* --- YOU DO NOT HAVE TO HAVE THIS FUNCTION --- */
static
DatalinkProcessInfo(DLC_LAYER_ENTITY_TYPE *dlc_layer_entity,
					PDU_TYPE *pdu,
					DLC_Conn_Info_TYPE *dci)
{
      printf("Process Info\n");
     /* Check the address and if not correct discard frame and
         return 0;   */
     int node_address = GetNodeID( dlc_layer_entity );

        // Check if the pdu has the expected sequence number
        if (pdu->u.d_pdu.number == dci->rcv_nxt && pdu->u.d_pdu.p_bit == 0 ) {
            //If expected PDU, then increment rcv_nxt,
            dci->rcv_nxt = (dci->rcv_nxt+1) % 7;

            //set rej_already_sent = 0 and RR is sent
            dci->rej_already_sent = 0;
            SendRR(dlc_layer_entity, pdu, dci);

            /* --- Send pdu to application : Same as Lab1 --- */
            PDU_TYPE *pdu_to_application;

            pdu_to_application = pdu_alloc();
            pdu_to_application->type = TYPE_IS_A_PDU;

            //a_pdu field of received d_pdu
            pdu_to_application->u.a_pdu = pdu->u.d_pdu.a_pdu;

            //Then send a_pdu to application
            send_pdu_to_application_layer(dlc_layer_entity, pdu_to_application);

        //If info frame was out of order check if REJ needs to be sent
        } else {
          // If REJ hasn't been sent, set rej_already_sent to 1 and send REJ
            if (dci->rej_already_sent == 0){
                dci->rej_already_sent = 1;
                SendREJ(dlc_layer_entity, pdu, dci);

            // } else {
            //     dci->rr_pbit_sent = 0;
            //     SendRR(dlc_layer_entity, pdu, dci);
            }
        }

	/* Free pdu */
  pdu_free(pdu);
	return 0;
}

/**************************************************************/
/* --- DO NOT CHANGE NAME OF THIS FUNCTION --- */
/* The function is automatically called when the timer expires*/
static
DatalinkTimerExpired(DLC_LAYER_ENTITY_TYPE *dlc_layer_entity,
					 DLC_Conn_Info_TYPE *dci)
{
  printf("Timer Expired.\n");

  /* Only if there are a_pdu in the buffer, allocate a new d_pdu
  and fill in the needed fields for a command RR with P_Fbit=1 */
  if(window_open(dci) > 0) {
      dci->snd_nxt = dci->snd_una;
      PDU_TYPE *pdu;
      pdu = pdu_alloc();
      pdu->type = TYPE_IS_D_PDU;
      pdu->u.d_pdu.type = D_RR;
      pdu->u.d_pdu.number = dci->rcv_nxt;
      int receive_address = GetReceiverID( dlc_layer_entity );
      pdu->u.d_pdu.address = receive_address;
      pdu->u.d_pdu.p_bit = 1;
      pdu->u.d_pdu.error = NO;

      //Set rr_pbit_sent=1
      dci->rr_pbit_sent = 1;

      //Send to d_pdu to physical layer
      send_pdu_to_physical_layer(dlc_layer_entity, pdu);
      SetTimer(dlc_layer_entity, dci);
  }

	return 0;
}

/**************************************************************/
/* --- YOU DO NOT HAVE TO HAVE THIS FUNCTION --- */
static
SendInfo(DLC_LAYER_ENTITY_TYPE *dlc_layer_entity,
			  DLC_Conn_Info_TYPE *dci)
{
    printf("Send Info\n");
    /* get a_pdu to send */
  	PDU_TYPE *pdu_to_send;
  	pdu_to_send = GetPDUFromBuffer(dci);

  	/* Copy it to a new d_pdu and fill the remaining fields */
    PDU_TYPE *pdu_to_physical;

    /* --- Send d_pdu to physical layer: Same as Lab1 --- */
    pdu_to_physical = pdu_alloc();
    pdu_to_physical->type = TYPE_IS_D_PDU;
    pdu_to_physical->u.d_pdu.type = D_INFO;
    pdu_to_physical->u.d_pdu.number = dci->snd_nxt;

    // update address field, a_pdu field and error field;
    int receive_address = GetReceiverID( dlc_layer_entity );
    pdu_to_physical->u.d_pdu.address = receive_address;
    pdu_to_physical->u.d_pdu.a_pdu = pdu_to_send->u.a_pdu;
    pdu_to_physical->u.d_pdu.error = NO;
    pdu_to_physical->u.d_pdu.p_bit = 0;

    // Set timer. Use:
    SetTimer(dlc_layer_entity, dci);

     //Send to physical layer;
     send_pdu_to_physical_layer(dlc_layer_entity, pdu_to_physical);

     /* increment snd_nxt */
     dci->snd_nxt = (dci->snd_nxt+1)%7;


    return 0;
}

/************************************************************/
/* --- YOU DO NOT HAVE TO HAVE THIS FUNCTION --- */
static
SendRR(DLC_LAYER_ENTITY_TYPE *dlc_layer_entity, PDU_TYPE *pdu,
	   DLC_Conn_Info_TYPE *dci)
{
      printf("Send RR\n");
    	/* Allocate a new d_pdu and fill in the needed fields */
      PDU_TYPE *pdu_to_physical;
      pdu_to_physical = pdu_alloc();
      pdu_to_physical->type = TYPE_IS_D_PDU;
      pdu_to_physical->u.d_pdu.type = D_RR;
      pdu_to_physical->u.d_pdu.number = pdu->u.d_pdu.number + 1;
      dci->snd_una = pdu_to_physical->u.d_pdu.number;
      int receive_address = GetReceiverID( dlc_layer_entity );
      pdu_to_physical->u.d_pdu.address = receive_address;
      pdu_to_physical->u.d_pdu.a_pdu = pdu->u.a_pdu;
      pdu_to_physical->u.d_pdu.error = NO;
      if (dci->rr_pbit_sent == 1) {
        dci->rr_pbit_sent = 0;
      }
	/* Send to d_pdu to physical layer */
  send_pdu_to_physical_layer(dlc_layer_entity, pdu_to_physical);

	return 0;
}
/**************************************************************/
/* --- YOU DO NOT HAVE TO HAVE THIS FUNCTION --- */
static
SendREJ(DLC_LAYER_ENTITY_TYPE *dlc_layer_entity, PDU_TYPE *pdu,
		DLC_Conn_Info_TYPE *dci)
{
      printf("Send REJ\n");
	   /* Don't send REJ if rej_already_sent = 1, but
	   send RR */
     if (dci->rej_already_sent == 1) {
       pdu->u.d_pdu.p_bit = 0;
       SendRR(dlc_layer_entity, pdu, dci);
     } else {
       /* Allocate a new d_pdu and fill in the needed fields */
       PDU_TYPE *pdu_to_physical;
       pdu_to_physical = pdu_alloc();
       pdu_to_physical->type = TYPE_IS_D_PDU;
       pdu_to_physical->u.d_pdu.type = D_REJ;
       pdu_to_physical->u.d_pdu.number = pdu->u.d_pdu.number;
       int receive_address = GetReceiverID( dlc_layer_entity );
       pdu_to_physical->u.d_pdu.address = receive_address;
       pdu_to_physical->u.d_pdu.a_pdu = pdu->u.a_pdu;
       pdu_to_physical->u.d_pdu.error = NO;
       pdu_to_physical->u.d_pdu.p_bit = 0;
       dci->rr_pbit_sent = 0;

       /* Send to d_pdu to physical layer */
       send_pdu_to_physical_layer(dlc_layer_entity, pdu_to_physical);
       /* rej_already_sent set to 1 */
       dci->rej_already_sent = 1;
     }

	return 0;
}
