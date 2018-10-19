/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file PHY/NR_TRANSPORT/nr_sch_dmrs.c
* \brief
* \author
* \date
* \version
* \company Eurecom
* \email:
* \note
* \warning
*/

#include "PHY/defs_gNB.h"

#define NR_PDSCH_DMRS_ANTENNA_PORT0 1000
#define NR_PDSCH_DMRS_NB_ANTENNA_PORTS 12

int8_t *get_Wt(uint8_t ap, uint8_t config);

int8_t *get_Wf(uint8_t ap, uint8_t config);

uint8_t get_delta(uint8_t ap, uint8_t config);

uint8_t *get_l0(uint8_t config, uint8_t dmrs_typeA_position);