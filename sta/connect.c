/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology	5th	Rd.
 * Science-based Industrial	Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2004, Ralink Technology, Inc.
 *
 * All rights reserved.	Ralink's source	code is	an unpublished work	and	the
 * use of a	copyright notice does not imply	otherwise. This	source code
 * contains	confidential trade secret material of Ralink Tech. Any attemp
 * or participation	in deciphering,	decoding, reverse engineering or in	any
 * way altering	the	source code	is stricitly prohibited, unless	the	prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:
	connect.c

	Abstract:

	Revision History:
	Who			When			What
	--------	----------		----------------------------------------------
	John			2004-08-08			Major modification from RT2560
*/
#include "rt_config.h"

u8 CipherSuiteWpaNoneTkip[] = {
	0x00, 0x50, 0xf2, 0x01,	/* oui */
	0x01, 0x00,		/* Version */
	0x00, 0x50, 0xf2, 0x02,	/* Multicast */
	0x01, 0x00,		/* Number of unicast */
	0x00, 0x50, 0xf2, 0x02,	/* unicast */
	0x01, 0x00,		/* number of authentication method */
	0x00, 0x50, 0xf2, 0x00	/* authentication */
};
u8 CipherSuiteWpaNoneTkipLen =
    (sizeof (CipherSuiteWpaNoneTkip) / sizeof (u8));

u8 CipherSuiteWpaNoneAes[] = {
	0x00, 0x50, 0xf2, 0x01,	/* oui */
	0x01, 0x00,		/* Version */
	0x00, 0x50, 0xf2, 0x04,	/* Multicast */
	0x01, 0x00,		/* Number of unicast */
	0x00, 0x50, 0xf2, 0x04,	/* unicast */
	0x01, 0x00,		/* number of authentication method */
	0x00, 0x50, 0xf2, 0x00	/* authentication */
};
u8 CipherSuiteWpaNoneAesLen =
    (sizeof (CipherSuiteWpaNoneAes) / sizeof (u8));

/* The following MACRO is called after 1. starting an new IBSS, 2. succesfully JOIN an IBSS, */
/* or 3. succesfully ASSOCIATE to a BSS, 4. successfully RE_ASSOCIATE to a BSS */
/* All settings successfuly negotiated furing MLME state machines become final settings */
/* and are copied to pAd->StaActive */
#define COPY_SETTINGS_FROM_MLME_AUX_TO_ACTIVE_CFG(_pAd)                                 \
{                                                                                       \
	memset((_pAd)->CommonCfg.Ssid, 0, MAX_LEN_OF_SSID); 							\
	(_pAd)->CommonCfg.SsidLen = (_pAd)->MlmeAux.SsidLen;                                \
	memmove((_pAd)->CommonCfg.Ssid, (_pAd)->MlmeAux.Ssid, (_pAd)->MlmeAux.SsidLen); \
	COPY_MAC_ADDR((_pAd)->CommonCfg.Bssid, (_pAd)->MlmeAux.Bssid);                      \
	(_pAd)->CommonCfg.Channel = (_pAd)->MlmeAux.Channel;                                \
	(_pAd)->CommonCfg.CentralChannel = (_pAd)->MlmeAux.CentralChannel;                  \
	(_pAd)->StaActive.Aid = (_pAd)->MlmeAux.Aid;                                        \
	(_pAd)->StaActive.AtimWin = (_pAd)->MlmeAux.AtimWin;                                \
	(_pAd)->StaActive.CapabilityInfo = (_pAd)->MlmeAux.CapabilityInfo;                  \
	(_pAd)->StaActive.ExtCapInfo = (_pAd)->MlmeAux.ExtCapInfo;                  \
	(_pAd)->CommonCfg.BeaconPeriod = (_pAd)->MlmeAux.BeaconPeriod;                      \
	(_pAd)->StaActive.CfpMaxDuration = (_pAd)->MlmeAux.CfpMaxDuration;                  \
	(_pAd)->StaActive.CfpPeriod = (_pAd)->MlmeAux.CfpPeriod;                            \
	(_pAd)->StaActive.SupRateLen = (_pAd)->MlmeAux.SupRateLen;                          \
	memmove((_pAd)->StaActive.SupRate, (_pAd)->MlmeAux.SupRate, (_pAd)->MlmeAux.SupRateLen);\
	(_pAd)->StaActive.ExtRateLen = (_pAd)->MlmeAux.ExtRateLen;                          \
	memmove((_pAd)->StaActive.ExtRate, (_pAd)->MlmeAux.ExtRate, (_pAd)->MlmeAux.ExtRateLen);\
	memmove(&(_pAd)->CommonCfg.APEdcaParm, &(_pAd)->MlmeAux.APEdcaParm, sizeof(EDCA_PARM));\
	memmove(&(_pAd)->CommonCfg.APQosCapability, &(_pAd)->MlmeAux.APQosCapability, sizeof(QOS_CAPABILITY_PARM));\
	memmove(&(_pAd)->CommonCfg.APQbssLoad, &(_pAd)->MlmeAux.APQbssLoad, sizeof(QBSS_LOAD_PARM));\
	COPY_MAC_ADDR((_pAd)->MacTab.Content[BSSID_WCID].Addr, (_pAd)->MlmeAux.Bssid);      \
	(_pAd)->MacTab.Content[BSSID_WCID].PairwiseKey.CipherAlg = (_pAd)->StaCfg.PairCipher;\
	COPY_MAC_ADDR((_pAd)->MacTab.Content[BSSID_WCID].PairwiseKey.BssId, (_pAd)->MlmeAux.Bssid);\
	(_pAd)->MacTab.Content[BSSID_WCID].RateLen = (_pAd)->StaActive.SupRateLen + (_pAd)->StaActive.ExtRateLen;\
}

/*
	==========================================================================
	Description:

	IRQL = PASSIVE_LEVEL

	==========================================================================
*/
VOID MlmeCntlInit(
	IN struct rtmp_adapter *pAd,
	IN STATE_MACHINE *S,
	OUT STATE_MACHINE_FUNC Trans[])
{
	/* Control state machine differs from other state machines, the interface */
	/* follows the standard interface */
	pAd->Mlme.CntlMachine.CurrState = CNTL_IDLE;
}

/*
	==========================================================================
	Description:

	IRQL = DISPATCH_LEVEL

	==========================================================================
*/
VOID MlmeCntlMachinePerformAction(
	IN struct rtmp_adapter *pAd,
	IN STATE_MACHINE *S,
	IN MLME_QUEUE_ELEM *Elem)
{
	switch (pAd->Mlme.CntlMachine.CurrState) {
	case CNTL_IDLE:
		CntlIdleProc(pAd, Elem);
		break;
	case CNTL_WAIT_DISASSOC:
		CntlWaitDisassocProc(pAd, Elem);
		break;
	case CNTL_WAIT_JOIN:
		CntlWaitJoinProc(pAd, Elem);
		break;

		/* CNTL_WAIT_REASSOC is the only state in CNTL machine that does */
		/* not triggered directly or indirectly by "RTMPSetInformation(OID_xxx)". */
		/* Therefore not protected by NDIS's "only one outstanding OID request" */
		/* rule. Which means NDIS may SET OID in the middle of ROAMing attempts. */
		/* Current approach is to block new SET request at RTMPSetInformation() */
		/* when CntlMachine.CurrState is not CNTL_IDLE */
	case CNTL_WAIT_REASSOC:
		CntlWaitReassocProc(pAd, Elem);
		break;

	case CNTL_WAIT_START:
		CntlWaitStartProc(pAd, Elem);
		break;
	case CNTL_WAIT_AUTH:
		CntlWaitAuthProc(pAd, Elem);
		break;
	case CNTL_WAIT_AUTH2:
		CntlWaitAuthProc2(pAd, Elem);
		break;
	case CNTL_WAIT_ASSOC:
		CntlWaitAssocProc(pAd, Elem);
		break;

	case CNTL_WAIT_OID_LIST_SCAN:
		if (Elem->MsgType == MT2_SCAN_CONF) {
			unsigned short Status = MLME_SUCCESS;

			memmove(&Status, Elem->Msg, sizeof(unsigned short));
			/* Resume TxRing after SCANING complete. We hope the out-of-service time */
			/* won't be too long to let upper layer time-out the waiting frames */
			RTMPResumeMsduTransmission(pAd);

			pAd->Mlme.CntlMachine.CurrState = CNTL_IDLE;

			/* scan completed, init to not FastScan */
			pAd->StaCfg.bImprovedScan = false;

#ifdef RT_CFG80211_SUPPORT
			RT_CFG80211_SCAN_END(pAd, false);
#endif /* RT_CFG80211_SUPPORT */


			/* AP sent a 2040Coexistence mgmt frame, then station perform a scan, and then send back the respone. */
			if ((pAd->CommonCfg.BSSCoexist2040.field.InfoReq == 1)
			    && INFRA_ON(pAd)
			    && OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_SCAN_2040)) {
				Update2040CoexistFrameAndNotify(pAd, BSSID_WCID,
								true);
			}

#ifdef WPA_SUPPLICANT_SUPPORT
			if ( pAd->StaCfg.bAutoReconnect == true &&
				 pAd->IndicateMediaState != NdisMediaStateConnected &&
				(pAd->StaCfg.wpa_supplicant_info.WpaSupplicantUP != WPA_SUPPLICANT_ENABLE_WITH_WEB_UI))
			{
				BssTableSsidSort(pAd, &pAd->MlmeAux.SsidBssTab, (PCHAR)pAd->MlmeAux.Ssid, pAd->MlmeAux.SsidLen);
				pAd->MlmeAux.BssIdx = 0;
				IterateOnBssTab(pAd);
			}
#endif /* WPA_SUPPLICANT_SUPPORT */

				if (Status == MLME_SUCCESS)
				{
					{
						/*
							Maintain Scan Table
							MaxBeaconRxTimeDiff: 120 seconds
							MaxSameBeaconRxTimeCount: 1
						*/
						MaintainBssTable(pAd, &pAd->ScanTab, 120, 2);
					}

				{
					RTMPSendWirelessEvent(pAd, IW_SCAN_COMPLETED_EVENT_FLAG, NULL, BSS0, 0);

#ifdef WPA_SUPPLICANT_SUPPORT
					RtmpOSWrielessEventSend(pAd->net_dev, RT_WLAN_EVENT_SCAN, -1, NULL, NULL, 0);
#endif /* WPA_SUPPLICANT_SUPPORT */
				}

			}
		}
		break;

	case CNTL_WAIT_OID_DISASSOC:
		if (Elem->MsgType == MT2_DISASSOC_CONF) {
			LinkDown(pAd, false);
/*
for android system , if connect ap1 and want to change to ap2 ,
when disassoc from ap1 ,and send even_scan will direct connect to ap2 , not need to wait ui to scan and connect
*/
			pAd->Mlme.CntlMachine.CurrState = CNTL_IDLE;
		}
		break;

	case CNTL_WAIT_SCAN_FOR_CONNECT:
		if (Elem->MsgType == MT2_SCAN_CONF) {
			unsigned short Status = MLME_SUCCESS;
			memmove(&Status, Elem->Msg, sizeof(unsigned short));
			/* Resume TxRing after SCANING complete. We hope the out-of-service time */
			/* won't be too long to let upper layer time-out the waiting frames */
			RTMPResumeMsduTransmission(pAd);
			pAd->Mlme.CntlMachine.CurrState = CNTL_IDLE;
			BssTableSsidSort(pAd, &pAd->MlmeAux.SsidBssTab, (PCHAR)pAd->MlmeAux.Ssid, pAd->MlmeAux.SsidLen);
			pAd->MlmeAux.BssIdx = 0;
			IterateOnBssTab(pAd);
		}
		break;
	default:
		DBGPRINT_ERR(("!ERROR! CNTL - Illegal message type(=%ld)",
			      Elem->MsgType));
		break;
	}
}

/*
	==========================================================================
	Description:

	IRQL = DISPATCH_LEVEL

	==========================================================================
*/
VOID CntlIdleProc(
	IN struct rtmp_adapter *pAd,
	IN MLME_QUEUE_ELEM *Elem)
{
	MLME_DISASSOC_REQ_STRUCT DisassocReq;

	if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_RADIO_OFF))
		return;

	switch (Elem->MsgType) {
	case OID_802_11_SSID:
		CntlOidSsidProc(pAd, Elem);
		break;

	case OID_802_11_BSSID:
		CntlOidRTBssidProc(pAd, Elem);
		break;

	case OID_802_11_BSSID_LIST_SCAN:
		CntlOidScanProc(pAd, Elem);
		break;

	case OID_802_11_DISASSOCIATE:
		DisassocParmFill(pAd, &DisassocReq, pAd->CommonCfg.Bssid,
				 REASON_DISASSOC_STA_LEAVING);
		MlmeEnqueue(pAd, ASSOC_STATE_MACHINE, MT2_MLME_DISASSOC_REQ,
			    sizeof (MLME_DISASSOC_REQ_STRUCT), &DisassocReq, 0);
		pAd->Mlme.CntlMachine.CurrState = CNTL_WAIT_OID_DISASSOC;
#ifdef WPA_SUPPLICANT_SUPPORT
		if ((pAd->StaCfg.wpa_supplicant_info.WpaSupplicantUP & 0x7F) !=
		    WPA_SUPPLICANT_ENABLE_WITH_WEB_UI)
#endif /* WPA_SUPPLICANT_SUPPORT */
		{
			/* Set the AutoReconnectSsid to prevent it reconnect to old SSID */
			/* Since calling this indicate user don't want to connect to that SSID anymore. */
			pAd->MlmeAux.AutoReconnectSsidLen = 32;
			memset(pAd->MlmeAux.AutoReconnectSsid, 0,
				       pAd->MlmeAux.AutoReconnectSsidLen);
		}
		break;

	case MT2_MLME_ROAMING_REQ:
		CntlMlmeRoamingProc(pAd, Elem);
		break;

	case OID_802_11_MIC_FAILURE_REPORT_FRAME:
		WpaMicFailureReportFrame(pAd, Elem);
		break;



	default:
		DBGPRINT(RT_DEBUG_TRACE,
			 ("CNTL - Illegal message in CntlIdleProc(MsgType=%ld)\n",
			  Elem->MsgType));
		break;
	}
}

VOID CntlOidScanProc(
	IN struct rtmp_adapter *pAd,
	IN MLME_QUEUE_ELEM *Elem)
{
	MLME_SCAN_REQ_STRUCT ScanReq;
	ULONG BssIdx = BSS_NOT_FOUND;
/*	BSS_ENTRY                  CurrBss; */
	BSS_ENTRY *pCurrBss = NULL;

	/* allocate memory */
	pCurrBss = kmalloc(sizeof (BSS_ENTRY), GFP_ATOMIC);
	if (pCurrBss == NULL) {
		DBGPRINT(RT_DEBUG_ERROR,
			 ("%s: Allocate memory fail!!!\n", __FUNCTION__));
		return;
	}

	/* record current BSS if network is connected. */
	/* 2003-2-13 do not include current IBSS if this is the only STA in this IBSS. */
	if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED)) {
		BssIdx =
		    BssSsidTableSearch(&pAd->ScanTab, pAd->CommonCfg.Bssid,
				       (u8 *) pAd->CommonCfg.Ssid,
				       pAd->CommonCfg.SsidLen,
				       pAd->CommonCfg.Channel);
		if (BssIdx != BSS_NOT_FOUND) {
			memmove(pCurrBss, &pAd->ScanTab.BssEntry[BssIdx],
				       sizeof (BSS_ENTRY));
		}
	}


	ScanParmFill(pAd, &ScanReq, (char *) Elem->Msg, Elem->MsgLen, BSS_ANY,
		     SCAN_ACTIVE);
	MlmeEnqueue(pAd, SYNC_STATE_MACHINE, MT2_MLME_SCAN_REQ,
		    sizeof (MLME_SCAN_REQ_STRUCT), &ScanReq, 0);
	pAd->Mlme.CntlMachine.CurrState = CNTL_WAIT_OID_LIST_SCAN;

	if (pCurrBss != NULL)
		kfree(pCurrBss);
}

