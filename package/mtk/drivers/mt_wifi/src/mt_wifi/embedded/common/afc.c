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
	afc.c
*/
#include "rt_config.h"
#include "ap.h"

#if defined(CONFIG_6G_SUPPORT) && defined(CONFIG_6G_AFC_SUPPORT) && defined(DOT11_HE_AX)
/*******************************************************************************
 *    MACRO
 ******************************************************************************/

/*******************************************************************************
 *    DEFINITIONS
 ******************************************************************************/

#define AFC_PWR_LIMIT_TBL_PATH            "/etc/wireless/mediatek/AFCPwrLimitTbl.dat"
#define AFC_6G_FIRST_CHANNEL_INDEX        1
#define AFC_6G_LAST_CHANNEL_INDEX         233
#define AFC_6G_CHANNEL_FREQ_GAP           4

/* (((AFC_6G_LAST_CHANNEL_INDEX - AFC_6G_FIRST_CHANNEL_INDEX)/AFC_6G_CHANNEL_FREQ_GAP) + 1) */
#define AFC_6G_CHANNEL_20MHZ_NUM          59
#define AFC_6G_PWR_LIMIT_TBL_COL          7
#define AFC_6G_PWR_LIMIT_TBL_ROW          AFC_6G_CHANNEL_20MHZ_NUM

/* number of channels in unii-5 6G band */
#define AFC_20MHZ_CHAN_UNII_5             24
#define AFC_40MHZ_CHAN_UNII_5             12
#define AFC_80MHZ_CHAN_UNII_5             6
#define AFC_160MHZ_CHAN_UNII_5            3

/* number of channels in unii-7 6G band */
#define AFC_20MHZ_CHAN_UNII_7             17
#define AFC_40MHZ_CHAN_UNII_7             8
#define AFC_80MHZ_CHAN_UNII_7             3
#define AFC_160MHZ_CHAN_UNII_7            1

#define AFC_UNII_5_20MHZ_FIRST_CHAN_IDX   1
#define AFC_UNII_5_20MHZ_LAST_CHAN_IDX    93
#define AFC_UNII_5_40MHZ_FIRST_CHAN_IDX   1
#define AFC_UNII_5_40MHZ_LAST_CHAN_IDX    93
#define AFC_UNII_5_80MHZ_FIRST_CHAN_IDX   1
#define AFC_UNII_5_80MHZ_LAST_CHAN_IDX    93
#define AFC_UNII_5_160MHZ_FIRST_CHAN_IDX  1
#define AFC_UNII_5_160MHZ_LAST_CHAN_IDX   93
#define AFC_UNII_7_20MHZ_FIRST_CHAN_IDX   117
#define AFC_UNII_7_20MHZ_LAST_CHAN_IDX    181
#define AFC_UNII_7_40MHZ_FIRST_CHAN_IDX   121
#define AFC_UNII_7_40MHZ_LAST_CHAN_IDX    181
#define AFC_UNII_7_80MHZ_FIRST_CHAN_IDX   129
#define AFC_UNII_7_80MHZ_LAST_CHAN_IDX    173
#define AFC_UNII_7_160MHZ_FIRST_CHAN_IDX  129
#define AFC_UNII_7_160MHZ_LAST_CHAN_IDX   157

#define AFC_UNII_5_20MHZ_FIRST_ROW_IDX    0    /* Chan 1 */
#define AFC_UNII_5_20MHZ_LAST_ROW_IDX     23   /* Chan 93 */
#define AFC_UNII_5_40MHZ_FIRST_ROW_IDX    0    /* Chan 1 */
#define AFC_UNII_5_40MHZ_LAST_ROW_IDX     23   /* Chan 93 */
#define AFC_UNII_5_80MHZ_FIRST_ROW_IDX    0    /* Chan 1 */
#define AFC_UNII_5_80MHZ_LAST_ROW_IDX     23   /* Chan 93 */
#define AFC_UNII_5_160MHZ_FIRST_ROW_IDX   0    /* Chan 1 */
#define AFC_UNII_5_160MHZ_LAST_ROW_IDX    23   /* Chan 93 */
#define AFC_UNII_7_20MHZ_FIRST_ROW_IDX    29   /* Chan 117 */
#define AFC_UNII_7_20MHZ_LAST_ROW_IDX     45   /* Chan 181 */
#define AFC_UNII_7_40MHZ_FIRST_ROW_IDX    30   /* Chan 121 */
#define AFC_UNII_7_40MHZ_LAST_ROW_IDX     45   /* Chan 181 */
#define AFC_UNII_7_80MHZ_FIRST_ROW_IDX    32   /* Chan 129 */
#define AFC_UNII_7_80MHZ_LAST_ROW_IDX     43   /* Chan 173 */
#define AFC_UNII_7_160MHZ_FIRST_ROW_IDX   32   /* Chan 129 */
#define AFC_UNII_7_160MHZ_LAST_ROW_IDX    39   /* Chan 157 */

#define NON_AFC_CHANNEL                   126
#define AFC_TXPWR_VAL_INVALID             127
#define AFC_TXPWR_20MHZ_DELTA_PSD         26   /* 10log(20) = 13 db */
#define AFC_TXPWR_DOUBLE_IN_DB            6    /* 10log(2) = 3 db */

#define AFC_NUM_20MHZ_IN_20MHZ            1
#define AFC_NUM_20MHZ_IN_40MHZ            2
#define AFC_NUM_20MHZ_IN_80MHZ            4
#define AFC_NUM_20MHZ_IN_160MHZ           8

#define AFC_CEN_CH_BW_20_START            1
#define AFC_CEN_CH_BW_40_START            3
#define AFC_CEN_CH_BW_80_START            7
#define AFC_CEN_CH_BW_160_START           15

#define AFC_RU26_TXPWR_OFFSET_WITH_20MHZ  20 /* 10 db */
#define AFC_RU52_TXPWR_OFFSET_WITH_20MHZ  14 /* 7 db */
#define AFC_RU106_TXPWR_OFFSET_WITH_20MHZ 8  /* 4 db */

#define AFC_MAX_PSD  46 /* 23 db */
#define AFC_MAX_EIRP 72 /* 36 db */

#define IS_AFC_IDLE_STATE(pAd)      (pAd->afc_ctrl.AfcStateMachine.CurrState == AFC_IDLE)
#define IS_AFC_DISABLE_STATE(pAd)   (pAd->afc_ctrl.AfcStateMachine.CurrState == AFC_DISABLE)
#define IS_AFC_HOLD_STATE(pAd)      (pAd->afc_ctrl.AfcStateMachine.CurrState == AFC_HOLD)
#define IS_AFC_RUN_STATE(pAd)       (pAd->afc_ctrl.AfcStateMachine.CurrState == AFC_RUN)
#define IS_AFC_ENABLED(pAd)         (IS_AFC_HOLD_STATE(pAd) || IS_AFC_RUN_STATE(pAd))

#define AFC_TXPWR_ENV_NVAL_MAX            8  /* For CBW160 */
#define AFC_TXPWR_ENV_MAX_PSD             34 /* 17 dBm */
#define AFC_TXPWR_ENV_MAX_EIRP            60 /* 30 dBm */

#define AFC_6G_WDEV_NOT_AVAILABLE         255
/*******************************************************************************
 *    PRIVATE TYPES
 ******************************************************************************/

enum AFC_BW {
	AFC_BW_20,
	AFC_BW_40,
	AFC_BW_80,
	AFC_BW_160,
	AFC_BW_NUM
};

enum AFC_SET_PARAM_REF {
	AFC_RESET_AFC_PARAMS,
	AFC_SET_EIRP_PARAM,
	AFC_SET_PSD_PARAM,
	AFC_SET_EXPIRY_PARAM,
	AFC_SET_RESP_PARAM,
	AFC_UPD_CHANNEL_PARAMS,
	AFC_CFG_PWR_LMT_TBL,
	AFC_UPD_FW_TX_PWR,
	AFC_SET_TX_ENABLE,
	AFC_SET_AFC_DISABLE,
	AFC_SET_LOCK,
	AFC_SET_PARAM_NUM
};

UINT8 max_afc_chan_idx[BAND_6G_UNII_NUM][AFC_BW_NUM] = {
		{AFC_20MHZ_CHAN_UNII_5, AFC_40MHZ_CHAN_UNII_5,
			AFC_80MHZ_CHAN_UNII_5, AFC_160MHZ_CHAN_UNII_5},
		{AFC_20MHZ_CHAN_UNII_7, AFC_40MHZ_CHAN_UNII_7,
			AFC_80MHZ_CHAN_UNII_7, AFC_160MHZ_CHAN_UNII_7}
	};


INT8 g_afc_channel_pwr[AFC_6G_PWR_LIMIT_TBL_COL][AFC_6G_PWR_LIMIT_TBL_ROW];
struct afc_inquiredFrequencyRange  g_afc_Freq_range_tbl;
struct afcSkipChannelOpClass g_afc_opclass_skip_channel;
struct afc_channel_switch_params {
	struct freq_oper oper;
	UCHAR band_idx;
	BOOLEAN bScan;
	UCHAR Reserved[2];
};

struct afc_channel_switch_params _afc_switch_chan_params;

UINT _afc_daemon_lock = TRUE;
static UINT _is_afc_stop = FALSE;
/*******************************************************************************
 *    PRIVATE FUNCTION PROTOTYPES
 ******************************************************************************/

static void afc_update_pwr_limit_table(struct _RTMP_ADAPTER *pAd);
VOID afc_set_wdev_to_hold(struct _RTMP_ADAPTER *pAd, struct wifi_dev *wdev);
static INT afc_show_afc_params(IN struct _RTMP_ADAPTER *pAd);
static VOID afc_table_txpwr_print(struct _RTMP_ADAPTER *pAd);
static UINT8 afc_check_params_valid_channels(struct _RTMP_ADAPTER *pAd);

/*******************************************************************************
 *    PRIVATE FUNCTIONS
 ******************************************************************************/

static UINT_8 afc_get_num20MHz_in_bw(UINT_8 afc_bw)
{
	UINT_8 num20 = 0;

	if (afc_bw > AFC_TXPWR_TBL_BW160)
		return 0;

	switch (afc_bw) {
	case AFC_TXPWR_TBL_BW20:
		num20 = AFC_NUM_20MHZ_IN_20MHZ;
		break;

	case AFC_TXPWR_TBL_BW40:
		num20 = AFC_NUM_20MHZ_IN_40MHZ;
		break;

	case AFC_TXPWR_TBL_BW80:
		num20 = AFC_NUM_20MHZ_IN_80MHZ;
		break;

	case AFC_TXPWR_TBL_BW160:
		num20 = AFC_NUM_20MHZ_IN_160MHZ;
		break;

	default:
		break;
	}

	return num20;
}

static VOID afc_get_eirp_val_per_bw(
	struct _RTMP_ADAPTER *pAd, UINT_8 subband, UINT_8 afc_bw, INT8 **eirp
)
{
	struct AFC_TX_PWR_INFO *pwr_info = &pAd->afc_response_data.afc_txpwr_info[subband];

	switch (afc_bw) {

	case AFC_TXPWR_TBL_BW20:
		*eirp = (INT8 *)pwr_info->max_eirp_bw20;
		break;

	case AFC_TXPWR_TBL_BW40:
		*eirp = (INT8 *)pwr_info->max_eirp_bw40;
		break;

	case AFC_TXPWR_TBL_BW80:
		*eirp = (INT8 *)pwr_info->max_eirp_bw80;
		break;

	case AFC_TXPWR_TBL_BW160:
		*eirp = (INT8 *)pwr_info->max_eirp_bw160;
		break;

	default:
		break;
	}
}

static VOID afc_table_get_offset_params(
	UINT_8 subband, UINT_8 afc_bw,
	UINT_8 *row_offset, UINT_8 *psd_max, UINT_8 *bw_offset
)
{
	switch (afc_bw) {

	case AFC_TXPWR_TBL_BW20:
		if (subband == BAND_6G_UNII_5) {
			*row_offset = AFC_UNII_5_20MHZ_FIRST_ROW_IDX;
			*psd_max = AFC_20MHZ_CHAN_UNII_5;
		} else {
			*row_offset = AFC_UNII_7_20MHZ_FIRST_ROW_IDX;
			*psd_max = AFC_20MHZ_CHAN_UNII_7;
		}
		*bw_offset = AFC_NUM_20MHZ_IN_20MHZ;
		break;

	case AFC_TXPWR_TBL_BW40:
		if (subband == BAND_6G_UNII_5) {
			*row_offset = AFC_UNII_5_40MHZ_FIRST_ROW_IDX;
			*psd_max = AFC_40MHZ_CHAN_UNII_5;
		} else {
			*row_offset = AFC_UNII_7_40MHZ_FIRST_ROW_IDX;
			*psd_max = AFC_40MHZ_CHAN_UNII_7;
		}
		*bw_offset = AFC_NUM_20MHZ_IN_40MHZ;
		break;

	case AFC_TXPWR_TBL_BW80:
		if (subband == BAND_6G_UNII_5) {
			*row_offset = AFC_UNII_5_80MHZ_FIRST_ROW_IDX;
			*psd_max = AFC_80MHZ_CHAN_UNII_5;
		} else {
			*row_offset = AFC_UNII_7_80MHZ_FIRST_ROW_IDX;
			*psd_max = AFC_80MHZ_CHAN_UNII_7;
		}
		*bw_offset = AFC_NUM_20MHZ_IN_80MHZ;
		break;

	case AFC_TXPWR_TBL_BW160:
		if (subband == BAND_6G_UNII_5) {
			*row_offset = AFC_UNII_5_160MHZ_FIRST_ROW_IDX;
			*psd_max = AFC_160MHZ_CHAN_UNII_5;
		} else {
			*row_offset = AFC_UNII_7_160MHZ_FIRST_ROW_IDX;
			*psd_max = AFC_160MHZ_CHAN_UNII_7;
		}
		*bw_offset = AFC_NUM_20MHZ_IN_160MHZ;
		break;

	default:
		break;
	}
}

static INT8 afc_table_get_ru_txpwr_offset_from_20mhz(UINT_8 ColIndex)
{
	INT8 offset;

	switch (ColIndex) {
	case AFC_TXPWR_TBL_RU26:
		offset = AFC_RU26_TXPWR_OFFSET_WITH_20MHZ;
		break;

	case AFC_TXPWR_TBL_RU52:
		offset = AFC_RU52_TXPWR_OFFSET_WITH_20MHZ;
		break;

	case AFC_TXPWR_TBL_RU106:
		offset = AFC_RU106_TXPWR_OFFSET_WITH_20MHZ;
		break;

	default:
		break;
	}

	return offset;
}

static UINT8 afc_check_non_overlap_channel_freq(struct AFC_TX_PWR_INFO *pwr_info, UINT8 PsdIndex)
{
	if ((pwr_info->max_psd_bw20[PsdIndex] != AFC_TXPWR_VAL_INVALID)
		&& (pwr_info->max_eirp_bw20[PsdIndex] == AFC_TXPWR_VAL_INVALID))
		return TRUE;

	if ((pwr_info->max_psd_bw20[PsdIndex] == AFC_TXPWR_VAL_INVALID)
		&& (pwr_info->max_eirp_bw20[PsdIndex] != AFC_TXPWR_VAL_INVALID))
		return TRUE;

	return FALSE;
}

