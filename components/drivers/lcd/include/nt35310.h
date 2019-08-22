/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _NT35310_H_
#define _NT35310_H_

#include <stdint.h>
#include "system_config.h"

#if CONFIG_LCD_TYPE_ST7789

void tft_hard_init(void);
void tft_write_command(uint8_t cmd);
void tft_write_byte(uint8_t *data_buf, uint32_t length);
void tft_write_half(uint16_t *data_buf, uint32_t length);
void tft_write_word(uint32_t *data_buf, uint32_t length, uint32_t flag);
void tft_fill_data(uint32_t *data_buf, uint32_t length);

#endif
#endif