/*
	==========================================================================
	Description:
		Before calling this routine, user desired SSID should already been
		recorded in CommonCfg.Ssid[]
	IRQL = DISPATCH_LEVEL

	==========================================================================
*/
VOID CntlOidSsidProc(
	IN struct rtmp_adapter *pAd,
	IN MLME_QUEUE_ELEM *Elem)
{
	PNDIS_802_11_SSID pOidSsid = (NDIS_802_11_SSID *) Elem->Msg;
	MLME_DISASSOC_REQ_STRUCT DisassocReq;



	/* Step 1. record the desired user settings to MlmeAux */
	memset(pAd->MlmeAux.Ssid, 0, MAX_LEN_OF_SSID);
	memmove(pAd->MlmeAux.Ssid, pOidSsid->Ssid, pOidSsid->SsidLength);
	pAd->MlmeAux.SsidLen = (u8) pOidSsid->SsidLength;
	if (pAd->StaCfg.BssType == BSS_INFRA)
		memset(pAd->MlmeAux.Bssid, 0, MAC_ADDR_LEN);
	pAd->MlmeAux.BssType = pAd->StaCfg.BssType;

	pAd->StaCfg.bAutoConnectByBssid = false;

	/*save connect info*/
	memset(pAd->StaCfg.ConnectinfoSsid, 0, MAX_LEN_OF_SSID);
	memmove(pAd->StaCfg.ConnectinfoSsid, pOidSsid->Ssid, pOidSsid->SsidLength);
	pAd->StaCfg.ConnectinfoSsidLen = pOidSsid->SsidLength;
	pAd->StaCfg.ConnectinfoBssType = pAd->StaCfg.BssType;



	/* Update Reconnect Ssid, that user desired to connect. */
	memset(pAd->MlmeAux.AutoReconnectSsid, 0, MAX_LEN_OF_SSID);
	memmove(pAd->MlmeAux.AutoReconnectSsid, pAd->MlmeAux.Ssid,
		       pAd->MlmeAux.SsidLen);
	pAd->MlmeAux.AutoReconnectSsidLen = pAd->MlmeAux.SsidLen;

	/*
		step 2.
		find all matching BSS in the lastest SCAN result (inBssTab)
		and log them into MlmeAux.SsidBssTab for later-on iteration. Sort by RSSI order
	*/
	BssTableSsidSort(pAd, &pAd->MlmeAux.SsidBssTab,
			 (PCHAR) pAd->MlmeAux.Ssid, pAd->MlmeAux.SsidLen);

	DBGPRINT(RT_DEBUG_TRACE,
		 ("CntlOidSsidProc():CNTL - %d BSS of %d BSS match the desire ",
		  pAd->MlmeAux.SsidBssTab.BssNr, pAd->ScanTab.BssNr));
	if (pAd->MlmeAux.SsidLen == MAX_LEN_OF_SSID)
		hex_dump("\nSSID", pAd->MlmeAux.Ssid, pAd->MlmeAux.SsidLen);
	else
		DBGPRINT(RT_DEBUG_TRACE,
			 ("(%d)SSID - %s\n", pAd->MlmeAux.SsidLen,
			  pAd->MlmeAux.Ssid));

	if (INFRA_ON(pAd) &&
	    OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED) &&
	    (pAd->CommonCfg.SsidLen == pAd->MlmeAux.SsidBssTab.BssEntry[0].SsidLen)
	    && NdisEqualMemory(pAd->CommonCfg.Ssid, pAd->MlmeAux.SsidBssTab.BssEntry[0].Ssid, pAd->CommonCfg.SsidLen)
	    && MAC_ADDR_EQUAL(pAd->CommonCfg.Bssid, pAd->MlmeAux.SsidBssTab.BssEntry[0].Bssid)) {
		/*
			Case 1. already connected with an AP who has the desired SSID
					with highest RSSI
		*/
		struct rtmp_wifi_dev *wdev = &pAd->StaCfg.wdev;

		/* Add checking Mode "LEAP" for CCX 1.0 */
		if (((wdev->AuthMode == Ndis802_11AuthModeWPA) ||
		     (wdev->AuthMode == Ndis802_11AuthModeWPAPSK) ||
		     (wdev->AuthMode == Ndis802_11AuthModeWPA2) ||
		     (wdev->AuthMode == Ndis802_11AuthModeWPA2PSK)
		    ) &&
		    (wdev->PortSecured == WPA_802_1X_PORT_NOT_SECURED)) {
			/*
				case 1.1 For WPA, WPA-PSK,
				if port is not secured, we have to redo connection process
			*/
			DBGPRINT(RT_DEBUG_TRACE,
				 ("CntlOidSsidProc():CNTL - disassociate with current AP...\n"));
			DisassocParmFill(pAd, &DisassocReq,
					 pAd->CommonCfg.Bssid,
					 REASON_DISASSOC_STA_LEAVING);
			MlmeEnqueue(pAd, ASSOC_STATE_MACHINE,
				    MT2_MLME_DISASSOC_REQ,
				    sizeof (MLME_DISASSOC_REQ_STRUCT),
				    &DisassocReq, 0);
			pAd->Mlme.CntlMachine.CurrState = CNTL_WAIT_DISASSOC;
		} else if (pAd->bConfigChanged == true) {
			/* case 1.2 Important Config has changed, we have to reconnect to the same AP */
			DBGPRINT(RT_DEBUG_TRACE,
				 ("CntlOidSsidProc():CNTL - disassociate with current AP Because config changed...\n"));
			DisassocParmFill(pAd, &DisassocReq,
					 pAd->CommonCfg.Bssid,
					 REASON_DISASSOC_STA_LEAVING);
			MlmeEnqueue(pAd, ASSOC_STATE_MACHINE,
				    MT2_MLME_DISASSOC_REQ,
				    sizeof (MLME_DISASSOC_REQ_STRUCT),
				    &DisassocReq, 0);
			pAd->Mlme.CntlMachine.CurrState = CNTL_WAIT_DISASSOC;
		} else {
			/* case 1.3. already connected to the SSID with highest RSSI. */
			DBGPRINT(RT_DEBUG_TRACE,
				 ("CntlOidSsidProc():CNTL - already with this BSSID. ignore this SET_SSID request\n"));
			/*
				(HCT 12.1) 1c_wlan_mediaevents required
				media connect events are indicated when associating with the same AP
			*/
			if (INFRA_ON(pAd)) {
				/*
					Since MediaState already is NdisMediaStateConnected
					We just indicate the connect event again to meet the WHQL required.
				*/
				RTMP_IndicateMediaState(pAd, NdisMediaStateConnected);
				pAd->ExtraInfo = GENERAL_LINK_UP;	/* Update extra information to link is up */
			}

			pAd->Mlme.CntlMachine.CurrState = CNTL_IDLE;
#ifdef NATIVE_WPA_SUPPLICANT_SUPPORT
			RtmpOSWrielessEventSend(pAd->net_dev,
						RT_WLAN_EVENT_CGIWAP, -1,
						&pAd->MlmeAux.Bssid[0], NULL,
						0);
#endif /* NATIVE_WPA_SUPPLICANT_SUPPORT */
		}
	} else if (INFRA_ON(pAd)) {
		/*
			For RT61
			[88888] OID_802_11_SSID should have returned NDTEST_WEP_AP2(Returned: )
			RT61 may lost SSID, and not connect to NDTEST_WEP_AP2 and will connect to NDTEST_WEP_AP2 by Autoreconnect
			But media status is connected, so the SSID not report correctly.
		*/
		if (!SSID_EQUAL(pAd->CommonCfg.Ssid, pAd->CommonCfg.SsidLen, pAd->MlmeAux.Ssid, pAd->MlmeAux.SsidLen)) {
			/* Different SSID means not Roaming case, so we let LinkDown() to Indicate a disconnect event. */
			pAd->MlmeAux.CurrReqIsFromNdis = true;
		}

		/*
			case 2. active INFRA association existent
			Roaming is done within miniport driver, nothing to do with configuration
			utility. so upon a new SET(OID_802_11_SSID) is received, we just
			disassociate with the current associated AP,
			then perform a new association with this new SSID, no matter the
			new/old SSID are the same or not.
		*/
		DBGPRINT(RT_DEBUG_TRACE,
			 ("CntlOidSsidProc():CNTL - disassociate with current AP...\n"));
		DisassocParmFill(pAd, &DisassocReq, pAd->CommonCfg.Bssid,
				 REASON_DISASSOC_STA_LEAVING);
		MlmeEnqueue(pAd, ASSOC_STATE_MACHINE, MT2_MLME_DISASSOC_REQ,
			    sizeof (MLME_DISASSOC_REQ_STRUCT), &DisassocReq, 0);
		pAd->Mlme.CntlMachine.CurrState = CNTL_WAIT_DISASSOC;
	} else {
		if (ADHOC_ON(pAd)) {
			DBGPRINT(RT_DEBUG_TRACE,
				 ("CntlOidSsidProc():CNTL - drop current ADHOC\n"));
			LinkDown(pAd, false);
			OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED);
			RTMP_IndicateMediaState(pAd, NdisMediaStateDisconnected);
			pAd->ExtraInfo = GENERAL_LINK_DOWN;
			DBGPRINT(RT_DEBUG_TRACE,
				 ("CntlOidSsidProc():NDIS_STATUS_MEDIA_DISCONNECT Event C!\n"));
		}

		if ((pAd->MlmeAux.SsidBssTab.BssNr == 0) &&
		    (pAd->StaCfg.bAutoReconnect == true) &&
		    (((pAd->MlmeAux.BssType == BSS_INFRA) && (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS)))
		     || ((pAd->MlmeAux.BssType == BSS_ADHOC) && !pAd->StaCfg.bNotFirstScan))
		    && (MlmeValidateSSID(pAd->MlmeAux.Ssid, pAd->MlmeAux.SsidLen) == true)
		    ) {
			MLME_SCAN_REQ_STRUCT ScanReq;

			if (pAd->MlmeAux.BssType == BSS_ADHOC)
				pAd->StaCfg.bNotFirstScan = true;
			DBGPRINT(RT_DEBUG_TRACE, ("CntlOidSsidProc():CNTL - No matching BSS, start a new scan\n"));

			if((pAd->StaCfg.ConnectinfoChannel != 0) && (pAd->StaCfg.Connectinfoflag == true))
			{
				ScanParmFill(pAd, &ScanReq, (char *) pAd->MlmeAux.Ssid,
					pAd->MlmeAux.SsidLen, BSS_ANY, SCAN_ACTIVE);
				MlmeEnqueue(pAd, SYNC_STATE_MACHINE, MT2_MLME_FORCE_SCAN_REQ,
					sizeof (MLME_SCAN_REQ_STRUCT), &ScanReq, 0);
				pAd->Mlme.CntlMachine.CurrState =	CNTL_WAIT_SCAN_FOR_CONNECT;
			}
			else
			{
				ScanParmFill(pAd, &ScanReq, (char *) pAd->MlmeAux.Ssid,
					     pAd->MlmeAux.SsidLen, BSS_ANY, SCAN_ACTIVE);
				MlmeEnqueue(pAd, SYNC_STATE_MACHINE, MT2_MLME_SCAN_REQ,
					    sizeof (MLME_SCAN_REQ_STRUCT), &ScanReq, 0);
				pAd->Mlme.CntlMachine.CurrState = CNTL_WAIT_OID_LIST_SCAN;
			}

			/* Reset Missed scan number */
			NdisGetSystemUpTime(&pAd->StaCfg.LastScanTime);
			pAd->StaCfg.bNotFirstScan = true;
		} else {

			if ((pAd->CommonCfg.CountryRegion & 0x7f) == REGION_33_BG_BAND)
			{
				BSS_ENTRY *entry;
				// TODO: shiang-6590, check this "SavedPhyMode"!
				entry = &pAd->MlmeAux.SsidBssTab.BssEntry[pAd->MlmeAux.BssIdx];
				if ((entry->Channel == 14) && (pAd->CommonCfg.SavedPhyMode = 0xff)) {
					pAd->CommonCfg.SavedPhyMode = pAd->CommonCfg.PhyMode;
					RTMPSetPhyMode(pAd, WMODE_B);
				} else if (pAd->CommonCfg.SavedPhyMode != 0xff) {
					RTMPSetPhyMode(pAd, pAd->CommonCfg.SavedPhyMode);
					pAd->CommonCfg.SavedPhyMode = 0xFF;
				} else {
					DBGPRINT(RT_DEBUG_ERROR, ("%s():SavedPhyMode=0x%x! Channel=%d\n",
								__FUNCTION__, pAd->CommonCfg.SavedPhyMode, entry->Channel));
				}
			}
			pAd->MlmeAux.BssIdx = 0;
			IterateOnBssTab(pAd);
		}

#ifdef RT_CFG80211_SUPPORT
		if ((pAd->MlmeAux.SsidBssTab.BssNr == 0) && (pAd->MlmeAux.BssType == BSS_INFRA)
		    && (pAd->cfg80211_ctrl.FlgCfg80211Connecting == true))
		{
			DBGPRINT(RT_DEBUG_TRACE, ("CFG80211_MLME: No matching BSS, Report cfg80211_layer SM to Idle --> %ld\n",
								pAd->Mlme.CntlMachine.CurrState));

			pAd->cfg80211_ctrl.Cfg_pending_SsidLen = pAd->MlmeAux.SsidLen;
			memset(pAd->cfg80211_ctrl.Cfg_pending_Ssid, 0, MAX_LEN_OF_SSID + 1);
			memmove(pAd->cfg80211_ctrl.Cfg_pending_Ssid, pAd->MlmeAux.Ssid, pAd->MlmeAux.SsidLen);

			RT_CFG80211_CONN_RESULT_INFORM(pAd, pAd->MlmeAux.Bssid, NULL, 0, NULL, 0, 0);
		}
#endif /* RT_CFG80211_SUPPORT */
	}
}


