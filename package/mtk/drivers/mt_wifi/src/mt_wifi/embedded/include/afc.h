/*
 * Copyright (c) [2022], MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws.
 * The information contained herein is confidential and proprietary to
 * MediaTek Inc. and/or its licensors.
 * Except as otherwise provided in the applicable licensing terms with
 * MediaTek Inc. and/or its licensors, any reproduction, modification, use or
 * disclosure of MediaTek Software, and information contained herein, in whole
 * or in part, shall be strictly prohibited.
*/
/*
 ***************************************************************************
 ***************************************************************************

	Module Name:
	afc.h
*/

#ifndef __AFC_H__
#define __AFC_H__

#if defined(CONFIG_6G_SUPPORT) && defined(CONFIG_6G_AFC_SUPPORT) && defined(DOT11_HE_AX)
/*******************************************************************************
 *    INCLUDED FILES
 ******************************************************************************/
#include "rt_config.h"
/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/
#define TLV_HEADER 3
#define AFC_MAX_TXPWR_LIMIT 36

#define OP_CLASS_131 131
#define OP_CLASS_132 132
#define OP_CLASS_133 133
#define OP_CLASS_134 134
#define OP_CLASS_135 135
#define OP_CLASS_136 136
#define OP_CLASS_137 137
#define AFC_MAX_OP_CLASS 4
#define AFC_UNII_BAND_FREQ_NUM 2

#define MAX_20MHZ_CHANNEL_IN_6G_UNII   24
#define MAX_40MHZ_CHANNEL_IN_6G_UNII   12
#define MAX_80MHZ_CHANNEL_IN_6G_UNII   6
#define MAX_160MHZ_CHANNEL_IN_6G_UNII  3

#define UNII_7_BW20_START_INX  (MAX_20MHZ_CHANNEL_IN_6G_UNII + 5)
#define UNII_7_BW40_START_INX  (MAX_40MHZ_CHANNEL_IN_6G_UNII + 3)
#define UNII_7_BW80_START_INX  (MAX_80MHZ_CHANNEL_IN_6G_UNII + 2)
#define UNII_7_BW160_START_INX (MAX_160MHZ_CHANNEL_IN_6G_UNII + 1)

#define UNII_5_STARTING_FREQ 5925
#define UNII_5_ENDING_FREQ 6425
#define UNII_7_STARTING_FREQ 6525
#define UNII_7_ENDING_FREQ 6875
#define BW20_MHZ             20
#define AFC_STATUS_LEN 4
#define AFC_INQUIRY_EVENT_TIMEOUT 2
#define AFC_NO_RESPONSE_FROM_SERVER 0xFFFF
#define AFC_RESPONSE_INIT_VALUE 0xFFFE
#define AFC_INVALID_CH_INDEX -1
#define AFC_MACHINE_BASE   0
#define AFC_IDLE           0
#define AFC_DISABLE        1
#define AFC_HOLD           2
#define AFC_RUN            3
#define AFC_MAX_STATE      4

#define AFC_PRE_CHECK_MSG_PASS     0
#define AFC_PRE_CHECK_MSG_FAIL     1
#define AFC_INF_UP_INQ_EVENT       2
#define AFC_RESPONSE_SUCCESS       3
#define AFC_RESPONSE_FAIL          4
#define AFC_INF_DOWN_STOP_EVENT    5
#define AFC_MAX_MSG                6

#define AFC_FUNC_SIZE    (AFC_MAX_STATE * AFC_MAX_MSG)

enum AFC_6G_UNII {
	BAND_6G_UNII_5,
	BAND_6G_UNII_7,
	BAND_6G_UNII_NUM
};

enum AFC_OP_CLASS_6G {
	AFC_OP_CLASS_131,
	AFC_OP_CLASS_132,
	AFC_OP_CLASS_133,
	AFC_OP_CLASS_134,
	AFC_OP_CLASS_135,
	AFC_OP_CLASS_136,
	AFC_OP_CLASS_NUM
};

enum AFC_OP_CLASS_CHANNEL_NUM {
	AFC_OP_CLASS_131_CHL_NUM = 59,
	AFC_OP_CLASS_132_CHL_NUM = 29,
	AFC_OP_CLASS_133_CHL_NUM = 14,
	AFC_OP_CLASS_134_CHL_NUM = 7,
	AFC_OP_CLASS_135_CHL_NUM = 14,
	AFC_OP_CLASS_136_CHL_NUM = 3
};
enum AFC_RESPONSE_TAG {
	VERSION = 0,
	ASI_RESPONSE,
	FREQ_INFO,
	CHANNELS_ALLOWED,
	OPER_CLASS,
	CHANNEL_LIST,
	EXPIRY_TIME,
	RESPONSE
};