static VOID afc_fill_void_params(struct _RTMP_ADAPTER *pAd)
{
	UINT_8 PsdIndex;
	BOOLEAN IsEirpUpdate = FALSE;
	struct AFC_TX_PWR_INFO *pwr_info;


	if (pAd->CommonCfg.AfcSpectrumType == AFC_SPEC_OP_CLASS_ONLY) {
		pwr_info = &pAd->afc_response_data.afc_txpwr_info[BAND_6G_UNII_5];
		for (PsdIndex = 0; PsdIndex < AFC_20MHZ_CHAN_UNII_5; PsdIndex++)
			pwr_info->max_psd_bw20[PsdIndex] = AFC_MAX_PSD;

		pwr_info = &pAd->afc_response_data.afc_txpwr_info[BAND_6G_UNII_7];
		for (PsdIndex = 0; PsdIndex < AFC_20MHZ_CHAN_UNII_7; PsdIndex++)
			pwr_info->max_psd_bw20[PsdIndex] = AFC_MAX_PSD;

		if (pAd->afc_ctrl.u1FlagEirp20 == TRUE) {
			pwr_info = &pAd->afc_response_data.afc_txpwr_info[BAND_6G_UNII_5];
			for (PsdIndex = 0; PsdIndex < AFC_20MHZ_CHAN_UNII_5; PsdIndex++) {
				if (afc_check_non_overlap_channel_freq(pwr_info, PsdIndex)) {
					if (pwr_info->max_psd_bw20[PsdIndex] != AFC_TXPWR_VAL_INVALID)
						pwr_info->max_eirp_bw20[PsdIndex] = pwr_info->max_psd_bw20[PsdIndex] + AFC_TXPWR_20MHZ_DELTA_PSD;
					}
			}

			pwr_info = &pAd->afc_response_data.afc_txpwr_info[BAND_6G_UNII_7];
			for (PsdIndex = 0; PsdIndex < AFC_20MHZ_CHAN_UNII_7; PsdIndex++) {
				if (afc_check_non_overlap_channel_freq(pwr_info, PsdIndex)) {
					if (pwr_info->max_psd_bw20[PsdIndex] != AFC_TXPWR_VAL_INVALID)
						pwr_info->max_eirp_bw20[PsdIndex] = pwr_info->max_psd_bw20[PsdIndex] + AFC_TXPWR_20MHZ_DELTA_PSD;
					}
			}
		}
	}

	if (pAd->CommonCfg.AfcSpectrumType == AFC_SPEC_FREQ_ONLY) {
		pwr_info = &pAd->afc_response_data.afc_txpwr_info[BAND_6G_UNII_5];
		for (PsdIndex = 0; PsdIndex < AFC_20MHZ_CHAN_UNII_5; PsdIndex++)
			pwr_info->max_eirp_bw20[PsdIndex] = AFC_MAX_EIRP;

		pwr_info = &pAd->afc_response_data.afc_txpwr_info[BAND_6G_UNII_5];
		for (PsdIndex = 0; PsdIndex < AFC_40MHZ_CHAN_UNII_5; PsdIndex++)
			pwr_info->max_eirp_bw40[PsdIndex] = AFC_MAX_EIRP;

		pwr_info = &pAd->afc_response_data.afc_txpwr_info[BAND_6G_UNII_5];
		for (PsdIndex = 0; PsdIndex < AFC_80MHZ_CHAN_UNII_5; PsdIndex++)
			pwr_info->max_eirp_bw80[PsdIndex] = AFC_MAX_EIRP;

		pwr_info = &pAd->afc_response_data.afc_txpwr_info[BAND_6G_UNII_5];
		for (PsdIndex = 0; PsdIndex < AFC_160MHZ_CHAN_UNII_5; PsdIndex++)
			pwr_info->max_eirp_bw160[PsdIndex] = AFC_MAX_EIRP;

		pwr_info = &pAd->afc_response_data.afc_txpwr_info[BAND_6G_UNII_7];
		for (PsdIndex = 0; PsdIndex < AFC_20MHZ_CHAN_UNII_7; PsdIndex++)
			pwr_info->max_eirp_bw20[PsdIndex] = AFC_MAX_EIRP;

		pwr_info = &pAd->afc_response_data.afc_txpwr_info[BAND_6G_UNII_7];
		for (PsdIndex = 0; PsdIndex < AFC_40MHZ_CHAN_UNII_7; PsdIndex++)
			pwr_info->max_eirp_bw40[PsdIndex] = AFC_MAX_EIRP;

		pwr_info = &pAd->afc_response_data.afc_txpwr_info[BAND_6G_UNII_7];
		for (PsdIndex = 0; PsdIndex < AFC_80MHZ_CHAN_UNII_7; PsdIndex++)
			pwr_info->max_eirp_bw80[PsdIndex] = AFC_MAX_EIRP;

		pwr_info = &pAd->afc_response_data.afc_txpwr_info[BAND_6G_UNII_7];
		for (PsdIndex = 0; PsdIndex < AFC_160MHZ_CHAN_UNII_7; PsdIndex++)
			pwr_info->max_eirp_bw160[PsdIndex] = AFC_MAX_EIRP;
	}

	if (pAd->CommonCfg.AfcSpectrumType <= AFC_SPEC_OP_CLASS_THEN_FREQ) {

		// update for UNI_BAND5
		pwr_info = &pAd->afc_response_data.afc_txpwr_info[BAND_6G_UNII_5];
		for (PsdIndex = 0; PsdIndex < AFC_20MHZ_CHAN_UNII_5; PsdIndex++) {
			if (afc_check_non_overlap_channel_freq(pwr_info, PsdIndex)) {
				if (pwr_info->max_psd_bw20[PsdIndex] != AFC_TXPWR_VAL_INVALID) {
					pwr_info->max_eirp_bw20[PsdIndex] = pwr_info->max_psd_bw20[PsdIndex] + AFC_TXPWR_20MHZ_DELTA_PSD;
					IsEirpUpdate = TRUE;
				} else
					pwr_info->max_psd_bw20[PsdIndex] = pwr_info->max_eirp_bw20[PsdIndex] - AFC_TXPWR_20MHZ_DELTA_PSD;
				}
		}

		if (IsEirpUpdate) {
			for (PsdIndex = 0; PsdIndex < AFC_40MHZ_CHAN_UNII_5; PsdIndex++) {
				if (pwr_info->max_eirp_bw40[PsdIndex] == AFC_TXPWR_VAL_INVALID)
					pwr_info->max_eirp_bw40[PsdIndex] = pwr_info->max_eirp_bw20[PsdIndex] + AFC_TXPWR_DOUBLE_IN_DB;
			}

			for (PsdIndex = 0; PsdIndex < AFC_80MHZ_CHAN_UNII_5; PsdIndex++) {
				if (pwr_info->max_eirp_bw80[PsdIndex] == AFC_TXPWR_VAL_INVALID)
					pwr_info->max_eirp_bw80[PsdIndex] = pwr_info->max_eirp_bw40[PsdIndex] + AFC_TXPWR_DOUBLE_IN_DB;
			}

			for (PsdIndex = 0; PsdIndex < AFC_160MHZ_CHAN_UNII_5; PsdIndex++) {
				if (pwr_info->max_eirp_bw160[PsdIndex] == AFC_TXPWR_VAL_INVALID)
					pwr_info->max_eirp_bw160[PsdIndex] = pwr_info->max_eirp_bw80[PsdIndex] + AFC_TXPWR_DOUBLE_IN_DB;
			}
		}

		// update for UNI_BAND7
		IsEirpUpdate = FALSE;
		pwr_info = &pAd->afc_response_data.afc_txpwr_info[BAND_6G_UNII_7];
		for (PsdIndex = 0; PsdIndex < AFC_20MHZ_CHAN_UNII_7; PsdIndex++) {
			if (afc_check_non_overlap_channel_freq(pwr_info, PsdIndex)) {
				if (pwr_info->max_psd_bw20[PsdIndex] != AFC_TXPWR_VAL_INVALID) {
					pwr_info->max_eirp_bw20[PsdIndex] = pwr_info->max_psd_bw20[PsdIndex] + AFC_TXPWR_20MHZ_DELTA_PSD;
					IsEirpUpdate = TRUE;
				} else
					pwr_info->max_psd_bw20[PsdIndex] = pwr_info->max_eirp_bw20[PsdIndex] - AFC_TXPWR_20MHZ_DELTA_PSD;
				}
		}

		if (IsEirpUpdate) {
			for (PsdIndex = 0; PsdIndex < AFC_40MHZ_CHAN_UNII_7; PsdIndex++) {
				if (pwr_info->max_eirp_bw40[PsdIndex] == AFC_TXPWR_VAL_INVALID)
					pwr_info->max_eirp_bw40[PsdIndex] = pwr_info->max_eirp_bw20[PsdIndex] + AFC_TXPWR_DOUBLE_IN_DB;
			}

			for (PsdIndex = 0; PsdIndex < AFC_80MHZ_CHAN_UNII_7; PsdIndex++) {
				if (pwr_info->max_eirp_bw80[PsdIndex] == AFC_TXPWR_VAL_INVALID)
					pwr_info->max_eirp_bw80[PsdIndex] = pwr_info->max_eirp_bw40[PsdIndex] + AFC_TXPWR_DOUBLE_IN_DB;
			}

			for (PsdIndex = 0; PsdIndex < AFC_160MHZ_CHAN_UNII_7; PsdIndex++) {
				if (pwr_info->max_eirp_bw160[PsdIndex] == AFC_TXPWR_VAL_INVALID)
					pwr_info->max_eirp_bw160[PsdIndex] = pwr_info->max_eirp_bw80[PsdIndex] + AFC_TXPWR_DOUBLE_IN_DB;
			}
		}

	}

}


static UINT8 afc_check_params_valid_channels(struct _RTMP_ADAPTER *pAd)
{
	UINT_8 PsdIndex;
	struct AFC_TX_PWR_INFO *pwr_info;

	pwr_info = &pAd->afc_response_data.afc_txpwr_info[BAND_6G_UNII_5];
	for (PsdIndex = 0; PsdIndex < AFC_20MHZ_CHAN_UNII_5; PsdIndex++) {
		if (pwr_info->max_psd_bw20[PsdIndex] < AFC_TXPWR_VAL_INVALID || pwr_info->max_eirp_bw20[PsdIndex] < AFC_TXPWR_VAL_INVALID)
			return TRUE;
	}

	for (PsdIndex = 0; PsdIndex < AFC_40MHZ_CHAN_UNII_5; PsdIndex++) {
		if (pwr_info->max_eirp_bw40[PsdIndex] < AFC_TXPWR_VAL_INVALID) {
			pAd->afc_ctrl.u1FlagEirp20 = TRUE;
			return TRUE;
		}
	}
	for (PsdIndex = 0; PsdIndex < AFC_80MHZ_CHAN_UNII_5; PsdIndex++) {
		if (pwr_info->max_eirp_bw80[PsdIndex] < AFC_TXPWR_VAL_INVALID) {
			pAd->afc_ctrl.u1FlagEirp20 = TRUE;
			return TRUE;
		}
	}
	for (PsdIndex = 0; PsdIndex < AFC_160MHZ_CHAN_UNII_5; PsdIndex++) {
		if (pwr_info->max_eirp_bw160[PsdIndex] < AFC_TXPWR_VAL_INVALID) {
			pAd->afc_ctrl.u1FlagEirp20 = TRUE;
			return TRUE;
		}
	}

	pwr_info = &pAd->afc_response_data.afc_txpwr_info[BAND_6G_UNII_7];
	for (PsdIndex = 0; PsdIndex < AFC_20MHZ_CHAN_UNII_7; PsdIndex++) {
		if (pwr_info->max_psd_bw20[PsdIndex] < AFC_TXPWR_VAL_INVALID || pwr_info->max_eirp_bw20[PsdIndex] < AFC_TXPWR_VAL_INVALID)
			return TRUE;
	}
	for (PsdIndex = 0; PsdIndex < AFC_40MHZ_CHAN_UNII_7; PsdIndex++) {
		if (pwr_info->max_eirp_bw40[PsdIndex] < AFC_TXPWR_VAL_INVALID) {
			pAd->afc_ctrl.u1FlagEirp20 = TRUE;
			return TRUE;
		}
	}
	for (PsdIndex = 0; PsdIndex < AFC_80MHZ_CHAN_UNII_7; PsdIndex++) {
		if (pwr_info->max_eirp_bw80[PsdIndex] < AFC_TXPWR_VAL_INVALID) {
			pAd->afc_ctrl.u1FlagEirp20 = TRUE;
			return TRUE;
		}
	}
	for (PsdIndex = 0; PsdIndex < AFC_160MHZ_CHAN_UNII_7; PsdIndex++) {
		if (pwr_info->max_eirp_bw160[PsdIndex] < AFC_TXPWR_VAL_INVALID) {
			pAd->afc_ctrl.u1FlagEirp20 = TRUE;
			return TRUE;
		}
	}
	return FALSE;
}

static VOID afc_table_update_non_afc_channels(struct _RTMP_ADAPTER *pAd)
{
	UINT_8 RowIndex, Index1;

	MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO,
			 "%s\n", __func__);

	/* Update 20 MHz non-AFC channel lists */
	for (RowIndex = 0; RowIndex < AFC_6G_PWR_LIMIT_TBL_ROW; RowIndex++) {
		Index1 = (RowIndex * AFC_6G_CHANNEL_FREQ_GAP) + 1;

		if (Index1 >= AFC_UNII_5_20MHZ_FIRST_CHAN_IDX && Index1 <= AFC_UNII_5_20MHZ_LAST_CHAN_IDX)
			continue;

		if (Index1 >= AFC_UNII_7_20MHZ_FIRST_CHAN_IDX && Index1 <= AFC_UNII_7_20MHZ_LAST_CHAN_IDX)
			continue;

		g_afc_channel_pwr[AFC_TXPWR_TBL_BW20][RowIndex] = NON_AFC_CHANNEL;
	}

	/* Update 40 MHz non-AFC channel lists */
	for (RowIndex = 0; RowIndex < AFC_6G_PWR_LIMIT_TBL_ROW; RowIndex++) {
		Index1 = (RowIndex * AFC_6G_CHANNEL_FREQ_GAP) + 1;

		if (Index1 >= AFC_UNII_5_40MHZ_FIRST_CHAN_IDX && Index1 <= AFC_UNII_5_40MHZ_LAST_CHAN_IDX)
			continue;

		if (Index1 >= AFC_UNII_7_40MHZ_FIRST_CHAN_IDX && Index1 <= AFC_UNII_7_40MHZ_LAST_CHAN_IDX)
			continue;

		g_afc_channel_pwr[AFC_TXPWR_TBL_BW40][RowIndex] = NON_AFC_CHANNEL;
	}

	/* Update 80 MHz non-AFC channel lists */
	for (RowIndex = 0; RowIndex < AFC_6G_PWR_LIMIT_TBL_ROW; RowIndex++) {
		Index1 = (RowIndex * AFC_6G_CHANNEL_FREQ_GAP) + 1;

		if (Index1 >= AFC_UNII_5_80MHZ_FIRST_CHAN_IDX && Index1 <= AFC_UNII_5_80MHZ_LAST_CHAN_IDX)
			continue;

		if (Index1 >= AFC_UNII_7_80MHZ_FIRST_CHAN_IDX && Index1 <= AFC_UNII_7_80MHZ_LAST_CHAN_IDX)
			continue;

		g_afc_channel_pwr[AFC_TXPWR_TBL_BW80][RowIndex] = NON_AFC_CHANNEL;
	}

	/* Update 160 MHz non-AFC channel lists */
	for (RowIndex = 0; RowIndex < AFC_6G_PWR_LIMIT_TBL_ROW; RowIndex++) {
		Index1 = (RowIndex * AFC_6G_CHANNEL_FREQ_GAP) + 1;

		if (Index1 >= AFC_UNII_5_160MHZ_FIRST_CHAN_IDX && Index1 <= AFC_UNII_5_160MHZ_LAST_CHAN_IDX)
			continue;

		if (Index1 >= AFC_UNII_7_160MHZ_FIRST_CHAN_IDX && Index1 <= AFC_UNII_7_160MHZ_LAST_CHAN_IDX)
			continue;

		g_afc_channel_pwr[AFC_TXPWR_TBL_BW160][RowIndex] = NON_AFC_CHANNEL;
	}
}

static VOID afc_table_update_bw20_txpwr_with_psd(struct _RTMP_ADAPTER *pAd)
{
	UINT_8 PsdIndex, RowIndex, i;
	UINT Index1;
	struct AFC_TX_PWR_INFO *pwr_info;

	pwr_info = &pAd->afc_response_data.afc_txpwr_info[BAND_6G_UNII_5];
	for (PsdIndex = 0; PsdIndex < AFC_20MHZ_CHAN_UNII_5; PsdIndex++) {
		RowIndex = PsdIndex + AFC_UNII_5_20MHZ_FIRST_ROW_IDX;

		if (pwr_info->max_psd_bw20[PsdIndex] != AFC_TXPWR_VAL_INVALID)
			g_afc_channel_pwr[AFC_TXPWR_TBL_BW20][RowIndex] = pwr_info->max_psd_bw20[PsdIndex] + AFC_TXPWR_20MHZ_DELTA_PSD;
		else
			g_afc_channel_pwr[AFC_TXPWR_TBL_BW20][RowIndex] = pwr_info->max_psd_bw20[PsdIndex];
	}

	pwr_info = &pAd->afc_response_data.afc_txpwr_info[BAND_6G_UNII_7];
	for (PsdIndex = 0; PsdIndex < AFC_20MHZ_CHAN_UNII_7; PsdIndex++) {
		RowIndex = PsdIndex + AFC_UNII_7_20MHZ_FIRST_ROW_IDX;

		if (pwr_info->max_psd_bw20[PsdIndex] != AFC_TXPWR_VAL_INVALID)
			g_afc_channel_pwr[AFC_TXPWR_TBL_BW20][RowIndex] = pwr_info->max_psd_bw20[PsdIndex] + AFC_TXPWR_20MHZ_DELTA_PSD;
		else
			g_afc_channel_pwr[AFC_TXPWR_TBL_BW20][RowIndex] = pwr_info->max_psd_bw20[PsdIndex];
	}

	MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO,
			 "\n\n%s:: Step 2\n", __func__);
	afc_table_txpwr_print(pAd);

	for (RowIndex = 0; RowIndex < AFC_6G_PWR_LIMIT_TBL_ROW; RowIndex++) {
		Index1 = (RowIndex * AFC_6G_CHANNEL_FREQ_GAP) + 1;

		for (i = 0; i < pAd->ApCfg.AutoChannelSkipListNum6G; i++) {
			if (pAd->afc_ctrl.AfcAutoChannelSkipList6G[i] == Index1)
				g_afc_channel_pwr[AFC_TXPWR_TBL_BW20][RowIndex] = AFC_TXPWR_VAL_INVALID;
		}
	}
}

static VOID afc_table_update_txpwr_with_psd(struct _RTMP_ADAPTER *pAd)
{
	UINT_8 RowIndex, ColIndex;

	afc_table_update_bw20_txpwr_with_psd(pAd);

	MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO,
			 "\n\n%s:: Step 3\n", __func__);
	afc_table_txpwr_print(pAd);

	for (ColIndex = AFC_TXPWR_TBL_BW40; ColIndex <= AFC_TXPWR_TBL_BW160; ColIndex++) {
		for (RowIndex = 0; RowIndex < AFC_6G_PWR_LIMIT_TBL_ROW; RowIndex++) {
			if (g_afc_channel_pwr[ColIndex][RowIndex] == NON_AFC_CHANNEL)
				continue;

			if (g_afc_channel_pwr[AFC_TXPWR_TBL_BW20][RowIndex] != AFC_TXPWR_VAL_INVALID)
				g_afc_channel_pwr[ColIndex][RowIndex] = g_afc_channel_pwr[AFC_TXPWR_TBL_BW20][RowIndex] + (AFC_TXPWR_DOUBLE_IN_DB * ColIndex);
			else
				g_afc_channel_pwr[ColIndex][RowIndex] = g_afc_channel_pwr[AFC_TXPWR_TBL_BW20][RowIndex];
		}
	}

	MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			 "\n\n%s:: Step 4\n", __func__);
	afc_table_txpwr_print(pAd);

}

static INT8 afc_table_check_min_txpwr(
	UINT_8 start_index, UINT_8 afc_bw, UINT_8 uniband)
{
	UINT_8 RowIndex, MaxIndex;
	INT8 min_txpwr = AFC_TXPWR_VAL_INVALID;

	if (afc_bw > AFC_TXPWR_TBL_BW160)
		return AFC_TXPWR_VAL_INVALID;

	MaxIndex = start_index + afc_get_num20MHz_in_bw(afc_bw) - 1;

	for (RowIndex = start_index; RowIndex <= MaxIndex; RowIndex++) {
		if (g_afc_channel_pwr[afc_bw][RowIndex] == AFC_TXPWR_VAL_INVALID) {
			min_txpwr = AFC_TXPWR_VAL_INVALID;
			break;
		}

		min_txpwr = min(min_txpwr, g_afc_channel_pwr[afc_bw][RowIndex]);
	}

	return min_txpwr;
}

static VOID afc_table_assign_txpwr_by_index(
	struct _RTMP_ADAPTER *pAd, UINT_8 start_index, UINT_8 afc_bw, INT8 txpwr)
{
	UINT_8 RowIndex, MaxIndex;

	if (afc_bw > AFC_TXPWR_TBL_BW160)
		return;

	MaxIndex = start_index + afc_get_num20MHz_in_bw(afc_bw) - 1;

	for (RowIndex = start_index; RowIndex <= MaxIndex; RowIndex++)
		g_afc_channel_pwr[afc_bw][RowIndex] = txpwr;
}