/*
	==========================================================================
	Description:

	IRQL = DISPATCH_LEVEL

	==========================================================================
*/
VOID CntlOidRTBssidProc(
	IN struct rtmp_adapter *pAd,
	IN MLME_QUEUE_ELEM *Elem)
{
	ULONG BssIdx;
	u8 *pOidBssid = (u8 *) Elem->Msg;
	MLME_DISASSOC_REQ_STRUCT DisassocReq;
	MLME_JOIN_REQ_STRUCT JoinReq;
	BSS_ENTRY *pInBss = NULL;
	struct rtmp_wifi_dev *wdev = &pAd->StaCfg.wdev;

	/* record user desired settings */
	COPY_MAC_ADDR(pAd->MlmeAux.Bssid, pOidBssid);
	pAd->MlmeAux.BssType = pAd->StaCfg.BssType;

	/*save connect info*/
	memset(pAd->StaCfg.ConnectinfoBssid, 0, MAC_ADDR_LEN);
	memmove(pAd->StaCfg.ConnectinfoBssid, pOidBssid, MAC_ADDR_LEN);
	DBGPRINT(RT_DEBUG_TRACE, ("ANDROID IOCTL::SIOCSIWAP %02x:%02x:%02x:%02x:%02x:%02x\n",
				PRINT_MAC(pAd->StaCfg.ConnectinfoBssid)));

	/* find the desired BSS in the latest SCAN result table */
	BssIdx = BssTableSearch(&pAd->ScanTab, pOidBssid, pAd->MlmeAux.Channel);

#ifdef WPA_SUPPLICANT_SUPPORT
	if (pAd->StaCfg.wpa_supplicant_info.WpaSupplicantUP & WPA_SUPPLICANT_ENABLE_WPS) ;
	else
#ifdef NATIVE_WPA_SUPPLICANT_SUPPORT
	if (pAd->StaCfg.wpa_supplicant_info.WpaSupplicantUP & WPA_SUPPLICANT_ENABLE) ;
	else
#endif /* NATIVE_WPA_SUPPLICANT_SUPPORT */
#endif /* WPA_SUPPLICANT_SUPPORT */
	if (
		(BssIdx != BSS_NOT_FOUND))
	{
		pInBss = &pAd->ScanTab.BssEntry[BssIdx];

		/*
			If AP's SSID has been changed, STA cannot connect to this AP.
		*/
		if (SSID_EQUAL(pAd->MlmeAux.Ssid, pAd->MlmeAux.SsidLen, pInBss->Ssid, pInBss->SsidLen) == false)
			BssIdx = BSS_NOT_FOUND;

		if (wdev->AuthMode <= Ndis802_11AuthModeAutoSwitch) {
			if (wdev->WepStatus != pInBss->WepStatus)
				BssIdx = BSS_NOT_FOUND;
		} else {
			/* Check AuthMode and AuthModeAux for matching, in case AP support dual-mode */
			if ((wdev->AuthMode != pInBss->AuthMode) &&
			    (wdev->AuthMode != pInBss->AuthModeAux))
				BssIdx = BSS_NOT_FOUND;
		}
	}


	if (BssIdx == BSS_NOT_FOUND) {
		if (((pAd->StaCfg.BssType == BSS_INFRA) && (!RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS))) ||
		    (pAd->StaCfg.bNotFirstScan == false)) {
			MLME_SCAN_REQ_STRUCT ScanReq;

			DBGPRINT(RT_DEBUG_TRACE,
				 ("CNTL - BSSID not found. reply NDIS_STATUS_NOT_ACCEPTED\n"));
			if (pAd->StaCfg.BssType == BSS_ADHOC)
				pAd->StaCfg.bNotFirstScan = true;

			DBGPRINT(RT_DEBUG_TRACE,
				 ("CNTL - BSSID not found. start a new scan\n"));

			if((pAd->StaCfg.ConnectinfoChannel  != 0)&& (pAd->StaCfg.Connectinfoflag == true))
			{
				DBGPRINT(RT_DEBUG_OFF, ("CntlOidRTBssidProc BSSID %02x:%02x:%02x:%02x:%02x:%02x\n",
					PRINT_MAC(pAd->StaCfg.ConnectinfoBssid)));
				pAd->CommonCfg.Channel = pAd->StaCfg.ConnectinfoChannel;
				MlmeEnqueue(pAd, SYNC_STATE_MACHINE, MT2_MLME_FORCE_JOIN_REQ,
					sizeof (MLME_JOIN_REQ_STRUCT), &JoinReq, 0);
				pAd->Mlme.CntlMachine.CurrState = CNTL_WAIT_JOIN;
			}
			else	{
				ScanParmFill(pAd, &ScanReq, (char *) pAd->MlmeAux.Ssid,
				     pAd->MlmeAux.SsidLen, BSS_ANY, SCAN_ACTIVE);
				MlmeEnqueue(pAd, SYNC_STATE_MACHINE, MT2_MLME_SCAN_REQ,
					    sizeof (MLME_SCAN_REQ_STRUCT), &ScanReq, 0);
				pAd->Mlme.CntlMachine.CurrState = CNTL_WAIT_OID_LIST_SCAN;
			}
			/* Reset Missed scan number */
			NdisGetSystemUpTime(&pAd->StaCfg.LastScanTime);
		}
		else
		{
			MLME_START_REQ_STRUCT StartReq;
			DBGPRINT(RT_DEBUG_TRACE,
				 ("CNTL - BSSID not found. start a new ADHOC (Ssid=%s)...\n",
				  pAd->MlmeAux.Ssid));
			StartParmFill(pAd, &StartReq, (PCHAR)pAd->MlmeAux.Ssid,
				      pAd->MlmeAux.SsidLen);
			MlmeEnqueue(pAd, SYNC_STATE_MACHINE, MT2_MLME_START_REQ,
				    sizeof (MLME_START_REQ_STRUCT), &StartReq, 0);

			pAd->Mlme.CntlMachine.CurrState = CNTL_WAIT_START;
		}
		return;
	}

	pInBss = &pAd->ScanTab.BssEntry[BssIdx];
	/* Update Reconnect Ssid, that user desired to connect. */
	memset(pAd->MlmeAux.AutoReconnectSsid, 0, MAX_LEN_OF_SSID);
	pAd->MlmeAux.AutoReconnectSsidLen = pInBss->SsidLen;
	memmove(pAd->MlmeAux.AutoReconnectSsid, pInBss->Ssid, pInBss->SsidLen);


	/* copy the matched BSS entry from ScanTab to MlmeAux.SsidBssTab. Why? */
	/* Because we need this entry to become the JOIN target in later on SYNC state machine */
	pAd->MlmeAux.BssIdx = 0;
	pAd->MlmeAux.SsidBssTab.BssNr = 1;
	memmove(&pAd->MlmeAux.SsidBssTab.BssEntry[0], pInBss,
		       sizeof (BSS_ENTRY));

	{
		if (INFRA_ON(pAd)) {
			/* disassoc from current AP first */
			DBGPRINT(RT_DEBUG_TRACE,
				 ("CNTL - disassociate with current AP ...\n"));
			DisassocParmFill(pAd, &DisassocReq,
					 pAd->CommonCfg.Bssid,
					 REASON_DISASSOC_STA_LEAVING);
			MlmeEnqueue(pAd, ASSOC_STATE_MACHINE,
				    MT2_MLME_DISASSOC_REQ,
				    sizeof (MLME_DISASSOC_REQ_STRUCT),
				    &DisassocReq, 0);

			pAd->Mlme.CntlMachine.CurrState = CNTL_WAIT_DISASSOC;
		} else {
			if (ADHOC_ON(pAd)) {
				DBGPRINT(RT_DEBUG_TRACE,
					 ("CNTL - drop current ADHOC\n"));
				LinkDown(pAd, false);
				OPSTATUS_CLEAR_FLAG(pAd,
						    fOP_STATUS_MEDIA_STATE_CONNECTED);
				RTMP_IndicateMediaState(pAd,
							NdisMediaStateDisconnected);
				pAd->ExtraInfo = GENERAL_LINK_DOWN;
				DBGPRINT(RT_DEBUG_TRACE,
					 ("NDIS_STATUS_MEDIA_DISCONNECT Event C!\n"));
			}

			pInBss = &pAd->MlmeAux.SsidBssTab.BssEntry[0];

			pAd->StaCfg.PairCipher = wdev->WepStatus;
#ifdef WPA_SUPPLICANT_SUPPORT
			if (pAd->StaCfg.wpa_supplicant_info.WpaSupplicantUP == WPA_SUPPLICANT_DISABLE)
#endif /* WPA_SUPPLICANT_SUPPORT */
			pAd->StaCfg.GroupCipher = wdev->WepStatus;

			/* Check cipher suite, AP must have more secured cipher than station setting */
			/* Set the Pairwise and Group cipher to match the intended AP setting */
			/* We can only connect to AP with less secured cipher setting */
			if ((wdev->AuthMode == Ndis802_11AuthModeWPA)
			    || (wdev->AuthMode == Ndis802_11AuthModeWPAPSK)) {
				pAd->StaCfg.GroupCipher = pInBss->WPA.GroupCipher;

				if (wdev->WepStatus == pInBss->WPA.PairCipher)
					pAd->StaCfg.PairCipher = pInBss->WPA.PairCipher;
				else if (pInBss->WPA.PairCipherAux != Ndis802_11WEPDisabled)
					pAd->StaCfg.PairCipher = pInBss->WPA.PairCipherAux;
				else	/* There is no PairCipher Aux, downgrade our capability to TKIP */
					pAd->StaCfg.PairCipher = Ndis802_11TKIPEnable;
			} else
			    if ((wdev->AuthMode == Ndis802_11AuthModeWPA2)
				|| (wdev->AuthMode == Ndis802_11AuthModeWPA2PSK)) {
				pAd->StaCfg.GroupCipher = pInBss->WPA2.GroupCipher;

				if (wdev->WepStatus == pInBss->WPA2.PairCipher)
					pAd->StaCfg.PairCipher = pInBss->WPA2.PairCipher;
				else if (pInBss->WPA2.PairCipherAux != Ndis802_11WEPDisabled)
					pAd->StaCfg.PairCipher = pInBss->WPA2.PairCipherAux;
				else	/* There is no PairCipher Aux, downgrade our capability to TKIP */
					pAd->StaCfg.PairCipher = Ndis802_11TKIPEnable;

				/* RSN capability */
				pAd->StaCfg.RsnCapability = pInBss->WPA2.RsnCapability;
			}

			/* Set Mix cipher flag */
			pAd->StaCfg.bMixCipher = (pAd->StaCfg.PairCipher == pAd->StaCfg.GroupCipher) ? false : true;

			/* No active association, join the BSS immediately */
			DBGPRINT(RT_DEBUG_TRACE,
				 ("CNTL - joining %02x:%02x:%02x:%02x:%02x:%02x ...\n",
				  PRINT_MAC(pOidBssid)));

			JoinParmFill(pAd, &JoinReq, pAd->MlmeAux.BssIdx);
			MlmeEnqueue(pAd, SYNC_STATE_MACHINE, MT2_MLME_JOIN_REQ,
				    sizeof (MLME_JOIN_REQ_STRUCT), &JoinReq, 0);

			pAd->Mlme.CntlMachine.CurrState = CNTL_WAIT_JOIN;
		}
	}
}

/* Roaming is the only external request triggering CNTL state machine */
/* despite of other "SET OID" operation. All "SET OID" related oerations */
/* happen in sequence, because no other SET OID will be sent to this device */
/* until the the previous SET operation is complete (successful o failed). */
/* So, how do we quarantee this ROAMING request won't corrupt other "SET OID"? */
/* or been corrupted by other "SET OID"? */
/* */
/* IRQL = DISPATCH_LEVEL */
VOID CntlMlmeRoamingProc(
	IN struct rtmp_adapter *pAd,
	IN MLME_QUEUE_ELEM *Elem)
{

	DBGPRINT(RT_DEBUG_TRACE, ("CNTL - Roaming in MlmeAux.RoamTab...\n"));

	{

		/*Let BBP register at 20MHz to do (fast) roaming. */
		mt7612u_bbp_set_bw(pAd, BW_20);

		memmove(&pAd->MlmeAux.SsidBssTab, &pAd->MlmeAux.RoamTab,
			       sizeof (pAd->MlmeAux.RoamTab));
		pAd->MlmeAux.SsidBssTab.BssNr = pAd->MlmeAux.RoamTab.BssNr;

		BssTableSortByRssi(&pAd->MlmeAux.SsidBssTab, false);
		pAd->MlmeAux.BssIdx = 0;
		IterateOnBssTab(pAd);
	}
}


/*
	==========================================================================
	Description:

	IRQL = DISPATCH_LEVEL

	==========================================================================
*/
VOID CntlWaitDisassocProc(
	IN struct rtmp_adapter *pAd,
	IN MLME_QUEUE_ELEM *Elem)
{
	MLME_START_REQ_STRUCT StartReq;

	if (Elem->MsgType == MT2_DISASSOC_CONF) {
		DBGPRINT(RT_DEBUG_TRACE, ("CNTL - Dis-associate successful\n"));

		RTMPSendWirelessEvent(pAd, IW_DISASSOC_EVENT_FLAG, NULL, BSS0,
				      0);

		LinkDown(pAd, false);

		/* case 1. no matching BSS, and user wants ADHOC, so we just start a new one */
		if ((pAd->MlmeAux.SsidBssTab.BssNr == 0)
		    && (pAd->StaCfg.BssType == BSS_ADHOC)) {
			DBGPRINT(RT_DEBUG_TRACE,
				 ("CNTL - No matching BSS, start a new ADHOC (Ssid=%s)...\n",
				  pAd->MlmeAux.Ssid));
			StartParmFill(pAd, &StartReq, (PCHAR) pAd->MlmeAux.Ssid,
				      pAd->MlmeAux.SsidLen);
			MlmeEnqueue(pAd, SYNC_STATE_MACHINE, MT2_MLME_START_REQ,
				    sizeof (MLME_START_REQ_STRUCT), &StartReq,
				    0);
			pAd->Mlme.CntlMachine.CurrState = CNTL_WAIT_START;
		}
		/* case 2. try each matched BSS */
		else {
			/*
			   Some customer would set AP1 & AP2 same SSID, AuthMode & EncrypType but different WPAPSK,
			   therefore we need to try next AP here.
			 */
			/*pAd->MlmeAux.BssIdx = 0;*/
			pAd->MlmeAux.BssIdx++;

				IterateOnBssTab(pAd);
		}
	}
}

/*
	==========================================================================
	Description:

	IRQL = DISPATCH_LEVEL

	==========================================================================
*/
VOID CntlWaitJoinProc(
	IN struct rtmp_adapter *pAd,
	IN MLME_QUEUE_ELEM *Elem)
{
	unsigned short Reason;
	MLME_AUTH_REQ_STRUCT AuthReq;

	if (Elem->MsgType == MT2_JOIN_CONF) {
		memmove(&Reason, Elem->Msg, sizeof (unsigned short));
		if (Reason == MLME_SUCCESS) {
			/* 1. joined an IBSS, we are pretty much done here */
			if (pAd->MlmeAux.BssType == BSS_ADHOC) {
				/* */
				/* 5G bands rules of Japan: */
				/* Ad hoc must be disabled in W53(ch52,56,60,64) channels. */
				/* */

				if ((pAd->CommonCfg.bIEEE80211H == 1) &&
				    RadarChannelCheck(pAd, pAd->CommonCfg.Channel))
				{
					pAd->Mlme.CntlMachine.CurrState = CNTL_IDLE;
					DBGPRINT(RT_DEBUG_TRACE,
						 ("CNTL - Channel=%d, Join adhoc on W53(52,56,60,64) Channels are not accepted\n",
						  pAd->CommonCfg.Channel));
					return;
				}

				LinkUp(pAd, BSS_ADHOC);
				pAd->Mlme.CntlMachine.CurrState = CNTL_IDLE;
				DBGPRINT(RT_DEBUG_TRACE,
					 ("CNTL - join the IBSS = %02x:%02x:%02x:%02x:%02x:%02x ...\n",
					  PRINT_MAC(pAd->CommonCfg.Bssid)));

				RTMP_IndicateMediaState(pAd, NdisMediaStateConnected);
				pAd->ExtraInfo = GENERAL_LINK_UP;

				RTMPSendWirelessEvent(pAd, IW_JOIN_IBSS_FLAG, NULL, BSS0, 0);
			}
			/* 2. joined a new INFRA network, start from authentication */
			else
			{
				{
					/* either Ndis802_11AuthModeShared or Ndis802_11AuthModeAutoSwitch, try shared key first */
					if ((pAd->StaCfg.wdev.AuthMode == Ndis802_11AuthModeShared)
					    || (pAd->StaCfg.wdev.AuthMode == Ndis802_11AuthModeAutoSwitch))
					{
						AuthParmFill(pAd, &AuthReq,
							     pAd->MlmeAux.Bssid,
							     AUTH_MODE_KEY);
					} else {
						AuthParmFill(pAd, &AuthReq,
							     pAd->MlmeAux.Bssid,
							     AUTH_MODE_OPEN);
					}
					MlmeEnqueue(pAd, AUTH_STATE_MACHINE,
						    MT2_MLME_AUTH_REQ,
						    sizeof(MLME_AUTH_REQ_STRUCT),
						    &AuthReq, 0);
				}

				pAd->Mlme.CntlMachine.CurrState = CNTL_WAIT_AUTH;
			}
		}
		else
		{
			/* 3. failed, try next BSS */
			pAd->MlmeAux.BssIdx++;
			IterateOnBssTab(pAd);
		}
	}
}

/*
	==========================================================================
	Description:

	IRQL = DISPATCH_LEVEL

	==========================================================================
*/
VOID CntlWaitStartProc(
	IN struct rtmp_adapter *pAd,
	IN MLME_QUEUE_ELEM *Elem)
{
	unsigned short Result;
	RT_PHY_INFO *rt_phy_info;


	if (Elem->MsgType == MT2_START_CONF) {
		memmove(&Result, Elem->Msg, sizeof (unsigned short));
		if (Result == MLME_SUCCESS) {
			/* */
			/* 5G bands rules of Japan: */
			/* Ad hoc must be disabled in W53(ch52,56,60,64) channels. */
			/* */

			if ((pAd->CommonCfg.bIEEE80211H == 1) &&
			    RadarChannelCheck(pAd, pAd->CommonCfg.Channel)
			    ) {
				pAd->Mlme.CntlMachine.CurrState = CNTL_IDLE;
				DBGPRINT(RT_DEBUG_TRACE,
					 ("CNTL - Channel=%d, Start adhoc on W53(52,56,60,64) Channels are not accepted\n",
					  pAd->CommonCfg.Channel));
				return;
			}

			rt_phy_info = &pAd->StaActive.SupportedPhyInfo;
			memset(&rt_phy_info->MCSSet[0], 0, 16);
			if (WMODE_CAP_N(pAd->CommonCfg.PhyMode)
			    && (pAd->StaCfg.bAdhocN == true)
			    && (!pAd->CommonCfg.HT_DisallowTKIP
				|| !IS_INVALID_HT_SECURITY(pAd->StaCfg.wdev.WepStatus)))
			{
				N_ChannelCheck(pAd);
				SetCommonHT(pAd);
				pAd->MlmeAux.CentralChannel = get_cent_ch_by_htinfo(pAd,
												&pAd->CommonCfg.AddHTInfo,
												&pAd->CommonCfg.HtCapability);
				memmove(&pAd->MlmeAux.AddHtInfo,
					       &pAd->CommonCfg.AddHTInfo,
					       sizeof (ADD_HT_INFO_IE));
				RTMPCheckHt(pAd, BSSID_WCID,
					    &pAd->CommonCfg.HtCapability,
					    &pAd->CommonCfg.AddHTInfo);
				rt_phy_info->bHtEnable = true;
				memmove(&rt_phy_info->MCSSet[0],
					       &pAd->CommonCfg.HtCapability.MCSSet[0], 16);
				COPY_HTSETTINGS_FROM_MLME_AUX_TO_ACTIVE_CFG(pAd);
				if (WMODE_CAP_AC(pAd->CommonCfg.PhyMode) &&
					pAd->MlmeAux.vht_cap_len) {
					RT_VHT_CAP *rt_vht_cap = &pAd->StaActive.SupVhtCap;

					COPY_VHT_FROM_MLME_AUX_TO_ACTIVE_CFG(pAd);
					rt_vht_cap->vht_bw = BW_80;
					rt_vht_cap->sgi_80m = pAd->MlmeAux.vht_cap.vht_cap.sgi_80M;
					rt_vht_cap->vht_txstbc = pAd->MlmeAux.vht_cap.vht_cap.tx_stbc;
					rt_vht_cap->vht_rxstbc = pAd->MlmeAux.vht_cap.vht_cap.rx_stbc;
					rt_vht_cap->vht_htc = pAd->MlmeAux.vht_cap.vht_cap.htc_vht_cap;
				}
			}
			else
			{
				pAd->StaActive.SupportedPhyInfo.bHtEnable = false;
			}
			pAd->StaCfg.bAdhocCreator = true;
			LinkUp(pAd, BSS_ADHOC);
			pAd->Mlme.CntlMachine.CurrState = CNTL_IDLE;
			/* Before send beacon, driver need do radar detection */

			if ((pAd->CommonCfg.Channel > 14)
			    && (pAd->CommonCfg.bIEEE80211H == 1)
			    && RadarChannelCheck(pAd, pAd->CommonCfg.Channel)) {
				pAd->Dot11_H.RDMode = RD_SILENCE_MODE;
				pAd->Dot11_H.RDCount = 0;
			}

			DBGPRINT(RT_DEBUG_TRACE,
				 ("CNTL - start a new IBSS = %02x:%02x:%02x:%02x:%02x:%02x ...\n",
				  PRINT_MAC(pAd->CommonCfg.Bssid)));

			RTMPSendWirelessEvent(pAd, IW_START_IBSS_FLAG, NULL, BSS0, 0);
		} else {
			DBGPRINT(RT_DEBUG_TRACE, ("CNTL - Start IBSS fail. BUG!!!!!\n"));
			pAd->Mlme.CntlMachine.CurrState = CNTL_IDLE;
		}
	}
}