enum AFC_TXPWR_TBL_BW {
	AFC_TXPWR_TBL_BW20,
	AFC_TXPWR_TBL_BW40,
	AFC_TXPWR_TBL_BW80,
	AFC_TXPWR_TBL_BW160,
	AFC_TXPWR_TBL_RU26,
	AFC_TXPWR_TBL_RU52,
	AFC_TXPWR_TBL_RU106,
	AFC_TXPWR_TBL_BW_NUM
};

enum AFC_DEVICE_TYPE {
	AFC_NON_STANDARD_POWER_DEVICE = 0,
	AFC_STANDARD_POWER_DEVICE,
	AFC_MAX_DEVICE_TYPE
};

enum AFC_RESPONSE_TYPE {
	AFC_RESPONSE_TYPE_SUCCESS = 0,
	AFC_RESPONSE_TYPE_FAIL,
	AFC_MAX_RESPONSE_TYPE
};

enum AFC_SPECTRUM_TYPE {
	AFC_SPEC_OP_CLASS_AND_FREQ = 0,
	AFC_SPEC_FREQ_THEN_OP_CLASS,
	AFC_SPEC_OP_CLASS_THEN_FREQ,
	AFC_SPEC_FREQ_ONLY,
	AFC_SPEC_OP_CLASS_ONLY,
	AFC_MAX_SPECTRUM_TYPE
};

enum AFC_TXPWR_IDX {
	AFC_TXPWR_CCK_IDX = 0,
	AFC_TXPWR_OFDM_IDX = 4,
	AFC_TXPWR_HT20_IDX = 12,
	AFC_TXPWR_HT40_IDX = 20,
	AFC_TXPWR_HT40_MCS32_IDX = 28,
	AFC_TXPWR_VHT20_IDX = 29,
	AFC_TXPWR_VHT40_IDX = 41,
	AFC_TXPWR_VHT80_IDX = 53,
	AFC_TXPWR_VHT160_IDX = 65,
	AFC_TXPWR_RU26_IDX = 77,
	AFC_TXPWR_RU52_IDX = 89,
	AFC_TXPWR_RU106_IDX = 101,
	AFC_TXPWR_RU242_IDX = 113,
	AFC_TXPWR_RU484_IDX = 125,
	AFC_TXPWR_RU996_IDX = 137,
	AFC_TXPWR_RU996X2_IDX = 149,
	AFC_TXPWR_END_IDX = 161
};

enum AFC_BACKOFF_START_OFFSET {
	AFC_BACKOFF_CCK_BF_OFF_IDX = 0,
	AFC_BACKOFF_OFDM_BF_OFF_IDX = 4,
	AFC_BACKOFF_OFDM_BF_ON_IDX = 8,
	AFC_BACKOFF_HT20_BF_OFF_IDX = 11,
	AFC_BACKOFF_HT20_BF_ON_IDX = 21,
	AFC_BACKOFF_HT40_BF_OFF_IDX = 30,
	AFC_BACKOFF_HT40_BF_ON_IDX = 40,
	AFC_BACKOFF_VHT20_BF_OFF_IDX = 49,
	AFC_BACKOFF_VHT20_BF_ON_IDX = 59,
	AFC_BACKOFF_VHT40_BF_OFF_IDX = 68,
	AFC_BACKOFF_VHT40_BF_ON_IDX = 78,
	AFC_BACKOFF_VHT80_BF_OFF_IDX = 87,
	AFC_BACKOFF_VHT80_BF_ON_IDX = 97,
	AFC_BACKOFF_VHT160_BF_OFF_IDX = 106,
	AFC_BACKOFF_VHT160_BF_ON_IDX = 116,
	AFC_BACKOFF_RU26_BF_OFF_IDX = 125,
	AFC_BACKOFF_RU26_BF_ON_IDX = 135,
	AFC_BACKOFF_RU52_BF_OFF_IDX = 144,
	AFC_BACKOFF_RU52_BF_ON_IDX = 155,
	AFC_BACKOFF_RU106_BF_OFF_IDX = 165,
	AFC_BACKOFF_RU106_BF_ON_IDX = 175,
	AFC_BACKOFF_END_IDX = 185
};

/*******************************************************************************
 *    MACRO
 ******************************************************************************/

/*******************************************************************************
 *    TYPES
 ******************************************************************************/
struct afc_frequency_range {
	UINT16 lowFrequency;
	UINT16 highFrequency;
};

struct afc_inquiredFrequencyRange {
	UINT8 NumOfOpFreqRange;
	struct afc_frequency_range *freqRange;
};

struct afc_inquiredFrequency {
	UINT8 NumOfOpFreqRange;
	struct afc_frequency_range freqRange[0];
};

