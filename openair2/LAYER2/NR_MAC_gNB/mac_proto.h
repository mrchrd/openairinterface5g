
#ifndef __LAYER2_NR_MAC_PROTO_H__
#define __LAYER2_NR_MAC_PROTO_H__

#include "nr_mac_gNB.h"
#include "PHY/defs_gNB.h"

void mac_top_init_gNB(void);

int rrc_mac_config_req_gNB(module_id_t Mod_idP, 
                           int CC_id,
                           int p_gNB,
                           int eutra_bandP,
                           int dl_CarrierFreqP,
                           int dl_BandwidthP,
                           NR_BCCH_BCH_Message_t *mib,
                           NR_ServingCellConfigCommon_t *servingcellconfigcommon
                           );

int  is_nr_UL_sf(NR_COMMON_channels_t * ccP, sub_frame_t subframeP);

void clear_nr_nfapi_information(gNB_MAC_INST * gNB, 
                                int CC_idP,
                                frame_t frameP, 
                                sub_frame_t subframeP);

void gNB_dlsch_ulsch_scheduler(module_id_t module_idP, 
                               frame_t frameP,
                               sub_frame_t subframeP);

void schedule_nr_mib(module_id_t module_idP, frame_t frameP, sub_frame_t subframeP);

#endif /*__LAYER2_NR_MAC_PROTO_H__*/