/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/include/p9_config.h $                                     */
/*                                                                        */
/* OpenPOWER OnChipController Project                                     */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2015,2016                        */
/* [+] International Business Machines Corp.                              */
/*                                                                        */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/*                                                                        */
/* IBM_PROLOG_END_TAG                                                     */

/// \file p9_config.h
/// \brief Chip configuration data structures for P9 OCC procedures

#ifndef __P9_GPE_CONFIG_H__
#define __P9_GPE_CONFIG_H__


#include <stdint.h>

#define THERM_DTS_RESULT        0x00050000

typedef union dts_sensor_result_reg
{
    uint64_t value;
    struct
    {
        uint16_t  reading[2];
        uint16_t  unused_hw2;
        uint16_t  unused_hw3;
    } half_words;
} dts_sensor_result_reg_t;

typedef union sensor_result
{
    uint16_t result;
    struct
    {
        uint16_t reading : 12;
        uint16_t thermal_trip : 2;
        uint16_t spare : 1;
        uint16_t valid : 1;
    } fields;

} sensor_result_t;


/// SCOM address Ranges:
// Cores (EX chiplet): 0x20000000 - 0x37000000
// Caches: 0x10000000 - 0x15000000
//
#define CHIPLET_CORE_SCOM_BASE  0x20000000
#define CHIPLET_CACHE_SCOM_BASE 0x10000000
#define CHIPLET_NEST_SCOM_BASE  0x02000000

#define CHIPLET_CORE_ID(n) \
    (((n) << 24) + CHIPLET_CORE_SCOM_BASE)

#define CHIPLET_CACHE_ID(n) \
    (((n) << 24) + CHIPLET_CACHE_SCOM_BASE)

#define CHIPLET_NEST_ID(n) \
    (((n) << 24) + CHIPLET_NEST_SCOM_BASE)

#endif  /* __P9_GPE_CONFIG_H__ */