/*
	==========================================================================
	Description:

	IRQL = DISPATCH_LEVEL

	==========================================================================
*/
VOID CntlWaitAuthProc(
	IN struct rtmp_adapter *pAd,
	IN MLME_QUEUE_ELEM *Elem)
{
	unsigned short Reason;
	MLME_ASSOC_REQ_STRUCT AssocReq;
	MLME_AUTH_REQ_STRUCT AuthReq;

	if (Elem->MsgType == MT2_AUTH_CONF) {
		memmove(&Reason, Elem->Msg, sizeof (unsigned short));
		if (Reason == MLME_SUCCESS) {
			DBGPRINT(RT_DEBUG_TRACE, ("CNTL - AUTH OK\n"));
			AssocParmFill(pAd, &AssocReq, pAd->MlmeAux.Bssid,
				      pAd->MlmeAux.CapabilityInfo,
				      ASSOC_TIMEOUT,
				      pAd->StaCfg.DefaultListenCount);

			{
				MlmeEnqueue(pAd, ASSOC_STATE_MACHINE,
					    MT2_MLME_ASSOC_REQ,
					    sizeof (MLME_ASSOC_REQ_STRUCT),
					    &AssocReq, 0);

				pAd->Mlme.CntlMachine.CurrState = CNTL_WAIT_ASSOC;
			}
		} else {
			/* This fail may because of the AP already keep us in its MAC table without */
			/* ageing-out. The previous authentication attempt must have let it remove us. */
			/* so try Authentication again may help. For D-Link DWL-900AP+ compatibility. */
			DBGPRINT(RT_DEBUG_TRACE,
				 ("CNTL - AUTH FAIL, try again...\n"));
			{
				if ((pAd->StaCfg.wdev.AuthMode == Ndis802_11AuthModeShared)
				    || (pAd->StaCfg.wdev.AuthMode == Ndis802_11AuthModeAutoSwitch))
				{
					/* either Ndis802_11AuthModeShared or Ndis802_11AuthModeAutoSwitch, try shared key first */
					AuthParmFill(pAd, &AuthReq,
						     pAd->MlmeAux.Bssid,
						     AUTH_MODE_KEY);
				} else {
					AuthParmFill(pAd, &AuthReq,
						     pAd->MlmeAux.Bssid,
						     AUTH_MODE_OPEN);
				}
				MlmeEnqueue(pAd, AUTH_STATE_MACHINE,
					    MT2_MLME_AUTH_REQ,
					    sizeof (MLME_AUTH_REQ_STRUCT),
					    &AuthReq, 0);

			}
			pAd->Mlme.CntlMachine.CurrState = CNTL_WAIT_AUTH2;
		}
	}
}

/*
	==========================================================================
	Description:

	IRQL = DISPATCH_LEVEL

	==========================================================================
*/
VOID CntlWaitAuthProc2(
	IN struct rtmp_adapter *pAd,
	IN MLME_QUEUE_ELEM *Elem)
{
	unsigned short Reason;
	MLME_ASSOC_REQ_STRUCT AssocReq;
	MLME_AUTH_REQ_STRUCT AuthReq;

	if (Elem->MsgType == MT2_AUTH_CONF) {
		memmove(&Reason, Elem->Msg, sizeof (unsigned short));
		if (Reason == MLME_SUCCESS) {
			DBGPRINT(RT_DEBUG_TRACE, ("CNTL - AUTH OK\n"));
			AssocParmFill(pAd, &AssocReq, pAd->MlmeAux.Bssid,
				      pAd->MlmeAux.CapabilityInfo,
				      ASSOC_TIMEOUT,
				      pAd->StaCfg.DefaultListenCount);
			{
				MlmeEnqueue(pAd, ASSOC_STATE_MACHINE,
					    MT2_MLME_ASSOC_REQ,
					    sizeof (MLME_ASSOC_REQ_STRUCT),
					    &AssocReq, 0);

				pAd->Mlme.CntlMachine.CurrState = CNTL_WAIT_ASSOC;
			}
		}
		else
		{
			if ((pAd->StaCfg.wdev.AuthMode == Ndis802_11AuthModeAutoSwitch)
				    && (pAd->MlmeAux.Alg == Ndis802_11AuthModeShared)) {
				DBGPRINT(RT_DEBUG_TRACE,
					 ("CNTL - AUTH FAIL, try OPEN system...\n"));
				AuthParmFill(pAd, &AuthReq, pAd->MlmeAux.Bssid,
					     Ndis802_11AuthModeOpen);
				MlmeEnqueue(pAd, AUTH_STATE_MACHINE,
					    MT2_MLME_AUTH_REQ,
					    sizeof (MLME_AUTH_REQ_STRUCT),
					    &AuthReq, 0);

				pAd->Mlme.CntlMachine.CurrState = CNTL_WAIT_AUTH2;
			} else {
				/* not success, try next BSS */
				DBGPRINT(RT_DEBUG_TRACE,
					 ("CNTL - AUTH FAIL, give up; try next BSS\n"));
#ifdef RT_CFG80211_SUPPORT
                RT_CFG80211_CONN_RESULT_INFORM(pAd, pAd->MlmeAux.Bssid, NULL, 0,
                                                            NULL, 0, 0);
#endif /* RT_CFG80211_SUPPORT */
				RTMP_STA_ENTRY_MAC_RESET(pAd, BSSID_WCID);
				pAd->Mlme.CntlMachine.CurrState = CNTL_IDLE;
				pAd->MlmeAux.BssIdx++;

				IterateOnBssTab(pAd);
			}
		}
	}
}

/*
	==========================================================================
	Description:

	IRQL = DISPATCH_LEVEL

	==========================================================================
*/
VOID CntlWaitAssocProc(
	IN struct rtmp_adapter *pAd,
	IN MLME_QUEUE_ELEM *Elem)
{
	unsigned short Reason;

	if (Elem->MsgType == MT2_ASSOC_CONF) {
		memmove(&Reason, Elem->Msg, sizeof (unsigned short));
		if (Reason == MLME_SUCCESS) {
			RTMPSendWirelessEvent(pAd, IW_ASSOC_EVENT_FLAG, NULL, BSS0, 0);

			LinkUp(pAd, BSS_INFRA);
			pAd->Mlme.CntlMachine.CurrState = CNTL_IDLE;
			DBGPRINT(RT_DEBUG_TRACE,
				 ("CNTL - Association successful on BSS #%ld\n", pAd->MlmeAux.BssIdx));
		} else {
			/* not success, try next BSS */
			DBGPRINT(RT_DEBUG_TRACE,
				 ("CNTL - Association fails on BSS #%ld\n", pAd->MlmeAux.BssIdx));
#ifdef RT_CFG80211_SUPPORT
            RT_CFG80211_CONN_RESULT_INFORM(pAd, pAd->MlmeAux.Bssid, NULL, 0,
            	                                  NULL, 0, 0);
#endif /* RT_CFG80211_SUPPORT */
			RTMP_STA_ENTRY_MAC_RESET(pAd, BSSID_WCID);
			pAd->MlmeAux.BssIdx++;
			IterateOnBssTab(pAd);
		}
	}
}

/*
	==========================================================================
	Description:

	IRQL = DISPATCH_LEVEL

	==========================================================================
*/
VOID CntlWaitReassocProc(
	IN struct rtmp_adapter *pAd,
	IN MLME_QUEUE_ELEM *Elem)
{
	unsigned short Result;

	if (Elem->MsgType == MT2_REASSOC_CONF) {
		memmove(&Result, Elem->Msg, sizeof (unsigned short));
		if (Result == MLME_SUCCESS) {
			/* send wireless event - for association */
			RTMPSendWirelessEvent(pAd, IW_ASSOC_EVENT_FLAG, NULL,
					      BSS0, 0);


			/* NDIS requires a new Link UP indication but no Link Down for RE-ASSOC */
			LinkUp(pAd, BSS_INFRA);

			pAd->Mlme.CntlMachine.CurrState = CNTL_IDLE;
			DBGPRINT(RT_DEBUG_TRACE,
				 ("CNTL - Re-assocition successful on BSS #%ld\n",
				  pAd->MlmeAux.RoamIdx));
		} else {
			/* reassoc failed, try to pick next BSS in the BSS Table */
			DBGPRINT(RT_DEBUG_TRACE,
				 ("CNTL - Re-assocition fails on BSS #%ld\n",
				  pAd->MlmeAux.RoamIdx));
			{
				pAd->MlmeAux.RoamIdx++;
				IterateOnBssTab2(pAd);
			}
		}
	}
}

VOID AdhocTurnOnQos(struct rtmp_adapter *pAd)
{
#define AC0_DEF_TXOP		0
#define AC1_DEF_TXOP		0
#define AC2_DEF_TXOP		94
#define AC3_DEF_TXOP		47

	/* Turn on QOs if use HT rate. */
	if (pAd->CommonCfg.APEdcaParm.bValid == false) {
		pAd->CommonCfg.APEdcaParm.bValid = true;
		pAd->CommonCfg.APEdcaParm.Aifsn[0] = 3;
		pAd->CommonCfg.APEdcaParm.Aifsn[1] = 7;
		pAd->CommonCfg.APEdcaParm.Aifsn[2] = 1;
		pAd->CommonCfg.APEdcaParm.Aifsn[3] = 1;

		pAd->CommonCfg.APEdcaParm.Cwmin[0] = 4;
		pAd->CommonCfg.APEdcaParm.Cwmin[1] = 4;
		pAd->CommonCfg.APEdcaParm.Cwmin[2] = 3;
		pAd->CommonCfg.APEdcaParm.Cwmin[3] = 2;

		pAd->CommonCfg.APEdcaParm.Cwmax[0] = 10;
		pAd->CommonCfg.APEdcaParm.Cwmax[1] = 6;
		pAd->CommonCfg.APEdcaParm.Cwmax[2] = 4;
		pAd->CommonCfg.APEdcaParm.Cwmax[3] = 3;

		pAd->CommonCfg.APEdcaParm.Txop[0] = 0;
		pAd->CommonCfg.APEdcaParm.Txop[1] = 0;
		pAd->CommonCfg.APEdcaParm.Txop[2] = AC2_DEF_TXOP;
		pAd->CommonCfg.APEdcaParm.Txop[3] = AC3_DEF_TXOP;
	}
	AsicSetEdcaParm(pAd, &pAd->CommonCfg.APEdcaParm);
}