static VOID afc_table_update_txpwr_with_eirp(struct _RTMP_ADAPTER *pAd)
{
	UINT_8 subband, BwIndex, PsdIndex, RowIndex, PsdMax = 0, RowOffset = 0, BwOffset = 0;
	INT8 txpwr;
	INT8 *eirp;

	for (subband = BAND_6G_UNII_5; subband < BAND_6G_UNII_NUM; subband++) {
		for (BwIndex = AFC_TXPWR_TBL_BW20; BwIndex <= AFC_TXPWR_TBL_BW160; BwIndex++) {
			afc_table_get_offset_params(subband, BwIndex, &RowOffset, &PsdMax, &BwOffset);
			afc_get_eirp_val_per_bw(pAd, subband, BwIndex, &eirp);

			for (PsdIndex = 0; PsdIndex < PsdMax; PsdIndex++) {
				RowIndex = (PsdIndex * BwOffset) + RowOffset;
				txpwr = afc_table_check_min_txpwr(RowIndex, BwIndex, subband);

				if ((txpwr != AFC_TXPWR_VAL_INVALID) && (*(eirp + PsdIndex) != AFC_TXPWR_VAL_INVALID))
					txpwr = min(txpwr, *(eirp + PsdIndex));
				else
					txpwr = AFC_TXPWR_VAL_INVALID;

				afc_table_assign_txpwr_by_index(pAd, RowIndex, BwIndex, txpwr);
			}
		}
	}
}

static VOID afc_table_update_ru_txpwr(struct _RTMP_ADAPTER *pAd)
{
	UINT_8 RowIndex, ColIndex;
	INT8 txpwr_offset = 0;

	for (ColIndex = AFC_TXPWR_TBL_RU26; ColIndex < AFC_TXPWR_TBL_BW_NUM; ColIndex++) {
		txpwr_offset = afc_table_get_ru_txpwr_offset_from_20mhz(ColIndex);
		for (RowIndex = 0; RowIndex < AFC_6G_PWR_LIMIT_TBL_ROW; RowIndex++) {
			if (g_afc_channel_pwr[AFC_TXPWR_TBL_BW20][RowIndex] < NON_AFC_CHANNEL)
				g_afc_channel_pwr[ColIndex][RowIndex] = g_afc_channel_pwr[AFC_TXPWR_TBL_BW20][RowIndex] - txpwr_offset;
			else
				g_afc_channel_pwr[ColIndex][RowIndex] = g_afc_channel_pwr[AFC_TXPWR_TBL_BW20][RowIndex];
		}
	}
}

static VOID afc_table_update_unallowed_afc_channels(struct _RTMP_ADAPTER *pAd)
{
	UINT_8 subband, BwIndex, PsdIndex, RowIndex, PsdMax = 0, RowOffset = 0, BwOffset = 0;
	INT8 txpwr;

	for (subband = BAND_6G_UNII_5; subband < BAND_6G_UNII_NUM; subband++) {
		for (BwIndex = AFC_TXPWR_TBL_BW40; BwIndex <= AFC_TXPWR_TBL_BW160; BwIndex++) {
			afc_table_get_offset_params(subband, BwIndex, &RowOffset, &PsdMax, &BwOffset);

			for (PsdIndex = 0; PsdIndex < PsdMax; PsdIndex++) {
				RowIndex = (PsdIndex * BwOffset) + RowOffset;
				txpwr = afc_table_check_min_txpwr(RowIndex, (BwIndex - 1), subband);
				if (txpwr != AFC_TXPWR_VAL_INVALID)
					txpwr = afc_table_check_min_txpwr(RowIndex + (BwOffset >> 1),
									(BwIndex - 1), subband);

				if (txpwr == AFC_TXPWR_VAL_INVALID)
					afc_table_assign_txpwr_by_index(pAd, RowIndex, BwIndex, txpwr);
			}
		}
	}
}

static VOID afc_table_txpwr_print(struct _RTMP_ADAPTER *pAd)
{
	static char *col_str[AFC_6G_PWR_LIMIT_TBL_COL] = {
		"BW20", "BW40", "BW80", "BW160", "RU26", "RU52", "RU106"};
	INT RowIndex, Index1;

	MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO,
			 "Band:6G\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
			 col_str[0], col_str[1], col_str[2], col_str[3],
			 col_str[4], col_str[5], col_str[6]);

	for (RowIndex = 0; RowIndex < AFC_6G_PWR_LIMIT_TBL_ROW; RowIndex++) {
		Index1 = (RowIndex * AFC_6G_CHANNEL_FREQ_GAP) + 1;

		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO,
				 "Ch%d  \t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\n",
				 Index1, g_afc_channel_pwr[AFC_TXPWR_TBL_BW20][RowIndex],
				 g_afc_channel_pwr[AFC_TXPWR_TBL_BW40][RowIndex],
				 g_afc_channel_pwr[AFC_TXPWR_TBL_BW80][RowIndex],
				 g_afc_channel_pwr[AFC_TXPWR_TBL_BW160][RowIndex],
				 g_afc_channel_pwr[AFC_TXPWR_TBL_RU26][RowIndex],
				 g_afc_channel_pwr[AFC_TXPWR_TBL_RU52][RowIndex],
				 g_afc_channel_pwr[AFC_TXPWR_TBL_RU106][RowIndex]);
	}
}

static VOID afc_table_update_tx_pwr_limits(struct _RTMP_ADAPTER *pAd)
{
	MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			 "%s\n", __func__);

	memset(&g_afc_channel_pwr[0][0], 0, sizeof(g_afc_channel_pwr));

	afc_table_update_non_afc_channels(pAd);

	MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			 "\n\n%s:: Step 1\n", __func__);
	afc_table_txpwr_print(pAd);

	/* Update 20/40/80/160 MHz as per PSD */
	afc_table_update_txpwr_with_psd(pAd);

	MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO,
			 "\n\n%s:: After Updating PSD\n", __func__);
	afc_table_txpwr_print(pAd);

	/* Update 20/40/80/160 MHz as per EIRP */
	afc_table_update_txpwr_with_eirp(pAd);

	MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			 "\n\n%s:: Step 5 as per EIRP\n", __func__);
	afc_table_txpwr_print(pAd);

	/* Update RU 26/52/106 as per 20 MHz TxPwr value */
	afc_table_update_ru_txpwr(pAd);

	MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			 "\n\n%s:: Step 6 After Updating RU\n", __func__);
	afc_table_txpwr_print(pAd);

	/* Update Unavailable Channels */
	afc_table_update_unallowed_afc_channels(pAd);

	MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			 "\n\n%s:: Step 7 After Updating Unavailable Channels\n", __func__);
	afc_table_txpwr_print(pAd);
}

static void afc_update_pwr_limit_table(struct _RTMP_ADAPTER *pAd)
{
	RTMP_OS_FD fd = NULL;
	RTMP_OS_FS_INFO osFSInfo;
	UCHAR buf[2048];
	UINT32 buf_size = 2048;
	UINT32 write_size;
	CHAR *fname = NULL;
	static char *col_str[AFC_6G_PWR_LIMIT_TBL_COL] = {
		"BW20", "BW40", "BW80", "BW160", "RU26", "RU52", "RU106"};
	INT RowIndex, Index1;

	if (pAd->afc_response_data.response_code) {
		MTWF_DBG(NULL, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				"Failure! Response Code is 0x%X\n", pAd->afc_response_data.response_code);
		return;
	}

	fname = AFC_PWR_LIMIT_TBL_PATH;

	if (!fname) {
		MTWF_DBG(NULL, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				" fname is NULL\n");
		return;
	}

	RtmpOSFSInfoChange(&osFSInfo, TRUE);
	fd = RtmpOSFileOpen(fname, O_WRONLY | O_CREAT, 0);
	if (IS_FILE_OPEN_ERR(fd)) {
		RtmpOSFSInfoChange(&osFSInfo, FALSE);
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			"open file fail\n");
		return;
	}

	memset(buf, 0, buf_size);

	if (strlen(buf) < (buf_size - 1))
		snprintf(buf + strlen(buf),
		buf_size - strlen(buf),
		"Band:6G\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
			col_str[0], col_str[1], col_str[2], col_str[3],
			col_str[4], col_str[5], col_str[6]);

	afc_table_update_tx_pwr_limits(pAd);

	for (RowIndex = 0; RowIndex < AFC_6G_PWR_LIMIT_TBL_ROW; RowIndex++) {
		Index1 = (RowIndex * AFC_6G_CHANNEL_FREQ_GAP) + 1;

		if (strlen(buf) < (buf_size - 1))
			snprintf(buf + strlen(buf),
			buf_size - strlen(buf),
			"Ch%d  \t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
				Index1, g_afc_channel_pwr[AFC_TXPWR_TBL_BW20][RowIndex],
				g_afc_channel_pwr[AFC_TXPWR_TBL_BW40][RowIndex],
				g_afc_channel_pwr[AFC_TXPWR_TBL_BW80][RowIndex],
				g_afc_channel_pwr[AFC_TXPWR_TBL_BW160][RowIndex],
				g_afc_channel_pwr[AFC_TXPWR_TBL_RU26][RowIndex],
				g_afc_channel_pwr[AFC_TXPWR_TBL_RU52][RowIndex],
				g_afc_channel_pwr[AFC_TXPWR_TBL_RU106][RowIndex]);
	}

	write_size = strlen(buf);
	RtmpOSFileWrite(fd, buf, write_size);
	memset(buf, 0, buf_size);

	RtmpOSFileClose(fd);
	RtmpOSFSInfoChange(&osFSInfo, FALSE);
}

static VOID afc_cmd_set_eirp_from_index(
	struct _RTMP_ADAPTER *pAd, UINT8 unii, UINT8 bw,
	UINT8 index1, UINT8 index2, INT8 value)
{
	struct AFC_TX_PWR_INFO *pwr_info;
	UINT8 index, max_index = 0;

	MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO,
				"%s::unii:%u bw:%u index1:%u index2:%u value:%d\n",
				__func__, unii, bw, index1, index2, value);

	if (unii >= BAND_6G_UNII_NUM) {
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					 "%s: Max Unii is %u\n", __func__, BAND_6G_UNII_NUM);
		return;
	}

	if (bw >= AFC_BW_NUM) {
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					 "%s: Max BW is %u\n", __func__, AFC_BW_NUM);
		return;
	}

	max_index = max_afc_chan_idx[unii][bw];

	if (index1 >= max_index) {
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					 "%s:Index1 %u > max index :%u\n", __func__, index1, max_index);
		return;
	}

	if (index2 >= max_index) {
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO,
					 "%s:Index2 %u > max index :%u. Lets consider Index2 %u\n",
					 __func__, index2, max_index, max_index);
		index2 = max_index - 1;
	}

	pwr_info = &pAd->afc_response_data.afc_txpwr_info[unii];

	switch (bw) {
	case AFC_BW_20:
		if (index2) {
			for (index = index1; index <= index2; index++)
				pwr_info->max_eirp_bw20[index] = value;
		} else
			pwr_info->max_eirp_bw20[index1] = value;
		break;

	case AFC_BW_40:
		if (index2) {
			for (index = index1; index <= index2; index++)
				pwr_info->max_eirp_bw40[index] = value;
		} else
			pwr_info->max_eirp_bw40[index1] = value;
		break;

	case AFC_BW_80:
		if (index2) {
			for (index = index1; index <= index2; index++)
				pwr_info->max_eirp_bw80[index] = value;
		} else
			pwr_info->max_eirp_bw80[index1] = value;
		break;

	case AFC_BW_160:
		if (index2) {
			for (index = index1; index <= index2; index++)
				pwr_info->max_eirp_bw160[index] = value;
		} else
			pwr_info->max_eirp_bw160[index1] = value;
		break;

	default:
		break;
	}
}

static VOID afc_cmd_set_psd_from_index(
	struct _RTMP_ADAPTER *pAd, UINT8 unii, UINT8 index1, UINT8 index2, INT8 value)
{
	struct AFC_TX_PWR_INFO *pwr_info;
	UINT8 index, max_index = 0;

	MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO,
				"%s::unii:%u index1:%u index2:%u value:%d\n",
		__func__, unii, index1, index2, value);

	if (unii >= BAND_6G_UNII_NUM) {
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					 "%s: Max Unii is %u\n", __func__, BAND_6G_UNII_NUM);
		return;
	}

	max_index = max_afc_chan_idx[unii][AFC_BW_20];

	if (index1 >= max_index) {
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					 "%s:Index1 %u > max index :%u\n", __func__, index1, max_index);
		return;
	}

	if (index2 >= max_index)
		index2 = max_index - 1;

	pwr_info = &pAd->afc_response_data.afc_txpwr_info[unii];

	if (index2) {
		for (index = index1; index <= index2; index++)
			pwr_info->max_psd_bw20[index] = value;
	} else
		pwr_info->max_psd_bw20[index1] = value;
}

INT afc_cmd_set_afc_params(IN struct _RTMP_ADAPTER *pAd, IN RTMP_STRING * arg)
{
	INT32 Ret = TRUE;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev =
		get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);
	PCHAR pch = NULL;
	UINT8 paramref, band, bw, index1, index2, enable;
	INT8 value;

	if (!IS_AFC_ENABLED(pAd)) {
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					 "%s::AFC is not Enabled(%d)\n", __func__,
					 pAd->afc_ctrl.AfcStateMachine.CurrState);
		Ret = FALSE;
		goto error;
	}

	if (wdev != NULL) {
		if (!WMODE_CAP_6G(wdev->PhyMode)) {
			MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
						 "%s::Params is valid only for 6G Interface\n", __func__);
			Ret = FALSE;
			goto error;
		}
	} else {
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					"%s::wdev is NULL\n", __func__);
		Ret = FALSE;
		goto error;
	}

	/* setafc=0 */
	/* setafc=1-UniBand-BW-EirpValue-Index1-Index2 */
	/* setafc=2-UniBand-MaxPsdValue-Index1-Index2 */
	/* setafc=3-Expiry_Value */
	/* setafc=4-Response_Value */
	/* setafc=5 */
	/* setafc=6 */

	pch = strsep(&arg, ":");
	/* Get ParamRef */
	if (pch != NULL)
		paramref = (UINT8)os_str_toul(pch, 0, 10);
	else {
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
		 "%s::paramref is NULL\n", __func__);
		Ret = FALSE;
		goto error;
	}

	MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO,
			 "%s::paramref = %u\n", __func__, paramref);

	if (paramref >= AFC_SET_PARAM_NUM) {
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				 "%s::paramref max val is %u\n", __func__, AFC_SET_PARAM_NUM);
		Ret = FALSE;
		goto error;
	}

	if (paramref != AFC_SET_LOCK && _afc_daemon_lock == TRUE) {
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					"%s::_afc_daemon_lock is locked. Please unlock first\n", __func__);
		Ret = FALSE;
		goto error;
	}

	if (paramref == AFC_RESET_AFC_PARAMS) {
		/* Reset all Value */
		os_zero_mem(&(pAd->afc_response_data), sizeof(pAd->afc_response_data));
	} else if (paramref == AFC_SET_EIRP_PARAM || paramref == AFC_SET_PSD_PARAM) {
		pch = strsep(&arg, ":");
		/*Get Band*/
		if (pch != NULL)
			band = (UINT8)os_str_toul(pch, 0, 10);
		else {
			MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			 "%s::band is NULL\n", __func__);
			Ret = FALSE;
			goto error;
		}

		if (paramref == AFC_SET_EIRP_PARAM) {
			pch = strsep(&arg, ":");
			/*Get BW*/
			if (pch != NULL)
				bw = (UINT8)os_str_toul(pch, 0, 10);
			else {
				MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				 "%s::bw is NULL\n", __func__);
				Ret = FALSE;
				goto error;
			}
		}

		pch = strsep(&arg, ":");
		/*Get Value*/
		if (pch != NULL)
			value = (UINT8)os_str_tol(pch, 0, 10);
		else {
			MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			 "%s::value is NULL\n", __func__);
			Ret = FALSE;
			goto error;
		}

		pch = strsep(&arg, ":");
		/*Get Index1*/
		if (pch != NULL)
			index1 = (UINT8)os_str_toul(pch, 0, 10);
		else {
			MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				 "%s::index1 is NULL\n", __func__);
			Ret = FALSE;
			goto error;
		}

		pch = strsep(&arg, ":");
		/*Get Index2*/
		if (pch != NULL)
			index2 = (UINT8)os_str_toul(pch, 0, 10);
		else
			index2 = 0;

		if (index2 < index1)
			index2 = 0;

		if (paramref == AFC_SET_EIRP_PARAM)
			afc_cmd_set_eirp_from_index(pAd, band, bw, index1, index2, value);
		else
			afc_cmd_set_psd_from_index(pAd, band, index1, index2, value);
	} else if (paramref == AFC_SET_EXPIRY_PARAM) {
		/*Get Exipry Time*/
		pch = strsep(&arg, ":");
		if (pch != NULL)
			pAd->afc_response_data.expiry_time = (UINT32)os_str_toul(pch, 0, 10);
		else {
			Ret = FALSE;
			goto error;
		}
	} else if (paramref == AFC_SET_RESP_PARAM) {
		/*Get Response Code*/
		pch = strsep(&arg, ":");
		if (pch != NULL)
			pAd->afc_response_data.response_code = (UINT16)os_str_toul(pch, 0, 10);
		else {
			Ret = FALSE;
			goto error;
		}
	} else if (paramref == AFC_UPD_CHANNEL_PARAMS) {

		pch = strsep(&arg, ":");
		/*Get Value*/
		if (pch != NULL)
			_afc_switch_chan_params.oper.bw = (UINT8)os_str_tol(pch, 0, 10);
		else {
			MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			 "%s::bw is NULL\n", __func__);
			Ret = FALSE;
			goto error;
		}

		pch = strsep(&arg, ":");
		/*Get Index1*/
		if (pch != NULL)
			_afc_switch_chan_params.oper.cen_ch_1 = (UINT8)os_str_toul(pch, 0, 10);
		else {
			MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				 "%s::cen_ch_1 is NULL\n", __func__);
			Ret = FALSE;
			goto error;
		}

		pch = strsep(&arg, ":");
		/*Get Value*/
		if (pch != NULL)
			_afc_switch_chan_params.oper.cen_ch_2 = (UINT8)os_str_tol(pch, 0, 10);
		else {
			MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			 "%s::cen_ch_2 is NULL\n", __func__);
			Ret = FALSE;
			goto error;
		}

		pch = strsep(&arg, ":");
		/*Get Index1*/
		if (pch != NULL)
			_afc_switch_chan_params.oper.ext_cha = (UINT8)os_str_toul(pch, 0, 10);
		else {
			MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				 "%s::ext_cha is NULL\n", __func__);
			Ret = FALSE;
			goto error;
		}

		pch = strsep(&arg, ":");
		/*Get Index1*/
		if (pch != NULL)
			_afc_switch_chan_params.oper.prim_ch = (UINT8)os_str_toul(pch, 0, 10);
		else {
			MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				 "%s::prim_ch is NULL\n", __func__);
			Ret = FALSE;
			goto error;
		}

		_afc_switch_chan_params.oper.ht_bw = (_afc_switch_chan_params.oper.bw > BW_20) ? HT_BW_40 : HT_BW_20;
		_afc_switch_chan_params.oper.vht_bw = rf_bw_2_vht_bw(_afc_switch_chan_params.oper.bw);

	MTWF_DBG_NP(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_NOTICE,
		"Band:%u Scan:%u Ch_Band:%u HTBW:%u VHTBW:%u BW:%u\n\n",
		_afc_switch_chan_params.band_idx,
		_afc_switch_chan_params.bScan,
		_afc_switch_chan_params.oper.ch_band,
		_afc_switch_chan_params.oper.ht_bw,
		_afc_switch_chan_params.oper.vht_bw,
		_afc_switch_chan_params.oper.bw);

	MTWF_DBG_NP(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_NOTICE,
		"ExtCh:%u PCh:%u CenCh1:%u CenCh2:%u RxStream:%u APBW:%u ApCenCh:%u\n\n",
		_afc_switch_chan_params.oper.ext_cha,
		_afc_switch_chan_params.oper.prim_ch,
		_afc_switch_chan_params.oper.cen_ch_1,
		_afc_switch_chan_params.oper.cen_ch_2,
		_afc_switch_chan_params.oper.rx_stream,
		_afc_switch_chan_params.oper.ap_bw,
		_afc_switch_chan_params.oper.ap_cen_ch);
	} else if (paramref == AFC_CFG_PWR_LMT_TBL) {

		/* Config AFC Power Limit */
		afc_update_pwr_limit_table(pAd);
	} else if (paramref == AFC_UPD_FW_TX_PWR) {

		/* Update TX Power in FW */
		afc_update_channel_params(pAd, (UINT)pObj->ioctl_if);
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
						 "%s:: htbw:%u VhtBw:%u\n", __func__,
						 wlan_operate_get_ht_bw(wdev), wlan_operate_get_vht_bw(wdev));

		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_NOTICE,
			 "%s::Update FW\n", __func__);
	} else if (paramref == AFC_SET_TX_ENABLE) {
		pch = strsep(&arg, ":");
		if (pch != NULL)
			enable = os_str_toul(pch, 0, 10) ? TRUE : FALSE;
		else {
			MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				 "%s:: enable is NULL\n", __func__);
			Ret = FALSE;
			goto error;
		}
		if (!enable)
			afc_set_wdev_to_hold(pAd, wdev);
		else
			afc_enable_tx(pAd, wdev->wdev_idx);

	} else if (paramref == AFC_SET_AFC_DISABLE) {
		if (IS_AFC_RUN_STATE(pAd)) {
			os_zero_mem(&(pAd->afc_response_data), sizeof(pAd->afc_response_data));
			pAd->afc_response_data.response_code = AFC_NO_RESPONSE_FROM_SERVER;
			afc_set_wdev_to_hold(pAd, wdev);
		}
	} else if (paramref == AFC_SET_LOCK) {
		pch = strsep(&arg, ":");
		if (pch != NULL)
			_afc_daemon_lock = os_str_toul(pch, 0, 10) ? TRUE : FALSE;
		else {
			MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				 "%s::_afc_daemon_lock is NULL\n", __func__);
			Ret = FALSE;
			goto error;
		}
	}

