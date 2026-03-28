#ifndef UAVCAN_EQUIPMENT_AIR_DATA_STATICPRESSURE_1028_H
#define UAVCAN_EQUIPMENT_AIR_DATA_STATICPRESSURE_1028_H

#include "dronecan_communication.h"

#define UAVCAN_EQUIPMENT_AIR_DATA_STATICPRESSURE_ID 1028
#define UAVCAN_EQUIPMENT_AIR_DATA_STATICPRESSURE_SIGNATURE 0x44DC4133A6B487BAULL

/*
    msg name: uavcan.equipment.air_data.StaticPressure
    msg ID: 1028
*/
bool publish_1028_staticPressure(float pressure_pa, float variance_pa2);

#endif // UAVCAN_EQUIPMENT_AIR_DATA_STATICPRESSURE_1028_H