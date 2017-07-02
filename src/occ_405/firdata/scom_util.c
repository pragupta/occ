/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/occ_405/firdata/scom_util.c $                             */
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

/* Support for SCOM operations */
#include <fsi.h>
#include <native.h>
#include <occhw_async.h>
#include "occ_common.h"
#include "gpe_export.h"
#include <scom_util.h>
#include "scom_addr_util.h"
#include <sbe_fifo.h>

bool G_request_created = FALSE;
GPE_BUFFER(ipc_scom_op_t G_scom_op);
GpeRequest G_request;

enum {
    /*FSI addresses are byte offsets, so need to multiply by 4
      since each register is 4 bytes long.
      prefix with 0x10xx for FSI2PIB engine offset */
    DATA0_REG         = 0x1000,  /* SCOM Data Register 0 (0x00) */
    DATA1_REG         = 0x1004,  /* SCOM Data Register 1 (0x01) */
    COMMAND_REG       = 0x1008,  /* SCOM Command Register (0x02) */
    ENGINE_RESET_REG  = 0x1018,  /* Engine Reset Register (0x06) */
    STATUS_REG        = 0x101C,  /* STATUS Register (0x07) */
    PIB_RESET_REG     = 0x101C,  /* PIB Reset Register (0x07) */

    PIB_ABORT_BIT     = 0x00100000, /* 12= PIB Abort */
    PIB_ERROR_BITS    = 0x00007000, /* 17:19= PCB/PIB Errors */
};

/**
 * @brief Indirect SCOM Status
 */
typedef union
{
    uint64_t data64;
    struct
    {
        uint64_t :12;        /*0:11*/
        uint64_t addr:20;    /*12:31*/
        uint64_t done:1;     /*32*/
        uint64_t piberr:3;   /*33:35*/
        uint64_t userstat:4; /*36:39*/
        uint64_t :8;         /*40:47*/
        uint64_t data:16;    /*48:63*/
    };
} IndirectScom_t;

enum {
    MCS_MASK =             0xFFFFFFFF7FFFFF80,
    MCS_BASEADDR =         0x0000000002011800,
    MCS_DMI_BASEADDR =     0x0000000002011A00,
    IND_MCS_BASEADDR =     0x8000006002011A00,
    IND_MCS_DMI_BASEADDR = 0x8000006002011A3F,
    MBA_MASK =             0xFFFFFFFF7FFFFC00,
    MBA_BASEADDR =         0x0000000003010400,
    TCM_MBA_BASEADDR =     0x0000000003010800,
    IND_MBA_BASEADDR =     0x800000000301143f,
};

/**
 * @brief  Translates a relative unit address to a real physical address
 * @param  i_trgt Chip/unit to SCOM.
 * @param  i_addr Address to SCOM (the unit's 0th address for unit SCOMs).
 * @param  o_addr Return address.
 * @return Non-SUCCESS if an internal function fails. SUCCESS otherwise.
 */
