/*
 * Copyright (C) 2017  Inria
 *               2017  OTA keys
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     boards_nucleo-l432kc
 * @{
 *
 * @file
 * @brief       Peripheral MCU configuration for the nucleo-l432kc board
 *
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 * @author      Vincent Dupont <vincent@otakeys.com>
 */

#ifndef PERIPH_CONF_H
#define PERIPH_CONF_H

/* Add specific clock configuration (HSE, LSE) for this board here */
#ifndef CONFIG_BOARD_HAS_LSE
#define CONFIG_BOARD_HAS_LSE            0
#endif

#include "periph_cpu.h"
#include "clk_conf.h"
#include "cfg_i2c1_pb6_pb7.h"
#include "cfg_rtt_default.h"
#include "cfg_timer_tim2.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name UART configuration
 * @{
 */
static const uart_conf_t uart_config[] = {
    {
        .dev        = USART1,
        .rcc_mask   = RCC_APB2ENR_USART1EN,
        .rx_pin     = GPIO_PIN(PORT_B, 7),
        .tx_pin     = GPIO_PIN(PORT_B, 6),
        .rx_af      = GPIO_AF7,
        .tx_af      = GPIO_AF7,
        .bus        = APB2,
        .irqn       = USART1_IRQn,
        .type       = STM32_USART,
        .clk_src    = 0, /* Use APB clock */
    },
};

#define UART_0_ISR          (isr_usart1)

#define UART_NUMOF          ARRAY_SIZE(uart_config)
/** @} */

#define EN_POW_RF_PIN_NUM           5
#define EN_POW_RF_PORT_NUM          PORT_A
#define EN_POW_CPU_PIN_NUM          15
#define EN_POW_CPU_PORT_NUM         PORT_C
#define EN_POW_ANTS_PIN_NUM         15
#define EN_POW_ANTS_PORT_NUM        PORT_A
#define EN_POW_I2C_PIN_NUM          3
#define EN_POW_I2C_PORT_NUM         PORT_H

#define SPI_CS_TR1_PIN_NUM          0
#define SPI_CS_TR1_PORT_NUM         PORT_A
#define SPI_CS_R2_PIN_NUM           3
#define SPI_CS_R2_PORT_NUM          PORT_B
#define INT_TR1_PIN_NUM             14
#define INT_TR1_PORT_NUM            PORT_A
#define INT_R2_PIN_NUM              13
#define INT_R2_PORT_NUM             PORT_A
/**
 * @name   SPI configuration
 * @{
 */
static const spi_conf_t spi_config[] = {
    {
        .dev      = SPI1,
        .mosi_pin = GPIO_PIN(PORT_B, 5),
        .miso_pin = GPIO_PIN(PORT_B, 4),
        .sclk_pin = GPIO_PIN(PORT_A, 1),
        .cs_pin   = SPI_CS_UNDEF,
        .mosi_af  = GPIO_AF5,
        .miso_af  = GPIO_AF5,
        .sclk_af  = GPIO_AF5,
        .cs_af    = GPIO_AF5,
        .rccmask  = RCC_APB2ENR_SPI1EN,
        .apbbus   = APB2
    }
};

#define SPI_NUMOF           ARRAY_SIZE(spi_config)
/** @} */

/**
 * @name   DAC configuration
 * @{
 */
static const dac_conf_t dac_config[] = {
    { .pin = GPIO_PIN(PORT_A,  4), .chan = 0 }
};

#define DAC_NUMOF           ARRAY_SIZE(dac_config)
/** @} */


#ifdef __cplusplus
}
#endif

#endif /* PERIPH_CONF_H */
/** @} */
