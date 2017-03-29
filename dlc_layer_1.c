//
//  dlc_layer.c
//  CSE3461_Lab1
//
//  Colin Dolan on 5/19/16.
//  CSE 3461
//  Prof. Babic
//
/* ----- DO NOT REMOVE OR MODIFY ----- */
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

static dlc_layer_receive(DLC_LAYER_ENTITY_TYPE *dlc_layer_entity, GENERIC_LAYER_ENTITY_TYPE *generic_layer_entity, PDU_TYPE *pdu)
{
    if (DatalinkFromApplication(generic_layer_entity)) {
        FromApplicationToDatalink(dlc_layer_entity, pdu);
    } else if (DatalinkFromPhysical(generic_layer_entity)) {
        FromPhysicalToDatalink(dlc_layer_entity, pdu);
    }
    return 0;
}

/* ----- START HERE AND COMPLETE THE FOLLOWING TWO FUNCTIONS ----- */


FromApplicationToDatalink(DLC_LAYER_ENTITY_TYPE *dlc_layer_entity, PDU_TYPE *pdu_from_application)
{
    PDU_TYPE *pdu_to_physical; /* use this with pdu_alloc() */
    
    // Just a sanity check
    if(pdu_from_application->type!=TYPE_IS_A_PDU) cse_panic("Empty a_pdu\n");
    
    // Create d_pdu using pdu_alloc()
    pdu_to_physical = pdu_alloc();
    pdu_to_physical->type = TYPE_IS_D_PDU;
    
    // update address field, a_pdu field and error field;
    int receive_address = GetReceiverID( dlc_layer_entity );
    //printf("address= %d",&node_address);
    pdu_to_physical->u.d_pdu.address = receive_address;
    pdu_to_physical->u.d_pdu.a_pdu = pdu_from_application->u.a_pdu;
    pdu_to_physical->u.d_pdu.error = NO;

    //Send to physical layer;
    send_pdu_to_physical_layer(dlc_layer_entity, pdu_to_physical);
     
    
    //Free a_pdu;
    pdu_free(pdu_from_application);
    
    
    return 0;
}


FromPhysicalToDatalink(DLC_LAYER_ENTITY_TYPE *dlc_layer_entity,
                       PDU_TYPE *pdu_from_physical)
{
    PDU_TYPE *pdu_to_application;
    
    /* just a sanity check */
    if (pdu_from_physical->type != TYPE_IS_D_PDU) cse_panic("Empty d_pdu\n");
    
    
    pdu_to_application = pdu_alloc();
    pdu_to_application->type = TYPE_IS_A_PDU;
    
    //Get address of node receiving PDU to check if PDU address is correct
    int node_address = GetNodeID( dlc_layer_entity );
    
    //If no error and address is correct, set value of new a_pdu to a_pdu field of received d_pdu
    //Then send a_pdu to application
    if (pdu_from_physical->u.d_pdu.error == NO && pdu_from_physical->u.d_pdu.address == node_address) {
        pdu_to_application->u.a_pdu = pdu_from_physical->u.d_pdu.a_pdu;
        send_pdu_to_application_layer(dlc_layer_entity, pdu_to_application);
    } else {
        printf("Error in transmission\n");
    }
    
    //Free PDU after use or if error is found
    pdu_free(pdu_from_physical);
    
    return 0;
}