int32_t translate_addr( SCOM_Trgt_t i_trgt, uint64_t i_addr, uint64_t * o_addr )
{
    #define FUNC "[translate_addr] "

    int32_t rc = SUCCESS;
    uint8_t l_chip_unit_num = SCOM_Trgt_getChipUnitPos(i_trgt);

    *o_addr = i_addr;

    //The following translation logic is a copy of p9_scominfo_createChipUnitScomAddr
    //function from EKB (chips/p9/common/scominfo/p9_scominfo.C)

    if(i_trgt.type == TRGT_PROC || i_trgt.type == TRGT_MEMBUF)
    {
        //No need to translate here.
        //We already assigned i_addr to o_addr above, so just return SUCCESS.
        return rc;
    }
    else if(i_trgt.type == TRGT_CAPP) //CAPP
    {
        set_chiplet_id(N0_CHIPLET_ID + (l_chip_unit_num * 2), o_addr);
    }
    else if(i_trgt.type == TRGT_XBUS) //XBUS
    {
        uint8_t l_ring_id = get_ring(i_addr) & 0xF;
        if(l_ring_id <= XB_IOX_2_RING_ID && l_ring_id >= XB_IOX_0_RING_ID)
        {
            set_ring((XB_IOX_0_RING_ID + l_chip_unit_num) & 0xF, o_addr);
        }
        else if(l_ring_id <= XB_PBIOX_2_RING_ID &&
                l_ring_id >= XB_PBIOX_0_RING_ID)
        {
            set_ring((XB_PBIOX_0_RING_ID + l_chip_unit_num) & 0xF, o_addr);
        }
    }
    else if(i_trgt.type == TRGT_OBUS) //OBUS
    {
        set_chiplet_id(OB0_CHIPLET_ID + l_chip_unit_num, o_addr);
    }
    else if(i_trgt.type == TRGT_PEC) //PEC
    {
        if(get_chiplet_id(i_addr) == N2_CHIPLET_ID)
        {
            // nest
            set_ring((N2_PCIS0_0_RING_ID + l_chip_unit_num) & 0xF, o_addr);
        }
        else
        {
            // iopci / pci
            set_chiplet_id(PCI0_CHIPLET_ID + l_chip_unit_num, o_addr);
        }
    }
    else if (i_trgt.type == TRGT_PHB) //PHB
    {
        if(get_chiplet_id(i_addr) == N2_CHIPLET_ID)
        {
            // nest
            if(l_chip_unit_num == 0)
            {
                set_ring(N2_PCIS0_0_RING_ID & 0xF, o_addr);
                set_sat_id((get_sat_id(i_addr) < 4) ?  1 : 4, o_addr);
            }
            else
            {
                set_ring((N2_PCIS0_0_RING_ID + (l_chip_unit_num / 3) + 1) & 0xF,
                                                                        o_addr);
                set_sat_id(((get_sat_id(i_addr) < 4) ? 1 : 4) +
                           ((l_chip_unit_num % 2) ? 0 : 1) +
                           (2 * l_chip_unit_num / 5), o_addr);
            }
        }
        else
        {
            // pci
            if(l_chip_unit_num == 0)
            {
                set_chiplet_id(PCI0_CHIPLET_ID, o_addr);
                set_sat_id(((get_sat_id(i_addr) < 4) ? 1 : 4), o_addr);
            }
            else
            {
                set_chiplet_id(PCI0_CHIPLET_ID + (l_chip_unit_num / 3) + 1, o_addr);
                set_sat_id(((get_sat_id(i_addr) < 4) ? 1 : 4) +
                           ((l_chip_unit_num % 2) ? 0 : 1) +
                           2 * l_chip_unit_num / 5, o_addr);
            }
        }
    }
    else if(i_trgt.type == TRGT_EX) //EX
    {
        if(get_chiplet_id(i_addr) <= EP05_CHIPLET_ID &&
           get_chiplet_id(i_addr) >= EP00_CHIPLET_ID)
        {
            set_chiplet_id(EP00_CHIPLET_ID + (l_chip_unit_num / 2), o_addr);
            uint8_t l_ring_id = get_ring(i_addr) & 0xF;
            l_ring_id = (l_ring_id - (l_ring_id % 2)) + (l_chip_unit_num % 2);
            set_ring(l_ring_id & 0xF, o_addr);
        }
        else if(get_chiplet_id(i_addr) <= EC23_CHIPLET_ID &&
                get_chiplet_id(i_addr) >= EC00_CHIPLET_ID)
        {
            set_chiplet_id(EC00_CHIPLET_ID +
                           (get_chiplet_id(i_addr) % 2) +
                           (l_chip_unit_num * 2), o_addr);
        }
    }
    else if(i_trgt.type == TRGT_EQ) //EQ
    {
        set_chiplet_id(EP00_CHIPLET_ID + l_chip_unit_num, o_addr);
    }
    else if(i_trgt.type == TRGT_EC) //EC
    {
        set_chiplet_id(EC00_CHIPLET_ID + l_chip_unit_num, o_addr);
    }
    else if(i_trgt.type == TRGT_MBA) //MBA
    {
        if( (i_addr & MBA_MASK) == MBA_BASEADDR )
        {
            /* 0x00000000_03010400  MBA 0  # MBA01 */
            /* 0x00000000_03010C00  MBA 1  # MBA23 */
            if( l_chip_unit_num == 1 )
            {
                *o_addr |= 0x00000800;
            }
        }
        else if( (i_addr & MBA_MASK) == TCM_MBA_BASEADDR )
        {
            /* 0x00000000_03010880  MBA 0  # Trace for MBA01 */
            /* 0x00000000_030110C0  MBA 1  # Trace for MBA23 */
            *o_addr |= (l_chip_unit_num * 0x840);
        }
        else if( (i_addr & MBA_MASK) == IND_MBA_BASEADDR )
        {
            /* 0x00000000_03011400  MBA 0  # DPHY01 (indirect addressing) */
            /* 0x00000000_03011800  MBA 1  # DPHY23 (indirect addressing) */
            /* 0x80000000_0301143f  MBA 0  # DPHY01 (indirect addressing) */
            /* 0x80000000_0301183f  MBA 1  # DPHY23 (indirect addressing) */
            /* 0x80000000_0701143f  MBA 0  # DPHY01 (indirect addressing) */
            /* 0x80000000_0701183f  MBA 1  # DPHY23 (indirect addressing) */
            if( l_chip_unit_num == 1 )
            {
                /* 030114zz->030118zz */
                *o_addr &= 0xFFFFFFFFFFFFFBFF;
                *o_addr |= 0x0000000000000800;
            }
        }
    }
    else if(i_trgt.type == TRGT_MCS || //MCS
            i_trgt.type == TRGT_MI) //MI TODO RTC 175488
    {
        set_chiplet_id(N3_CHIPLET_ID - (2 * (l_chip_unit_num / 2)), o_addr);
        set_sat_id(2 * (l_chip_unit_num % 2), o_addr);
    }
    else if(i_trgt.type == TRGT_MCBIST || //MCBIST
            i_trgt.type == TRGT_MC) //MC TODO RTC 175488
    {
        set_chiplet_id(MC01_CHIPLET_ID + l_chip_unit_num, o_addr);
    }
    else if(i_trgt.type == TRGT_MCA) //MCA
    {
        if(get_chiplet_id(i_addr) == MC01_CHIPLET_ID ||
           get_chiplet_id(i_addr) == MC23_CHIPLET_ID)
        {
            set_chiplet_id(MC01_CHIPLET_ID + (l_chip_unit_num / 4), o_addr);

            if((get_ring(i_addr) & 0xf) == MC_MC01_0_RING_ID)
            {
                // mc
                uint8_t l_sat_id = get_sat_id(i_addr);
                set_sat_id((l_sat_id - (l_sat_id % 4)) + (l_chip_unit_num % 4)
                                                                      , o_addr);
            }
            else
            {
                // iomc
                set_ring((MC_IOM01_0_RING_ID + (l_chip_unit_num % 4)) & 0xF
                                                                      , o_addr);
            }
        }
        else
        {
            // mcs->mca registers
            uint8_t l_mcs_unitnum = l_chip_unit_num / 2;
            set_chiplet_id(N3_CHIPLET_ID - (2 * (l_mcs_unitnum / 2)) , o_addr);
            set_sat_id(2 * (l_chip_unit_num % 2), o_addr);
            uint8_t l_mcs_sat_offset = 0x2F & get_sat_offset(i_addr);
            l_mcs_sat_offset |= (l_chip_unit_num % 2) << 4;
            set_sat_offset(l_mcs_sat_offset, o_addr);
        }
    }
    else if(i_trgt.type == TRGT_DMI) //DMI  TODO RTC 175488
    {
        if(get_chiplet_id(i_addr) == N3_CHIPLET_ID || get_chiplet_id(i_addr) == N1_CHIPLET_ID)
        {
            set_chiplet_id(N3_CHIPLET_ID - 2 * (l_chip_unit_num / 4), o_addr);
            set_sat_id(2 * ((l_chip_unit_num / 2) % 2), o_addr);
            uint8_t l_sat_offset = get_sat_offset(i_addr);
            l_sat_offset = (l_sat_offset & 0xF) + ((2 + (l_chip_unit_num % 2)) << 4);
            set_sat_offset(l_sat_offset, o_addr);
        }
        else if(get_chiplet_id(i_addr) == MC01_CHIPLET_ID || get_chiplet_id(i_addr) == MC23_CHIPLET_ID)
        {
            if(get_ring(i_addr) == P9C_MC_CHAN_RING_ID)
            {
                set_chiplet_id(MC01_CHIPLET_ID + l_chip_unit_num / 4, o_addr);
                uint8_t l_sat_id = get_sat_id(i_addr);
                l_sat_id = l_sat_id & 0xC;
                set_sat_id(l_sat_id + l_chip_unit_num % 4, o_addr);
            }
            else if(get_ring(i_addr) == P9C_MC_BIST_RING_ID)
            {
                set_chiplet_id(MC01_CHIPLET_ID + l_chip_unit_num / 4, o_addr);
                uint8_t l_sat_offset = get_sat_offset(i_addr);
                l_sat_offset = (l_sat_offset & 0xF) + ((l_chip_unit_num % 2) << 4);
                set_sat_offset(l_sat_offset, o_addr);
            }
            else if(get_ring(i_addr) == P9C_MC_IO_RING_ID)
            {
                set_chiplet_id(MC01_CHIPLET_ID + l_chip_unit_num / 4, o_addr);
                uint8_t l_rxtx_group = get_rxtx_group_id(i_addr);
                l_rxtx_group = l_rxtx_group & 0xF0;

                switch(l_chip_unit_num % 4)
                {
                    case 0:
                        l_rxtx_group += 3;
                        break;
                    case 1:
                        l_rxtx_group += 2;
                        break;
                    case 2:
                        l_rxtx_group += 0;
                        break;
                    case 3:
                        l_rxtx_group += 1;
                        break;
                    default:
                        break;
                }
                set_rxtx_group_id(l_rxtx_group, o_addr);
            }
        }
    }
    else
    {
        TRAC_ERR( FUNC"unsupported unit type %d", i_trgt.type );
        rc = FAIL;
    }

    return rc;

    #undef FUNC
}

