#include "dronecan_accepter.h"
#include "dronecan_dna_receiver.h"
#include "canard.h"

#include "messages/uavcan.protocol.GetNodeInfo-1.h"
#include "messages/uavcan.protocol.RestartNode-5.h"
#include "messages/uavcan.protocol.param.GetSet-11.h"
#include "messages/uavcan.protocol.param.ExecuteOpcode-10.h"
#include "messages/uavcan.protocol.file.BeginFirmwareUpdate-40.h"
#include "messages/uavcan.protocol.file.Read-48.h"
#include "messages/uavcan.protocol.dynamic_node_id.Allocation-1.h"
#include "messages/uavcan.protocol.GetTransportStats-4.h"

bool should_accept_transfer(const CanardInstance *ins, uint64_t *out_data_type_signature, uint16_t data_type_id, CanardTransferType transfer_type, uint8_t source_node_id)
{
    if (canardGetLocalNodeID(ins) == CANARD_BROADCAST_NODE_ID) // node 0 will work only with allocation
    {
        return should_accept_transfer_for_dna(ins, out_data_type_signature, data_type_id, transfer_type, source_node_id);
    }
    else if (transfer_type == CanardTransferTypeRequest)
    {
        switch (data_type_id)
        {
        case UAVCAN_GET_NODE_INFO_ID:
            *out_data_type_signature = UAVCAN_GET_NODE_INFO_SIGNATURE;
            return true;
        case UAVCAN_RESTART_NODE_ID:
            *out_data_type_signature = UAVCAN_RESTART_NODE_SIGNATURE;
            return true;
        case UAVCAN_PARAM_GETSET_ID:
            *out_data_type_signature = UAVCAN_PARAM_GETSET_SIGNATURE;
            return true;
        case UAVCAN_PROTOCOL_PARAM_EXECUTE_OPCODE_ID:
            *out_data_type_signature = UAVCAN_PROTOCOL_PARAM_EXECUTE_OPCODE_SIGNATURE;
            return true;
        case UAVCAN_FILE_BEGIN_FIRMWARE_UPDATE_ID:
            *out_data_type_signature = UAVCAN_FILE_BEGIN_FIRMWARE_UPDATE_SIGNATURE;
            return true;
        case UAVCAN_PROTOCOL_GET_TRANSPORT_STATS_ID:
            *out_data_type_signature = UAVCAN_PROTOCOL_GET_TRANSPORT_STATS_SIGNATURE;
            return true;
        }
    }
    else if (transfer_type == CanardTransferTypeResponse)
    {
        switch (data_type_id)
        {
        case UAVCAN_FILE_READ_ID:
            *out_data_type_signature = UAVCAN_FILE_READ_SIGNATURE;
            return true;
        }
    }
    return false;
}