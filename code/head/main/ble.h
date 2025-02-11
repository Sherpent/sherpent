//
// Created by Mathieu Durand on 2025-02-11.
//

#ifndef SEGMENT_BLE_H
#define SEGMENT_BLE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Attributes State Machine */
enum
{
    IDX_SVC,

    IDX_CHAR_CHANNEL,
    IDX_CHAR_VAL_CHANNEL,

    IDX_CHAR_ID,
    IDX_CHAR_VAL_ID,

    HRS_IDX_NB,
};

#endif //SEGMENT_BLE_H
