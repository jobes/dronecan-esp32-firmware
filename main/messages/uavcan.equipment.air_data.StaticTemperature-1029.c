#include "uavcan.equipment.air_data.StaticTemperature-1029.h"

bool publish_1029_staticTemperature(_Float16 temperature_k, _Float16 variance_v2)
{
    static uint8_t transfer_id = 0;
    uint8_t buffer[4] = {0};

    canardEncodeScalar(buffer, 0, 16, &temperature_k);
    canardEncodeScalar(buffer, 16, 16, &variance_v2);

    return dronecan_broadcast(UAVCAN_EQUIPMENT_AIR_DATA_STATICTEMPERATURE_SIGNATURE,
                              UAVCAN_EQUIPMENT_AIR_DATA_STATICTEMPERATURE_ID,
                              CANARD_TRANSFER_PRIORITY_LOW,
                              buffer,
                              sizeof(buffer),
                              &transfer_id);
}

bool publish_1029_staticTemperatureCelsius(_Float16 temperature_c, _Float16 variance_c2)
{
    _Float16 temperature_k = temperature_c + 273.15f;
    return publish_1029_staticTemperature(temperature_k, variance_c2);
}