/**
 * @brief  Performs a getscom operation with no address translation.
 * @param  i_chip Chip to SCOM.
 * @param  i_addr Address to SCOM.
 * @param  o_val  Returned value.
 * @return Non-SUCCESS if an internal function fails. SUCCESS otherwise.
 */
int32_t getscomraw( SCOM_Trgt_t i_chip, uint32_t i_addr, uint64_t * o_val )
{
    int32_t l_rc = SUCCESS;

    //Use SBE FIFO if it's a slave proc
    if(!i_chip.isMaster)
    {
        return getFifoScom(&i_chip, i_addr, o_val);
    }

    if(!G_request_created) //Only need to create request once
    {
        l_rc = gpe_request_create(&G_request,              //Request
                                  &G_async_gpe_queue0,     //Queue
                                  IPC_ST_SCOM_OPERATION,   //Function ID
                                  &G_scom_op,              //GPE arguments
                                  SSX_SECONDS(5),          //Timeout
                                  NULL,                    //Callback function
                                  NULL,                    //Callback args
                                  ASYNC_REQUEST_BLOCKING); //Options
        if(l_rc)
        {
            return l_rc;
        }
        else
        {
            G_request_created = TRUE;
        }
    }

    //Pack in the request data
    G_scom_op.read = TRUE;
    G_scom_op.addr = i_addr;

    l_rc = gpe_request_schedule(&G_request);
    if(l_rc)
    {
        return l_rc;
    }

    //Since it's a blocking request, it should be completed by the time we
    //get here. If it's not, then a timeout has occurred.
    if(G_request.request.completion_state == ASYNC_REQUEST_STATE_COMPLETE)
    {
        *o_val = G_scom_op.data;
    }
    else
    {
        *o_val = 0;
        l_rc = FAIL;
    }

    return l_rc;
}

