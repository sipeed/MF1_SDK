#ifndef __QUIRC_H
#define __QUIRC_H

#include <stdio.h>
#include <stdint.h>

#include "maix_qrcode.h"

typedef struct qrcode_res
{
    /* Various parameters of the QR-code. These can mostly be
     * ignored if you only care about the data.
     */
    int version;
    int ecc_level;
    int mask;

    /* This field is the highest-valued data type found in the QR
     * code.
     */
    int data_type;

    /* Data payload. For the Kanji datatype, payload is encoded as
     * Shift-JIS. For all other datatypes, payload is ASCII text.
     */
    uint8_t payload[QUIRC_MAX_PAYLOAD];
    int payload_len;

    /* ECI assignment number */
    uint32_t eci;
} qrcode_result_t __attribute__((aligned(8)));

uint8_t find_qrcodes(qrcode_result_t *out, qrcode_image_t *img, qrcode_scan_t *scan, uint8_t convert);

#endif