/*
	==========================================================================
	Description:

	IRQL = DISPATCH_LEVEL

	==========================================================================
*/
VOID LinkUp(struct rtmp_adapter *pAd, u8 BssType)
{
	ULONG Now;
	uint32_t Data;
	bool Cancelled;
	u8 idx = 0;
	MAC_TABLE_ENTRY *pEntry = NULL;
	uint32_t pbf_reg = 0, pbf_val, burst_txop;
	struct rtmp_wifi_dev *wdev = &pAd->StaCfg.wdev;

	/* Init ChannelQuality to prevent DEAD_CQI at initial LinkUp */
	pAd->Mlme.ChannelQuality = 50;

	/* init to not doing improved scan */
	pAd->StaCfg.bImprovedScan = false;
	pAd->StaCfg.bNotFirstScan = true;
	pAd->StaCfg.bAutoConnectByBssid = false;

	/* Within 10 seconds after link up, don't allow to go to sleep. */
	pAd->CountDowntoPsm = STAY_10_SECONDS_AWAKE;


	pEntry = &pAd->MacTab.Content[BSSID_WCID];

	/*
		ASSOC - DisassocTimeoutAction
		CNTL - Dis-associate successful
		!!! LINK DOWN !!!
		[88888] OID_802_11_SSID should have returned NDTEST_WEP_AP2(Returned: )
	*/

	/*
		To prevent DisassocTimeoutAction to call Link down after we link up,
		cancel the DisassocTimer no matter what it start or not.
	*/
	RTMPCancelTimer(&pAd->MlmeAux.DisassocTimer, &Cancelled);

	COPY_SETTINGS_FROM_MLME_AUX_TO_ACTIVE_CFG(pAd);

	COPY_HTSETTINGS_FROM_MLME_AUX_TO_ACTIVE_CFG(pAd);

	if (BssType == BSS_ADHOC) {
		OPSTATUS_SET_FLAG(pAd, fOP_STATUS_ADHOC_ON);
		OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_INFRA_ON);


		if (WMODE_CAP_N(pAd->CommonCfg.PhyMode)
		    && (pAd->StaCfg.bAdhocN == true))
			AdhocTurnOnQos(pAd);

		InitChannelRelatedValue(pAd);
	} else {
		OPSTATUS_SET_FLAG(pAd, fOP_STATUS_INFRA_ON);
		OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_ADHOC_ON);
		OPSTATUS_SET_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED);
	}
	DBGPRINT(RT_DEBUG_TRACE, ("!!!%s LINK UP !!! \n", (BssType == BSS_ADHOC ? "ADHOC" : "Infra")));

	DBGPRINT(RT_DEBUG_TRACE,
		 ("!!! LINK UP !!! (BssType=%d, AID=%d, ssid=%s, Channel=%d, CentralChannel = %d)\n",
		  BssType, pAd->StaActive.Aid, pAd->CommonCfg.Ssid,
		  pAd->CommonCfg.Channel, pAd->CommonCfg.CentralChannel));

	DBGPRINT(RT_DEBUG_TRACE, ("!!! LINK UP !!! (Density =%d, )\n", pAd->MacTab.Content[BSSID_WCID].MpduDensity));

	/*
		We cannot move AsicSetBssid to PeerBeaconAtJoinAction because
		PeerBeaconAtJoinAction wouldn't be invoked in roaming case.
	*/
		AsicSetBssid(pAd, pAd->CommonCfg.Bssid);


	AsicSetSlotTime(pAd, true);
	AsicSetEdcaParm(pAd, &pAd->CommonCfg.APEdcaParm);


	/*
		Call this for RTS protectionfor legacy rate, we will always enable RTS threshold,
		but normally it will not hit
	*/
	AsicUpdateProtect(pAd, 0, (OFDMSETPROTECT | CCKSETPROTECT), true, false);

	if ((pAd->StaActive.SupportedPhyInfo.bHtEnable == true)) {
		/* Update HT protectionfor based on AP's operating mode. */
		if (pAd->MlmeAux.AddHtInfo.AddHtInfo2.NonGfPresent == 1) {
			AsicUpdateProtect(pAd,
					  pAd->MlmeAux.AddHtInfo.AddHtInfo2.OperaionMode, ALLN_SETPROTECT, false,
					  true);
		} else
			AsicUpdateProtect(pAd,
					  pAd->MlmeAux.AddHtInfo.AddHtInfo2.OperaionMode, ALLN_SETPROTECT, false,
					  false);
	}

	NdisGetSystemUpTime(&Now);
	pAd->StaCfg.LastBeaconRxTime = Now;	/* last RX timestamp */

	if ((pAd->CommonCfg.TxPreamble != Rt802_11PreambleLong) &&
	    CAP_IS_SHORT_PREAMBLE_ON(pAd->StaActive.CapabilityInfo)) {
		MlmeSetTxPreamble(pAd, Rt802_11PreambleShort);
	}

	OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_AGGREGATION_INUSED);

	pAd->Dot11_H.RDMode = RD_NORMAL_MODE;

	if (wdev->WepStatus <= Ndis802_11WEPDisabled)
	{
#ifdef WPA_SUPPLICANT_SUPPORT
		if (pAd->StaCfg.wpa_supplicant_info.WpaSupplicantUP &&
		    (wdev->WepStatus == Ndis802_11WEPEnabled) &&
		    (wdev->IEEE8021X == true))
		    ;
		else
#endif /* WPA_SUPPLICANT_SUPPORT */
		{
			wdev->PortSecured = WPA_802_1X_PORT_SECURED;
			pAd->StaCfg.PrivacyFilter = Ndis802_11PrivFilterAcceptAll;
		}
	}

	if (BssType == BSS_ADHOC) {
		MakeIbssBeacon(pAd);

		if ((pAd->CommonCfg.Channel > 14)
		    && (pAd->CommonCfg.bIEEE80211H == 1)
		    && RadarChannelCheck(pAd, pAd->CommonCfg.Channel)) {
			;	/*Do nothing */
		}
		else
		{
			AsicEnableIbssSync(pAd);
		}

		/* In ad hoc mode, use MAC table from index 1. */
		/* p.s ASIC use all 0xff as termination of WCID table search.To prevent it's 0xff-ff-ff-ff-ff-ff, Write 0 here. */
		AsicDelWcidTab(pAd, MCAST_WCID);
		AsicDelWcidTab(pAd, 1);

		/* If WEP is enabled, add key material and cipherAlg into Asic */
		/* Fill in Shared Key Table(offset: 0x6c00) and Shared Key Mode(offset: 0x7000) */

		if (wdev->WepStatus == Ndis802_11WEPEnabled) {
			u8 CipherAlg;

			for (idx = 0; idx < SHARE_KEY_NUM; idx++) {
				CipherAlg = pAd->SharedKey[BSS0][idx].CipherAlg;

				if (pAd->SharedKey[BSS0][idx].KeyLen > 0) {
					/* Set key material and cipherAlg to Asic */
					AsicAddSharedKeyEntry(pAd, BSS0, idx,
							      &pAd->SharedKey[BSS0][idx]);

					if (idx == wdev->DefaultKeyId) {
						INT cnt;

						/* Generate 3-bytes IV randomly for software encryption using */
						for (cnt = 0; cnt < LEN_WEP_TSC; cnt++)
							pAd->SharedKey[BSS0][idx].TxTsc[cnt] = RandomByte(pAd);

						/* Update WCID attribute table and IVEIV table for this group key table */
						RTMPSetWcidSecurityInfo(pAd,
									BSS0,
									idx,
									CipherAlg,
									MCAST_WCID,
									SHAREDKEYTABLE);
					}
				}
			}
		}
		/* If WPANone is enabled, add key material and cipherAlg into Asic */
		/* Fill in Shared Key Table(offset: 0x6c00) and Shared Key Mode(offset: 0x7000) */
		else if (wdev->AuthMode == Ndis802_11AuthModeWPANone) {
			wdev->DefaultKeyId = 0;	/* always be zero */

			memset(&pAd->SharedKey[BSS0][0], 0, sizeof (CIPHER_KEY));
			pAd->SharedKey[BSS0][0].KeyLen = LEN_TK;
			memmove(pAd->SharedKey[BSS0][0].Key, pAd->StaCfg.PMK, LEN_TK);

			if (pAd->StaCfg.PairCipher == Ndis802_11TKIPEnable) {
				memmove(pAd->SharedKey[BSS0][0].RxMic,
					       &pAd->StaCfg.PMK[16], LEN_TKIP_MIC);
				memmove(pAd->SharedKey[BSS0][0].TxMic,
					       &pAd->StaCfg.PMK[16], LEN_TKIP_MIC);
			}

			/* Decide its ChiperAlg */
			if (pAd->StaCfg.PairCipher == Ndis802_11TKIPEnable)
				pAd->SharedKey[BSS0][0].CipherAlg = CIPHER_TKIP;
			else if (pAd->StaCfg.PairCipher == Ndis802_11AESEnable)
				pAd->SharedKey[BSS0][0].CipherAlg = CIPHER_AES;
			else {
				DBGPRINT(RT_DEBUG_TRACE,
					 ("Unknow Cipher (=%d), set Cipher to AES\n",
					  pAd->StaCfg.PairCipher));
				pAd->SharedKey[BSS0][0].CipherAlg = CIPHER_AES;
			}

			/* Set key material and cipherAlg to Asic */
			AsicAddSharedKeyEntry(pAd,
					      BSS0, 0, &pAd->SharedKey[BSS0][0]);

			/* Update WCID attribute table and IVEIV table for this group key table */
			RTMPSetWcidSecurityInfo(pAd,
						BSS0, 0,
						pAd->SharedKey[BSS0][0].CipherAlg, MCAST_WCID,
						SHAREDKEYTABLE);
		}
	}
	else
	{		/* BSS_INFRA */
		/* Check the new SSID with last SSID */
		while (Cancelled == true) {
			if (pAd->CommonCfg.LastSsidLen == pAd->CommonCfg.SsidLen) {
				if (RTMPCompareMemory(pAd->CommonCfg.LastSsid, pAd->CommonCfg.Ssid, pAd->CommonCfg.LastSsidLen) == 0) {
					/* Link to the old one no linkdown is required. */
					break;
				}
			}
			/* Send link down event before set to link up */
			RTMP_IndicateMediaState(pAd, NdisMediaStateDisconnected);
			pAd->ExtraInfo = GENERAL_LINK_DOWN;
			DBGPRINT(RT_DEBUG_TRACE, ("NDIS_STATUS_MEDIA_DISCONNECT Event AA!\n"));
			break;
		}

		/*
			On WPA mode, Remove All Keys if not connect to the last BSSID
			Key will be set after 4-way handshake
		*/
		if (wdev->AuthMode >= Ndis802_11AuthModeWPA) {
			/* Remove all WPA keys */
/*
 		 for dhcp,issue ,wpa_supplicant ioctl too fast , at link_up, it will add key before driver remove key
		 move to assoc.c
 */
/*			RTMPWPARemoveAllKeys(pAd);*/
			wdev->PortSecured = WPA_802_1X_PORT_NOT_SECURED;
			pAd->StaCfg.PrivacyFilter = Ndis802_11PrivFilter8021xWEP;


#ifdef SOFT_ENCRYPT
			/* There are some situation to need to encryption by software
			   1. The Client support PMF. It shall ony support AES cipher.
			   2. The Client support WAPI.
			   If use RT3883 or later, HW can handle the above.
			 */


#endif /* SOFT_ENCRYPT */

		}

		/* NOTE: */
		/* the decision of using "short slot time" or not may change dynamically due to */
		/* new STA association to the AP. so we have to decide that upon parsing BEACON, not here */

		/* NOTE: */
		/* the decision to use "RTC/CTS" or "CTS-to-self" protection or not may change dynamically */
		/* due to new STA association to the AP. so we have to decide that upon parsing BEACON, not here */

		ComposePsPoll(pAd);
		ComposeNullFrame(pAd);

#ifdef RT_CFG80211_P2P_CONCURRENT_DEVICE
		if (CFG_P2PGO_ON(pAd))
			AsicEnableApBssSync(pAd);
		else
#endif /* RT_CFG80211_P2P_CONCURRENT_DEVICE */
			AsicEnableBssSync(pAd);

		/*
			Add BSSID to WCID search table
			We cannot move this to PeerBeaconAtJoinAction because PeerBeaconAtJoinAction wouldn't be invoked in roaming case.
		*/
		AsicUpdateRxWCIDTable(pAd, BSSID_WCID, pAd->CommonCfg.Bssid);
		COPY_MAC_ADDR(wdev->bssid, pAd->CommonCfg.Bssid);

		/* If WEP is enabled, add paiewise and shared key */
#ifdef WPA_SUPPLICANT_SUPPORT
		if (((pAd->StaCfg.wpa_supplicant_info.WpaSupplicantUP) &&
		     (wdev->WepStatus == Ndis802_11WEPEnabled) &&
		     (wdev->PortSecured == WPA_802_1X_PORT_SECURED)) ||
		    ((pAd->StaCfg.wpa_supplicant_info.WpaSupplicantUP == WPA_SUPPLICANT_DISABLE) &&
		     (wdev->WepStatus == Ndis802_11WEPEnabled)))
#else
		if (wdev->WepStatus == Ndis802_11WEPEnabled)
#endif /* WPA_SUPPLICANT_SUPPORT */
		{
			u8 CipherAlg;

			for (idx = 0; idx < SHARE_KEY_NUM; idx++) {
				CipherAlg = pAd->SharedKey[BSS0][idx].CipherAlg;

				if (pAd->SharedKey[BSS0][idx].KeyLen > 0) {
					/* Set key material and cipherAlg to Asic */
					AsicAddSharedKeyEntry(pAd, BSS0, idx,
							      &pAd->SharedKey[BSS0][idx]);

					if (idx == wdev->DefaultKeyId) {
						/* STA doesn't need to set WCID attribute for group key */

						/* Assign pairwise key info to Asic */
						pEntry->Aid = BSSID_WCID;
						pEntry->wcid = BSSID_WCID;
						RTMPSetWcidSecurityInfo(pAd,
									BSS0,
									idx,
									CipherAlg,
									pEntry->wcid,
									SHAREDKEYTABLE);
					}
				}
			}
		}
		/* For GUI ++ */
		if (wdev->AuthMode < Ndis802_11AuthModeWPA)
		{
#ifdef WPA_SUPPLICANT_SUPPORT
			if (((pAd->StaCfg.wpa_supplicant_info.WpaSupplicantUP) && (wdev->WepStatus == Ndis802_11WEPEnabled) && (wdev->PortSecured == WPA_802_1X_PORT_SECURED))
				|| ((pAd->StaCfg.wpa_supplicant_info.WpaSupplicantUP == WPA_SUPPLICANT_DISABLE) && (wdev->WepStatus == Ndis802_11WEPEnabled))
				|| (wdev->WepStatus == Ndis802_11WEPDisabled))
#endif /* WPA_SUPPLICANT_SUPPORT */
			{
				pAd->ExtraInfo = GENERAL_LINK_UP;
				RTMP_IndicateMediaState(pAd, NdisMediaStateConnected);
			}
		} else if ((wdev->AuthMode == Ndis802_11AuthModeWPAPSK) ||
			   (wdev->AuthMode == Ndis802_11AuthModeWPA2PSK))
		{
#ifdef WPA_SUPPLICANT_SUPPORT
			if (pAd->StaCfg.wpa_supplicant_info.WpaSupplicantUP == WPA_SUPPLICANT_DISABLE)
#endif /* WPA_SUPPLICANT_SUPPORT */
				RTMPSetTimer(&pAd->Mlme.LinkDownTimer, LINK_DOWN_TIMEOUT);

		}



		DBGPRINT(RT_DEBUG_TRACE, ("!!! LINK UP !!!  ClientStatusFlags=%lx)\n",
			  pAd->MacTab.Content[BSSID_WCID].ClientStatusFlags));



		MlmeUpdateTxRates(pAd, true, BSS0);
		MlmeUpdateHtTxRates(pAd, BSS0);
		DBGPRINT(RT_DEBUG_TRACE, ("!!! LINK UP !! (StaActive.bHtEnable =%d)\n",
			  pAd->StaActive.SupportedPhyInfo.bHtEnable));

		if (pAd->CommonCfg.bAggregationCapable) {
			if ((pAd->CommonCfg.bPiggyBackCapable)
			    && (pAd->MlmeAux.APRalinkIe & 0x00000003) == 3) {
				OPSTATUS_SET_FLAG(pAd, fOP_STATUS_PIGGYBACK_INUSED);
				OPSTATUS_SET_FLAG(pAd, fOP_STATUS_AGGREGATION_INUSED);
				CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_AGGREGATION_CAPABLE);
				CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_PIGGYBACK_CAPABLE);
				RTMPSetPiggyBack(pAd, true);
				DBGPRINT(RT_DEBUG_TRACE, ("Turn on Piggy-Back\n"));
			} else if (pAd->MlmeAux.APRalinkIe & 0x00000001) {
				OPSTATUS_SET_FLAG(pAd, fOP_STATUS_AGGREGATION_INUSED);
				CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_AGGREGATION_CAPABLE);
				DBGPRINT(RT_DEBUG_TRACE, ("Ralink Aggregation\n"));
			}
		}

		if (pAd->MlmeAux.APRalinkIe != 0x0) {
			if (CLIENT_STATUS_TEST_FLAG(pEntry, fCLIENT_STATUS_RDG_CAPABLE))
				AsicSetRDG(pAd, true);

			OPSTATUS_SET_FLAG(pAd, fCLIENT_STATUS_RALINK_CHIPSET);
			CLIENT_STATUS_SET_FLAG(pEntry, fCLIENT_STATUS_RALINK_CHIPSET);
		} else {
			OPSTATUS_CLEAR_FLAG(pAd, fCLIENT_STATUS_RALINK_CHIPSET);
			CLIENT_STATUS_CLEAR_FLAG(pEntry, fCLIENT_STATUS_RALINK_CHIPSET);
		}
	}


	DBGPRINT(RT_DEBUG_TRACE,
		 ("NDIS_STATUS_MEDIA_CONNECT Event B!.BACapability = %x. ClientStatusFlags = %lx\n",
		  pAd->CommonCfg.BACapability.word,
		  pAd->MacTab.Content[BSSID_WCID].ClientStatusFlags));

	pAd->Mlme.PeriodicRound = 0;
	pAd->Mlme.OneSecPeriodicRound = 0;
	pAd->bConfigChanged = false;	/* Reset config flag */
	pAd->ExtraInfo = GENERAL_LINK_UP;	/* Update extra information to link is up */

	/* Set asic auto fall back */
	{
		u8 TableSize = 0;

		MlmeSelectTxRateTable(pAd, pEntry,
				      &pEntry->pTable, &TableSize,
				      &pAd->CommonCfg.TxRateIndex);
		AsicUpdateAutoFallBackTable(pAd, pEntry->pTable);
	}

	RTMP_SEM_LOCK(&pAd->MacTabLock);
	pEntry->HTPhyMode.word = wdev->HTPhyMode.word;
	pEntry->MaxHTPhyMode.word = wdev->MaxHTPhyMode.word;
	if (WMODE_CAP_AC(pAd->CommonCfg.PhyMode) &&
            pAd->MlmeAux.vht_cap_len && pAd->MlmeAux.vht_op_len) {
	    vht_mode_adjust(pAd, pEntry, &pAd->MlmeAux.vht_cap, &pAd->MlmeAux.vht_op);
	}

	if (wdev->bAutoTxRateSwitch == false) {
		pEntry->bAutoTxRateSwitch = false;
		if (pEntry->HTPhyMode.field.MCS == 32)
			pEntry->HTPhyMode.field.ShortGI = GI_800;

		if ((pEntry->HTPhyMode.field.MCS > MCS_7)
		    || (pEntry->HTPhyMode.field.MCS == 32))
			pEntry->HTPhyMode.field.STBC = STBC_NONE;
		/* If the legacy mode is set, overwrite the transmit setting of this entry. */
		if (pEntry->HTPhyMode.field.MODE <= MODE_OFDM)
			RTMPUpdateLegacyTxSetting((u8) wdev->DesiredTransmitSetting.field.FixedTxMode, pEntry);
	} else
		pEntry->bAutoTxRateSwitch = true;

	pEntry->wdev = wdev;
	wdev->allow_data_tx = true;
	RTMP_SEM_UNLOCK(&pAd->MacTabLock);

	/*  Let Link Status Page display first initial rate. */
	pAd->LastTxRate = (unsigned short) (pEntry->HTPhyMode.word);

	/* Select DAC according to HT or Legacy */
	if (pAd->StaActive.SupportedPhyInfo.MCSSet[0] != 0x00)
	{
		{
			if (pAd->Antenna.field.TxPath >= 2)
				mt7612u_bbp_set_txdac(pAd, 2);
			else
				mt7612u_bbp_set_txdac(pAd, 0);
		}
	} else {
		mt7612u_bbp_set_txdac(pAd, 0);
	}

	if ((pAd->StaActive.SupportedPhyInfo.bHtEnable == true)) {
		uint32_t factor = 0;

		/* If HT AP doesn't support MaxRAmpduFactor = 1, we need to set max PSDU to 0. */
		/* Because our Init value is 1 at MACRegTable. */
		switch (pEntry->MaxRAmpduFactor)
		{
			case 0:
				factor = 0x000A0fff;
				break;
			case 1:
				factor = 0x000A1fff;
				break;
			case 2:
				factor = 0x000A2fff;
				break;
			case 3:
				factor = 0x000A3fff;
				break;
			default:
				factor = 0x000A1fff;
				break;
		}
		if (factor)
			mt7612u_write32(pAd, MAX_LEN_CFG, factor);
		DBGPRINT(RT_DEBUG_TRACE, ("MaxRAmpduFactor=%d\n", pEntry->MaxRAmpduFactor));
	}

	/* Txop can only be modified when RDG is off, WMM is disable and TxBurst is enable */
	/* if 1. Legacy AP WMM on,  or 2. 11n AP, AMPDU disable.  Force turn off burst no matter what bEnableTxBurst is. */
	if (!((pAd->CommonCfg.RxStream == 1) && (pAd->CommonCfg.TxStream == 1))
	    && (pAd->StaCfg.bForceTxBurst == false)
	    &&
	    (((pAd->StaActive.SupportedPhyInfo.bHtEnable == false)
	      && OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_WMM_INUSED))
	     || ((pAd->StaActive.SupportedPhyInfo.bHtEnable == true)
		 && (pAd->CommonCfg.BACapability.field.Policy == BA_NOTUSE)))) {
		burst_txop = 0;
		pbf_val = 0x1F3F7F9F;
	} else
		if (pAd->CommonCfg.bEnableTxBurst) {
			burst_txop = 0x60;
			pbf_val = 0x1F3FBF9F;
		} else {
			burst_txop = 0;
			pbf_val = 0x1F3F7F9F;
		}

	pbf_reg = RLT_PBF_MAX_PCNT;

	Data = mt7612u_read32(pAd, EDCA_AC0_CFG);
	Data &= 0xFFFFFF00;
	Data |= burst_txop;
	mt7612u_write32(pAd, EDCA_AC0_CFG, Data);

	/* Re-check to turn on TX burst or not. */
	if ((pAd->CommonCfg.IOTestParm.bLastAtheros == true)
	    && ((STA_WEP_ON(pAd)) || (STA_TKIP_ON(pAd)))) {
		if (pAd->CommonCfg.bEnableTxBurst) {
			uint32_t MACValue = 0;
			/* Force disable TXOP value in this case. The same action in MLMEUpdateProtect too */
			MACValue = mt7612u_read32(pAd, EDCA_AC0_CFG);
			MACValue &= 0xFFFFFF00;
			mt7612u_write32(pAd, EDCA_AC0_CFG, MACValue);
		}
	}

	pAd->CommonCfg.IOTestParm.bLastAtheros = false;

