#ifndef UAVCAN_EQUIPMENT_AIR_DATA_STATICTEMPERATURE_1029_H
#define UAVCAN_EQUIPMENT_AIR_DATA_STATICTEMPERATURE_1029_H

#include "dronecan_communication.h"

#define UAVCAN_EQUIPMENT_AIR_DATA_STATICTEMPERATURE_ID 1029
#define UAVCAN_EQUIPMENT_AIR_DATA_STATICTEMPERATURE_SIGNATURE 0x19932AA9E9558988ULL

/*
    msg name: uavcan.equipment.air_data.StaticTemperature
    msg ID: 1029
*/
bool publish_1029_staticTemperature(float temperature_k, float variance_v2);

bool publish_1029_staticTemperatureCelsius(float temperature_c, float variance_c2);

#endif // UAVCAN_EQUIPMENT_AIR_DATA_STATICTEMPERATURE_1029_H