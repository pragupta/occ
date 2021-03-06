/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/common/mca_addresses.h $                                  */
/*                                                                        */
/* OpenPOWER OnChipController Project                                     */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2015,2017                        */
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

#ifndef _MCA_ADDRESSES_H
#define _MCA_ADDRESSES_H

#define NUM_NIMBUS_MC_PAIRS 2
#define MAX_NUM_MCU_PORTS   4

// Base Address of NIMBUS MCA.
#define DIMM_MCA_BASE_ADDRESS     0x07010800

#define MCA_MCPAIR_SPACE          0x01000000
#define MCA_PORT_SPACE            0x40
#define MC_PORT_SPACE(mc,port)    ((MCA_MCPAIR_SPACE * (mc)) + ( MCA_PORT_SPACE * (port)))


#define POWER_CTRL_REG0_OFFSET    0x0134
#define POWER_CTRL_REG0_ADDRESS   (DIMM_MCA_BASE_ADDRESS + POWER_CTRL_REG0_OFFSET)

#define STR_REG0_OFFSET           0x0135
#define STR_REG0_ADDRESS          (DIMM_MCA_BASE_ADDRESS + STR_REG0_OFFSET)


#define N_M_TCR_OFFSET            0x0116
#define N_M_TCR_ADDRESS           (DIMM_MCA_BASE_ADDRESS + N_M_TCR_OFFSET)


#define DEADMAN_TIMER_OFFSET      0x013C
#define DEADMAN_TIMER_ADDRESS     (DIMM_MCA_BASE_ADDRESS + DEADMAN_TIMER_OFFSET)


// Memory Power Control

//Power Control Register 0 and STR Register 0 there are 4 each (1 per MCU port)
//OCC knows present MCU ports from the memory throttle config packet

//Power control reg 0:  MCP.PORT#.SRQ.PC.MBARPC0Q
//STR reg 0:            MCP.PORT#.SRQ.PC.MBASTR0Q

/*                                 PWR_CTRL/STR REG   Power Ctl reg 0     STR reg 0
MC/Port Address MCA Port Address    Control Addr       SCOM Address     SCOM Address
mc01.port0        0x07010800        + 0x00000134/5     = 0x07010934     = 0x07010935
mc01.port1        0x07010840        + 0x00000134/5     = 0x07010974     = 0x07010975
mc01.port2        0x07010880        + 0x00000134/5     = 0x070109B4     = 0x070109B5
mc01.port3        0x070108C0        + 0x00000134/5     = 0x070109F4     = 0x070109F5
mc23.port0        0x08010800        + 0x00000134/5     = 0x08010934     = 0x08010935
mc23.port1        0x08010840        + 0x00000134/5     = 0x08010974     = 0x08010975
mc23.port2        0x08010880        + 0x00000134/5     = 0x080109B4     = 0x080109B5
mc23.port3        0x080108C0        + 0x00000134/5     = 0x080109F4     = 0x080109F5
 */

#define POWER_CTRL_REG0(mc,port) (POWER_CTRL_REG0_ADDRESS + MC_PORT_SPACE(mc,port))

#define STR_REG0(mc,port) (STR_REG0_ADDRESS + MC_PORT_SPACE(mc,port))


// DIMM Control
/*
MC/Port Address MCA Port Address    Control Addr        SCOM Address
mc01.port0        0x07010800        + 0x00000116        = 0x07010916
mc01.port1        0x07010840        + 0x00000116        = 0x07010956
mc01.port2        0x07010880        + 0x00000116        = 0x07010996
mc01.port3        0x070108C0        + 0x00000116        = 0x070109D6
mc23.port0        0x08010800        + 0x00000116        = 0x08010916
mc23.port1        0x08010840        + 0x00000116        = 0x08010956
mc23.port2        0x08010880        + 0x00000116        = 0x08010996
mc23.port3        0x080108C0        + 0x00000116        = 0x080109D6
 */

//  N/M DIMM Throttling Control SCOM Register Addresses macro
#define N_M_DIMM_TCR(mc,port) (N_M_TCR_ADDRESS + MC_PORT_SPACE(mc,port))

/*
MC/Port Address MCA Port Address   Deadman Offset       SCOM Address
mc01.port0        0x07010800        + 0x0000013C        = 0x0701093C
mc01.port1        0x07010840        + 0x0000013C        = 0x0701097C
mc01.port2        0x07010880        + 0x0000013C        = 0x070109BC
mc01.port3        0x070108C0        + 0x0000013C        = 0x070109FC
mc23.port0        0x08010800        + 0x0000013C        = 0x0801093C
mc23.port1        0x08010840        + 0x0000013C        = 0x0801097C
mc23.port2        0x08010880        + 0x0000013C        = 0x080109BC
mc23.port3        0x080108C0        + 0x0000013C        = 0x080109FC
 */

//  NIMBUS DIMM Deadman SCOM Register Addresses macro
#define DEADMAN_TIMER_PORT(mc,port) (DEADMAN_TIMER_ADDRESS +  MC_PORT_SPACE(mc,port))

#define DEADMAN_TIMER_MCA(mca) (DEADMAN_TIMER_ADDRESS + MC_PORT_SPACE((mca>>2),(mca&3)))

#endif // _MCA_ADDRESSES_H
