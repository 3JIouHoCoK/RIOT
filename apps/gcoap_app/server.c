/*
 * Copyright (c) 2015-2017 Ken Bannister. All rights reserved.
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       gcoap CLI support
 *
 * @author      Ken Bannister <kb2ma@runbox.com>
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fmt.h"
#include "net/gcoap.h"
#include "net/utils.h"
#include "od.h"

#include "gcoap_example.h"

#define ENABLE_DEBUG 0
#include "debug.h"

#include "kernel_defines.h"
#include "board.h"
#include "periph/i2c.h"
#include "periph/spi.h"
#include "luid.h"
#include "ztimer.h"

#include "arducam.h"

#include "lm75.h"
#include "lm75_regs.h"
#include "lm75_params.h"

#if IS_USED(MODULE_GCOAP_DTLS)
#include "net/credman.h"
#include "net/dsm.h"
#include "tinydtls_keys.h"

/* Example credential tag for credman. Tag together with the credential type needs to be unique. */
#define GCOAP_DTLS_CREDENTIAL_TAG 10

static const uint8_t psk_id_0[] = PSK_DEFAULT_IDENTITY;
static const uint8_t psk_key_0[] = PSK_DEFAULT_KEY;
static const credman_credential_t credential = {
    .type = CREDMAN_TYPE_PSK,
    .tag = GCOAP_DTLS_CREDENTIAL_TAG,
    .params = {
        .psk = {
            .key = { .s = psk_key_0, .len = sizeof(psk_key_0) - 1, },
            .id = { .s = psk_id_0, .len = sizeof(psk_id_0) - 1, },
        }
    },
};
#endif

char photo_test[1024];
uint32_t photo_size;
static uint32_t _make_photo(uint8_t type);
lm75_t lm75_dev;

static ssize_t _encode_link(const coap_resource_t *resource, char *buf,
                            size_t maxlen, coap_link_encoder_ctx_t *context);
static ssize_t _stats_handler(coap_pkt_t* pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx);
static ssize_t _riot_board_handler(coap_pkt_t* pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx);
static ssize_t _led_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx);
static ssize_t _bat_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx);
static ssize_t _light_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx);
static ssize_t _photo_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx);
static ssize_t _temp_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx);
static ssize_t _photo_length_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx);
/* CoAP resources. Must be sorted by path (ASCII order). */
static const coap_resource_t _resources[] = {
    { "/cli/stats", COAP_GET | COAP_PUT, _stats_handler, NULL },
    { "/led", COAP_GET | COAP_POST, _led_handler, NULL },
    { "/photo", COAP_GET | COAP_POST, _photo_handler, NULL },
    { "/photo/length", COAP_GET , _photo_length_handler, NULL },
    { "/sensors/bat", COAP_GET, _bat_handler, NULL },
    { "/sensors/light", COAP_GET, _light_handler, NULL },
    { "/temprature", COAP_GET, _temp_handler, NULL },
    { "/riot/board", COAP_GET, _riot_board_handler, NULL }
};

static const char *_link_params[] = {
    ";ct=0;rt=\"count\";obs",
    NULL
};

static gcoap_listener_t _listener = {
    &_resources[0],
    ARRAY_SIZE(_resources),
    GCOAP_SOCKET_TYPE_UNDEF,
    _encode_link,
    NULL,
    NULL
};


/* Adds link format params to resource list */
static ssize_t _encode_link(const coap_resource_t *resource, char *buf,
                            size_t maxlen, coap_link_encoder_ctx_t *context) {
    ssize_t res = gcoap_encode_link(resource, buf, maxlen, context);
    if (res > 0) {
        if (_link_params[context->link_pos]
                && (strlen(_link_params[context->link_pos]) < (maxlen - res))) {
            if (buf) {
                memcpy(buf+res, _link_params[context->link_pos],
                       strlen(_link_params[context->link_pos]));
            }
            return res + strlen(_link_params[context->link_pos]);
        }
    }

    return res;
}

/*
 * Server callback for /cli/stats. Accepts either a GET or a PUT.
 *
 * GET: Returns the count of packets sent by the CLI.
 * PUT: Updates the count of packets. Rejects an obviously bad request, but
 *      allows any two byte value for example purposes. Semantically, the only
 *      valid action is to set the value to 0.
 */
static ssize_t _stats_handler(coap_pkt_t* pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx)
{
    (void)ctx;

    /* read coap method type in packet */
    unsigned method_flag = coap_method2flag(coap_get_code_detail(pdu));

    switch (method_flag) 
    {
        case COAP_GET:
            gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
            coap_opt_add_format(pdu, COAP_FORMAT_TEXT);
            size_t resp_len = coap_opt_finish(pdu, COAP_OPT_FINISH_PAYLOAD);

            /* write the response buffer with the request count value */
            resp_len += fmt_u16_dec((char *)pdu->payload, req_count);
            return resp_len;

        case COAP_PUT:
            /* convert the payload to an integer and update the internal
               value */
            if (pdu->payload_len <= 5) {
                char payload[6] = { 0 };
                memcpy(payload, (char *)pdu->payload, pdu->payload_len);
                req_count = (uint16_t)strtoul(payload, NULL, 10);
                return gcoap_response(pdu, buf, len, COAP_CODE_CHANGED);
            }
            else {
                return gcoap_response(pdu, buf, len, COAP_CODE_BAD_REQUEST);
            }
    }

    return 0;
}

