/* Copyright 2019 Sipeed Inc.
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

#ifndef _LAYER_H
#define _LAYER_H

void layer_conv_init(kpu_task_t *task, uint16_t w, uint16_t h, uint8_t ch_in, uint8_t ch_out, float *conv_data);
void layer_conv_run(kpu_task_t *task, uint8_t *img_src, uint8_t *img_dst, plic_irq_callback_t callback);

#endif