#ifdef WPA_SUPPLICANT_SUPPORT
	/*
	   If STA connects to different AP, STA couldn't send EAPOL_Start for WpaSupplicant.
	 */
	if ((pAd->StaCfg.BssType == BSS_INFRA) &&
	    (wdev->AuthMode == Ndis802_11AuthModeWPA2) &&
	    (NdisEqualMemory(pAd->CommonCfg.Bssid, pAd->CommonCfg.LastBssid, MAC_ADDR_LEN) == false) &&
	     (pAd->StaCfg.wpa_supplicant_info.bLostAp == true)) {
		pAd->StaCfg.wpa_supplicant_info.bLostAp = false;
	}
#endif /* WPA_SUPPLICANT_SUPPORT */

	/*
	   Need to check this COPY. This COPY is from Windows Driver.
	 */

	COPY_MAC_ADDR(pAd->CommonCfg.LastBssid, pAd->CommonCfg.Bssid);

	/* BSSID add in one MAC entry too.  Because in Tx, ASIC need to check Cipher and IV/EIV, BAbitmap */
	/* Pther information in MACTab.Content[BSSID_WCID] is not necessary for driver. */
	/* Note: As STA, The MACTab.Content[BSSID_WCID]. PairwiseKey and Shared Key for BSS0 are the same. */

	RTMP_SEM_LOCK(&pAd->MacTabLock);
	pEntry->PortSecured = wdev->PortSecured;
	RTMP_SEM_UNLOCK(&pAd->MacTabLock);

	/* */
	/* Patch Atheros AP TX will breakdown issue. */
	/* AP Model: DLink DWL-8200AP */
	/* */
	if (INFRA_ON(pAd) && OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_WMM_INUSED)
	    && STA_TKIP_ON(pAd)) {
		mt7612u_write32(pAd, RX_PARSER_CFG, 0x01);
	} else {
		mt7612u_write32(pAd, RX_PARSER_CFG, 0x00);
	}

	RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS);
	/*RTMP_CLEAR_PSFLAG(pAd, fRTMP_PS_GO_TO_SLEEP_NOW); */

	if (INFRA_ON(pAd)) {
		if ((pAd->CommonCfg.bBssCoexEnable == true)
		    && (pAd->CommonCfg.Channel <= 14)
		    && (pAd->StaActive.SupportedPhyInfo.bHtEnable == true)
		    && (pAd->MlmeAux.ExtCapInfo.BssCoexistMgmtSupport == 1)) {
			OPSTATUS_SET_FLAG(pAd, fOP_STATUS_SCAN_2040);
			BuildEffectedChannelList(pAd);
			DBGPRINT(RT_DEBUG_TRACE,
				 ("LinkUP AP supports 20/40 BSS COEX, Dot11BssWidthTriggerScanInt[%d]\n",
				  pAd->CommonCfg.Dot11BssWidthTriggerScanInt));
		} else {
			DBGPRINT(RT_DEBUG_TRACE, ("not supports 20/40 BSS COEX !!! \n"));
			DBGPRINT(RT_DEBUG_TRACE,
				 ("pAd->CommonCfg Info: bBssCoexEnable=%d, Channel=%d, CentralChannel=%d, PhyMode=%d\n",
					pAd->CommonCfg.bBssCoexEnable, pAd->CommonCfg.Channel,
					pAd->CommonCfg.CentralChannel, pAd->CommonCfg.PhyMode));
			DBGPRINT(RT_DEBUG_TRACE,
				 ("pAd->StaActive.SupportedHtPhy.bHtEnable=%d\n",
				  pAd->StaActive.SupportedPhyInfo.bHtEnable));
			DBGPRINT(RT_DEBUG_TRACE,
				 ("pAd->MlmeAux.ExtCapInfo.BssCoexstSup=%d\n",
				  pAd->MlmeAux.ExtCapInfo.BssCoexistMgmtSupport));
		}
	}

#ifdef WPA_SUPPLICANT_SUPPORT
	/*
	   When AuthMode is WPA2-Enterprise and AP reboot or STA lost AP,
	   WpaSupplicant would not send EapolStart to AP after STA re-connect to AP again.
	   In this case, driver would send EapolStart to AP.
	 */
	if ((pAd->StaCfg.BssType == BSS_INFRA) &&
	    (wdev->AuthMode == Ndis802_11AuthModeWPA2) &&
	    (NdisEqualMemory
	     (pAd->CommonCfg.Bssid, pAd->CommonCfg.LastBssid, MAC_ADDR_LEN))
	    && (pAd->StaCfg.wpa_supplicant_info.bLostAp == true)) {
		WpaSendEapolStart(pAd, pAd->CommonCfg.Bssid);
		pAd->StaCfg.wpa_supplicant_info.bLostAp = false;
	}
#endif /* WPA_SUPPLICANT_SUPPORT */



	if (IS_MT76x2(pAd)) {
		mt76x2_reinit_agc_gain(pAd, pAd->hw_cfg.cent_ch);
		mt76x2_reinit_hi_lna_gain(pAd, pAd->hw_cfg.cent_ch);
		mt76x2_get_agc_gain(pAd, true);
		mt76x2_calibration(pAd, pAd->hw_cfg.cent_ch);
	}

	pAd->MacTab.MsduLifeTime = 5; /* default 5 seconds */

}


/*
	==========================================================================

	Routine	Description:
		Disconnect current BSSID

	Arguments:
		pAd				- Pointer to our adapter
		IsReqFromAP		- Request from AP

	Return Value:
		None

	IRQL = DISPATCH_LEVEL

	Note:
		We need more information to know it's this requst from AP.
		If yes! we need to do extra handling, for example, remove the WPA key.
		Otherwise on 4-way handshaking will faied, since the WPA key didn't be
		remove while auto reconnect.
		Disconnect request from AP, it means we will start afresh 4-way handshaking
		on WPA mode.

	==========================================================================
*/
VOID LinkDown(
	IN struct rtmp_adapter *pAd,
	IN bool IsReqFromAP)
{
	u8 i;
	struct rtmp_wifi_dev *wdev = &pAd->StaCfg.wdev;

	/* Do nothing if monitor mode is on */
	if (MONITOR_ON(pAd))
		return;

	RTMPSendWirelessEvent(pAd, IW_STA_LINKDOWN_EVENT_FLAG, NULL, BSS0, 0);

	DBGPRINT(RT_DEBUG_TRACE, ("!!! LINK DOWN !!!\n"));
	OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_AGGREGATION_INUSED);

	/* reset to not doing improved scan */
	pAd->StaCfg.bImprovedScan = false;

#ifdef RT_CFG80211_SUPPORT
    if (CFG80211DRV_OpsScanRunning(pAd))
		CFG80211DRV_OpsScanInLinkDownAction(pAd);
#endif /* RT_CFG80211_SUPPORT */

	if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_DOZE)
/*||	RTMP_TEST_PSFLAG(pAd, fRTMP_PS_SET_PCI_CLK_OFF_COMMAND) */
	    || RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_IDLE_RADIO_OFF)) {
		AUTO_WAKEUP_STRUC AutoWakeupCfg;
		AsicForceWakeup(pAd, true);
		AutoWakeupCfg.word = 0;
		mt7612u_write32(pAd, AUTO_WAKEUP_CFG, AutoWakeupCfg.word);
		OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_DOZE);
	}

	if (ADHOC_ON(pAd)) {	/* Adhoc mode link down */
		DBGPRINT(RT_DEBUG_TRACE, ("!!! LINK DOWN 1!!!\n"));


		OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_ADHOC_ON);
		OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED);
		RTMP_IndicateMediaState(pAd, NdisMediaStateDisconnected);
		pAd->ExtraInfo = GENERAL_LINK_DOWN;
		BssTableDeleteEntry(&pAd->ScanTab, pAd->CommonCfg.Bssid,
				    pAd->CommonCfg.Channel);
		DBGPRINT(RT_DEBUG_TRACE,
			 ("!!! MacTab.Size=%d !!!\n", pAd->MacTab.Size));
	} else {		/* Infra structure mode */

		DBGPRINT(RT_DEBUG_TRACE, ("!!! LINK DOWN 2!!!\n"));




		OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_INFRA_ON);
		OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED);


		/* Saved last SSID for linkup comparison */
		pAd->CommonCfg.LastSsidLen = pAd->CommonCfg.SsidLen;
		memmove(pAd->CommonCfg.LastSsid, pAd->CommonCfg.Ssid,
			       pAd->CommonCfg.LastSsidLen);
		COPY_MAC_ADDR(pAd->CommonCfg.LastBssid, pAd->CommonCfg.Bssid);
		if (pAd->MlmeAux.CurrReqIsFromNdis == true) {
			DBGPRINT(RT_DEBUG_TRACE,
				 ("NDIS_STATUS_MEDIA_DISCONNECT Event A!\n"));
			pAd->MlmeAux.CurrReqIsFromNdis = false;
		} else {
			if ((wdev->PortSecured == WPA_802_1X_PORT_SECURED)
			    || (wdev->AuthMode < Ndis802_11AuthModeWPA)) {
			/*
				If disassociation request is from NDIS, then we don't need
				to delete BSSID from entry. Otherwise lost beacon or receive
				De-Authentication from AP, then we should delete BSSID
				from BssTable.

				If we don't delete from entry, roaming will fail.
			*/
				BssTableDeleteEntry(&pAd->ScanTab,
						    pAd->CommonCfg.Bssid,
						    pAd->CommonCfg.Channel);
			}
		}

	/*
		restore back to -
		1. long slot (20 us) or short slot (9 us) time
		2. turn on/off RTS/CTS and/or CTS-to-self protection
		3. short preamble
	*/
		OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_BG_PROTECTION_INUSED);


	}


	for (i = 1; i < MAX_LEN_OF_MAC_TABLE; i++) {
		if (IS_ENTRY_CLIENT(&pAd->MacTab.Content[i])
			)
			MacTableDeleteEntry(pAd, pAd->MacTab.Content[i].Aid,
					    pAd->MacTab.Content[i].Addr);
	}

	AsicSetSlotTime(pAd, true);	/*false); */
	AsicSetEdcaParm(pAd, NULL);


		AsicDisableSync(pAd);

	pAd->Mlme.PeriodicRound = 0;
	pAd->Mlme.OneSecPeriodicRound = 0;

	memset(&pAd->MlmeAux.HtCapability, 0, sizeof (HT_CAPABILITY_IE));
	memset(&pAd->MlmeAux.AddHtInfo, 0, sizeof (ADD_HT_INFO_IE));
	pAd->MlmeAux.HtCapabilityLen = 0;
	pAd->MlmeAux.NewExtChannelOffset = 0xff;

	DBGPRINT(RT_DEBUG_TRACE, ("LinkDownCleanMlmeAux.ExtCapInfo!\n"));
	memset((u8 *) (&pAd->MlmeAux.ExtCapInfo), 0,
		       sizeof (EXT_CAP_INFO_ELEMENT));

	/* Reset WPA-PSK state. Only reset when supplicant enabled */
	if (pAd->StaCfg.WpaState != SS_NOTUSE) {
		pAd->StaCfg.WpaState = SS_START;
		/* Clear Replay counter */
		memset(pAd->StaCfg.ReplayCounter, 0, 8);

	}

	/*
		if link down come from AP, we need to remove all WPA keys on WPA mode.
		otherwise will cause 4-way handshaking failed, since the WPA key not empty.
	*/
	if ((IsReqFromAP) && (wdev->AuthMode >= Ndis802_11AuthModeWPA)) {
		/* Remove all WPA keys */
		RTMPWPARemoveAllKeys(pAd);
	}

	/* 802.1x port control */
#ifdef WPA_SUPPLICANT_SUPPORT
	/* Prevent clear PortSecured here with static WEP */
	/* NetworkManger set security policy first then set SSID to connect AP. */
	if (pAd->StaCfg.wpa_supplicant_info.WpaSupplicantUP &&
	    (wdev->WepStatus == Ndis802_11WEPEnabled) &&
	    (wdev->IEEE8021X == false)) {
		wdev->PortSecured = WPA_802_1X_PORT_SECURED;
	} else