static ssize_t _riot_board_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx)
{
    (void)ctx;
    gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
    coap_opt_add_format(pdu, COAP_FORMAT_TEXT);
    size_t resp_len = coap_opt_finish(pdu, COAP_OPT_FINISH_PAYLOAD);

    /* write the RIOT board name in the response buffer */
    if (pdu->payload_len >= strlen(RIOT_BOARD)) {
        memcpy(pdu->payload, RIOT_BOARD, strlen(RIOT_BOARD));
        return resp_len + strlen(RIOT_BOARD);
    }
    else {
        puts("gcoap_cli: msg buffer too small");
        return gcoap_response(pdu, buf, len, COAP_CODE_INTERNAL_SERVER_ERROR);
    }
}

static ssize_t _led_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx)
{
    (void)ctx;
    unsigned method_flag = coap_method2flag(coap_get_code_detail(pdu));
    switch (method_flag){
        case COAP_GET:
            /* Prepare packet to respond */
            gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
            coap_opt_add_format(pdu, COAP_FORMAT_TEXT);
            size_t resp_len = coap_opt_finish(pdu, COAP_OPT_FINISH_PAYLOAD);
            
            if(pdu->payload_len >= 3) {
            /* Check current state of led */
                if(gpio_read(LED_USB_LINK))
                        sprintf((char*)pdu->payload, "On");
                else 
                        sprintf((char*)pdu->payload, "Off");

                return resp_len + strlen((char*)pdu->payload);
            }
            else {
                puts("gcoap_cli: msg buffer too small\n");
                return gcoap_response(pdu, buf, len, COAP_CODE_INTERNAL_SERVER_ERROR);
            }

        case COAP_POST:
            if(atoi((char*)pdu->payload) == 0){
                gpio_write(LED_USB_LINK, 0);
                return gcoap_response(pdu, buf, len, COAP_CODE_CHANGED);
            }
            else if (atoi((char*)pdu->payload) == 1){
                gpio_write(LED_USB_LINK, 1);
                return gcoap_response(pdu, buf, len, COAP_CODE_CHANGED); 
            }
            else {
                puts("gcoap_cli: error state number\n");
                return gcoap_response(pdu, buf, len, COAP_CODE_INTERNAL_SERVER_ERROR);
            }
    }
    return 0;
}
static ssize_t _bat_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx)
{
    (void)ctx;
    gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
    coap_opt_add_format(pdu, COAP_FORMAT_TEXT);
    size_t resp_len = coap_opt_finish(pdu, COAP_OPT_FINISH_PAYLOAD);   

    i2c_acquire(0);
    uint16_t vbat = 0;
    le_uint16_t reg_value;
    i2c_read_regs(0, 0x40, 0x02, &reg_value.u16, 2, 0);
    vbat = byteorder_ltobs(reg_value).u16;
    vbat = (vbat>>3)*4;
    sprintf((char*)pdu->payload, "%d", vbat);
    i2c_release(0);
    return strlen((char*)pdu->payload) + resp_len;
}