error:
	MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_NOTICE,
			 "%s:Ret = %d\n", __func__, Ret);
	return Ret;
}

static INT afc_show_afc_params(IN struct _RTMP_ADAPTER *pAd)
{
	struct AFC_TX_PWR_INFO *pwr_info;
	UINT8 unii_index, index, max_index = 0;

	for (unii_index = BAND_6G_UNII_5; unii_index < BAND_6G_UNII_NUM; unii_index++) {
		pwr_info = &pAd->afc_response_data.afc_txpwr_info[unii_index];

		MTWF_DBG_NP(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_NOTICE,
			 "UNII_INDEX %u\n", unii_index);

		max_index = max_afc_chan_idx[unii_index][AFC_BW_20];

		MTWF_DBG_NP(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_NOTICE,
			 "\nPSD      :");
		for (index = 0; index < max_index; index++)
			MTWF_DBG_NP(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_NOTICE,
				" %d", pwr_info->max_psd_bw20[index]);

		MTWF_DBG_NP(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_NOTICE,
			 "\nEIRP 20  :");
		for (index = 0; index < max_index; index++)
			MTWF_DBG_NP(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_NOTICE,
				" %d", pwr_info->max_eirp_bw20[index]);


		MTWF_DBG_NP(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_NOTICE,
			 "\nEIRP 40  :");
		max_index = max_afc_chan_idx[unii_index][AFC_BW_40];
		for (index = 0; index < max_index; index++)
			MTWF_DBG_NP(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_NOTICE,
				" %d", pwr_info->max_eirp_bw40[index]);

		MTWF_DBG_NP(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_NOTICE,
			 "\nEIRP 80  :");
		max_index = max_afc_chan_idx[unii_index][AFC_BW_80];
		for (index = 0; index < max_index; index++)
			MTWF_DBG_NP(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_NOTICE,
				" %d", pwr_info->max_eirp_bw80[index]);

		MTWF_DBG_NP(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_NOTICE,
			 "\nEIRP 160 :");
		max_index = max_afc_chan_idx[unii_index][AFC_BW_160];
		for (index = 0; index < max_index; index++)
			MTWF_DBG_NP(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_NOTICE,
				" %d", pwr_info->max_eirp_bw160[index]);

		MTWF_DBG_NP(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_NOTICE,
						"\n\n");
	}

	MTWF_DBG_NP(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_NOTICE,
		"Expiry Time   : %d\n", pAd->afc_response_data.expiry_time);

	MTWF_DBG_NP(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_NOTICE,
		"Response Code : %d\n\n", pAd->afc_response_data.response_code);

	MTWF_DBG_NP(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_NOTICE,
		"Band:%u Scan:%u Ch_Band:%u HTBW:%u VHTBW:%u BW:%u\n\n",
		_afc_switch_chan_params.band_idx,
		_afc_switch_chan_params.bScan,
		_afc_switch_chan_params.oper.ch_band,
		_afc_switch_chan_params.oper.ht_bw,
		_afc_switch_chan_params.oper.vht_bw,
		_afc_switch_chan_params.oper.bw);

	MTWF_DBG_NP(DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_NOTICE,
		"ExtCh:%u PCh:%u CenCh1:%u CenCh2:%u RxStream:%u APBW:%u ApCenCh:%u\n\n",
		_afc_switch_chan_params.oper.ext_cha,
		_afc_switch_chan_params.oper.prim_ch,
		_afc_switch_chan_params.oper.cen_ch_1,
		_afc_switch_chan_params.oper.cen_ch_2,
		_afc_switch_chan_params.oper.rx_stream,
		_afc_switch_chan_params.oper.ap_bw,
		_afc_switch_chan_params.oper.ap_cen_ch);

	return TRUE;
}

INT afc_cmd_show_afc_params(IN struct _RTMP_ADAPTER *pAd, IN RTMP_STRING * arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	struct wifi_dev *wdev =
		get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);

	if (!IS_AFC_ENABLED(pAd)) {
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					 "%s::AFC is not Enabled(%d)\n", __func__,
					 pAd->afc_ctrl.AfcStateMachine.CurrState);
		return FALSE;
	}

	if (wdev != NULL) {
		if (!WMODE_CAP_6G(wdev->PhyMode)) {
			MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
						 "%s::Params is valid only for 6G Interface\n", __func__);
			return FALSE;
		}
	} else {
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					"%s::wdev is NULL\n", __func__);
		return FALSE;
	}

	afc_show_afc_params(pAd);

	return TRUE;
}

INT afc_cmd_show_skip_channel(IN struct _RTMP_ADAPTER *pAd, IN RTMP_STRING * arg)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	INT rv, value;
	struct wifi_dev *wdev =
		get_wdev_by_ioctl_idx_and_iftype(pAd, pObj->ioctl_if, pObj->ioctl_if_type);

	if (!IS_AFC_ENABLED(pAd)) {
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					 "%s::AFC is not Enabled(%d)\n", __func__,
					 pAd->afc_ctrl.AfcStateMachine.CurrState);
		return FALSE;
	}

	if (wdev != NULL) {
		if (!WMODE_CAP_6G(wdev->PhyMode)) {
			MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
						 "%s::Params is valid only for 6G Interface\n", __func__);
			return FALSE;
		}
	} else {
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					"%s::wdev is NULL\n", __func__);
		return FALSE;
	}

	rv = sscanf(arg, "%d", &value);

	if (rv == 1) {
		if (value == FALSE) {
			MTWF_PRINT("DUMP original backed skip channel list\n");
			afc_dump_original_skip_channel_list(pAd);
		} else {
			MTWF_PRINT("DUMP all skip channel list\n");
			afc_dump_skip_channel_list(pAd);
		}
	}

	return TRUE;
}

VOID afc_save_switch_channel_params(
	struct _RTMP_ADAPTER *pAd, UCHAR band_idx, struct freq_oper *oper, BOOLEAN bScan)
{
	if (oper->ch_band != CMD_CH_BAND_6G)
		return;

	if (!IS_AFC_ENABLED(pAd)) {
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					 "%s::AFC is not Enabled(%d)\n", __func__,
					 pAd->afc_ctrl.AfcStateMachine.CurrState);
		return;
	}

	MTWF_PRINT("%s(): Save Params\n", __func__);
	MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			 "\n\n%s:: bw:%u prim_ch:%u cen_ch_1:%u cen_ch_2:%u\n",
			 __func__, oper->bw, oper->prim_ch, oper->cen_ch_1, oper->cen_ch_2);

	_afc_switch_chan_params.band_idx = band_idx;
	_afc_switch_chan_params.bScan = bScan;
	_afc_switch_chan_params.oper.ch_band = oper->ch_band;
	_afc_switch_chan_params.oper.ht_bw = oper->ht_bw;
	_afc_switch_chan_params.oper.vht_bw = oper->vht_bw;
	_afc_switch_chan_params.oper.bw = oper->bw;
	_afc_switch_chan_params.oper.ext_cha = oper->ext_cha;
	_afc_switch_chan_params.oper.prim_ch = oper->prim_ch;
	_afc_switch_chan_params.oper.cen_ch_1 = oper->cen_ch_1;
	_afc_switch_chan_params.oper.cen_ch_2 = oper->cen_ch_2;
	_afc_switch_chan_params.oper.rx_stream = oper->rx_stream;
	_afc_switch_chan_params.oper.ap_bw = oper->ap_bw;
	_afc_switch_chan_params.oper.ap_cen_ch = oper->ap_cen_ch;
}

INT afc_set_beacon_bw(struct _RTMP_ADAPTER *pAd, char *arg)
{
	/* iwpriv rax0 set AFCBeaonBW=[BW] */
	INT_8 Value = os_str_tol(arg, 0, 10);

	if (Value > 3) {
		MTWF_DBG(pAd, DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				 "BW number: 0:BW20, 1:BW40, 2:BW80, 3:BW160, others are unavailable.\n");
		return TRUE;
	}

	pAd->CommonCfg.AFCbeaconBW = Value;
	return TRUE;
}

INT afc_test_mode(struct _RTMP_ADAPTER *pAd, char *arg)
{
	/* iwpriv rax0 set AFCtest=[BW20_power_in_dB] */
	INT_8 Value = os_str_tol(arg, 0, 10);
	UINT i = 0;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	UINT ifIndex = pObj->ioctl_if;

	if (Value*2 > AFC_MAX_EIRP) {
		MTWF_DBG(pAd, DBG_CAT_PROTO, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				 "Maximum power is 36dB, set power value = 36\n");
		Value = 36;
	}
	Value = Value*2 - AFC_TXPWR_20MHZ_DELTA_PSD;
	for (i = 0; i < AFC_20MHZ_CHAN_UNII_5; i++)
		pAd->afc_response_data.afc_txpwr_info[BAND_6G_UNII_5].max_psd_bw20[i] = Value;
	for (i = 0; i < AFC_20MHZ_CHAN_UNII_7; i++)
		pAd->afc_response_data.afc_txpwr_info[BAND_6G_UNII_7].max_psd_bw20[i] = Value;

	pAd->CommonCfg.AfcSpectrumType = AFC_SPEC_FREQ_ONLY;
	pAd->afc_response_data.response_code = AFC_RESPONSE_TYPE_SUCCESS;
	pAd->afc_ctrl.AfcStateMachine.CurrState = AFC_RUN;
	afc_fill_void_params(pAd);
	afc_update_pwr_limit_table(pAd);
	afc_update_auto_channel_skip_list(pAd);
	SetSKUCtrl(pAd, "1");
	SetBfBackoffCtrl(pAd, "1");

	if (afc_switch_channel(pAd, ifIndex))
		afc_enable_tx(pAd, ifIndex);
	return TRUE;
}

INT set_afc_event(struct _RTMP_ADAPTER	*pAd, char *arg)
{
	UINT_8 Value = os_str_tol(arg, 0, 10);

	if (!IS_AFC_ENABLED(pAd)) {
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					 "%s::AFC is not Enabled(%d)\n", __func__,
					 pAd->afc_ctrl.AfcStateMachine.CurrState);
		return FALSE;
	}

	MTWF_PRINT("%s():%d\n", __func__, Value);
	/*iwpriv ra0 set afcevents=0 to trigger AFC_INQ_EVENT event from driver to daemon*/
	if (Value == 0)
		RtmpOSWrielessEventSend(pAd->net_dev,
				RT_WLAN_EVENT_CUSTOM,
				AFC_INQ_EVENT,
				NULL,
				(char *)NULL, 0);
	/*iwpriv ra0 set afcevents=1 to trigger AFC_INQ_EVENT event from driver to daemon*/
	if (Value == 1)
		RtmpOSWrielessEventSend(pAd->net_dev,
				RT_WLAN_EVENT_CUSTOM,
				AFC_STOP_EVENT,
				NULL,
				(char *)NULL, 0);

	return TRUE;
}

INT set_afc_device_type(struct _RTMP_ADAPTER *pAd, char *arg)
{
	RTMP_STRING *ptr;
	INT DeviceType;

	ptr = rstrtok(arg, ";");
	kstrtol(ptr, 10, (long *)&DeviceType);

	if (DeviceType >= AFC_MAX_DEVICE_TYPE)
		MTWF_PRINT("Invalid afc device parameter, enter 1 for standard device else 0!\n");
	else
		pAd->CommonCfg.AfcDeviceType = (UINT8)DeviceType;

	return TRUE;
}
INT set_afc_spectrum_type(struct _RTMP_ADAPTER *pAd, char *arg)
{
	UINT_8 SpectrumType = os_str_tol(arg, 0, 10);

	pAd->CommonCfg.AfcSpectrumType = SpectrumType;

	return TRUE;
}

INT set_afc_freq_range(struct _RTMP_ADAPTER	*pAd, char *arg)
{
	RTMP_STRING *macptr = NULL;
	INT i = 0, j = 0;
	INT NumOfFreqRange = 0;
	UINT Frequency = 0;

	for (i = 0, macptr = rstrtok(arg, ":"); macptr; macptr = rstrtok(NULL, ":"), i++) {
		if (i == 0) {
			kstrtol(macptr, 10, (long *)&NumOfFreqRange);
			afc_update_freq_num(pAd, NumOfFreqRange);
			continue;
		}

		kstrtol(macptr, 10, (long *)&Frequency);
		afc_update_freq_range(i, j, (UINT16)Frequency);

		if (i % 2 == 0)
			j++;
	}

	return TRUE;
}

INT set_afc_op_class131(struct _RTMP_ADAPTER	*pAd, char *arg)
{
	RTMP_STRING *macptr = NULL;
	INT i = 0, count = 0;
	UINT Enable = 0;
	UINT Channel = 0;

	for (i = 0, macptr = rstrtok(arg, ";"); macptr; macptr = rstrtok(NULL, ";"), i++) {
		if (i == 0) {
			kstrtol(macptr, 10, (long *)&Enable);
			afc_update_op_class_en(AFC_OP_CLASS_131, (UINT8)Enable);
			continue;
		}

		kstrtol(macptr, 10, (long *)&Channel);
		afc_update_op_class_channel(AFC_OP_CLASS_131, (UINT8)count, (UINT8)Channel);
		count++;
	}

	afc_update_op_class_channel_count(AFC_OP_CLASS_131, (UINT8)count);

	return TRUE;
}

INT set_afc_op_class132(struct _RTMP_ADAPTER	*pAd, char *arg)
{
	RTMP_STRING *macptr = NULL;
	INT i = 0, count = 0;
	UINT Enable = 0;
	UINT Channel = 0;

	for (i = 0, macptr = rstrtok(arg, ";"); macptr; macptr = rstrtok(NULL, ";"), i++) {
		if (i == 0) {
			kstrtol(macptr, 10, (long *)&Enable);
			afc_update_op_class_en(AFC_OP_CLASS_132, (UINT8)Enable);
			continue;
		}

		kstrtol(macptr, 10, (long *)&Channel);
		afc_update_op_class_channel(AFC_OP_CLASS_132, (UINT8)count, (UINT8)Channel);
		count++;
	}

	afc_update_op_class_channel_count(AFC_OP_CLASS_132, (UINT8)count);

	return TRUE;
}

INT set_afc_op_class133(struct _RTMP_ADAPTER	*pAd, char *arg)
{
	RTMP_STRING *macptr = NULL;
	INT i = 0, count = 0;
	UINT Enable = 0;
	UINT Channel = 0;

	for (i = 0, macptr = rstrtok(arg, ";"); macptr; macptr = rstrtok(NULL, ";"), i++) {
		if (i == 0) {
			kstrtol(macptr, 10, (long *)&Enable);
			afc_update_op_class_en(AFC_OP_CLASS_133, (UINT8)Enable);
			continue;
		}

		kstrtol(macptr, 10, (long *)&Channel);
		afc_update_op_class_channel(AFC_OP_CLASS_133, (UINT8)count, (UINT8)Channel);
		count++;
	}

	afc_update_op_class_channel_count(AFC_OP_CLASS_133, (UINT8)count);

	return TRUE;
}

INT set_afc_op_class134(struct _RTMP_ADAPTER	*pAd, char *arg)
{
	RTMP_STRING *macptr = NULL;
	INT i = 0, count = 0;
	UINT Enable = 0;
	UINT Channel = 0;

	for (i = 0, macptr = rstrtok(arg, ";"); macptr; macptr = rstrtok(NULL, ";"), i++) {
		if (i == 0) {
			kstrtol(macptr, 10, (long *)&Enable);
			afc_update_op_class_en(AFC_OP_CLASS_134, (UINT8)Enable);
			continue;
		}

		kstrtol(macptr, 10, (long *)&Channel);
		afc_update_op_class_channel(AFC_OP_CLASS_134, (UINT8)count, (UINT8)Channel);
		count++;
	}

	afc_update_op_class_channel_count(AFC_OP_CLASS_134, (UINT8)count);

	return TRUE;
}