struct afc_glblOperClass {
	UINT8 Opclass;
	UINT8 NumofChannels;
	UINT8 ChannelList[59];
};

struct afcSkipChannelOpClass {
	UINT8 OPClassEnable[AFC_OP_CLASS_NUM];
	UINT8 OPClassNumOfChannel[AFC_OP_CLASS_NUM];
	UINT8 OPClass131[AFC_OP_CLASS_131_CHL_NUM];
	UINT8 OPClass132[AFC_OP_CLASS_132_CHL_NUM];
	UINT8 OPClass133[AFC_OP_CLASS_133_CHL_NUM];
	UINT8 OPClass134[AFC_OP_CLASS_134_CHL_NUM];
	UINT8 OPClass135[AFC_OP_CLASS_135_CHL_NUM];
	UINT8 OPClass136[AFC_OP_CLASS_136_CHL_NUM];
};
struct afc_device_info {
	UINT8 regionCode[6];
	UINT8 SpectrumType;
	UINT8 glblOperClassNum;
	struct afc_glblOperClass glblOperClass[6];
	struct afc_inquiredFrequency InqfreqRange;
};

struct afc_response {
	UINT16 status;
	UINT8 Resv[2];
	UINT8 data[0];
};

struct AFC_TX_PWR_INFO {
	INT8 max_psd_bw20[MAX_20MHZ_CHANNEL_IN_6G_UNII];
	INT8 max_eirp_bw20[MAX_20MHZ_CHANNEL_IN_6G_UNII];
	INT8 max_eirp_bw40[MAX_40MHZ_CHANNEL_IN_6G_UNII];
	INT8 max_eirp_bw80[MAX_80MHZ_CHANNEL_IN_6G_UNII];
	INT8 max_eirp_bw160[MAX_160MHZ_CHANNEL_IN_6G_UNII];
};

struct AFC_RESPONSE_DATA {
	struct AFC_TX_PWR_INFO afc_txpwr_info[BAND_6G_UNII_NUM];
	UINT32 expiry_time;
	UINT16 response_code;
};

struct AFC_CTRL {
	STATE_MACHINE_FUNC AfcFunc[AFC_FUNC_SIZE];
	STATE_MACHINE AfcStateMachine;
	UCHAR AfcAutoChannelSkipListNum6G; /* number of rejected channel list for 6G */
	UCHAR AfcAutoChannelSkipList6G[MAX_NUM_OF_CHANNELS + 1];
	BOOLEAN bAfcBeaconInit;
	BOOLEAN bAfcTimer;
	UINT8 u1AfcTimeCnt;
	UINT8 u1FlagEirp20;
};

/*******************************************************************************
 *    GLOBAL VARIABLES
 ******************************************************************************/

/*******************************************************************************
 *    FUNCTION PROTOTYPES
 ******************************************************************************/

/** SET **/
INT afc_cmd_set_afc_params(IN struct _RTMP_ADAPTER *pAd, IN RTMP_STRING * arg);

/** GET **/
INT afc_cmd_show_afc_params(IN struct _RTMP_ADAPTER *pAd, IN RTMP_STRING * arg);
INT afc_cmd_show_skip_channel(IN struct _RTMP_ADAPTER *pAd, IN RTMP_STRING *arg);

VOID afc_save_switch_channel_params(
	struct _RTMP_ADAPTER *pAd, UCHAR band_idx, struct freq_oper *oper, BOOLEAN bScan);
VOID ap_update_rf_ch_for_mbss(
	struct _RTMP_ADAPTER *ad, struct wifi_dev *wdev, struct freq_oper *OperCh);
VOID UpdatedRaBfInfoBwByWdev(
	struct _RTMP_ADAPTER *pAd, struct wifi_dev *wdev, UINT_8 BW);
INT afc_update_channel_params(
	struct _RTMP_ADAPTER *pAd, UINT ifIndex);


/* Event Handler andes_mt.c */
VOID event_afc_handler(struct _RTMP_ADAPTER *pAd, UINT8 *Data, UINT_32 Length);

INT afc_set_beacon_bw(struct _RTMP_ADAPTER *pAd, char *arg);
INT afc_test_mode(struct _RTMP_ADAPTER *pAd, char *arg);
INT set_afc_event(struct _RTMP_ADAPTER *pAd, char *arg);
INT set_afc_device_type(struct _RTMP_ADAPTER *pAd, char *arg);
INT set_afc_spectrum_type(struct _RTMP_ADAPTER *pAd, char *arg);
INT set_afc_freq_range(struct _RTMP_ADAPTER	*pAd, char *arg);
INT set_afc_op_class131(struct _RTMP_ADAPTER	*pAd, char *arg);
INT set_afc_op_class132(struct _RTMP_ADAPTER	*pAd, char *arg);
INT set_afc_op_class133(struct _RTMP_ADAPTER	*pAd, char *arg);
INT set_afc_op_class134(struct _RTMP_ADAPTER	*pAd, char *arg);
INT set_afc_op_class136(struct _RTMP_ADAPTER	*pAd, char *arg);
void afc_update_params_from_response(struct _RTMP_ADAPTER *pAd, UINT8 *buf_data, UINT32 buf_len);