/**
 * @brief  Perform a putscom operation with no address translation.
 * @param  i_chip Chip to SCOM.
 * @param  i_addr Address to SCOM.
 * @param  i_val  Value to write.
 * @return Non-SUCCESS if an internal function fails. SUCCESS otherwise.
 */
int32_t putscomraw( SCOM_Trgt_t i_chip, uint32_t i_addr, uint64_t i_val )
{
    int32_t l_rc = SUCCESS;

    //Use SBE FIFO if it's a slave proc
    if(!i_chip.isMaster)
    {
        return putFifoScom(&i_chip, i_addr, i_val);
    }

    if(!G_request_created) //Only need to create request once
    {
        l_rc = gpe_request_create(&G_request,              //Request
                                  &G_async_gpe_queue0,     //Queue
                                  IPC_ST_SCOM_OPERATION,   //Function ID
                                  &G_scom_op,              //GPE arguments
                                  SSX_SECONDS(5),          //Timeout
                                  NULL,                    //Callback function
                                  NULL,                    //Callback args
                                  ASYNC_REQUEST_BLOCKING); //Options
        if(l_rc)
        {
            return l_rc;
        }
        else
        {
            G_request_created = TRUE;
        }
    }

    //Pack in the request data
    G_scom_op.read = FALSE;
    G_scom_op.addr = i_addr;
    G_scom_op.data = i_val;

    l_rc = gpe_request_schedule(&G_request);
    if(l_rc)
    {
        return l_rc;
    }

    //It's a blocking request, so if the request hasn't been completed by this
    //time, then a timeout has occurred.
    if(G_request.request.completion_state != ASYNC_REQUEST_STATE_COMPLETE)
    {
        l_rc = FAIL;
    }

    return l_rc;
}

/**
 * @brief  Executes standard getscom.
 * @param  i_trgt Chip to SCOM.
 * @param  i_addr Address to SCOM.
 * @param  o_val  Returned value.
 * @return Non-SUCCESS if an internal function fails. SUCCESS otherwise.
 */
