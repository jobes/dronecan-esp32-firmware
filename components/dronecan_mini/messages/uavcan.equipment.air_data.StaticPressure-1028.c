#include "uavcan.equipment.air_data.StaticPressure-1028.h"

bool publish_1028_staticPressure(_Float32 pressure_pa, _Float16 variance_pa2)
{
    static uint8_t transfer_id = 0;
    uint8_t buffer[6] = {0};

    canardEncodeScalar(buffer, 0, 32, &pressure_pa);
    canardEncodeScalar(buffer, 32, 16, &variance_pa2);

    return dronecan_broadcast(UAVCAN_EQUIPMENT_AIR_DATA_STATICPRESSURE_SIGNATURE,
                              UAVCAN_EQUIPMENT_AIR_DATA_STATICPRESSURE_ID,
                              CANARD_TRANSFER_PRIORITY_MEDIUM,
                              buffer,
                              sizeof(buffer),
                              &transfer_id);
}