INT set_afc_op_class136(struct _RTMP_ADAPTER	*pAd, char *arg)
{
	RTMP_STRING *macptr = NULL;
	INT i = 0, count = 0;
	UINT Enable = 0;
	UINT Channel = 0;

	for (i = 0, macptr = rstrtok(arg, ";"); macptr; macptr = rstrtok(NULL, ";"), i++) {
		if (i == 0) {
			kstrtol(macptr, 10, (long *)&Enable);
			afc_update_op_class_en(AFC_OP_CLASS_136, (UINT8)Enable);
			continue;
		}

		kstrtol(macptr, 10, (long *)&Channel);
		afc_update_op_class_channel(AFC_OP_CLASS_136, (UINT8)count, (UINT8)Channel);
		count++;
	}

	afc_update_op_class_channel_count(AFC_OP_CLASS_136, (UINT8)count);

	return TRUE;
}

INT8 afc_get_psd(CHAR *strPsd)
{
	INT8 val_psd = 0;
	UINT32 fractional_part = 0;
	CHAR *token = NULL;

	token = strsep((char **)&strPsd, ".");
	val_psd = os_str_tol(token, 0, 10);
	if (val_psd > AFC_MAX_TXPWR_LIMIT)
		val_psd = AFC_MAX_TXPWR_LIMIT;
	val_psd = 2 * val_psd;

	if (strPsd != NULL) {
		fractional_part = os_str_tol(strPsd, 0, 10);
		if (fractional_part > 0) {
			if (val_psd > 0) {
				if (strPsd[0] >= '5')
					val_psd++;
			} else {
				if (strPsd[0] > '5')
					val_psd -= 2;
				else
					val_psd--;
			}
		}
	}

	return val_psd;

}

INT afc_get_6g_wdev(struct _RTMP_ADAPTER *pAd)
{
	UCHAR i = 0;
	struct wifi_dev *wdev = NULL;

	/*Get the Wdev*/
	for (i = 0; i < WDEV_NUM_MAX; i++) {
		wdev = pAd->wdev_list[i];

		if (wdev && WMODE_CAP_6G(wdev->PhyMode) &&
			(wdev->wdev_type == WDEV_TYPE_AP)) {
			break;
		}
	}

	if (wdev && (i < WDEV_NUM_MAX))
		return wdev->wdev_idx;
	else
		return AFC_6G_WDEV_NOT_AVAILABLE;
}

void afc_update_psd(struct _RTMP_ADAPTER *pAd, UINT16 lowFrequency, UINT16 highFrequency, INT8 i1Psd)
{
	UINT8 channel_idx = 0;
	UINT16 start_freq = 0, last_freq = 0;
	struct AFC_TX_PWR_INFO *pAfcTxPwrInfo = &pAd->afc_response_data.afc_txpwr_info[BAND_6G_UNII_5];

	MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO,
						"MaxPsd Setting for UNII-5 band: \n");
	for (channel_idx = 0; channel_idx < MAX_20MHZ_CHANNEL_IN_6G_UNII; channel_idx++) {
		start_freq = UNII_5_STARTING_FREQ + (channel_idx * BW20_MHZ);
		last_freq = start_freq + BW20_MHZ;

		if ((last_freq > lowFrequency) && (start_freq < highFrequency)) {

			if ((!pAfcTxPwrInfo[BAND_6G_UNII_5].max_psd_bw20[channel_idx]) ||
				(pAfcTxPwrInfo[BAND_6G_UNII_5].max_psd_bw20[channel_idx] > i1Psd))

				pAfcTxPwrInfo[BAND_6G_UNII_5].max_psd_bw20[channel_idx] = i1Psd;
		}
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO,
				"[%d,%d], ", channel_idx, pAfcTxPwrInfo[BAND_6G_UNII_5].max_psd_bw20[channel_idx]);
	}

	MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO,
						"MaxPsd Setting for UNII-7 band: \n");

	for (channel_idx = 0; channel_idx < MAX_20MHZ_CHANNEL_IN_6G_UNII; channel_idx++) {
		start_freq = UNII_7_STARTING_FREQ + (channel_idx * BW20_MHZ);
		last_freq = start_freq + BW20_MHZ;

		if ((last_freq > lowFrequency) && (start_freq < highFrequency)) {

			if ((!pAfcTxPwrInfo[BAND_6G_UNII_7].max_psd_bw20[channel_idx]) ||
				(pAfcTxPwrInfo[BAND_6G_UNII_7].max_psd_bw20[channel_idx] > i1Psd))
				pAfcTxPwrInfo[BAND_6G_UNII_7].max_psd_bw20[channel_idx] = i1Psd;
		}
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO,
				"[%d,%d], ", channel_idx, pAfcTxPwrInfo[BAND_6G_UNII_7].max_psd_bw20[channel_idx]);
	}
}

void afc_update_eirp(struct _RTMP_ADAPTER *pAd, UINT8 opclass, UINT8 channelNum, INT8 i1Eirp)
{
	struct wifi_dev *wdev = NULL;
	POS_COOKIE obj = (POS_COOKIE)pAd->OS_Cookie;

	struct AFC_TX_PWR_INFO *pAfcTxPwrInfo = &pAd->afc_response_data.afc_txpwr_info[BAND_6G_UNII_5];
	UINT8 channel_idx = 0, channel_set_num = 0, unused_ch = 0;
	PCHAR channel_set, pmax_eirp = NULL;

	wdev = get_wdev_by_ioctl_idx_and_iftype(pAd, obj->ioctl_if, obj->ioctl_if_type);
	channel_set = get_channelset_by_reg_class(pAd, opclass, wdev->PhyMode);
	if (!channel_set) {
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO,
			"Null of channel_set\n");
		return;
	}
	channel_set_num = get_channel_set_num(channel_set);

	for (channel_idx = 0; channel_idx < channel_set_num; channel_idx++) {

		switch (opclass) {
		case OP_CLASS_131:
			if (channel_idx == 0)
				pmax_eirp = &pAfcTxPwrInfo[BAND_6G_UNII_5].max_eirp_bw20[0];
			else if (channel_idx == MAX_20MHZ_CHANNEL_IN_6G_UNII) {
				channel_idx = UNII_7_BW20_START_INX;
				unused_ch = channel_idx;
				pmax_eirp = &pAfcTxPwrInfo[BAND_6G_UNII_7].max_eirp_bw20[0];
			}
			break;
		case OP_CLASS_132:
			if (channel_idx == 0)
				pmax_eirp = &pAfcTxPwrInfo[BAND_6G_UNII_5].max_eirp_bw40[0];
			else if (channel_idx == MAX_40MHZ_CHANNEL_IN_6G_UNII) {
				channel_idx = UNII_7_BW40_START_INX;
				unused_ch = channel_idx;
				pmax_eirp = &pAfcTxPwrInfo[BAND_6G_UNII_7].max_eirp_bw40[0];
			}
			break;
		case OP_CLASS_133:
		case OP_CLASS_135:
			if (channel_idx == 0)
				pmax_eirp = &pAfcTxPwrInfo[BAND_6G_UNII_5].max_eirp_bw80[0];
			else if (channel_idx == MAX_80MHZ_CHANNEL_IN_6G_UNII) {
				channel_idx = UNII_7_BW80_START_INX;
				unused_ch = channel_idx;
				pmax_eirp = &pAfcTxPwrInfo[BAND_6G_UNII_7].max_eirp_bw80[0];
			}
			break;
		case OP_CLASS_134:
			if (channel_idx == 0)
				pmax_eirp = &pAfcTxPwrInfo[BAND_6G_UNII_5].max_eirp_bw160[0];
			else if (channel_idx == MAX_160MHZ_CHANNEL_IN_6G_UNII) {
				channel_idx = UNII_7_BW160_START_INX;
				unused_ch = channel_idx;
				pmax_eirp = &pAfcTxPwrInfo[BAND_6G_UNII_7].max_eirp_bw160[0];
			}
			break;
		case OP_CLASS_136:
		case OP_CLASS_137:
		default:
			MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO,
						"global OpClass %d is not Supported\n", opclass);
			break;

		}
		if (((UINT8)channel_set[channel_idx]) == channelNum && pmax_eirp != NULL) {
			pmax_eirp[channel_idx - unused_ch] = i1Eirp;
			break;
		}
	}
}

void afc_update_params_from_response(struct _RTMP_ADAPTER	*pAd,
	UINT8 *buf_data, UINT32 buf_len)
{
	UINT16 idx = 0;
	UINT16 lowFrequency = 0, highFrequency = 0;
	INT8  i1Psd = 0, i1Eirp = 0;
	UINT8 opclass = 0, channelNum = 0;
	CHAR *strPsd = NULL, *strEirp = NULL;
	UINT16 sub_len;


	if ((buf_data[idx] ==  ASI_RESPONSE) && (buf_len > TLV_HEADER)) {
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			"Length of ASI response = %d\n", *(UINT16 *)(buf_data + 1));
		idx += TLV_HEADER;
	} else {
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR, "Invalid data Received\n");
		return;
	}

	memset(pAd->afc_response_data.afc_txpwr_info, AFC_TXPWR_VAL_INVALID,
						sizeof(struct AFC_TX_PWR_INFO) * BAND_6G_UNII_NUM);
	while (1) {
		if (idx < 0 || idx+TLV_HEADER >= buf_len)
			return;

		switch (buf_data[idx]) {

		case FREQ_INFO:
			idx = idx + TLV_HEADER;
			sub_len = *(UINT16 *)&buf_data[idx - 2];
			if ((idx + sub_len) >= buf_len
				|| (sub_len <= 4)) {
				MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO,
					"Invalid data: idx = %d, sub_len = %d, buf_len = %d\n",
					idx, sub_len, buf_len);
				return;
			}

			lowFrequency = *(UINT16 *)(buf_data + idx);
			highFrequency = *(UINT16 *)(buf_data + idx + sizeof(UINT16));
			strPsd = &buf_data[idx + (2 * sizeof(UINT16))];

			if (strPsd) {
				if (strlen(strPsd)+(2 * sizeof(UINT16)) > sub_len) {
					MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_DEBUG,
						"Ivalid data: strlen(strPsd) = %lu\n",
						strlen(strPsd));
					return;
				}
				i1Psd = afc_get_psd(strPsd);
			}


			MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					"frequency Info: low: %d, high: %d, Max Psd = %d\n",
					lowFrequency, highFrequency, i1Psd);

			afc_update_psd(pAd, lowFrequency, highFrequency, i1Psd);
			idx = idx + *(UINT16 *)&buf_data[idx - 2];
			break;
		case CHANNELS_ALLOWED:
			idx = idx + TLV_HEADER;
			break;
		case OPER_CLASS:
			idx = idx + TLV_HEADER;
			if (idx >= buf_len) {
				MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO,
					"Calculated Idx would exceed the remain buffer\n");
				return;
			}

			opclass = buf_data[idx];
			idx = idx + sizeof(UINT8);

			break;
		case CHANNEL_LIST:
			idx = idx + TLV_HEADER;
			sub_len = *(UINT16 *)&buf_data[idx - 2];
			if ((idx + sub_len) >= buf_len
				|| !sub_len) {
				MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO,
					"Invalid data: idx = %d, sub_len = %d, buf_len = %d\n",
					idx, sub_len, buf_len);
				return;
			}

			channelNum = buf_data[idx];
			strEirp = &buf_data[idx + sizeof(UINT8)];

			if (strEirp) {
				if (strlen(strEirp) + sizeof(UINT8) > sub_len) {
					MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_DEBUG,
						"Ivalid data: strlen(strEirp) = %lu\n",
						strlen(strEirp));
					return;
				}
				i1Eirp = afc_get_psd(strEirp);
			}

			afc_update_eirp(pAd, opclass, channelNum, i1Eirp);
			MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					"\nOpClass : %d, Channel Num : %d Max Eirp : %d\n",
					opclass, channelNum, i1Eirp);

			idx = idx + *(UINT16 *)&buf_data[idx - 2];
			break;
		case EXPIRY_TIME:
			idx = idx + TLV_HEADER;
			if (idx + sizeof(UINT32) >= buf_len) {
				MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO,
					"Calculated Idx would exceed the remain buffer\n");
				return;
			}

			pAd->afc_response_data.expiry_time = *(UINT32 *)&buf_data[idx];
			MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					"Expiry Time : %d\n", pAd->afc_response_data.expiry_time);

			idx = idx + *(UINT16 *)&buf_data[idx - 2];
			break;
		case RESPONSE:
			idx = idx + TLV_HEADER;
			if (idx + sizeof(UINT16) >= buf_len) {
				MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO,
					"Calculated Idx would exceed the remain buffer\n");
				return;
			}

			pAd->afc_response_data.response_code = *(UINT16 *)(buf_data + idx);
			MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					"AFC System Response code : %d\n", pAd->afc_response_data.response_code);

			idx = idx + *(UINT16 *)&buf_data[idx - 2];
			break;
		default:
			MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					"AFC response parsing completed\n");
			return;
		}
	}
}

NDIS_STATUS afc_daemon_response(struct _RTMP_ADAPTER *pAd, struct __RTMP_IOCTL_INPUT_STRUCT *wrq, POS_COOKIE pObj)
{
	struct afc_response *afc_resp = NULL;
	NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
	UINT ifIndex = pObj->ioctl_if;
	struct wifi_dev *wdev = NULL;

	if (!IS_AFC_ENABLED(pAd)) {
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					 "%s::AFC is not Enabled(%d)\n", __func__,
					 pAd->afc_ctrl.AfcStateMachine.CurrState);
		Status = NDIS_STATUS_RESOURCES;
		return Status;
	}

	ifIndex = afc_get_6g_wdev(pAd);

	if (ifIndex != AFC_6G_WDEV_NOT_AVAILABLE) {
		wdev = pAd->wdev_list[ifIndex];
	} else {
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				"%s:: 6Ghz interface not available\n", __func__);
		Status = NDIS_STATUS_RESOURCES;
		return Status;
	}

	MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				"%s::ifIndex = %d\n", __func__, ifIndex);

	if (_afc_daemon_lock == FALSE) {
		Status = NDIS_STATUS_RESOURCES;
		return Status;
	}

	os_alloc_mem(pAd, (UCHAR **)&afc_resp, wrq->u.data.length);
	if (afc_resp == NULL) {
		Status = NDIS_STATUS_RESOURCES;
		return Status;
	}

	Status = copy_from_user(afc_resp, wrq->u.data.pointer, wrq->u.data.length);

	if (Status == NDIS_STATUS_SUCCESS) {
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			"Response status %d: buffer length = %d\n", afc_resp->status, wrq->u.data.length);

		os_zero_mem(&(pAd->afc_response_data), sizeof(pAd->afc_response_data));

		if (!afc_resp->status) {
			afc_update_params_from_response(pAd, afc_resp->data, wrq->u.data.length - AFC_STATUS_LEN);

			if (pAd->afc_response_data.response_code) {
				MTWF_DBG(pAd, DBG_CAT_SEC, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					"AFC response code not success: %d !!!\n", pAd->afc_response_data.response_code);
				pAd->afc_response_data.response_code = AFC_NO_RESPONSE_FROM_SERVER;
				afc_response_fail_update_to_hold(pAd, ifIndex);
			}

			if (afc_check_params_valid_channels(pAd) == TRUE) {
				afc_fill_void_params(pAd);
				if (pAd->afc_response_data.response_code == AFC_RESPONSE_TYPE_SUCCESS)
					pAd->afc_ctrl.AfcStateMachine.CurrState = AFC_RUN;

				afc_show_afc_params(pAd);
				afc_update_pwr_limit_table(pAd);
				afc_update_auto_channel_skip_list(pAd);

				if (pAd->CommonCfg.AcsAfterAfc) {
					if (afc_use_acs_switch_channel(pAd, ifIndex))
						afc_enable_tx(pAd, ifIndex);
				} else if (afc_switch_channel(pAd, ifIndex))
					afc_enable_tx(pAd, ifIndex);
				else
					afc_response_fail_update_to_hold(pAd, ifIndex);

				if (pAd->afc_ctrl.AfcStateMachine.CurrState == AFC_RUN) {
					SetSKUCtrl(pAd, "1");
					SetBfBackoffCtrl(pAd, "1");
				}
			} else {
				MTWF_DBG(pAd, DBG_CAT_SEC, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					"N0 channels available for 6G !!!\n");
				afc_response_fail_update_to_hold(pAd, ifIndex);
			}
		} else {
			MTWF_DBG(pAd, DBG_CAT_SEC, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				"AFC response Timeout/Invalid recieved !!!\n");
			pAd->afc_response_data.response_code = AFC_NO_RESPONSE_FROM_SERVER;
			afc_response_fail_update_to_hold(pAd, ifIndex);
		}
	} else
		Status = NDIS_STATUS_FAILURE;

	os_free_mem(afc_resp);

	return Status;
}

