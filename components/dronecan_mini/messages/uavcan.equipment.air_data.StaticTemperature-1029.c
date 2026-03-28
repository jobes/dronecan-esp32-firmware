#include "uavcan.equipment.air_data.StaticTemperature-1029.h"

bool publish_1029_staticTemperature(float temperature_k, float variance_v2)
{
    static uint8_t transfer_id = 0;
    uint8_t buffer[4] = {0};

    uint16_t temp_f16 = canardConvertNativeFloatToFloat16(temperature_k);
    uint16_t var_f16 = canardConvertNativeFloatToFloat16(variance_v2);
    canardEncodeScalar(buffer, 0, 16, &temp_f16);
    canardEncodeScalar(buffer, 16, 16, &var_f16);

    return dronecan_broadcast(UAVCAN_EQUIPMENT_AIR_DATA_STATICTEMPERATURE_SIGNATURE,
                              UAVCAN_EQUIPMENT_AIR_DATA_STATICTEMPERATURE_ID,
                              CANARD_TRANSFER_PRIORITY_LOW,
                              buffer,
                              sizeof(buffer),
                              &transfer_id);
}

bool publish_1029_staticTemperatureCelsius(float temperature_c, float variance_c2)
{
    float temperature_k = temperature_c + 273.15f;
    return publish_1029_staticTemperature(temperature_k, variance_c2);
}