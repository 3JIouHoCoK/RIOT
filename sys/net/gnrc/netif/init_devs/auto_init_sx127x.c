/*
 * Copyright (C) 2017 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 */

/**
 * @ingroup     sys_auto_init_gnrc_netif
 * @{
 *
 * @file
 * @brief       Auto initialization for SX1272/SX1276 LoRa interfaces
 *
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 */

#include <assert.h>

#include "log.h"
#include "board.h"
#include "net/gnrc/netif/lorawan_base.h"
#include "net/gnrc/netif/raw.h"
#include "net/gnrc.h"
#include "include/init_devs.h"

#include "sx127x.h"
#include "sx127x_params.h"


/**
 * @brief   Calculate the number of configured SX127x devices
 */
#define SX127X_NUMOF        ARRAY_SIZE(sx127x_params)

/**
 * @brief   Define stack parameters for the MAC layer thread
 */
#define SX127X_STACKSIZE           (GNRC_NETIF_STACKSIZE_DEFAULT)
#ifndef SX127X_PRIO
#define SX127X_PRIO                (GNRC_NETIF_PRIO)
#endif

/**
 * @brief   Allocate memory for device descriptors, stacks, and GNRC adaption
 */
static sx127x_t sx127x_devs[SX127X_NUMOF];
static char sx127x_stacks[SX127X_NUMOF][SX127X_STACKSIZE];
static gnrc_netif_t _netif[SX127X_NUMOF];

#include "net/netdev/ieee802154_submac.h"
#include "net/gnrc/netif/ieee802154.h"
static netdev_ieee802154_submac_t sx127x_netdev[SX127X_NUMOF];


void auto_init_sx127x(void)
{
    for (unsigned i = 0; i < SX127X_NUMOF; ++i) {
    LOG_DEBUG("[auto_init_netif] initializing sx1276 hal #%u\n", i);
        netdev_register(&sx127x_netdev[i].dev.netdev, NETDEV_SX127X, i);
        netdev_ieee802154_submac_init(&sx127x_netdev[i]);
        
        sx127x_hal_setup(&sx127x_devs[i], &sx127x_netdev[i].submac.dev);
        sx127x_setup(&sx127x_devs[i], &sx127x_params[i], i);
        sx127x_init(&sx127x_devs[i], &sx127x_netdev[i].submac.dev);
        
        
            gnrc_netif_ieee802154_create(&_netif[i], sx127x_stacks[i],
                                  SX127X_STACKSIZE, SX127X_PRIO,
                                  "sx127x", &sx127x_netdev[i].dev.netdev);

    }
}
/** @} */