VOID afc_fill_channel_info(IN PRTMP_ADAPTER pAd, UINT OpClass, UINT PhyMode, UINT8 *ChannelList, UINT8 *NumOfChannels)
{
	PUCHAR channel_set = NULL;
	UINT8 elem = 0;
	UINT8 j = 0, k = 0, ChannelNum = 0, NumofChannelsToSkip = 0;

	switch (OpClass) {
	case AFC_OP_CLASS_131:
		ChannelNum = 0;
		NumofChannelsToSkip = g_afc_opclass_skip_channel.OPClassNumOfChannel[OpClass];
		channel_set = get_channelset_by_reg_class(pAd, OP_CLASS_131, PhyMode);

		if (channel_set != NULL) {
			for (j = 0; j < AFC_OP_CLASS_131_CHL_NUM; j++) {
				elem = channel_set[j];

				for (k = 0; k < NumofChannelsToSkip; k++)
					if (elem == g_afc_opclass_skip_channel.OPClass131[k])
						break;

				if (k == NumofChannelsToSkip) {
					ChannelList[ChannelNum] = elem;
					ChannelNum++;
				}
			}

			*NumOfChannels = ChannelNum;
		}
		break;

	case AFC_OP_CLASS_132:
		ChannelNum = 0;
		NumofChannelsToSkip = g_afc_opclass_skip_channel.OPClassNumOfChannel[OpClass];
		channel_set = get_channelset_by_reg_class(pAd, OP_CLASS_132, PhyMode);

		if (channel_set != NULL) {
			for (j = 0; j < AFC_OP_CLASS_132_CHL_NUM; j++) {
				elem = channel_set[j];

				for (k = 0; k < NumofChannelsToSkip; k++)
					if (elem == g_afc_opclass_skip_channel.OPClass132[k])
						break;

				if (k == NumofChannelsToSkip) {
					ChannelList[ChannelNum] = elem;
					ChannelNum++;
				}
			}

			*NumOfChannels = ChannelNum;
		}
		break;

	case AFC_OP_CLASS_133:
		ChannelNum = 0;
		NumofChannelsToSkip = g_afc_opclass_skip_channel.OPClassNumOfChannel[OpClass];
		channel_set = get_channelset_by_reg_class(pAd, OP_CLASS_133, PhyMode);

		if (channel_set != NULL) {
			for (j = 0; j < AFC_OP_CLASS_133_CHL_NUM; j++) {
				elem = channel_set[j];

				for (k = 0; k < NumofChannelsToSkip; k++)
					if (elem == g_afc_opclass_skip_channel.OPClass133[k])
						break;

				if (k == NumofChannelsToSkip) {
					ChannelList[ChannelNum] = elem;
					ChannelNum++;
				}
			}

			*NumOfChannels = ChannelNum;
		}
		break;

	case AFC_OP_CLASS_134:
		ChannelNum = 0;
		NumofChannelsToSkip = g_afc_opclass_skip_channel.OPClassNumOfChannel[OpClass];
		channel_set = get_channelset_by_reg_class(pAd, OP_CLASS_134, PhyMode);

		if (channel_set != NULL) {
			for (j = 0; j < AFC_OP_CLASS_134_CHL_NUM; j++) {
				elem = channel_set[j];

				for (k = 0; k < NumofChannelsToSkip; k++)
					if (elem == g_afc_opclass_skip_channel.OPClass134[k])
						break;

				if (k == NumofChannelsToSkip) {
					ChannelList[ChannelNum] = elem;
					ChannelNum++;
				}
			}

			*NumOfChannels = ChannelNum;
		}
		break;

	case AFC_OP_CLASS_135:
		ChannelNum = 0;
		NumofChannelsToSkip = g_afc_opclass_skip_channel.OPClassNumOfChannel[OpClass];
		channel_set = get_channelset_by_reg_class(pAd, OP_CLASS_135, PhyMode);

		if (channel_set != NULL) {
			for (j = 0; j < AFC_OP_CLASS_135_CHL_NUM; j++) {
				elem = channel_set[j];

				for (k = 0; k < NumofChannelsToSkip; k++)
					if (elem == g_afc_opclass_skip_channel.OPClass135[k])
						break;

				if (k == NumofChannelsToSkip) {
					ChannelList[ChannelNum] = elem;
					ChannelNum++;
				}
			}

			*NumOfChannels = ChannelNum;
		}
		break;

	case AFC_OP_CLASS_136:
		ChannelNum = 0;
		NumofChannelsToSkip = g_afc_opclass_skip_channel.OPClassNumOfChannel[OpClass];
		channel_set = get_channelset_by_reg_class(pAd, OP_CLASS_136, PhyMode);

		if (channel_set != NULL) {
			for (j = 0; j < AFC_OP_CLASS_136_CHL_NUM; j++) {
				elem = channel_set[j];

				for (k = 0; k < NumofChannelsToSkip; k++)
					if (elem == g_afc_opclass_skip_channel.OPClass136[k])
						break;

				if (k == NumofChannelsToSkip) {
					ChannelList[ChannelNum] = elem;
					ChannelNum++;
				}
			}

			*NumOfChannels = ChannelNum;
		}
		break;

	default:
		MTWF_PRINT("OP class%d is not supported!", OpClass);
		break;
	}
}

VOID afc_set_op_class(IN PRTMP_ADAPTER pAd, UINT ifIndex, struct afc_glblOperClass *GlobolOpClass, UINT8 *GlobalOpClassNum)
{
	struct wifi_dev *wdev = NULL;
	UINT8 i = 0, count = 0;

	if (ifIndex >= MAX_BEACON_NUM)
		return;

	wdev = &pAd->ApCfg.MBSSID[ifIndex].wdev;

	if (wdev == NULL) {
		MTWF_DBG(pAd, DBG_CAT_SEC, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			"wdev is NULL, return\n");
		return;
	}

	if (WMODE_CAP_AX(wdev->PhyMode)) {
		for (i = 0; i < AFC_OP_CLASS_NUM; i++) {
			if (g_afc_opclass_skip_channel.OPClassEnable[i]) {

				switch (i) {
				case AFC_OP_CLASS_131:
					GlobolOpClass[count].Opclass = OP_CLASS_131;

					if (g_afc_opclass_skip_channel.OPClass131[0] != 0xFF)
						afc_fill_channel_info(pAd, AFC_OP_CLASS_131, wdev->PhyMode, &GlobolOpClass[count].ChannelList[0], &GlobolOpClass[count].NumofChannels);
					count++;
					break;

				case AFC_OP_CLASS_132:
					GlobolOpClass[count].Opclass = OP_CLASS_132;

					if (g_afc_opclass_skip_channel.OPClass132[0] != 0xFF)
						afc_fill_channel_info(pAd, AFC_OP_CLASS_132, wdev->PhyMode, &GlobolOpClass[count].ChannelList[0], &GlobolOpClass[count].NumofChannels);
					count++;
					break;

				case AFC_OP_CLASS_133:
					GlobolOpClass[count].Opclass = OP_CLASS_133;
					if (g_afc_opclass_skip_channel.OPClass133[0] != 0xFF)
						afc_fill_channel_info(pAd, AFC_OP_CLASS_133, wdev->PhyMode, &GlobolOpClass[count].ChannelList[0], &GlobolOpClass[count].NumofChannels);
					count++;
					break;

				case AFC_OP_CLASS_134:
					GlobolOpClass[count].Opclass = OP_CLASS_134;
					if (g_afc_opclass_skip_channel.OPClass134[0] != 0xFF)
						afc_fill_channel_info(pAd, AFC_OP_CLASS_134, wdev->PhyMode, &GlobolOpClass[count].ChannelList[0], &GlobolOpClass[count].NumofChannels);
					count++;
					break;

				case AFC_OP_CLASS_135:
					GlobolOpClass[count].Opclass = OP_CLASS_135;

					if (g_afc_opclass_skip_channel.OPClass135[0] != 0xFF)
						afc_fill_channel_info(pAd, AFC_OP_CLASS_135, wdev->PhyMode, &GlobolOpClass[count].ChannelList[0], &GlobolOpClass[count].NumofChannels);
					count++;
					break;

				case AFC_OP_CLASS_136:
					GlobolOpClass[count].Opclass = OP_CLASS_136;

					if (g_afc_opclass_skip_channel.OPClass136[0] != 0xFF)
						afc_fill_channel_info(pAd, AFC_OP_CLASS_136, wdev->PhyMode, &GlobolOpClass[count].ChannelList[0], &GlobolOpClass[count].NumofChannels);
					count++;
					break;

				default:
					MTWF_PRINT("OP class%d is not supported!", i);
					break;
				}
			}
		}
		*GlobalOpClassNum = count;
	}
}

VOID afc_set_freq_range(struct _RTMP_ADAPTER *pAd, UINT ifIndex, struct afc_frequency_range *freqRange, UINT8 *FrequencyNum)
{
	struct afc_frequency_range freqData[AFC_UNII_BAND_FREQ_NUM] = {
						{UNII_5_STARTING_FREQ, UNII_7_STARTING_FREQ},
						{UNII_5_ENDING_FREQ, UNII_7_ENDING_FREQ}
	};

	struct wifi_dev *wdev = NULL;
	UINT16 i = 0;
	struct afc_frequency_range *PfreqData = g_afc_Freq_range_tbl.freqRange;

	if (ifIndex >= MAX_BEACON_NUM)
		return;

	wdev = &pAd->ApCfg.MBSSID[ifIndex].wdev;

	if (wdev == NULL) {
		MTWF_DBG(pAd, DBG_CAT_SEC, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			"wdev is NULL, return\n");
		return;
	}

	*FrequencyNum = g_afc_Freq_range_tbl.NumOfOpFreqRange;


	for (i = 0; i < *FrequencyNum; i++) {
		freqRange[i].lowFrequency = PfreqData[i].lowFrequency;
		freqRange[i].highFrequency = PfreqData[i].highFrequency;
	}
}

int afc_daemon_channel_info(struct _RTMP_ADAPTER *pAd, struct __RTMP_IOCTL_INPUT_STRUCT *wrq, POS_COOKIE pObj)
{
	struct afc_device_info *afc_data;
	int Status = NDIS_STATUS_FAILURE;
	UINT ifIndex = pObj->ioctl_if;
	struct wifi_dev *wdev = &pAd->ApCfg.MBSSID[ifIndex].wdev;
	UINT16 size = 0;

	ifIndex = afc_get_6g_wdev(pAd);

	if (!g_afc_Freq_range_tbl.NumOfOpFreqRange)
		size = sizeof(struct afc_device_info) + (sizeof(struct afc_frequency_range) * AFC_UNII_BAND_FREQ_NUM);
	else
		size = sizeof(struct afc_device_info) + (sizeof(struct afc_frequency_range) * g_afc_Freq_range_tbl.NumOfOpFreqRange);

	os_alloc_mem(pAd, (PUCHAR *)&afc_data, size);
	memset(afc_data, 0, size);

	if (ifIndex != AFC_6G_WDEV_NOT_AVAILABLE) {
		wdev = pAd->wdev_list[ifIndex];
	} else {
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				"%s:: 6Ghz interface not available%d\n", __func__);
		Status = NDIS_STATUS_RESOURCES;
		return Status;
	}

	if (!IS_AFC_ENABLED(pAd)) {
		MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			"%s:AFC is not Enabled(%d)\n", __func__,
			pAd->afc_ctrl.AfcStateMachine.CurrState);
		return Status;
	}

	if (wdev == NULL) {
		MTWF_DBG(pAd, DBG_CAT_SEC, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			"wdev is NULL, return\n");
		return Status;
	}

	if (GetCountryRegionFromCountryCode(pAd->CommonCfg.CountryCode) == FCC)
		strcpy(afc_data->regionCode, "FCC");
	else {
		MTWF_DBG(pAd, DBG_CAT_SEC, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					"Country Region Code is not FCC !!!!\n");
		return Status;
	}
	afc_data->SpectrumType = pAd->CommonCfg.AfcSpectrumType;

	afc_set_op_class(pAd, ifIndex, &afc_data->glblOperClass[0], &afc_data->glblOperClassNum);
	afc_set_freq_range(pAd, ifIndex, afc_data->InqfreqRange.freqRange, &afc_data->InqfreqRange.NumOfOpFreqRange);

	wrq->u.data.length = size;
	Status = copy_to_user(wrq->u.data.pointer, afc_data, wrq->u.data.length);
	MTWF_DBG(pAd, DBG_CAT_SEC, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					"current Afc_state is %d\n", pAd->afc_ctrl.AfcStateMachine.CurrState);
	MTWF_DBG(pAd, DBG_CAT_SEC, DBG_SUBCAT_ALL, DBG_LVL_INFO,
					"regionCode is %s, SpectrumType = %d\n", afc_data->regionCode, afc_data->SpectrumType);
	MTWF_DBG(pAd, DBG_CAT_SEC, DBG_SUBCAT_ALL, DBG_LVL_INFO,
					"glblOperClassNum is %d\n", afc_data->glblOperClassNum);

	afc_initiate_request_to_daemon(pAd, wdev);
	os_free_mem(afc_data);

	return Status;
}

INT afc_get_channel_index(UINT8 u1DesiredChannel)
{
	UINT16 RowIndex, Channel;

	for (RowIndex = 0; RowIndex < AFC_6G_PWR_LIMIT_TBL_ROW; RowIndex++) {
		Channel = (RowIndex * AFC_6G_CHANNEL_FREQ_GAP) + 1;
		if (Channel == u1DesiredChannel)
			return RowIndex;
	}

	MTWF_PRINT("Desired Channel %d is does not match!", u1DesiredChannel);
	return AFC_INVALID_CH_INDEX;
}

UINT8 afc_pwr_calculation(IN PRTMP_ADAPTER pAd, UINT8 u1ParamIdx, UINT16 u2AfcChannelIdx, INT8 i1PwrValue, UINT8 TblType)
{
	INT8 i1AfcPwrVal = 0;
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	INT32 ifIndex = pObj->ioctl_if;
	struct wifi_dev *wdev;

	ifIndex = afc_get_6g_wdev(pAd);

	if (ifIndex != AFC_6G_WDEV_NOT_AVAILABLE) {
		wdev = pAd->wdev_list[ifIndex];
	} else {
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				"%s:: 6Ghz interface not available\n", __func__);
		return i1PwrValue;
	}

	if (!(WMODE_CAP_6G(wdev->PhyMode) && (wdev->wdev_type == WDEV_TYPE_AP)))
		return i1PwrValue;

	if (!IS_AFC_RUN_STATE(pAd))
		return i1PwrValue;

	i1AfcPwrVal = afc_get_pwr_value(u1ParamIdx, u2AfcChannelIdx, TblType);
	i1PwrValue = afc_get_min_pwr(i1PwrValue, i1AfcPwrVal);

	return i1PwrValue;
}
INT afc_get_pwr_value(UINT8 u1ParamIdx, UINT16 u2RowIndex, UINT8 TblType)
{
	INT8 i1PwrVal = 0;
	UINT8 u1Bw = 0;
	INT8 power_offset_CCK_OFDM[11] = {0, 6, 10, 12, 0, 6, 10, 12, 12, 20, 24};
	INT8 power_offset[10] = {0, 6, 10, 12, 0, 4, 6, 0, 3, 0};
	UINT8 power_offset_idx = 0, BF_enable = 0;

	if (TblType == POWER_LIMIT_TABLE_TYPE_SKU) {
		if ((u1ParamIdx < AFC_TXPWR_HT40_IDX)  || ((u1ParamIdx >= AFC_TXPWR_VHT20_IDX) && (u1ParamIdx < AFC_TXPWR_VHT40_IDX)))
			u1Bw = AFC_TXPWR_TBL_BW20;
		else if (((u1ParamIdx >= AFC_TXPWR_HT40_IDX) && (u1ParamIdx < AFC_TXPWR_VHT20_IDX)) || ((u1ParamIdx >= AFC_TXPWR_VHT40_IDX) && (u1ParamIdx < AFC_TXPWR_VHT80_IDX)))
			u1Bw = AFC_TXPWR_TBL_BW40;
		else if ((u1ParamIdx >= AFC_TXPWR_VHT80_IDX) && (u1ParamIdx < AFC_TXPWR_VHT160_IDX))
			u1Bw = AFC_TXPWR_TBL_BW80;
		else if ((u1ParamIdx >= AFC_TXPWR_VHT160_IDX) && (u1ParamIdx < AFC_TXPWR_RU26_IDX))
			u1Bw = AFC_TXPWR_TBL_BW160;
		else if ((u1ParamIdx >= AFC_TXPWR_RU26_IDX) && (u1ParamIdx < AFC_TXPWR_RU52_IDX))
			u1Bw = AFC_TXPWR_TBL_RU26;
		else if ((u1ParamIdx >= AFC_TXPWR_RU52_IDX) && (u1ParamIdx < AFC_TXPWR_RU106_IDX))
			u1Bw = AFC_TXPWR_TBL_RU52;
		else if ((u1ParamIdx >= AFC_TXPWR_RU106_IDX) && (u1ParamIdx < AFC_TXPWR_RU242_IDX))
			u1Bw = AFC_TXPWR_TBL_RU106;
		else if ((u1ParamIdx >= AFC_TXPWR_RU242_IDX) && (u1ParamIdx < AFC_TXPWR_RU484_IDX))
			u1Bw = AFC_TXPWR_TBL_BW20;
		else if ((u1ParamIdx >= AFC_TXPWR_RU484_IDX) && (u1ParamIdx < AFC_TXPWR_RU996_IDX))
			u1Bw = AFC_TXPWR_TBL_BW40;
		else if ((u1ParamIdx >= AFC_TXPWR_RU996_IDX) && (u1ParamIdx < AFC_TXPWR_RU996X2_IDX))
			u1Bw = AFC_TXPWR_TBL_BW80;
		else if ((u1ParamIdx >= AFC_TXPWR_RU996X2_IDX) && (u1ParamIdx < AFC_TXPWR_END_IDX))
			u1Bw = AFC_TXPWR_TBL_BW160;
	} else if (TblType == POWER_LIMIT_TABLE_TYPE_BACKOFF) {
		if ((u1ParamIdx >= AFC_BACKOFF_CCK_BF_OFF_IDX) && (u1ParamIdx < AFC_BACKOFF_HT40_BF_OFF_IDX))
			u1Bw = AFC_TXPWR_TBL_BW20;
		else if ((u1ParamIdx >= AFC_BACKOFF_HT40_BF_OFF_IDX) && (u1ParamIdx < AFC_BACKOFF_VHT20_BF_OFF_IDX))
			u1Bw = AFC_TXPWR_TBL_BW40;
		else if ((u1ParamIdx >= AFC_BACKOFF_VHT20_BF_OFF_IDX) && (u1ParamIdx < AFC_BACKOFF_VHT40_BF_OFF_IDX))
			u1Bw = AFC_TXPWR_TBL_BW20;
		else if ((u1ParamIdx >= AFC_BACKOFF_VHT40_BF_OFF_IDX) && (u1ParamIdx < AFC_BACKOFF_VHT80_BF_OFF_IDX))
			u1Bw = AFC_TXPWR_TBL_BW40;
		else if ((u1ParamIdx >= AFC_BACKOFF_VHT80_BF_OFF_IDX) && (u1ParamIdx < AFC_BACKOFF_VHT160_BF_OFF_IDX))
			u1Bw = AFC_TXPWR_TBL_BW80;
		else if ((u1ParamIdx >= AFC_BACKOFF_VHT160_BF_OFF_IDX) && (u1ParamIdx < AFC_BACKOFF_RU26_BF_OFF_IDX))
			u1Bw = AFC_TXPWR_TBL_BW160;
		else if ((u1ParamIdx >= AFC_BACKOFF_RU26_BF_OFF_IDX) && (u1ParamIdx < AFC_BACKOFF_RU52_BF_OFF_IDX))
			u1Bw = AFC_TXPWR_TBL_RU26;
		else if ((u1ParamIdx >= AFC_BACKOFF_RU52_BF_OFF_IDX) && (u1ParamIdx < AFC_BACKOFF_RU106_BF_OFF_IDX))
			u1Bw = AFC_TXPWR_TBL_RU52;
		else if ((u1ParamIdx >= AFC_BACKOFF_RU106_BF_OFF_IDX) && (u1ParamIdx < AFC_BACKOFF_END_IDX))
			u1Bw = AFC_TXPWR_TBL_RU106;
	}


	if ((u1Bw < AFC_TXPWR_TBL_BW_NUM) && (u2RowIndex < AFC_6G_PWR_LIMIT_TBL_ROW))
		i1PwrVal = g_afc_channel_pwr[u1Bw][u2RowIndex];
	else
		i1PwrVal = AFC_TXPWR_VAL_INVALID;

	if (TblType == POWER_LIMIT_TABLE_TYPE_BACKOFF) {
		if (u1ParamIdx < AFC_BACKOFF_HT20_BF_OFF_IDX)
			i1PwrVal = i1PwrVal - power_offset_CCK_OFDM[u1ParamIdx];
		else if ((u1ParamIdx >= AFC_BACKOFF_HT20_BF_OFF_IDX) && (u1ParamIdx < AFC_BACKOFF_RU26_BF_OFF_IDX)) {
			power_offset_idx = (u1ParamIdx - AFC_BACKOFF_HT20_BF_OFF_IDX) % 19;
			if (power_offset_idx <= 9)
				i1PwrVal = i1PwrVal - power_offset[power_offset_idx];
			else {
				power_offset_idx = (power_offset_idx % 10) + 1;
				i1PwrVal = i1PwrVal - 2*power_offset[power_offset_idx];
			}
		} else if (u1ParamIdx >= AFC_BACKOFF_RU26_BF_OFF_IDX) {
			power_offset_idx = (u1ParamIdx - AFC_BACKOFF_RU26_BF_OFF_IDX) % 20;
			if (power_offset_idx <= 9)
				i1PwrVal = i1PwrVal - power_offset[power_offset_idx];
			else {
				power_offset_idx = power_offset_idx % 10;
				i1PwrVal = i1PwrVal - 2*power_offset[power_offset_idx];
			}
		}
	}

	MTWF_DBG(NULL, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_INFO,
			"%s: u1ParamIdx: %d, u2RowIndex: %d, u1Bw: %d, i1PwrVal: %d\n",
			__func__, u1ParamIdx, u2RowIndex, u1Bw, i1PwrVal);

	return i1PwrVal;
}


