#ifndef UAVCAN_EQUIPMENT_AIR_DATA_STATICTEMPERATURE_1029_H
#define UAVCAN_EQUIPMENT_AIR_DATA_STATICTEMPERATURE_1029_H

#include "dronecan_communication.h"

#define UAVCAN_EQUIPMENT_AIR_DATA_STATICTEMPERATURE_ID 1029
#define UAVCAN_EQUIPMENT_AIR_DATA_STATICTEMPERATURE_SIGNATURE 0x19932AA9E9558988ULL

/*
    msg name: uavcan.equipment.air_data.StaticTemperature
    msg ID: 1029
*/
bool publish_1029_staticTemperature(_Float16 temperature_k, _Float16 variance_v2);

bool publish_1029_staticTemperatureCelsius(_Float16 temperature_c, _Float16 variance_c2);

#endif // UAVCAN_EQUIPMENT_AIR_DATA_STATICTEMPERATURE_1029_H