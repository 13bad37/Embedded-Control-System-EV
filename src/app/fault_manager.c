#include "app/fault_manager.h"

void FaultManager_Init(void)
{
    /* Fault handling is just a placeholder rn, decide on safety rules first. */
}

void FaultManager_Run(void)
{
    /* TODO: check fault inputs and how safe shutdown works later. */
}

void FaultManager_RecordFault(EvFaultCode_t fault)
{
    (void)fault;
    /* TODO: store/report faults once we come up with a fault model*/
}

