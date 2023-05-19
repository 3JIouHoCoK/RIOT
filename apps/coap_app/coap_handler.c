/*
 * Copyright (C) 2016 Kaspar Schleiser <kaspar@schleiser.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fmt.h"
#include "net/nanocoap.h"
#include "hashes/sha256.h"
#include "kernel_defines.h"
#include "board.h"
#include "periph/i2c.h"
#include "luid.h"
#include "ztimer.h"
/* internal value that can be read/written via CoAP */

static ssize_t _led_handler(coap_pkt_t* pkt, uint8_t *buf, size_t len, coap_request_ctx_t *context){
    (void) context;

    char rsp[8];
    unsigned code = COAP_CODE_EMPTY;

    /* read coap method type in packet */
    unsigned method_flag = coap_method2flag(coap_get_code_detail(pkt));

    switch(method_flag) {
    case COAP_GET:
        /* write the response buffer with the internal value */
        if(gpio_read(LED_USB_LINK))
            sprintf(rsp, "On");
        else 
            sprintf(rsp, "Off");
        code = COAP_CODE_205;
        break;
    case COAP_POST:
        if (pkt->payload_len < 2) {
            if(pkt->payload[0] == '0'){
                gpio_write(LED_USB_LINK, 0);
                sprintf(rsp, "OK!");
            }
            else {
                gpio_write(LED_USB_LINK, 1);
                sprintf(rsp, "OK!");
            }
            code = COAP_CODE_CHANGED;
        }
        else {
            code = COAP_CODE_REQUEST_ENTITY_TOO_LARGE;
        }
    }

    return coap_reply_simple(pkt, code, buf, len,
            COAP_FORMAT_TEXT, (uint8_t*)rsp, strlen(rsp));
}

static ssize_t _bat_handler(coap_pkt_t* pkt, uint8_t *buf, size_t len, coap_request_ctx_t *context){
    (void) context;

    char rsp[8];
    unsigned code = COAP_CODE_EMPTY;
    i2c_acquire(0);
    uint16_t vbat = 0;
    le_uint16_t reg_value;
    i2c_read_regs(0, 0x40, 0x02, &reg_value.u16, 2, 0);
    vbat = byteorder_ltobs(reg_value).u16;
    vbat = (vbat>>3)*4;
    sprintf(rsp, "%d", vbat);
    i2c_release(0);
    code = COAP_CODE_205;

    return coap_reply_simple(pkt, code, buf, len,
            COAP_FORMAT_TEXT, (uint8_t*)rsp, strlen(rsp));
}

static ssize_t _light_handler(coap_pkt_t* pkt, uint8_t *buf, size_t len, coap_request_ctx_t *context){
    (void) context;

    char rsp[32];
    unsigned code = COAP_CODE_EMPTY;
    i2c_acquire(0);
    uint16_t light = 0;
    le_uint16_t light_value;
    i2c_write_byte(0,0x23, 0b00100000, 0);
    ztimer_sleep(ZTIMER_MSEC, 180);
    i2c_read_bytes(0, 0x23, &light_value.u16,2, 0);
    i2c_write_byte(0,0x23, 0b00000001, 0);
    light = byteorder_ltobs(light_value).u16;
    light = (light>>3)*4;
    sprintf(rsp, "%d", light);
    i2c_release(0);
    code = COAP_CODE_205;

    return coap_reply_simple(pkt, code, buf, len,
            COAP_FORMAT_TEXT, (uint8_t*)rsp, strlen(rsp));
}
NANOCOAP_RESOURCE(led) {
    .path = "/led", .methods = COAP_GET | COAP_POST, .handler = _led_handler
};
NANOCOAP_RESOURCE(light) {
    .path = "/sensors/light", .methods = COAP_GET, .handler = _light_handler
};
NANOCOAP_RESOURCE(bat) {
    .path = "/sensors/bat", .methods = COAP_GET, .handler = _bat_handler
};

/* we can also include the fileserver module */
#ifdef MODULE_GCOAP_FILESERVER
#include "net/gcoap/fileserver.h"
#include "vfs_default.h"

NANOCOAP_RESOURCE(fileserver) {
    .path = "/vfs",
    .methods = COAP_GET
#if IS_USED(MODULE_GCOAP_FILESERVER_PUT)
      | COAP_PUT
#endif
#if IS_USED(MODULE_GCOAP_FILESERVER_DELETE)
      | COAP_DELETE
#endif
      | COAP_MATCH_SUBTREE,
    .handler = gcoap_fileserver_handler,
    .context = VFS_DEFAULT_DATA
};
#endif
