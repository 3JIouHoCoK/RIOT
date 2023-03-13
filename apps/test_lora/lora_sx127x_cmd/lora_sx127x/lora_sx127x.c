/*
 * Copyright (C) 2017 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 *
 * @file
 * @author  Martine Lenders <m.lenders@fu-berlin.de>
 */

#include "shell.h"
#include <stdio.h>
#include "sx127x.h"
extern sx127x_t sx127x_devs[1];
int _sx127x_sf_cmd(char *arg1){
    uint8_t sf = atoi(arg1);
    if(sx127x_get_op_mode(&sx127x_devs[0]) == 3)
        {
            printf("Radio is transmit mode\n");
            return 1;
        }
    if(sf < 5 || sf > 12)
        {
            printf("Spreading factor must be > 5 and < 12\n ");
            return 1;
        }
    sx127x_set_spreading_factor(&sx127x_devs[0], sf);
    printf("Spreading factor set in %d \n", sf);
    return 0;
}

int _sx127x_bw_cmd(char *arg1){
    uint16_t bw = atoi(arg1);
    if(sx127x_get_op_mode(&sx127x_devs[0]) == 3)
        {
            printf("Radio is transmit mode\n");
            return 1;
        }
    if((bw != 125) && (bw != 250) && (bw != 500))
        {
            printf("Bandwidth must be 125, 250, 500\n ");
            return 1;
        }
    switch (bw){
        case 125: 
            bw = 0;
            break;
        case 250:
            bw = 1;
            break;
        case 500:
            bw = 2;
            break;
    }
    
    sx127x_set_bandwidth(&sx127x_devs[0], bw);
    printf("Bandwidth set in %d \n", bw);
    return 0;
}

int _sx127x_cr_cmd(char *arg1){
    if(sx127x_get_op_mode(&sx127x_devs[0]) == 3)
        {
            printf("Radio is transmit mode\n");
            return 1;
        }
    uint8_t cr = atoi(arg1);
    if((cr < 5) && (cr > 8))
        {
            printf("Coding rate must be in [5, 6, 7, 8]\n");
            return 1;
        }
    
    sx127x_set_coding_rate(&sx127x_devs[0], cr-4);
    printf("Coding rate set in 4/%d \n", cr);
    return 0;
}

static int _sx127x_cmd(int argc, char **argv)
{
if ((argc > 2) && (strcmp(argv[1], "sf") == 0)) {
        return _sx127x_sf_cmd(argv[2]);
    }
else if ((argc > 2) && (strcmp(argv[1], "bw") == 0)) {
        return _sx127x_bw_cmd(argv[2]);
    }
else if ((argc > 2) && (strcmp(argv[1], "cr") == 0)) {
        return _sx127x_cr_cmd(argv[2]);
    }
else if ((argc == 2) && (strcmp(argv[1], "info") == 0)){
    uint8_t sf = sx127x_get_spreading_factor(&sx127x_devs[0]);
    uint16_t bw = sx127x_get_bandwidth(&sx127x_devs[0]);
    uint32_t freq = sx127x_get_channel(&sx127x_devs[0]);
    uint8_t cr = sx127x_get_coding_rate(&sx127x_devs[0]);
    
        switch (bw){
        case 0: 
            bw = 125;
            break;
        case 1:
            bw = 250;
            break;
        case 2:
            bw = 500;
            break;
    }
    printf("SX127x module current state:\nCarrier frequency = %ld\nSpeading factor = %d\nBandwidth = %d\nCoding rate = 4/%d\n", freq, sf, bw, 4+cr);
}
else {
        printf("Usage sx127x [[sf|bw|cr <value>]|info] \n");
    }
    return 0;
}

SHELL_COMMAND(sx127x, "SX127x configuration tool", _sx127x_cmd);

/** @} */