#endif /* WPA_SUPPLICANT_SUPPORT */
	{
		wdev->PortSecured = WPA_802_1X_PORT_NOT_SECURED;
		pAd->StaCfg.PrivacyFilter = Ndis802_11PrivFilter8021xWEP;
	}

	RTMP_SEM_LOCK(&pAd->MacTabLock);
	memset(&pAd->MacTab.Content[BSSID_WCID], 0, sizeof(MAC_TABLE_ENTRY));
	pAd->MacTab.Content[BSSID_WCID].PortSecured = wdev->PortSecured;
	RTMP_SEM_UNLOCK(&pAd->MacTabLock);

	pAd->StaCfg.MicErrCnt = 0;

	RTMP_IndicateMediaState(pAd, NdisMediaStateDisconnected);
	/* Update extra information to link is up */
	pAd->ExtraInfo = GENERAL_LINK_DOWN;

	pAd->StaActive.SupportedPhyInfo.bHtEnable = false;

	pAd->bUsbTxBulkAggre = false;

	/* Clean association information */
	memset(&pAd->StaCfg.AssocInfo, 0,
		       sizeof (NDIS_802_11_ASSOCIATION_INFORMATION));
	pAd->StaCfg.AssocInfo.Length =
	    sizeof (NDIS_802_11_ASSOCIATION_INFORMATION);
	pAd->StaCfg.ReqVarIELen = 0;
	pAd->StaCfg.ResVarIELen = 0;


	/* Reset RSSI value after link down */
	memset((u8 *) (&pAd->StaCfg.RssiSample), 0,
		       sizeof (pAd->StaCfg.RssiSample));

	/* Restore MlmeRate */
	pAd->CommonCfg.MlmeRate = pAd->CommonCfg.BasicMlmeRate;
	pAd->CommonCfg.RtsRate = pAd->CommonCfg.BasicMlmeRate;

	// TODO: shiang-6590, why we need to fallback to BW_20 here? How about the BW_10?
	if (pAd->CommonCfg.BBPCurrentBW != BW_20) {
		{
			mt7612u_bbp_set_bw(pAd, BW_20);
		}
	}

	{
		/* Reset DAC */
		{
			if (pAd->Antenna.field.TxPath >= 2)
				mt7612u_bbp_set_txdac(pAd, 2);
			else
				mt7612u_bbp_set_txdac(pAd, 0);
		}
	}

	/* After Link down, reset piggy-back setting in ASIC. Disable RDG. */
	RTMPSetPiggyBack(pAd, false);
	OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_PIGGYBACK_INUSED);

	pAd->CommonCfg.BACapability.word = pAd->CommonCfg.REGBACapability.word;

	/* Restore all settings in the following. */
	AsicUpdateProtect(pAd, 0,
			  (ALLN_SETPROTECT | CCKSETPROTECT | OFDMSETPROTECT),
			  true, false);

	AsicSetRDG(pAd, false);

	OPSTATUS_CLEAR_FLAG(pAd, fOP_STATUS_SCAN_2040);
	pAd->CommonCfg.BSSCoexist2040.word = 0;
	TriEventInit(pAd);
	for (i = 0; i < (pAd->ChannelListNum - 1); i++) {
		pAd->ChannelList[i].bEffectedChannel = false;
	}

	mt7612u_write32(pAd, MAX_LEN_CFG, 0x1fff);
	RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS);
/* Allow go to sleep after linkdown steps. */


#ifdef WPA_SUPPLICANT_SUPPORT
#ifndef NATIVE_WPA_SUPPLICANT_SUPPORT
	if (pAd->StaCfg.wpa_supplicant_info.WpaSupplicantUP) {
		/*send disassociate event to wpa_supplicant */
		RtmpOSWrielessEventSend(pAd->net_dev, RT_WLAN_EVENT_CUSTOM,
					RT_DISASSOC_EVENT_FLAG, NULL, NULL, 0);
	}
#endif /* NATIVE_WPA_SUPPLICANT_SUPPORT */
#endif /* WPA_SUPPLICANT_SUPPORT */

#ifdef NATIVE_WPA_SUPPLICANT_SUPPORT
	RtmpOSWrielessEventSend(pAd->net_dev, RT_WLAN_EVENT_CGIWAP, -1, NULL,
				NULL, 0);
#endif /* NATIVE_WPA_SUPPLICANT_SUPPORT */

#ifdef RT_CFG80211_SUPPORT
	RT_CFG80211_LOST_AP_INFORM(pAd);
#endif /* RT_CFG80211_SUPPORT */


	if (pAd->StaCfg.BssType != BSS_ADHOC)
		pAd->StaCfg.bNotFirstScan = false;
	else
		pAd->StaCfg.bAdhocCreator = false;

/*After change from one ap to another , we need to re-init rssi for AdjustTxPower  */
	pAd->StaCfg.RssiSample.AvgRssi0	= -127;
	pAd->StaCfg.RssiSample.AvgRssi1	= -127;
	pAd->StaCfg.RssiSample.AvgRssi2	= -127;

	memset(pAd->StaCfg.ConnectinfoSsid, 0, MAX_LEN_OF_SSID);
	memset(pAd->StaCfg.ConnectinfoBssid, 0, MAC_ADDR_LEN);
	pAd->StaCfg.ConnectinfoSsidLen  = 0;
	pAd->StaCfg.ConnectinfoBssType  = 1;
	pAd->StaCfg.ConnectinfoChannel = 0;

	if (pAd->chipCap.FreqCalibrationSupport)
		StopFrequencyCalibration(pAd);
}


/*
	==========================================================================
	Description:

	IRQL = DISPATCH_LEVEL

	==========================================================================
*/
VOID IterateOnBssTab(struct rtmp_adapter *pAd)
{
	MLME_START_REQ_STRUCT StartReq;
	MLME_JOIN_REQ_STRUCT JoinReq;
	ULONG BssIdx;
	BSS_ENTRY *pInBss = NULL;
	struct rtmp_wifi_dev *wdev = &pAd->StaCfg.wdev;


	/* Change the wepstatus to original wepstatus */
	pAd->StaCfg.PairCipher = wdev->WepStatus;
#ifdef WPA_SUPPLICANT_SUPPORT
	if (pAd->StaCfg.wpa_supplicant_info.WpaSupplicantUP == WPA_SUPPLICANT_DISABLE)
#endif /* WPA_SUPPLICANT_SUPPORT */
	pAd->StaCfg.GroupCipher = wdev->WepStatus;

	BssIdx = pAd->MlmeAux.BssIdx;

	if (pAd->StaCfg.BssType == BSS_ADHOC) {
		if (BssIdx < pAd->MlmeAux.SsidBssTab.BssNr) {
			pInBss = &pAd->MlmeAux.SsidBssTab.BssEntry[BssIdx];
			DBGPRINT(RT_DEBUG_TRACE,
				 ("CNTL - iterate BSS %ld of %d\n", BssIdx,
				  pAd->MlmeAux.SsidBssTab.BssNr));
			JoinParmFill(pAd, &JoinReq, BssIdx);
			MlmeEnqueue(pAd, SYNC_STATE_MACHINE, MT2_MLME_JOIN_REQ,
				    sizeof (MLME_JOIN_REQ_STRUCT), &JoinReq, 0);
			pAd->Mlme.CntlMachine.CurrState = CNTL_WAIT_JOIN;
			return;
		}
		{
			DBGPRINT(RT_DEBUG_TRACE, ("CNTL - All BSS fail; start a new ADHOC (Ssid=%s)...\n",pAd->MlmeAux.Ssid));
			StartParmFill(pAd, &StartReq, (PCHAR) pAd->MlmeAux.Ssid,pAd->MlmeAux.SsidLen);
			MlmeEnqueue(pAd, SYNC_STATE_MACHINE, MT2_MLME_START_REQ, sizeof (MLME_START_REQ_STRUCT), &StartReq, 0);

			pAd->Mlme.CntlMachine.CurrState = CNTL_WAIT_START;
		}
	}
	else if ((BssIdx < pAd->MlmeAux.SsidBssTab.BssNr) &&
		   (pAd->StaCfg.BssType == BSS_INFRA)) {
		pInBss = &pAd->MlmeAux.SsidBssTab.BssEntry[BssIdx];
		/* Check cipher suite, AP must have more secured cipher than station setting */
		/* Set the Pairwise and Group cipher to match the intended AP setting */
		/* We can only connect to AP with less secured cipher setting */
		if ((wdev->AuthMode == Ndis802_11AuthModeWPA)
		    || (wdev->AuthMode == Ndis802_11AuthModeWPAPSK)) {
			pAd->StaCfg.GroupCipher = pInBss->WPA.GroupCipher;

			if (wdev->WepStatus == pInBss->WPA.PairCipher)
				pAd->StaCfg.PairCipher = pInBss->WPA.PairCipher;
			else if (pInBss->WPA.PairCipherAux != Ndis802_11WEPDisabled)
				pAd->StaCfg.PairCipher = pInBss->WPA.PairCipherAux;
			else	/* There is no PairCipher Aux, downgrade our capability to TKIP */
				pAd->StaCfg.PairCipher = Ndis802_11TKIPEnable;
		} else if ((wdev->AuthMode == Ndis802_11AuthModeWPA2)
			   || (wdev->AuthMode == Ndis802_11AuthModeWPA2PSK)) {
			pAd->StaCfg.GroupCipher = pInBss->WPA2.GroupCipher;

			if (wdev->WepStatus == pInBss->WPA2.PairCipher)
				pAd->StaCfg.PairCipher = pInBss->WPA2.PairCipher;
			else if (pInBss->WPA2.PairCipherAux != Ndis802_11WEPDisabled)
				pAd->StaCfg.PairCipher = pInBss->WPA2.PairCipherAux;
			else	/* There is no PairCipher Aux, downgrade our capability to TKIP */
				pAd->StaCfg.PairCipher = Ndis802_11TKIPEnable;

			/* RSN capability */
			pAd->StaCfg.RsnCapability = pInBss->WPA2.RsnCapability;
		}

		/* Set Mix cipher flag */
		pAd->StaCfg.bMixCipher =
		    (pAd->StaCfg.PairCipher == pAd->StaCfg.GroupCipher) ? false : true;
		DBGPRINT(RT_DEBUG_TRACE,
			 ("CNTL - iterate BSS %ld of %d\n", BssIdx,
			  pAd->MlmeAux.SsidBssTab.BssNr));
		JoinParmFill(pAd, &JoinReq, BssIdx);
		MlmeEnqueue(pAd, SYNC_STATE_MACHINE, MT2_MLME_JOIN_REQ,
			    sizeof (MLME_JOIN_REQ_STRUCT), &JoinReq, 0);
		pAd->Mlme.CntlMachine.CurrState = CNTL_WAIT_JOIN;
	}
	else
	{
		/* no more BSS */
		{
			DBGPRINT(RT_DEBUG_TRACE,
				 ("CNTL - All roaming failed, restore to channel %d, Total BSS[%02d]\n",
				  pAd->CommonCfg.Channel, pAd->ScanTab.BssNr));
		}

		{
		pAd->MlmeAux.SsidBssTab.BssNr = 0;
		BssTableDeleteEntry(&pAd->ScanTab,
				    pAd->MlmeAux.SsidBssTab.BssEntry[0].Bssid,
					pAd->MlmeAux.SsidBssTab.BssEntry[0].Channel);
		}

		pAd->Mlme.CntlMachine.CurrState = CNTL_IDLE;
	}
}


/* for re-association only */
/* IRQL = DISPATCH_LEVEL */
VOID IterateOnBssTab2(struct rtmp_adapter *pAd)
{
	MLME_REASSOC_REQ_STRUCT ReassocReq;
	ULONG BssIdx;
	BSS_ENTRY *pBss;

	BssIdx = pAd->MlmeAux.RoamIdx;
	pBss = &pAd->MlmeAux.RoamTab.BssEntry[BssIdx];

	if (BssIdx < pAd->MlmeAux.RoamTab.BssNr) {
		DBGPRINT(RT_DEBUG_TRACE,
			 ("CNTL - iterate BSS %ld of %d\n", BssIdx,
			  pAd->MlmeAux.RoamTab.BssNr));

		AsicSwitchChannel(pAd, pBss->Channel, false);
		AsicLockChannel(pAd, pBss->Channel);

		/* reassociate message has the same structure as associate message */
		AssocParmFill(pAd, &ReassocReq, pBss->Bssid,
			      pBss->CapabilityInfo, ASSOC_TIMEOUT,
			      pAd->StaCfg.DefaultListenCount);
		MlmeEnqueue(pAd, ASSOC_STATE_MACHINE, MT2_MLME_REASSOC_REQ,
			    sizeof (MLME_REASSOC_REQ_STRUCT), &ReassocReq, 0);

		pAd->Mlme.CntlMachine.CurrState = CNTL_WAIT_REASSOC;
	} else {
		/* no more BSS */
		u8 rf_channel = 0;
		UINT8 rf_bw, ext_ch;

		{
			rf_channel = pAd->CommonCfg.Channel;
			rf_bw = BW_20;
			ext_ch = EXTCHA_NONE;
		}

		if (rf_channel != 0) {
			AsicSetChannel(pAd, rf_channel, rf_bw, ext_ch, false);

			DBGPRINT(RT_DEBUG_TRACE,
				 ("%s():CNTL - All roaming failed, restore to Channel(Ctrl=%d, Central = %d)\n",
				  __FUNCTION__, pAd->CommonCfg.Channel,
				  pAd->CommonCfg.CentralChannel));
		}

		pAd->Mlme.CntlMachine.CurrState = CNTL_IDLE;
	}
}

/*
	==========================================================================
	Description:

	IRQL = DISPATCH_LEVEL

	==========================================================================
*/
VOID JoinParmFill(
	IN struct rtmp_adapter *pAd,
	IN OUT MLME_JOIN_REQ_STRUCT *JoinReq,
	IN ULONG BssIdx)
{
	JoinReq->BssIdx = BssIdx;
}

/*
	==========================================================================
	Description:

	IRQL = DISPATCH_LEVEL

	==========================================================================
*/

/*
	==========================================================================
	Description:

	IRQL = DISPATCH_LEVEL

	==========================================================================
*/
VOID StartParmFill(
	IN struct rtmp_adapter *pAd,
	IN OUT MLME_START_REQ_STRUCT *StartReq,
	IN CHAR Ssid[],
	IN u8 SsidLen)
{
	ASSERT(SsidLen <= MAX_LEN_OF_SSID);
	if (SsidLen > MAX_LEN_OF_SSID)
		SsidLen = MAX_LEN_OF_SSID;
	memmove(StartReq->Ssid, Ssid, SsidLen);
	StartReq->SsidLen = SsidLen;
}

/*
	==========================================================================
	Description:

	IRQL = DISPATCH_LEVEL

	==========================================================================
*/
VOID AuthParmFill(
	IN struct rtmp_adapter *pAd,
	IN OUT MLME_AUTH_REQ_STRUCT *AuthReq,
	IN u8 *pAddr,
	IN unsigned short Alg)
{
	COPY_MAC_ADDR(AuthReq->Addr, pAddr);
	AuthReq->Alg = Alg;
	AuthReq->Timeout = AUTH_TIMEOUT;
}

/*
	==========================================================================
	Description:

	IRQL = DISPATCH_LEVEL

	==========================================================================
 */

VOID MlmeCntlConfirm(
	IN struct rtmp_adapter *pAd,
	IN ULONG MsgType,
	IN unsigned short Msg)
{
	MlmeEnqueue(pAd, MLME_CNTL_STATE_MACHINE, MsgType, sizeof (unsigned short),
		    &Msg, 0);
}