VOID afc_save_autochannel_skip_init(IN PRTMP_ADAPTER pAd)
{
	UINT8 u1Num = 0;
	INT Index1;

	u1Num = pAd->ApCfg.AutoChannelSkipListNum6G;
	pAd->afc_ctrl.AfcAutoChannelSkipListNum6G = u1Num;
	memcpy(&pAd->afc_ctrl.AfcAutoChannelSkipList6G[0], &pAd->ApCfg.AutoChannelSkipList6G[0], u1Num);

	MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
		 "\n\n%s:: ",
		 __func__);

	for (Index1 = 0; Index1 < pAd->ApCfg.AutoChannelSkipListNum6G; Index1++)
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			 " %u", __func__, pAd->ApCfg.AutoChannelSkipList6G[Index1]);

	MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
		 "\n%s<<<<<<<<\n",
		 __func__);
}

VOID afc_dump_skip_channel_list(IN PRTMP_ADAPTER pAd)
{
	UINT8 i;

	MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			"afc_dump_skip_channel_list: AutoChannelSkipList6GNum=%d\n", pAd->ApCfg.AutoChannelSkipListNum6G);

	for (i = 0; i < pAd->ApCfg.AutoChannelSkipListNum6G; i++)
		MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			"afc_dump_skip_channel_list: AutoChannelSkipList6G[%d]=%d\n", i, pAd->ApCfg.AutoChannelSkipList6G[i]);
}

VOID afc_dump_original_skip_channel_list(IN PRTMP_ADAPTER pAd)
{
	UINT8 i;

	MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			"afc_dump_original_skip_channel_list: AutoChannelSkipList6GNum=%d\n", pAd->afc_ctrl.AfcAutoChannelSkipListNum6G);

	for (i = 0; i < pAd->ApCfg.AutoChannelSkipListNum6G; i++)
		MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			"afc_dump_original_skip_channel_list: AutoChannelSkipList6G[%d]=%d\n", i, pAd->afc_ctrl.AfcAutoChannelSkipList6G[i]);
}

VOID afc_update_auto_channel_skip_list(IN PRTMP_ADAPTER pAd)
{
	UINT8 i, u1RowIndex, u1Num = 0, u1MatchFound = 0;
	INT Index1;

	/* Re-init original skip channel list*/
	for (i = 0; i < pAd->ApCfg.AutoChannelSkipListNum6G; i++)
		pAd->ApCfg.AutoChannelSkipList6G[i] = 0;
	pAd->ApCfg.AutoChannelSkipListNum6G = 0;

	afc_dump_skip_channel_list(pAd);

	/* Update list with saved values */
	for (i = 0; i < pAd->afc_ctrl.AfcAutoChannelSkipListNum6G; i++)
		pAd->ApCfg.AutoChannelSkipList6G[i] = pAd->afc_ctrl.AfcAutoChannelSkipList6G[i];
	pAd->ApCfg.AutoChannelSkipListNum6G = pAd->afc_ctrl.AfcAutoChannelSkipListNum6G;

	afc_dump_skip_channel_list(pAd);

	/* Update list for unavailable channels */
	for (u1RowIndex = 0; u1RowIndex < AFC_6G_PWR_LIMIT_TBL_ROW; u1RowIndex++) {
		if (g_afc_channel_pwr[AFC_TXPWR_TBL_BW20][u1RowIndex] < NON_AFC_CHANNEL)
			continue;
		u1MatchFound = FALSE;
		Index1 = (u1RowIndex * AFC_6G_CHANNEL_FREQ_GAP) + 1;

		for (i = 0; i < pAd->afc_ctrl.AfcAutoChannelSkipListNum6G; i++) {
			if (pAd->afc_ctrl.AfcAutoChannelSkipList6G[i] == Index1) {
				u1MatchFound = TRUE;
				break;
			}
		}

		if ((u1MatchFound == FALSE) && (pAd->ApCfg.AutoChannelSkipListNum6G <= MAX_NUM_OF_CHANNELS)) {
			u1Num = pAd->ApCfg.AutoChannelSkipListNum6G;
			pAd->ApCfg.AutoChannelSkipList6G[u1Num] = Index1;
			pAd->ApCfg.AutoChannelSkipListNum6G++;
		}
	}

	afc_dump_skip_channel_list(pAd);
}

INT afc_default_channel_available(IN PRTMP_ADAPTER pAd)
{
	UINT8 u1Bw;
	INT RowIndex1, RowIndex2;

	u1Bw = _afc_switch_chan_params.oper.bw;
	RowIndex1 = ((_afc_switch_chan_params.oper.prim_ch - 1) / AFC_6G_CHANNEL_FREQ_GAP);

	MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			"afc_default_channel_available: Bw=%d, RowIndex1=%d\n", u1Bw, RowIndex1);

	if (u1Bw != BW_8080) {
		if (g_afc_channel_pwr[u1Bw][RowIndex1] < NON_AFC_CHANNEL) {
			MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				"afc_default_channel_available: power =%d\n", g_afc_channel_pwr[u1Bw][RowIndex1]);
			return TRUE;
		}
	} else if (_afc_switch_chan_params.oper.cen_ch_2) {
		RowIndex2 = ((_afc_switch_chan_params.oper.cen_ch_2 - 1) / AFC_6G_CHANNEL_FREQ_GAP);

		if ((g_afc_channel_pwr[u1Bw][RowIndex1] < NON_AFC_CHANNEL) && (g_afc_channel_pwr[u1Bw][RowIndex2] < NON_AFC_CHANNEL))
			return TRUE;
	}

	return FALSE;
}

INT afc_get_cen_ch(UINT16 RowIndex, UINT8 u1Bw)
{
	INT Index1 = 0;

	if (u1Bw == AFC_TXPWR_TBL_BW20)
		Index1 = (RowIndex * AFC_6G_CHANNEL_FREQ_GAP) + AFC_CEN_CH_BW_20_START;
	else if (u1Bw == AFC_TXPWR_TBL_BW40)
		Index1 = (RowIndex * (AFC_6G_CHANNEL_FREQ_GAP * AFC_NUM_20MHZ_IN_40MHZ)) + AFC_CEN_CH_BW_40_START;
	else if (u1Bw == AFC_TXPWR_TBL_BW80)
		Index1 = (RowIndex * (AFC_6G_CHANNEL_FREQ_GAP * AFC_NUM_20MHZ_IN_80MHZ)) + AFC_CEN_CH_BW_80_START;
	else if (u1Bw == AFC_TXPWR_TBL_BW160)
		Index1 = (RowIndex * (AFC_6G_CHANNEL_FREQ_GAP * AFC_NUM_20MHZ_IN_160MHZ)) + AFC_CEN_CH_BW_160_START;

	return Index1;
}

VOID afc_get_ext_ch(IN PRTMP_ADAPTER pAd, UINT ifIndex)
{
	struct wifi_dev *wdev;
	struct freq_cfg fcfg;
	UCHAR BandIdx = 0;

	if (ifIndex >= MAX_BEACON_NUM)
		return;

	os_zero_mem(&fcfg, sizeof(fcfg));
	wdev = &pAd->ApCfg.MBSSID[ifIndex].wdev;
	BandIdx = HcGetBandByWdev(wdev);

	if (pAd->CommonCfg.bBssCoexEnable &&
		pAd->CommonCfg.BssCoexScanLastResult[BandIdx].bNeedFallBack) {
		fcfg.ht_bw = wlan_operate_get_ht_bw(wdev);
		fcfg.ext_cha = wlan_operate_get_ext_cha(wdev);
	} else {
		fcfg.ht_bw = wlan_config_get_ht_bw(wdev);
		fcfg.ext_cha = wlan_config_get_ext_cha(wdev);
	}

	if (WMODE_CAP_6G(wdev->PhyMode)) {
		_afc_switch_chan_params.oper.ht_bw = fcfg.ht_bw;
		_afc_switch_chan_params.oper.ext_cha = fcfg.ext_cha;
		if (!is_testmode_wdev(wdev->wdev_type))
			ht_ext_cha_adjust(wdev->sys_handle, _afc_switch_chan_params.oper.prim_ch, &_afc_switch_chan_params.oper.ht_bw, &_afc_switch_chan_params.oper.ext_cha, wdev);
	}
}
INT afc_check_req_condition(IN PRTMP_ADAPTER pAd)
{
	if (pAd->CommonCfg.AcsAfterAfc == FALSE)
		return TRUE;

	if (strncmp((RTMP_STRING *) pAd->CommonCfg.CountryCode, "US", 2))
		return TRUE;

	if (GetCountryRegionFromCountryCode(pAd->CommonCfg.CountryCode) != FCC)
		return TRUE;

	if (pAd->CommonCfg.AfcDeviceType != AFC_STANDARD_POWER_DEVICE)
		return TRUE;

	return FALSE;
}

INT afc_check_bw_availabilty(UINT8 u1Bw)
{
	INT RowIndex = 0;

	if (u1Bw < AFC_TXPWR_TBL_BW160) {
		for (RowIndex = 0; RowIndex < AFC_6G_PWR_LIMIT_TBL_ROW; RowIndex++) {
			if (g_afc_channel_pwr[u1Bw][RowIndex] < NON_AFC_CHANNEL)
				return TRUE;
		}
	}

	return FALSE;
}

INT afc_get_available_bw(IN PRTMP_ADAPTER pAd, UINT8 *cfg_vht_bw, UINT8 *cfg_ht_bw)
{
	UINT8 u1Bw = 0;

	if (afc_check_req_condition(pAd))
		return TRUE;

	if (*cfg_ht_bw == BW_20)
		u1Bw = AFC_TXPWR_TBL_BW20;
	else if ((*cfg_ht_bw == BW_40) && (*cfg_vht_bw == VHT_BW_2040))
		u1Bw = AFC_TXPWR_TBL_BW40;
	else if ((*cfg_vht_bw == VHT_BW_80) || (*cfg_vht_bw == VHT_BW_8080))
		u1Bw = AFC_TXPWR_TBL_BW80;
	else if (*cfg_vht_bw == VHT_BW_160)
		u1Bw = AFC_TXPWR_TBL_BW160;

	for (; ((u1Bw >= AFC_TXPWR_TBL_BW20) && (u1Bw <= AFC_TXPWR_TBL_BW160)); u1Bw--) {
		if (afc_check_bw_availabilty(u1Bw)) {
			if (u1Bw == AFC_TXPWR_TBL_BW20) {
				*cfg_ht_bw = BW_20;
				*cfg_vht_bw = VHT_BW_2040;
			} else if (u1Bw == AFC_TXPWR_TBL_BW40) {
				*cfg_ht_bw = BW_40;
				*cfg_vht_bw = VHT_BW_2040;
			} else if (u1Bw == AFC_TXPWR_TBL_BW80)
				*cfg_vht_bw = VHT_BW_80;
			else if (u1Bw == AFC_TXPWR_TBL_BW160)
				*cfg_vht_bw = VHT_BW_160;

			return TRUE;
		}
	}

	return TRUE;
}

INT afc_check_valid_channel_by_bw(IN PRTMP_ADAPTER pAd, UCHAR Channel, UINT8 u1Bw)
{
	INT RowIndex = 0;
	BOOLEAN IsValid = FALSE;

	/* chech required conditions as per afc*/
	if (afc_check_req_condition(pAd))
		return TRUE;

	RowIndex = (Channel - 1) / (AFC_6G_CHANNEL_FREQ_GAP);

	if (g_afc_channel_pwr[u1Bw][RowIndex] < NON_AFC_CHANNEL)
		IsValid = TRUE;

	MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			"%s::: Bw=%d, RowIndex=%d, Channel=%d, IsValid=%d\n", __func__, u1Bw, RowIndex, Channel, IsValid);

	return IsValid;
}

INT afc_update_channel_params_by_bw(IN PRTMP_ADAPTER pAd, UINT8 u1Bw, UINT ifIndex)
{
	INT Index1, RowIndex;

	MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			"afc_update_channel_params_by_bw: Bw=%d\n", u1Bw);
	if (u1Bw < AFC_TXPWR_TBL_BW160) {
		for (RowIndex = 0; RowIndex < AFC_6G_PWR_LIMIT_TBL_ROW; RowIndex++) {
			Index1 = (RowIndex * AFC_6G_CHANNEL_FREQ_GAP) + 1;

			if (g_afc_channel_pwr[u1Bw][RowIndex] < NON_AFC_CHANNEL) {
				_afc_switch_chan_params.oper.prim_ch = Index1;
				_afc_switch_chan_params.oper.cen_ch_1 = afc_get_cen_ch(RowIndex, u1Bw);
				afc_get_ext_ch(pAd, ifIndex);

				MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					"afc_update_channel_params_by_bw: Bw=%d, RowIndex=%d\n", u1Bw, RowIndex);
				MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					"afc_update_channel_params_by_bw: prim_ch=%d, cen_ch_1=%d, ext_ch=%d\n", _afc_switch_chan_params.oper.prim_ch, _afc_switch_chan_params.oper.cen_ch_1, _afc_switch_chan_params.oper.ext_cha);
				return TRUE;
			}
		}
	}
	return FALSE;
}

VOID afc_update_oper_channel_params(IN PRTMP_ADAPTER pAd, UINT ifIndex)
{
	struct wifi_dev *wdev = NULL;
	struct freq_cfg fcfg;

	if (ifIndex >= MAX_BEACON_NUM)
		return;


	wdev = &pAd->ApCfg.MBSSID[ifIndex].wdev;

	if (wdev == NULL)
		return;

	MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
						"%s::1\n", __func__);

	os_zero_mem(&fcfg, sizeof(fcfg));

	fcfg.ht_bw = _afc_switch_chan_params.oper.ht_bw;
	fcfg.vht_bw = _afc_switch_chan_params.oper.vht_bw;
	fcfg.ch_band = _afc_switch_chan_params.oper.ch_band;
	fcfg.rx_stream = _afc_switch_chan_params.oper.rx_stream;

	if (fcfg.ht_bw == BW_40)
		fcfg.ext_cha = _afc_switch_chan_params.oper.ext_cha;
	else
		fcfg.ext_cha = EXTCHA_NONE;

	fcfg.prim_ch = _afc_switch_chan_params.oper.prim_ch;
	fcfg.cen_ch_2 = _afc_switch_chan_params.oper.cen_ch_2;

	wlan_operate_set_phy(wdev, &fcfg);
}

VOID afc_update_bw(IN PRTMP_ADAPTER pAd, UINT ifIndex)
{
	struct wifi_dev *wdev = NULL;
	struct freq_oper OperCh;
	UINT HtBw = 0, vht_bw = 0;

	if (ifIndex >= MAX_BEACON_NUM)
		return;


	wdev = &pAd->ApCfg.MBSSID[ifIndex].wdev;

	if (wdev == NULL)
		return;

	HtBw = _afc_switch_chan_params.oper.ht_bw;
	vht_bw = _afc_switch_chan_params.oper.vht_bw;

	MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
						"%s::1\n", __func__);


	afc_update_oper_channel_params(pAd, ifIndex);

	MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
						"%s::2\n", __func__);

	hc_oper_query_by_wdev(wdev, &OperCh);
	MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
						"%s::5\n", __func__);

	ap_update_rf_ch_for_mbss(pAd, wdev, &OperCh);

	MTWF_DBG(pAd, DBG_CAT_HW, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				"%s: OperCh.bw:%u\n",
				__func__, OperCh.bw);

	UpdatedRaBfInfoBwByWdev(pAd, wdev, OperCh.bw);
	SetCommonHtVht(pAd, wdev);
}

INT afc_update_channel_params(IN PRTMP_ADAPTER pAd, UINT ifIndex)
{
	UINT8 u1Bw = 0;
	struct wifi_dev *wdev;
	UCHAR BandIdx = 0;

	wdev = &pAd->ApCfg.MBSSID[ifIndex].wdev;
	BandIdx = HcGetBandByWdev(wdev);

	if (afc_default_channel_available(pAd)) {
		MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				"%s:: Got Default Channel Bw = (%d, %d, %d)\n",
				__func__, _afc_switch_chan_params.oper.bw,
				_afc_switch_chan_params.oper.cen_ch_1, _afc_switch_chan_params.oper.prim_ch);
		afc_update_bw(pAd, ifIndex);
		MtPwrLimitTblChProc(pAd, BandIdx,
					_afc_switch_chan_params.oper.ch_band,
					_afc_switch_chan_params.oper.prim_ch,
					_afc_switch_chan_params.oper.cen_ch_1);

		return TRUE;
	}

	if (_afc_switch_chan_params.oper.bw == BW_8080) {
		_afc_switch_chan_params.oper.bw = BW_160;
		_afc_switch_chan_params.oper.cen_ch_2 = 0;
	}

	for (u1Bw = _afc_switch_chan_params.oper.bw; ((u1Bw >= AFC_TXPWR_TBL_BW20) && (u1Bw <= AFC_TXPWR_TBL_BW160)); u1Bw--) {
		if (afc_update_channel_params_by_bw(pAd, u1Bw, ifIndex)) {
			_afc_switch_chan_params.oper.bw = u1Bw;
			_afc_switch_chan_params.oper.ht_bw = (_afc_switch_chan_params.oper.bw > BW_20) ? HT_BW_40 : HT_BW_20;
			_afc_switch_chan_params.oper.vht_bw = rf_bw_2_vht_bw(_afc_switch_chan_params.oper.bw);
			afc_update_bw(pAd, ifIndex);
			MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				"afc_update_channel_params: ht_bw=%d, vht_bw=%d\n", _afc_switch_chan_params.oper.ht_bw, _afc_switch_chan_params.oper.vht_bw);
			return TRUE;
		}
	}

	return FALSE;
}