static ssize_t _light_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx)
{
    (void)ctx;
    gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
    coap_opt_add_format(pdu, COAP_FORMAT_TEXT);
    size_t resp_len = coap_opt_finish(pdu, COAP_OPT_FINISH_PAYLOAD);   

    i2c_acquire(0);
    uint16_t light = 0;
    le_uint16_t light_value;
    i2c_write_byte(0,0x23, 0b00100000, 0);
    ztimer_sleep(ZTIMER_MSEC, 180);
    i2c_read_bytes(0, 0x23, &light_value.u16,2, 0);
    i2c_write_byte(0,0x23, 0b00000001, 0);
    light = byteorder_ltobs(light_value).u16;
    light = (light>>3)*4;
    sprintf((char*)pdu->payload, "%d", light);
    i2c_release(0);

    return strlen((char*)pdu->payload) + resp_len;
}
static ssize_t _photo_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx)
{
    (void)ctx;
    unsigned method_flag = coap_method2flag(coap_get_code_detail(pdu));

    switch (method_flag) 
    {
        case COAP_GET:
        {
            coap_block_slicer_t slicer;
            coap_block2_init(pdu, &slicer);

            gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
            coap_opt_add_format(pdu, COAP_FORMAT_OCTET);
            coap_opt_add_block2(pdu, &slicer, 1);
            ssize_t plen = coap_opt_finish(pdu, COAP_OPT_FINISH_PAYLOAD);
            
            plen += coap_blockwise_put_bytes(&slicer, buf+plen, photo_test, 1024);
            coap_block2_finish(&slicer);
            return plen;
        }
        case COAP_POST:
        {
            if((atoi((char*)pdu->payload) >= 0) && ((atoi((char*)pdu->payload) < 8))) //Сделать фото
            {
                photo_size = _make_photo((atoi((char*)pdu->payload)));
                return gcoap_response(pdu, buf, len, COAP_CODE_CHANGED);
            }
            else if(atoi((char*)pdu->payload) == 10) //Прочитать кусок из памяти
            {
                spi_acquire(1, SPI_CS_UNDEF, SPI_MODE_0, SPI_CLK_1MHZ);
                CS_LOW();
                spi_transfer_byte(1, SPI_CS_UNDEF, 1, ARDUCHIP_TEST1);
                CS_HIGH();

                CS_LOW();
                    spi_transfer_regs(1, SPI_CS_UNDEF, BURST_FIFO_READ, NULL, photo_test, 1024);
                CS_HIGH();
                spi_release(1);
                return gcoap_response(pdu, buf, len, COAP_CODE_CHANGED);
            }

            return gcoap_response(pdu, buf, len, COAP_CODE_BAD_REQUEST);
        }
    }
    return 0;
}
static uint32_t _make_photo(uint8_t type)   
{    
    spi_acquire(1, SPI_CS_UNDEF, SPI_MODE_0, SPI_CLK_1MHZ);
    i2c_acquire(1);
    OV5642_set_JPEG_size(type); 
	flush_fifo();
	clear_fifo_flag();
	start_capture(); 
	while(!get_bit(ARDUCHIP_TRIG , CAP_DONE_MASK)){;}
	//printf("ACK CMD capture done\r\n");
	uint32_t length= read_fifo_length();
    char buf[10];
    sprintf(buf, "%ld", length);
    puts(buf);
    i2c_release(1);
    spi_release(1);
    return length;
}
static ssize_t _photo_length_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx){
    (void)ctx;
    gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
    coap_opt_add_format(pdu, COAP_FORMAT_TEXT);
    size_t resp_len = coap_opt_finish(pdu, COAP_OPT_FINISH_PAYLOAD); 
    spi_acquire(1, SPI_CS_UNDEF, SPI_MODE_0, SPI_CLK_1MHZ);
    sprintf((char*)pdu->payload, "%ld", read_fifo_length());
    spi_release(1);
    return strlen((char*)pdu->payload) + resp_len;
}
static ssize_t _temp_handler(coap_pkt_t *pdu, uint8_t *buf, size_t len, coap_request_ctx_t *ctx){
    (void)ctx;
    gcoap_resp_init(pdu, buf, len, COAP_CODE_CONTENT);
    coap_opt_add_format(pdu, COAP_FORMAT_TEXT);
    size_t resp_len = coap_opt_finish(pdu, COAP_OPT_FINISH_PAYLOAD); 
    int temp = 0;
    lm75_get_temperature(&lm75_dev, &temp);
    sprintf((char*)pdu->payload, "%d", temp);
    return strlen((char*)pdu->payload) + resp_len;
}
void notify_observers(void)
{
    size_t len;
    uint8_t buf[CONFIG_GCOAP_PDU_BUF_SIZE];
    coap_pkt_t pdu;

    /* send Observe notification for /cli/stats */
    switch (gcoap_obs_init(&pdu, &buf[0], CONFIG_GCOAP_PDU_BUF_SIZE,
            &_resources[0])) {
    case GCOAP_OBS_INIT_OK:
        DEBUG("gcoap_cli: creating /cli/stats notification\n");
        coap_opt_add_format(&pdu, COAP_FORMAT_TEXT);
        len = coap_opt_finish(&pdu, COAP_OPT_FINISH_PAYLOAD);
        len += fmt_u16_dec((char *)pdu.payload, req_count);
        gcoap_obs_send(&buf[0], len, &_resources[0]);
        break;
    case GCOAP_OBS_INIT_UNUSED:
        DEBUG("gcoap_cli: no observer for /cli/stats\n");
        break;
    case GCOAP_OBS_INIT_ERR:
        DEBUG("gcoap_cli: error initializing /cli/stats notification\n");
        break;
    }
}

void server_init(void)
{
#if IS_USED(MODULE_GCOAP_DTLS)
    int res = credman_add(&credential);
    if (res < 0 && res != CREDMAN_EXIST) {
        /* ignore duplicate credentials */
        printf("gcoap: cannot add credential to system: %d\n", res);
        return;
    }
    sock_dtls_t *gcoap_sock_dtls = gcoap_get_sock_dtls();
    res = sock_dtls_add_credential(gcoap_sock_dtls, GCOAP_DTLS_CREDENTIAL_TAG);
    if (res < 0) {
        printf("gcoap: cannot add credential to DTLS sock: %d\n", res);
    }
#endif
    

    if(arducam_init() != 0) //CAMERA INITIALAZING
        puts("Error camera initialazing\n");
    
    if (lm75_init(&lm75_dev, lm75_params) != LM75_SUCCESS) {
        puts("Initialization  failed");
    }
    gcoap_register_listener(&_listener);
}