NDIS_STATUS afc_daemon_response(struct _RTMP_ADAPTER *pAd, struct __RTMP_IOCTL_INPUT_STRUCT *wrq, POS_COOKIE pObj);

int afc_daemon_channel_info(struct _RTMP_ADAPTER *pAd, struct __RTMP_IOCTL_INPUT_STRUCT *wrq, POS_COOKIE pObj);
INT afc_get_channel_index(UINT8 u1DesiredChannel);
UINT8 afc_pwr_calculation(struct _RTMP_ADAPTER *pAd, UINT8 u1ParamIdx, UINT16 u2AfcChannelIdx, INT8 i1PwrValue, UINT8 TblType);
INT afc_get_pwr_value(UINT8 u1ParamIdx, UINT16 u2RowIndex, UINT8 TblType);
VOID afc_save_autochannel_skip_init(struct _RTMP_ADAPTER *pAd);
VOID afc_update_auto_channel_skip_list(IN struct _RTMP_ADAPTER *pAd);
VOID afc_dump_skip_channel_list(IN struct _RTMP_ADAPTER *pAd);
VOID afc_dump_original_skip_channel_list(IN struct _RTMP_ADAPTER *pAd);
INT8 afc_get_min_pwr(INT8 i1PwrVal, INT8 i1AfcPwr);
INT afc_use_acs_switch_channel(struct _RTMP_ADAPTER *pAd, UINT ifIndex);
INT afc_get_available_bw(struct _RTMP_ADAPTER *pAd, UINT8 *cfg_vht_bw, UINT8 *cfg_ht_bw);
INT afc_check_valid_channel_by_bw(struct _RTMP_ADAPTER *pAd, UCHAR Channel, UINT8 u1Bw);
INT afc_switch_channel(struct _RTMP_ADAPTER *pAd, UINT ifIndex);
VOID afc_initiate_request_to_daemon(struct _RTMP_ADAPTER *pAd, IN struct wifi_dev *wdev);
INT afc_check_tx_enable(IN struct _RTMP_ADAPTER *pAd, IN struct wifi_dev *wdev);
VOID afc_update_txpwr_envelope_params(struct wifi_dev *wdev, UINT8 *tx_pwr_bw, UINT8 u1NValue, UINT8 txpwr_interpretation);
INT afc_bcn_check(IN struct _RTMP_ADAPTER *pAd);
INT is_afc_in_run_state(IN struct _RTMP_ADAPTER *pAd);
INT is_afc_stop(void);
INT afc_beacon_init_handler(IN struct _RTMP_ADAPTER *pAd, IN struct wifi_dev *wdev);
VOID afc_send_daemon_stop_event(IN struct _RTMP_ADAPTER *pAd, IN struct wifi_dev *wdev);
VOID afc_send_inquiry_event_timer(IN struct _RTMP_ADAPTER *pAd);
VOID afc_free_spectrum(void);

VOID afc_init(IN struct _RTMP_ADAPTER *pAd, IN struct wifi_dev *wdev);
VOID afc_update_freq_num(struct _RTMP_ADAPTER *pAd, INT NumOfFreqRanges);
VOID afc_update_freq_range(INT i, INT Index, UINT16 Frequency);
VOID afc_update_op_class_en(UINT OpClass, UINT8 Enable);
VOID afc_update_op_class_channel(UINT OpClass, UINT8 Index, UINT8 Channel);
VOID afc_update_op_class_channel_count(UINT OpClass, UINT8 Index);

VOID afc_check_pre_cond(IN struct _RTMP_ADAPTER *pAd, IN struct wifi_dev *wdev);
VOID afc_response_fail_update_to_hold(struct _RTMP_ADAPTER *pAd, UINT ifIndex);
VOID afc_update_state_to_disable(struct _RTMP_ADAPTER *pAd);
VOID afc_enable_tx(struct _RTMP_ADAPTER *pAd, UINT ifIndex);
VOID afc_inf_up_start_inquire_request(struct _RTMP_ADAPTER *pAd);
#endif /*CONFIG_6G_SUPPORT && */
		/*CONFIG_6G_AFC_SUPPORT && DOT11_HE_AX*/
#endif /* __AFC_H__ */