INT afc_use_acs_switch_channel(IN PRTMP_ADAPTER pAd, UINT ifIndex)
{
	struct wifi_dev *wdev;

	wdev = &pAd->ApCfg.MBSSID[ifIndex].wdev;

	if (WMODE_CAP_6G(wdev->PhyMode)) {
		AutoChSelScanStart(pAd, wdev);
		return TRUE;
	} else
		return FALSE;
}

INT afc_switch_channel(IN PRTMP_ADAPTER pAd, UINT ifIndex)
{
	INT status = TRUE;
	MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			"afc_switch_channel------->\n");
	if (afc_update_channel_params(pAd, ifIndex)) {

		MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			"afc_switch_channel: Channel is available: calling AsicSwitchChannel------->\n");
	} else {
		MTWF_PRINT("Channel Not Available!");
		status = FALSE;
	}
	MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			"afc_switch_channel<-------\n");

	return status;
}

INT8 afc_get_min_pwr(INT8 i1PwrVal, INT8 i1AfcPwr)
{
	if ((i1AfcPwr < NON_AFC_CHANNEL) && (i1PwrVal > i1AfcPwr))
		return i1AfcPwr;
	else
		return i1PwrVal;
}

VOID afc_initiate_request_to_daemon(struct _RTMP_ADAPTER *pAd, struct wifi_dev *wdev)
{
	/* If Interface is 6G and is UP, wait for timer 1-2sec and send request */
	if ((wdev->if_up_down_state) && (WMODE_CAP_6G(wdev->PhyMode))) {
		_is_afc_stop = FALSE;
		pAd->afc_ctrl.bAfcTimer = TRUE;
		pAd->afc_ctrl.u1AfcTimeCnt = 0;
		MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			"afc_initiate_request_to_daemon: bAfcTimer=%d, u1AfcTimeCnt=%d\n", pAd->afc_ctrl.bAfcTimer, pAd->afc_ctrl.u1AfcTimeCnt);
	}
}

INT afc_beacon_init_handler(IN PRTMP_ADAPTER pAd, IN struct wifi_dev *wdev)
{
	MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
		"afc_beacon_init_handler: CurrState: %d, bAfcBeaconInit= %d\n", pAd->afc_ctrl.AfcStateMachine.CurrState, pAd->afc_ctrl.bAfcBeaconInit);
	if (IS_AFC_HOLD_STATE(pAd) && (!pAd->afc_ctrl.bAfcBeaconInit)) {
		wdev->bAllowBeaconing = FALSE;
		return FALSE;
	}

	return TRUE;
}

INT afc_check_tx_enable(IN PRTMP_ADAPTER pAd, IN struct wifi_dev *wdev)
{
	if ((wdev) && (WMODE_CAP_6G(wdev->PhyMode)) && IS_AFC_HOLD_STATE(pAd)) {
		MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_INFO,
		"afc_check_tx_enable: Tx is disabled\n");
		return FALSE;
	}

	MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_INFO,
	"afc_check_tx_enable: Tx is enabled\n");

	return TRUE;
}

VOID afc_update_txpwr_envelope_params(
	struct wifi_dev *wdev, UINT8 *tx_pwr_bw, UINT8 u1NValue, UINT8 txpwr_interpretation)
{
	struct _RTMP_ADAPTER *pAd;
	UINT8 len, RowIndex;
	INT8 client_txPwr = 0;

	ASSERT(wdev->sys_handle);
	pAd = (RTMP_ADAPTER *)wdev->sys_handle;
	if (pAd == NULL) {
		MTWF_DBG(NULL, DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			"%s: pAd is NULL!\n", __func__);
		return;
	}

	if (wlan_config_get_ch_band(wdev) != CMD_CH_BAND_6G)
		return;

	MTWF_DBG(pAd, DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_INFO,
		"%s:: NValue:%u bw:%u\n", __func__, u1NValue, _afc_switch_chan_params.oper.bw);

	if (!IS_AFC_RUN_STATE(pAd))
		return;

	if (pAd->afc_response_data.response_code != AFC_RESPONSE_TYPE_SUCCESS)
		return;

	RowIndex = _afc_switch_chan_params.oper.prim_ch >> 2;
	if (txpwr_interpretation == TX_PWR_INTERPRET_REG_CLIENT_EIRP_PSD) {

		client_txPwr = g_afc_channel_pwr[_afc_switch_chan_params.oper.bw][RowIndex];

		MTWF_DBG(pAd, DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_INFO,
			"%s:: BW:%u RowIdx:%d Pwr:%d\n",
			__func__, _afc_switch_chan_params.oper.bw, RowIndex, client_txPwr);

		client_txPwr -= (AFC_TXPWR_20MHZ_DELTA_PSD + (AFC_TXPWR_DOUBLE_IN_DB * _afc_switch_chan_params.oper.bw));

		if (client_txPwr > AFC_TXPWR_ENV_MAX_PSD)
			client_txPwr = AFC_TXPWR_ENV_MAX_PSD;

		MTWF_DBG(pAd, DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_INFO,
			"%s:: Final Pwr:%d\n",
			__func__, client_txPwr);

		for (len = 0; len <= u1NValue; len++)
			tx_pwr_bw[len] = (UINT8) client_txPwr;
	} else {
		for (len = 0; len <= u1NValue; len++) {
			tx_pwr_bw[len] = g_afc_channel_pwr[len][RowIndex];
			if (tx_pwr_bw[len] > AFC_TXPWR_ENV_MAX_EIRP)
				tx_pwr_bw[len] = AFC_TXPWR_ENV_MAX_EIRP;
			MTWF_DBG(pAd, DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_INFO,
				":: EIRP BW:%d, Pwr:%d\n",
				len, tx_pwr_bw[len]);
		}
	}
}

INT afc_bcn_check(IN PRTMP_ADAPTER pAd)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	INT32 ifIndex = pObj->ioctl_if;
	struct wifi_dev *wdev;

	wdev = &pAd->ApCfg.MBSSID[ifIndex].wdev;

	if (IS_AFC_HOLD_STATE(pAd) &&
		WMODE_CAP_6G(wdev->PhyMode) && (wdev->wdev_type == WDEV_TYPE_AP))
		return FALSE;

	return TRUE;
}
INT is_afc_in_run_state(IN PRTMP_ADAPTER pAd)
{
	if (pAd)
		return IS_AFC_RUN_STATE(pAd);
	else
		return FALSE;
}

INT is_afc_stop(void)
{
	return _is_afc_stop;
}
VOID afc_send_daemon_stop_event(IN PRTMP_ADAPTER pAd, IN struct wifi_dev *wdev)
{
	MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
	"ap_inf_close---> afc_send_daemon_stop_event\n");

	if ((WMODE_CAP_6G(wdev->PhyMode)) && !IS_AFC_DISABLE_STATE(pAd)) {
		_is_afc_stop = TRUE;
		afc_set_wdev_to_hold(pAd, wdev);
		RtmpOSWrielessEventSend(pAd->net_dev,
				RT_WLAN_EVENT_CUSTOM,
				AFC_STOP_EVENT,
				NULL,
				(char *)NULL, 0);
	}
}

VOID afc_free_spectrum(void)
{
	os_free_mem(g_afc_Freq_range_tbl.freqRange);
}

VOID afc_send_inquiry_event_timer(IN PRTMP_ADAPTER pAd)
{
	POS_COOKIE pObj = (POS_COOKIE) pAd->OS_Cookie;
	INT32 ifIndex = pObj->ioctl_if;
	struct wifi_dev *wdev;

	ifIndex = afc_get_6g_wdev(pAd);

	if (ifIndex != AFC_6G_WDEV_NOT_AVAILABLE) {
		wdev = pAd->wdev_list[ifIndex];
	} else {
		MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				"%s:: 6Ghz interface not available\n", __func__);
		return;
	}
	if (!(WMODE_CAP_6G(wdev->PhyMode) && (wdev->wdev_type == WDEV_TYPE_AP)))
		return;

	if (pAd->afc_ctrl.bAfcTimer) {
		pAd->afc_ctrl.u1AfcTimeCnt++;
		if (!IS_AFC_ENABLED(pAd)) {
			MTWF_DBG(pAd, DBG_CAT_ALL, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
						 "%s::AFC state(%d)\n", __func__,
						 pAd->afc_ctrl.AfcStateMachine.CurrState);
			pAd->afc_ctrl.bAfcTimer = FALSE;
			pAd->afc_ctrl.u1AfcTimeCnt = 0;
			return;
		}

		if (pAd->afc_ctrl.u1AfcTimeCnt == AFC_INQUIRY_EVENT_TIMEOUT) {
			MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			"afc_initiate_request_to_daemon: Afc_state: %d, bAfcTimer=%d, u1AfcTimeCnt=%d\n", pAd->afc_ctrl.AfcStateMachine.CurrState, pAd->afc_ctrl.bAfcTimer, pAd->afc_ctrl.u1AfcTimeCnt);
			pAd->afc_ctrl.bAfcTimer = FALSE;
			pAd->afc_ctrl.u1AfcTimeCnt = 0;
			afc_inf_up_start_inquire_request(pAd);
		}
	}
}

VOID afc_check_pre_cond(
	IN PRTMP_ADAPTER pAd,
	IN struct wifi_dev *wdev)
{
	UINT8 u1Status = TRUE;

	/*After bootup, if Region is FCC, Country code is US
	   and device type is standard power,
	   enter AFC_HOLD state, else AFC_DISABLE
	*/
	if (strncmp((RTMP_STRING *) pAd->CommonCfg.CountryCode, "US", 2)) {
		MTWF_DBG(pAd, DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					"Country Code is %s\n", pAd->CommonCfg.CountryCode);
		u1Status = FALSE;
	}

	if (GetCountryRegionFromCountryCode(pAd->CommonCfg.CountryCode) != FCC) {
		MTWF_DBG(pAd, DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					"CountryRegion is %d\n", GetCountryRegionFromCountryCode(pAd->CommonCfg.CountryCode));
		u1Status = FALSE;
	}

	if (pAd->CommonCfg.AfcDeviceType != AFC_STANDARD_POWER_DEVICE) {
		MTWF_DBG(pAd, DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
					"Device is not standard power\n");
		u1Status = FALSE;
	}

	MTWF_DBG(pAd, DBG_CAT_AP, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
				"afc_check_pre_cond: u1Status = %d\n", u1Status);

	if (u1Status == TRUE)
		afc_set_wdev_to_hold(pAd, wdev);
	else
		afc_update_state_to_disable(pAd);

}

VOID afc_disconnect_all_sta(RTMP_ADAPTER *pAd, UCHAR apidx)
{
	INT i;
	MAC_TABLE_ENTRY *pEntry;

	APMlmeKickOutAllSta(pAd, apidx, REASON_DEAUTH_STA_LEAVING);

	for (i = 0; VALID_UCAST_ENTRY_WCID(pAd, i); i++) {
		pEntry = &pAd->MacTab.Content[i];

		if (IS_ENTRY_CLIENT(pEntry))
			MacTableDeleteEntry(pAd, pEntry->wcid, pEntry->Addr);
	}
}

VOID afc_set_wdev_to_hold(RTMP_ADAPTER *pAd, struct wifi_dev *wdev)
{
	if (IS_AFC_HOLD_STATE(pAd)) {
		MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR,
			"%s:already in HOLD state = %d\n", __func__, pAd->afc_ctrl.AfcStateMachine.CurrState);
		return;
	}

	if (wdev == NULL)
		return;

	afcTxStop(wdev);
	pAd->afc_ctrl.AfcStateMachine.CurrState = AFC_HOLD;
	UpdateBeaconHandler(pAd, wdev, BCN_UPDATE_DISABLE_TX);
	wdev->bAllowBeaconing = FALSE;
	MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_INFO, "%s: Update state to AFC_HOLD\n", __func__);
}

VOID afc_response_fail_update_to_hold(RTMP_ADAPTER *pAd, UINT ifIndex)
{
	struct wifi_dev *wdev = pAd->wdev_list[ifIndex];

	afc_disconnect_all_sta(pAd, ifIndex);
	afc_set_wdev_to_hold(pAd, wdev);
}

VOID afc_update_state_to_disable(RTMP_ADAPTER *pAd)
{
	pAd->afc_ctrl.AfcStateMachine.CurrState = AFC_DISABLE;
	MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, "%s: Update state to AFC_DISABLE\n", __func__);
}

VOID afc_enable_tx(RTMP_ADAPTER *pAd, UINT ifIndex)
{
	UINT16 u2ResponseCode = pAd->afc_response_data.response_code;
	struct wifi_dev *wdev = &pAd->ApCfg.MBSSID[ifIndex].wdev;

	if (wdev == NULL)
		return;

	if (u2ResponseCode == AFC_RESPONSE_TYPE_SUCCESS) {
		pAd->afc_ctrl.AfcStateMachine.CurrState = AFC_RUN;
		wdev->bAllowBeaconing = TRUE;
		afcTxStart(wdev);

		if ((pAd->afc_ctrl.bAfcBeaconInit == FALSE) && (WDEV_WITH_BCN_ABILITY(wdev))) {
			pAd->afc_ctrl.bAfcBeaconInit = TRUE;
			UpdateBeaconHandler(pAd, wdev, BCN_UPDATE_INIT);
		} else if (WDEV_WITH_BCN_ABILITY(wdev))
			UpdateBeaconHandler(pAd, wdev, BCN_UPDATE_ENABLE_TX);

		MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, "%s: Update state to AFC_RUN\n", __func__);
	}
}

VOID afc_inf_up_start_inquire_request(RTMP_ADAPTER *pAd)
{
	MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, "afc_inf_up_start_inquire_request: send request\n");

	RtmpOSWrielessEventSend(pAd->net_dev,
			RT_WLAN_EVENT_CUSTOM,
			AFC_INQ_EVENT,
			NULL,
			(char *)NULL, 0);
}

VOID afc_freq_range_init(RTMP_ADAPTER *pAd)
{
	g_afc_Freq_range_tbl.NumOfOpFreqRange = 0;
}

VOID afc_update_freq_num(RTMP_ADAPTER *pAd, INT NumOfFreqRanges)
{
	g_afc_Freq_range_tbl.NumOfOpFreqRange = NumOfFreqRanges;
	os_alloc_mem(pAd, (PUCHAR *)&g_afc_Freq_range_tbl.freqRange, (sizeof(struct afc_frequency_range) * NumOfFreqRanges));
}

VOID afc_update_freq_range(INT i, INT Index, UINT16 Frequency)
{
	if (Index >=  g_afc_Freq_range_tbl.NumOfOpFreqRange)
		return;

	if (i % 2 == 1)
		g_afc_Freq_range_tbl.freqRange[Index].lowFrequency = Frequency;
	else
		g_afc_Freq_range_tbl.freqRange[Index].highFrequency = Frequency;
}

VOID afc_update_op_class_en(UINT OpClass, UINT8 Enable)
{
	if (OpClass < AFC_OP_CLASS_NUM)
		g_afc_opclass_skip_channel.OPClassEnable[OpClass] = Enable;
}

VOID afc_update_op_class_channel(UINT OpClass, UINT8 Index, UINT8 Channel)
{
	if (OpClass >= AFC_OP_CLASS_NUM)
		return;

	switch (OpClass) {
	case AFC_OP_CLASS_131:
		g_afc_opclass_skip_channel.OPClass131[Index] = Channel;
		break;
	case AFC_OP_CLASS_132:
		g_afc_opclass_skip_channel.OPClass132[Index] = Channel;
		break;
	case AFC_OP_CLASS_133:
		g_afc_opclass_skip_channel.OPClass133[Index] = Channel;
		break;
	case AFC_OP_CLASS_134:
		g_afc_opclass_skip_channel.OPClass134[Index] = Channel;
		break;
	case AFC_OP_CLASS_135:
		g_afc_opclass_skip_channel.OPClass135[Index] = Channel;
		break;
	case AFC_OP_CLASS_136:
		g_afc_opclass_skip_channel.OPClass135[Index] = Channel;
		break;
	default:
		break;
	}
}

VOID afc_update_op_class_channel_count(UINT OpClass, UINT8 Index)
{
	if (OpClass < AFC_OP_CLASS_NUM)
		g_afc_opclass_skip_channel.OPClassNumOfChannel[OpClass] = Index;
}

VOID afc_state_machine_init(
	IN RTMP_ADAPTER *pAd,
	IN STATE_MACHINE * Sm,
	OUT STATE_MACHINE_FUNC Trans[])
{
	StateMachineInit(Sm, (STATE_MACHINE_FUNC *)Trans, AFC_MAX_STATE, AFC_MAX_MSG, (STATE_MACHINE_FUNC)Drop, AFC_IDLE, AFC_MACHINE_BASE);

}

VOID afc_init(
	IN PRTMP_ADAPTER pAd,
	IN struct wifi_dev *wdev)
{
	pAd->afc_ctrl.bAfcTimer = FALSE;
	pAd->afc_ctrl.u1AfcTimeCnt = 0;
	pAd->afc_ctrl.bAfcBeaconInit = FALSE;
	pAd->afc_response_data.response_code = AFC_RESPONSE_INIT_VALUE;
	pAd->CommonCfg.AFCbeaconBW = 0;
	os_zero_mem(&(_afc_switch_chan_params), sizeof(_afc_switch_chan_params));
	afc_state_machine_init(pAd, &pAd->afc_ctrl.AfcStateMachine, pAd->afc_ctrl.AfcFunc);
	MTWF_DBG(pAd, DBG_CAT_CFG, DBG_SUBCAT_ALL, DBG_LVL_ERROR, "%s: AFC Init Settings\n", __func__);
}

#endif /*CONFIG_6G_SUPPORT && */
		/*CONFIG_6G_AFC_SUPPORT && DOT11_HE_AX*/