/*
	==========================================================================
	Description:
		Pre-build a BEACON frame in the shared memory

	IRQL = PASSIVE_LEVEL
	IRQL = DISPATCH_LEVEL

	==========================================================================
*/
ULONG MakeIbssBeacon(
	IN struct rtmp_adapter *pAd)
{
	u8 DsLen = 1, IbssLen = 2;
	u8 LocalErpIe[3] = { IE_ERP, 1, 0x04 };
	HEADER_802_11 BcnHdr;
	unsigned short CapabilityInfo;
	LARGE_INTEGER FakeTimestamp;
	ULONG FrameLen = 0;
	TXWI_STRUC *pTxWI = &pAd->BeaconTxWI;
	u8 *pBeaconFrame = pAd->BeaconBuf;
	bool Privacy;
	u8 SupRate[MAX_LEN_OF_SUPPORTED_RATES];
	u8 SupRateLen = 0;
	u8 ExtRate[MAX_LEN_OF_SUPPORTED_RATES];
	u8 ExtRateLen = 0;
	struct rtmp_wifi_dev *wdev = &pAd->StaCfg.wdev;

	if (WMODE_EQUAL(pAd->CommonCfg.PhyMode, WMODE_B)
	    && (pAd->CommonCfg.Channel <= 14)) {
		SupRate[0] = 0x82;	/* 1 mbps */
		SupRate[1] = 0x84;	/* 2 mbps */
		SupRate[2] = 0x8b;	/* 5.5 mbps */
		SupRate[3] = 0x96;	/* 11 mbps */
		SupRateLen = 4;
		ExtRateLen = 0;
	} else if (pAd->CommonCfg.Channel > 14) {
		SupRate[0] = 0x8C;	/* 6 mbps, in units of 0.5 Mbps, basic rate */
		SupRate[1] = 0x12;	/* 9 mbps, in units of 0.5 Mbps */
		SupRate[2] = 0x98;	/* 12 mbps, in units of 0.5 Mbps, basic rate */
		SupRate[3] = 0x24;	/* 18 mbps, in units of 0.5 Mbps */
		SupRate[4] = 0xb0;	/* 24 mbps, in units of 0.5 Mbps, basic rate */
		SupRate[5] = 0x48;	/* 36 mbps, in units of 0.5 Mbps */
		SupRate[6] = 0x60;	/* 48 mbps, in units of 0.5 Mbps */
		SupRate[7] = 0x6c;	/* 54 mbps, in units of 0.5 Mbps */
		SupRateLen = 8;
		ExtRateLen = 0;

		/* */
		/* Also Update MlmeRate & RtsRate for G only & A only */
		/* */
		pAd->CommonCfg.MlmeRate = RATE_6;
		pAd->CommonCfg.RtsRate = RATE_6;
		pAd->CommonCfg.MlmeTransmit.field.MODE = MODE_OFDM;
		pAd->CommonCfg.MlmeTransmit.field.MCS = OfdmRateToRxwiMCS[pAd->CommonCfg.MlmeRate];
		pAd->MacTab.Content[BSS0Mcast_WCID].HTPhyMode.field.MODE = MODE_OFDM;
		pAd->MacTab.Content[BSS0Mcast_WCID].HTPhyMode.field.MCS = OfdmRateToRxwiMCS[pAd->CommonCfg.MlmeRate];
	} else {
		SupRate[0] = 0x82;	/* 1 mbps */
		SupRate[1] = 0x84;	/* 2 mbps */
		SupRate[2] = 0x8b;	/* 5.5 mbps */
		SupRate[3] = 0x96;	/* 11 mbps */
		SupRateLen = 4;

		ExtRate[0] = 0x0C;	/* 6 mbps, in units of 0.5 Mbps, */
		ExtRate[1] = 0x12;	/* 9 mbps, in units of 0.5 Mbps */
		ExtRate[2] = 0x18;	/* 12 mbps, in units of 0.5 Mbps, */
		ExtRate[3] = 0x24;	/* 18 mbps, in units of 0.5 Mbps */
		ExtRate[4] = 0x30;	/* 24 mbps, in units of 0.5 Mbps, */
		ExtRate[5] = 0x48;	/* 36 mbps, in units of 0.5 Mbps */
		ExtRate[6] = 0x60;	/* 48 mbps, in units of 0.5 Mbps */
		ExtRate[7] = 0x6c;	/* 54 mbps, in units of 0.5 Mbps */
		ExtRateLen = 8;
	}

	pAd->StaActive.SupRateLen = SupRateLen;
	memmove(pAd->StaActive.SupRate, SupRate, SupRateLen);
	pAd->StaActive.ExtRateLen = ExtRateLen;
	memmove(pAd->StaActive.ExtRate, ExtRate, ExtRateLen);

	/* compose IBSS beacon frame */
	MgtMacHeaderInit(pAd, &BcnHdr, SUBTYPE_BEACON, 0, BROADCAST_ADDR,
						pAd->CurrentAddress,
			 pAd->CommonCfg.Bssid);
	Privacy = (wdev->WepStatus == Ndis802_11WEPEnabled)
	    || (wdev->WepStatus == Ndis802_11TKIPEnable)
	    || (wdev->WepStatus == Ndis802_11AESEnable);
	CapabilityInfo =
	    CAP_GENERATE(0, 1, Privacy,
			 (pAd->CommonCfg.TxPreamble == Rt802_11PreambleShort),
			 0, 0);

	MakeOutgoingFrame(pBeaconFrame, &FrameLen,
			  sizeof (HEADER_802_11), &BcnHdr,
			  TIMESTAMP_LEN, &FakeTimestamp,
			  2, &pAd->CommonCfg.BeaconPeriod,
			  2, &CapabilityInfo,
			  1, &SsidIe,
			  1, &pAd->CommonCfg.SsidLen,
			  pAd->CommonCfg.SsidLen, pAd->CommonCfg.Ssid,
			  1, &SupRateIe,
			  1, &SupRateLen,
			  SupRateLen, SupRate,
			  1, &DsIe,
			  1, &DsLen,
			  1, &pAd->CommonCfg.Channel,
			  1, &IbssIe,
			  1, &IbssLen, 2, &pAd->StaActive.AtimWin, END_OF_ARGS);

	/* add ERP_IE and EXT_RAE IE of in 802.11g */
	if (ExtRateLen) {
		ULONG tmp;

		MakeOutgoingFrame(pBeaconFrame + FrameLen, &tmp,
				  3, LocalErpIe,
				  1, &ExtRateIe,
				  1, &ExtRateLen,
				  ExtRateLen, ExtRate, END_OF_ARGS);
		FrameLen += tmp;
	}

	/* If adhoc secruity is set for WPA-None, append the cipher suite IE */
	/* Modify by Eddy, support WPA2PSK in Adhoc mode */
	if ((wdev->AuthMode == Ndis802_11AuthModeWPANone)
	    ) {
		u8 RSNIe = IE_WPA;
		ULONG tmp;

		RTMPMakeRSNIE(pAd, wdev->AuthMode, wdev->WepStatus, BSS0);

		MakeOutgoingFrame(pBeaconFrame + FrameLen, &tmp,
				  1, &RSNIe,
				  1, &pAd->StaCfg.RSNIE_Len,
				  pAd->StaCfg.RSNIE_Len, pAd->StaCfg.RSN_IE,
				  END_OF_ARGS);
		FrameLen += tmp;
	}

	if (WMODE_CAP_N(pAd->CommonCfg.PhyMode)
	    && (pAd->StaCfg.bAdhocN == true)) {
		ULONG TmpLen;
		u8 HtLen, HtLen1;

#ifdef RT_BIG_ENDIAN
		HT_CAPABILITY_IE HtCapabilityTmp;
		ADD_HT_INFO_IE addHTInfoTmp;
		unsigned short b2lTmp, b2lTmp2;
#endif

		/* add HT Capability IE */
		HtLen = sizeof (pAd->CommonCfg.HtCapability);
		HtLen1 = sizeof (pAd->CommonCfg.AddHTInfo);
#ifndef RT_BIG_ENDIAN
		MakeOutgoingFrame(pBeaconFrame + FrameLen, &TmpLen,
				  1, &HtCapIe,
				  1, &HtLen,
				  HtLen, &pAd->CommonCfg.HtCapability,
				  1, &AddHtInfoIe,
				  1, &HtLen1,
				  HtLen1, &pAd->CommonCfg.AddHTInfo,
				  END_OF_ARGS);
#else
		memmove(&HtCapabilityTmp, &pAd->CommonCfg.HtCapability,
			       HtLen);
		*(unsigned short *) (&HtCapabilityTmp.HtCapInfo) =
		    SWAP16(*(unsigned short *) (&HtCapabilityTmp.HtCapInfo));
		*(unsigned short *) (&HtCapabilityTmp.ExtHtCapInfo) =
		    SWAP16(*(unsigned short *) (&HtCapabilityTmp.ExtHtCapInfo));

		memmove(&addHTInfoTmp, &pAd->CommonCfg.AddHTInfo,
			       HtLen1);
		*(unsigned short *) (&addHTInfoTmp.AddHtInfo2) =
		    SWAP16(*(unsigned short *) (&addHTInfoTmp.AddHtInfo2));
		*(unsigned short *) (&addHTInfoTmp.AddHtInfo3) =
		    SWAP16(*(unsigned short *) (&addHTInfoTmp.AddHtInfo3));

		MakeOutgoingFrame(pBeaconFrame + FrameLen, &TmpLen,
				  1, &HtCapIe,
				  1, &HtLen,
				  HtLen, &HtCapabilityTmp,
				  1, &AddHtInfoIe,
				  1, &HtLen1,
				  HtLen1, &addHTInfoTmp, END_OF_ARGS);
#endif
		FrameLen += TmpLen;
	}

	/*beacon use reserved WCID 0xff */
	if (pAd->CommonCfg.Channel > 14) {
		RTMPWriteTxWI(pAd, pTxWI, false, false, true, false, false,
			      true, 0, 0xff, FrameLen, PID_MGMT, PID_BEACON,
			      RATE_1, IFS_HTTXOP,
			      &pAd->CommonCfg.MlmeTransmit);
	} else {
		/* Set to use 1Mbps for Adhoc beacon. */
		HTTRANSMIT_SETTING Transmit;
		Transmit.word = 0;
		RTMPWriteTxWI(pAd, pTxWI, false, false, true, false, false,
			      true, 0, 0xff, FrameLen, PID_MGMT, PID_BEACON,
			      RATE_1, IFS_HTTXOP, &Transmit);
	}

#ifdef RT_BIG_ENDIAN
	RTMPFrameEndianChange(pAd, pBeaconFrame, DIR_WRITE, false);
	RTMPWIEndianChange(pAd, (u8 *) pTxWI, TYPE_TXWI);
#endif

	DBGPRINT(RT_DEBUG_TRACE,
		 ("MakeIbssBeacon (len=%ld), SupRateLen=%d, ExtRateLen=%d, Channel=%d, PhyMode=%d\n",
		  FrameLen, SupRateLen, ExtRateLen, pAd->CommonCfg.Channel,
		  pAd->CommonCfg.PhyMode));
	return FrameLen;
}

VOID InitChannelRelatedValue(struct rtmp_adapter *pAd)
{
	u8 rf_channel;
	UINT8 rf_bw, ext_ch;


	pAd->CommonCfg.CentralChannel = pAd->MlmeAux.CentralChannel;
	pAd->CommonCfg.Channel = pAd->MlmeAux.Channel;

	/* Change to AP channel */
	if ((pAd->CommonCfg.CentralChannel > pAd->CommonCfg.Channel)
	    && (pAd->MlmeAux.HtCapability.HtCapInfo.ChannelWidth == BW_40)) {
	    	rf_channel = pAd->CommonCfg.CentralChannel;
		rf_bw = BW_40;
		ext_ch = EXTCHA_ABOVE;
	}
	else if ((pAd->CommonCfg.CentralChannel < pAd->CommonCfg.Channel)
			&& (pAd->MlmeAux.HtCapability.HtCapInfo.ChannelWidth == BW_40))
	{
	    	rf_channel = pAd->CommonCfg.CentralChannel;
		rf_bw = BW_40;
		ext_ch = EXTCHA_BELOW;
	} else
	{
	    	rf_channel = pAd->CommonCfg.CentralChannel;
		rf_bw = BW_20;
		ext_ch = EXTCHA_NONE;
	}

	AsicSetChannel(pAd, rf_channel, rf_bw, ext_ch, false);

	DBGPRINT(RT_DEBUG_TRACE,
		 ("%s():BW_%s, CtrlChannel=%d, CentralChannel=%d\n",
		  __FUNCTION__, (rf_bw == BW_40 ? "40" : "20"),
		  pAd->CommonCfg.Channel,
		  pAd->CommonCfg.CentralChannel));

	/* Save BBP_R66 value, it will be used in RTUSBResumeMsduTransmission */
	mt7612u_phy_bbp_get_agc(pAd, &pAd->BbpTuning.R66CurrentValue, RX_CHAIN_0);
}

/* IRQL = DISPATCH_LEVEL */
VOID MaintainBssTable(
	IN struct rtmp_adapter *pAd,
	IN OUT BSS_TABLE *Tab,
	IN ULONG MaxRxTimeDiff,
	IN u8 MaxSameRxTimeCount)
{
	u8 i, j;
	u8 total_bssNr = Tab->BssNr;
	bool	bDelEntry = false;
	ULONG	now_time = 0;

	for (i = 0; i < total_bssNr; i++)
	{
		BSS_ENTRY *pBss = &Tab->BssEntry[i];

		bDelEntry = false;
		if (pBss->LastBeaconRxTimeA != pBss->LastBeaconRxTime)
		{
			pBss->LastBeaconRxTimeA = pBss->LastBeaconRxTime;
			pBss->SameRxTimeCount = 0;
		}
		else
			pBss->SameRxTimeCount++;

		NdisGetSystemUpTime(&now_time);
		if (RTMP_TIME_AFTER(now_time, pBss->LastBeaconRxTime + (MaxRxTimeDiff * OS_HZ)))
			bDelEntry = true;
		else if (pBss->SameRxTimeCount > MaxSameRxTimeCount)
			bDelEntry = true;

		if (OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED)
			&& NdisEqualMemory(pBss->Ssid, pAd->CommonCfg.Ssid, pAd->CommonCfg.SsidLen))
			bDelEntry = false;

		if (bDelEntry)
		{
			u8 *pOldAddr = NULL;

			for (j = i; j < total_bssNr - 1; j++)
			{
				pOldAddr = Tab->BssEntry[j].pVarIeFromProbRsp;
				memmove(&(Tab->BssEntry[j]), &(Tab->BssEntry[j + 1]), sizeof(BSS_ENTRY));
				if (pOldAddr)
				{
					RTMPZeroMemory(pOldAddr, MAX_VIE_LEN);
					memmove(pOldAddr,
								   Tab->BssEntry[j + 1].pVarIeFromProbRsp,
								   Tab->BssEntry[j + 1].VarIeFromProbeRspLen);
					Tab->BssEntry[j].pVarIeFromProbRsp = pOldAddr;
				}
			}

			pOldAddr = Tab->BssEntry[total_bssNr - 1].pVarIeFromProbRsp;
			memset(&(Tab->BssEntry[total_bssNr - 1]), 0, sizeof(BSS_ENTRY));
			if (pOldAddr)
			{
				RTMPZeroMemory(pOldAddr, MAX_VIE_LEN);
			}

			total_bssNr -= 1;
		}
	}
	Tab->BssNr = total_bssNr;
}

VOID AdjustChannelRelatedValue(
	IN struct rtmp_adapter *pAd,
	OUT u8 *pBwFallBack,
	IN unsigned short ifIndex,
	IN bool BandWidth,
	IN u8 PriCh,
	IN u8 ExtraCh)
{
	u8 rf_channel;
	UINT8 rf_bw = BW_20, ext_ch;


	// TODO: shiang-6590, this function need to revise to make sure two purpose can achieved!
	//	1. Channel-binding rule between STA and P2P-GO mode
	//	2. Stop MAC Tx/Rx when bandwidth change

	*pBwFallBack = 0;

	if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS))
		return;

	DBGPRINT(RT_DEBUG_TRACE, ("%s():CentralChannel=%d, Channel=%d, ChannelWidth=%d\n",
			__FUNCTION__, ExtraCh, PriCh, BandWidth));

	pAd->CommonCfg.CentralChannel = ExtraCh;
	pAd->CommonCfg.Channel = PriCh;
	/* Change to AP channel */
	if ((pAd->CommonCfg.CentralChannel > pAd->CommonCfg.Channel) && (BandWidth == BW_40))
	{
		rf_channel = pAd->CommonCfg.CentralChannel;
		rf_bw = BW_40;
		ext_ch = EXTCHA_ABOVE;
	}
	else if ((pAd->CommonCfg.CentralChannel < pAd->CommonCfg.Channel) && (BandWidth == BW_40))
	{
		rf_channel = pAd->CommonCfg.CentralChannel;
		rf_bw = BW_40;
		ext_ch = EXTCHA_BELOW;
	}
	else
	{
		rf_channel = pAd->CommonCfg.Channel;
		rf_bw = BW_20;
		ext_ch = EXTCHA_NONE;
	}

	pAd->CommonCfg.vht_cent_ch = ExtraCh;
	if (rf_bw == BW_40 &&
		pAd->StaActive.SupportedPhyInfo.bVhtEnable == true &&
		pAd->StaActive.SupportedPhyInfo.vht_bw == VHT_BW_80) {
		rf_bw = BW_80;
		rf_channel = pAd->CommonCfg.vht_cent_ch;
	}
	DBGPRINT(RT_DEBUG_OFF, ("%s(): Input BW=%d, rf_channel=%d, vht_bw=%d, Channel=%d, vht_cent_ch=%d!\n",
				__FUNCTION__, rf_bw, rf_channel, pAd->CommonCfg.vht_bw, pAd->CommonCfg.Channel,
				pAd->CommonCfg.vht_cent_ch));

	mt7612u_bbp_set_bw(pAd, rf_bw);
	mt7612u_bbp_set_ctrlch(pAd, ext_ch);
	mt7612u_mac_set_ctrlch(pAd, ext_ch);

	AsicSetChannel(pAd, rf_channel, rf_bw, ext_ch, false);

	DBGPRINT(RT_DEBUG_TRACE,
				 ("%s():BW_%s, RF-Ch=%d, CtrlCh=%d, HT-CentralCh=%d\n",
				__FUNCTION__, (rf_bw == BW_80 ? "80" : (rf_bw == BW_40 ? "40": "20")),
				pAd->LatchRfRegs.Channel,
				pAd->CommonCfg.Channel,
				pAd->CommonCfg.CentralChannel));
	DBGPRINT(RT_DEBUG_TRACE, ("VHT-CentralCh=%d\n", pAd->CommonCfg.vht_cent_ch));

	DBGPRINT(RT_DEBUG_TRACE, ("AdjustChannelRelatedValue ==> not any connection !!!\n"));
}