int32_t SCOM_getScom( SCOM_Trgt_t i_trgt, uint32_t i_addr, uint64_t * o_val )
{
    int32_t rc = SUCCESS;

    /* Get the parent chip. */
    SCOM_Trgt_t chip_targ = SCOM_Trgt_getParentChip(i_trgt);

    /* Get the address relative to the parent chip. */
    uint64_t trans_addr;
    rc = translate_addr( i_trgt, i_addr, &trans_addr );
    if ( SUCCESS == rc )
    {
        /* Do the SCOM. */
        rc = getscomraw( chip_targ, trans_addr, o_val );
    }
    TRAC_ERR("WGH SCOM_getScom returning 0x%08x %08x", (uint32_t)(*o_val>>32), (uint32_t)(*o_val));

    return rc;
}

/**
 * @brief  Execute indirect getscom.
 * @param  i_trgt Chip to SCOM.
 * @param  i_addr Address to SCOM.
 * @param  o_val  Returned value.
 * @return Non-SUCCESS if an internal function fails. SUCCESS otherwise.
 */
int32_t SCOM_getIdScom( SCOM_Trgt_t i_trgt, uint64_t i_addr, uint32_t * o_val )
{
    #define FUNC "[SCOM_getIdScom] "

    int32_t rc = SUCCESS;

    *o_val = 0;

    /* Get the parent chip */
    SCOM_Trgt_t chip_targ = SCOM_Trgt_getParentChip(i_trgt);

    /* Get the address relative to the parent chip. */
    uint64_t trans_addr;
    rc = translate_addr( i_trgt, i_addr, &trans_addr );
    if ( SUCCESS != rc ) return rc;

    /* An indirect SCOM is performed by putting the top of the 64-bit address
     * into the first data word of the SCOM */

    /* Zero out the indirect address from the buffer. */
    /* bit 0-31 - indirect area. */
    /* bit 32 - always 0 */
    /* bit 33-47 - bcast/chipletID/port */
    /* bit 48-63 - local addr */
    uint32_t phys_addr = trans_addr & 0x000000007FFFFFFF;

    /* To do a read we need to do a write first. */

    /* Start with the 20bit indirect address */
    uint64_t data_buffer = trans_addr & 0x001FFFFF00000000;
    /* Turn the read bit on. */
    data_buffer |= 0x8000000000000000;

    /* perform write before the read with the new */
    rc = putscomraw( i_trgt, phys_addr, data_buffer );
    if ( SUCCESS != rc ) return rc;

    // Loop on read until we see done, error, or we timeout
    IndirectScom_t scomout;
    uint64_t elapsed_indScom_time_ns = 0;
    do
    {
        /* Now perform the op requested using the passed in */
        /* IO_Buffer to pass the read data back to caller. */
        rc = getscomraw( chip_targ, phys_addr, &(scomout.data64) );
        if ( SUCCESS != rc ) return rc;

        /* Check for PIB error. */
        if ( scomout.piberr )
        {
            TRAC_ERR( FUNC"ID SCOM PIB error: phys_addr=0x%08x "
                      "trans_addr=0x%08x%08x", phys_addr,
                      (uint32_t)(trans_addr >> 32), (uint32_t)trans_addr );
            return FAIL;
        }

        /* Jump out when done. */
        if ( scomout.done ) break;

        sleep( 10000 ); /* sleep for 10,000 ns */
        elapsed_indScom_time_ns += 10000;

    } while ( elapsed_indScom_time_ns <= 100000 ); /* wait for .1ms */

    if ( !scomout.done )
    {
        TRAC_ERR( FUNC"ID SCOM loop timeout exceeded: phys_addr=0x%08x "
                      "trans_addr=0x%08x%08x", phys_addr,
                      (uint32_t)(trans_addr >> 32), (uint32_t)trans_addr );
        return FAIL;
    }

    *o_val = scomout.data;

    return rc;

    #undef FUNC
}

/**
 * @brief  Executes standard putscom.
 * @param  i_trgt Chip to SCOM.
 * @param  i_addr Address to SCOM.
 * @param  i_val  Value to put.
 * @return Non-SUCCESS if an internal function fails. SUCCESS otherwise.
*/
int32_t SCOM_putScom( SCOM_Trgt_t i_trgt, uint32_t i_addr, uint64_t i_val )
{
    int32_t l_rc = SUCCESS;

    /* Get the parent chip. */
    SCOM_Trgt_t l_chip_targ = SCOM_Trgt_getParentChip(i_trgt);

    /* Get the address relative to the parent chip. */
    uint64_t l_trans_addr;
    l_rc = translate_addr( i_trgt, i_addr, &l_trans_addr );
    if ( SUCCESS == l_rc )
    {
        /* Do the SCOM. */
        l_rc = putscomraw( l_chip_targ, l_trans_addr, i_val );
    }

    return l_rc;
}
