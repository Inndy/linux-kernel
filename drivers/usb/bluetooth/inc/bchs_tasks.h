#ifndef BCHS_TASKS_H__
#define BCHS_TASKS_H__
/*******************************************************************************

                © Cambridge Silicon Radio Ltd, 2006

                All rights reserved

FILE:           bchs_tasks.h

DESCRIPTION:    BCHS task/queue definitions

REVISION:       $Revision: #7 $

*******************************************************************************/

#include "sched/sched.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Make sure that the correct defines are set for BlueStack */
#if !defined(BUILD_FOR_HOST)
#define BUILD_FOR_HOST
#endif

#if !defined(INSTALL_L2CAP_MODULE)
#define INSTALL_L2CAP_MODULE
#endif

#if !defined(INSTALL_SDP_MODULE)
#define INSTALL_SDP_MODULE
#endif

#if !defined(EXCLUDE_BCHS_RFC_MODULE)
#if !defined(INSTALL_RFCOMM_MODULE)
#define INSTALL_RFCOMM_MODULE
#endif
#else
#undef INSTALL_RFCOMM_MODULE
#endif

#if !defined(EXCLUDE_SCO_MODULE)
#if !defined(INSTALL_DM_SYNC_MODULE)
#define INSTALL_DM_SYNC_MODULE
#endif
#else
#undef INSTALL_DM_SYNC_MODULE
#endif

/* PAN can only run on HCI firmware */
#if !defined(EXCLUDE_BNEP_MODULE) && defined(STACK_ON_HOST)
#if !defined(INSTALL_PAN_MODULE)
#define INSTALL_PAN_MODULE
#endif
#else
#if !defined(EXCLUDE_BNEP_MODULE)
#define EXCLUDE_BNEP_MODULE
#endif
#undef INSTALL_PAN_MODULE
#endif

/* First the common core stack parts */
extern void init_dm(void **gash);
extern void dm_iface_handler(void **gash);
#define DM_INIT init_dm
#define DM_TASK dm_iface_handler

extern void init_l2cap_proxy(void **gash);
extern void l2cap_proxy_iface_handler(void **gash);
#define L2CAP_PROXY_INIT init_l2cap_proxy
#ifdef ENABLE_SHUTDOWN
extern void deinit_l2cap_proxy(void **gash);
#define L2CAP_PROXY_DEINIT deinit_l2cap_proxy
#else
#define L2CAP_PROXY_DEINIT NULL
#endif
#define L2CAP_PROXY_TASK l2cap_proxy_iface_handler

#if !defined(EXCLUDE_BCHS_RFC_MODULE)
extern void init_rfcomm_proxy(void **gash);
extern void rfcomm_proxy_iface_handler(void **gash);
#define RFCOMM_PROXY_INIT init_rfcomm_proxy
#ifdef ENABLE_SHUTDOWN
extern void deinit_rfcomm_proxy(void **gash);
#define RFCOMM_PROXY_DEINIT deinit_rfcomm_proxy
#else
#define RFCOMM_PROXY_DEINIT NULL
#endif
#define RFCOMM_PROXY_TASK rfcomm_proxy_iface_handler
#else
#define RFCOMM_PROXY_INIT NULL
#define RFCOMM_PROXY_DEINIT NULL
#define RFCOMM_PROXY_TASK NULL
#endif

extern void init_sdp(void **gash);
extern void sdp_sda_handler (void **gash);

#define SDP_INIT init_sdp
#ifdef ENABLE_SHUTDOWN
extern void deinit_sdp_sda(void **gash);
#define SDP_DEINIT deinit_sdp_sda
#else
#define SDP_DEINIT NULL
#endif
#define SDP_TASK sdp_sda_handler

#ifdef STACK_ON_HOST /* then the HCI stack parts */

extern void dm_hci_handler(void **gash);
extern void dm_acl_handler(void **gash);
/*#define DM_DEINIT NULL*/
#define DM_HCI_INIT NULL
#define DM_HCI_TASK dm_hci_handler
#define DM_ACL_INIT NULL
#define DM_ACL_TASK dm_acl_handler

#ifdef ENABLE_SHUTDOWN
extern void dm_hci_deinit(void **gash);
extern void dm_acl_deinit(void **gash);
#define DM_HCI_DEINIT dm_hci_deinit
#define DM_ACL_DEINIT dm_acl_deinit
#else
#define DM_HCI_DEINIT NULL
#define DM_ACL_DEINIT NULL
#endif

extern void init_l2cap(void **gash);
extern void l2cap_iface_handler(void **gash);
extern void l2cap_dm_handler(void **gash);
#define L2CAP_INIT init_l2cap

#ifdef ENABLE_SHUTDOWN
extern void l2cap_deinit(void **gash);
extern void l2cap_dm_deinit(void **gash);
#define L2CAP_DEINIT l2cap_deinit
#define L2CAP_DM_DEINIT l2cap_dm_deinit
#else
#define L2CAP_DEINIT NULL
#define L2CAP_DM_DEINIT NULL
#endif

#define L2CAP_TASK l2cap_iface_handler
#define L2CAP_DM_INIT NULL
#define L2CAP_DM_TASK l2cap_dm_handler

#if !defined(EXCLUDE_BCHS_RFC_MODULE)
extern void init_rfcomm(void **gash);
extern void rfcomm_iface_handler(void **gash);
extern void rfcomm_l2cap_handler(void **gash);
extern void rfcomm_dm_handler(void **gash);
#define RFCOMM_INIT init_rfcomm
#define RFCOMM_TASK rfcomm_iface_handler
#define RFCOMM_L2CAP_INIT NULL
#define RFCOMM_L2CAP_TASK rfcomm_l2cap_handler
#define RFCOMM_DM_INIT NULL
#define RFCOMM_DM_TASK rfcomm_dm_handler

#ifdef ENABLE_SHUTDOWN
extern void rfcomm_deinit(void **gash);
extern void rfcomm_l2cap_deinit(void **gash);
extern void rfcomm_dm_deinit(void **gash);
#define RFCOMM_DEINIT rfcomm_deinit
#define RFCOMM_L2CAP_DEINIT rfcomm_l2cap_deinit
#define RFCOMM_DM_DEINIT rfcomm_dm_deinit
#else
#define RFCOMM_DEINIT NULL
#define RFCOMM_L2CAP_DEINIT NULL
#define RFCOMM_DM_DEINIT NULL
#endif
#else
#define RFCOMM_INIT NULL
#define RFCOMM_TASK NULL
#define RFCOMM_L2CAP_INIT NULL
#define RFCOMM_L2CAP_TASK NULL
#define RFCOMM_DM_INIT NULL
#define RFCOMM_DM_TASK NULL
#define RFCOMM_DEINIT NULL
#define RFCOMM_L2CAP_DEINIT NULL
#define RFCOMM_DM_DEINIT NULL
#endif

extern void sdp_l2cap_handler(void **gash);
#define SDP_L2CAP_INIT NULL
#define SDP_L2CAP_TASK sdp_l2cap_handler

extern void sdp_l2cap_deinit(void **gash);
#ifdef ENABLE_SHUTDOWN
#define SDP_L2CAP_DEINIT sdp_l2cap_deinit
#else
#define SDP_L2CAP_DEINIT NULL
#endif


#else /* !STACK_ON_HOST */ /* and the RFCOMM stack parts */

#define DM_HCI_INIT NULL
#define DM_HCI_DEINIT NULL
#define DM_HCI_TASK NULL
#define DM_ACL_INIT NULL
#define DM_ACL_DEINIT NULL
#define DM_ACL_TASK NULL

#define L2CAP_INIT NULL
#define L2CAP_DEINIT NULL
#define L2CAP_TASK NULL
#define L2CAP_DM_INIT NULL
#define L2CAP_DM_DEINIT NULL
#define L2CAP_DM_TASK NULL

#define RFCOMM_INIT NULL
#define RFCOMM_DEINIT NULL
#define RFCOMM_TASK NULL
#define RFCOMM_L2CAP_INIT NULL
#define RFCOMM_L2CAP_DEINIT NULL
#define RFCOMM_L2CAP_TASK NULL
#define RFCOMM_DM_INIT NULL
#define RFCOMM_DM_DEINIT NULL
#define RFCOMM_DM_TASK NULL

#define SDP_L2CAP_INIT NULL
#define SDP_L2CAP_TASK NULL
#define SDP_L2CAP_DEINIT NULL

#endif

#ifdef ENABLE_SHUTDOWN
extern void deinit_dm_iface(void **gash);
#define DM_DEINIT deinit_dm_iface
#else
#define DM_DEINIT NULL
#endif

#if !defined(EXCLUDE_BCCMD_MODULE)
extern void bccmd_init(void **gash);
#ifndef STACK_ON_HOST
#ifdef ENABLE_SHUTDOWN
extern void bccmd_deinit(void **gash);
#define BCCMD_DEINIT bccmd_deinit
#else
#define BCCMD_DEINIT NULL
#endif
#else
#define BCCMD_DEINIT NULL
#endif
extern void bccmd_handler(void **gash);
#define BCCMD_INIT bccmd_init
#define BCCMD_TASK bccmd_handler
#else
#define BCCMD_INIT NULL
#define BCCMD_DEINIT NULL
#define BCCMD_TASK NULL
#endif

/* BCHS Profiles */

#if !defined(EXCLUDE_CM_MODULE)
extern void cm_init(void **gash);
extern void cm_handler(void **gash);
extern void sc_init(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void cm_deinit(void **gash);
#define CM_DEINIT cm_deinit
extern void sc_deinit(void **gash);
#define SC_DEINIT sc_deinit
#else
#define CM_DEINIT NULL
#define SC_DEINIT NULL
#endif
extern void sc_handler(void **gash);
#define CM_INIT cm_init
#define CM_TASK cm_handler
#define SC_INIT sc_init
#define SC_TASK sc_handler
#else
#define CM_INIT NULL
#define CM_DEINIT NULL
#define CM_TASK NULL
#define SC_INIT NULL
#define SC_DEINIT NULL
#define SC_TASK NULL
#endif

#if !defined(EXCLUDE_DG_MODULE)
extern void dg_init(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void dg_deinit(void **gash);
#define DG_DEINIT dg_deinit
#else
#define DG_DEINIT NULL
#endif
extern void dg_handler(void **gash);
#define DG_INIT dg_init
#define DG_TASK dg_handler
#else
#define DG_INIT NULL
#define DG_DEINIT NULL
#define DG_TASK NULL
#endif

#if !defined(EXCLUDE_AT_MODULE)
extern void at_init(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void at_deinit(void **gash);
#define AT_DEINIT at_deinit
#else
#define AT_DEINIT NULL
#endif
extern void at_handler(void **gash);
#define AT_INIT at_init
#define AT_TASK at_handler
#else
#define AT_INIT NULL
#define AT_DEINIT NULL
#define AT_TASK NULL
#endif

#if !defined(EXCLUDE_DUNC_MODULE)
extern void dunc_init(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void dunc_deinit(void **gash);
#define DUNC_DEINIT dunc_deinit
#else
#define DUNC_DEINIT NULL
#endif
extern void dunc_handler(void **gash);
#define DUNC_INIT dunc_init
#define DUNC_TASK dunc_handler
#else
#define DUNC_INIT NULL
#define DUNC_DEINIT NULL
#define DUNC_TASK NULL
#endif

#if !defined(EXCLUDE_AG_MODULE)
extern void ag_init(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void ag_deinit(void **gash);
#define AG_DEINIT ag_deinit
#else
#define AG_DEINIT NULL
#endif
extern void ag_handler(void **gash);
#define AG_INIT ag_init
#define AG_TASK ag_handler
#else
#define AG_INIT NULL
#define AG_DEINIT NULL
#define AG_TASK NULL
#endif

#if !defined(EXCLUDE_OPS_MODULE)
extern void OpsInit(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void OpsDeinit(void **gash);
#define OPS_DEINIT OpsDeinit
#else
#define OPS_DEINIT NULL
#endif
extern void OpsHandler(void **gash);
#define OPS_INIT OpsInit
#define OPS_TASK OpsHandler
#else
#define OPS_INIT NULL
#define OPS_DEINIT NULL
#define OPS_TASK NULL
#endif

#if !defined(EXCLUDE_OPC_MODULE)
extern void OpcInit(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void OpcDeinit(void **gash);
#define OPC_DEINIT OpcDeinit
#else
#define OPC_DEINIT NULL
#endif
extern void OpcHandler(void **gash);
#define OPC_INIT OpcInit
#define OPC_TASK OpcHandler
#else
#define OPC_INIT NULL
#define OPC_DEINIT NULL
#define OPC_TASK NULL
#endif

#if !defined(EXCLUDE_SYNCS_MODULE)
extern void SyncsInit(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void SyncsDeinit(void **gash);
#define SYNCS_DEINIT SyncsDeinit
#else
#define SYNCS_DEINIT NULL
#endif
extern void SyncsHandler(void **gash);
#define SYNCS_INIT SyncsInit
#define SYNCS_TASK SyncsHandler
#else
#define SYNCS_INIT NULL
#define SYNCS_DEINIT NULL
#define SYNCS_TASK NULL
#endif

#if !defined(EXCLUDE_SMLC_MODULE)
extern  void    SmlcInit(void **gash);
#ifdef ENABLE_SHUTDOWN
extern  void    SmlcDeinit(void **gash);
#define SMLC_DEINIT		SmlcDeinit
#else
#define SMLC_DEINIT		NULL
#endif
extern  void    SmlcHandler(void **gash);
#define SMLC_INIT			SmlcInit
#define	SMLC_TASK			SmlcHandler
#else
#define SMLC_INIT			NULL
#define SMLC_DEINIT		NULL
#define	SMLC_TASK			NULL
#endif


#if !defined(EXCLUDE_SMLS_MODULE)
extern  void    SmlsInit(void **gash);
#ifdef ENABLE_SHUTDOWN
extern  void    SmlsDeinit(void **gash);
#define SMLS_DEINIT		SmlsDeinit
#else
#define SMLS_DEINIT		NULL
#endif
extern  void    SmlsHandler(void **gash);
#define SMLS_INIT			SmlsInit
#define	SMLS_TASK			SmlsHandler
#else
#define SMLS_INIT			NULL
#define SMLS_DEINIT		NULL
#define	SMLS_TASK			NULL
#endif
#if !defined(EXCLUDE_IWU_MODULE)
extern void iwu_init(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void iwu_deinit(void **gash);
#define IWU_DEINIT iwu_deinit
#else
#define IWU_DEINIT NULL
#endif
extern void iwu_handler(void **gash);
#define IWU_INIT iwu_init
#define IWU_TASK iwu_handler
#else
#define IWU_INIT NULL
#define IWU_DEINIT NULL
#define IWU_TASK NULL
#endif

#if !defined(EXCLUDE_SPP_MODULE)
extern void spp_init(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void spp_deinit(void **gash);
#define SPP_DEINIT spp_deinit
#else
#define SPP_DEINIT NULL
#endif
extern void spp_handler(void **gash);
#define SPP_INIT spp_init
#define SPP_TASK spp_handler
#else
#define SPP_INIT NULL
#define SPP_DEINIT NULL
#define SPP_TASK NULL
#endif

#if !defined(EXCLUDE_FAX_MODULE)
extern void fax_init(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void fax_deinit(void **gash);
#define FAX_DEINIT fax_deinit
#else
#define FAX_DEINIT NULL
#endif
extern void fax_handler(void **gash);
#define FAX_INIT fax_init
#define FAX_TASK fax_handler
#else
#define FAX_INIT NULL
#define FAX_DEINIT NULL
#define FAX_TASK NULL
#endif

#if !defined(EXCLUDE_FM_MODULE)
extern void FmInit(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void FmDeinit(void **gash);
#define FM_DEINIT FmDeinit
#else
#define FM_DEINIT NULL
#endif
extern void FmHandler(void **gash);
#define FM_INIT FmInit
#define FM_TASK FmHandler
#else
#define FM_INIT NULL
#define FM_DEINIT NULL
#define FM_TASK NULL
#endif

#if !defined(EXCLUDE_HQ_MODULE)
extern void HqInit(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void HqDeinit(void **gash);
#define HQ_DEINIT HqDeinit
#else
#define HQ_DEINIT NULL
#endif
extern void HqHandler(void **gash);
#define HQ_INIT HqInit
#define HQ_TASK HqHandler
#else
#define HQ_INIT NULL
#define HQ_DEINIT NULL
#define HQ_TASK NULL
#endif

#if !defined(EXCLUDE_HFG_MODULE)
extern void hfg_init(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void hfg_deinit(void **gash);
#define HFG_DEINIT hfg_deinit
#else
#define HFG_DEINIT NULL
#endif
extern void hfg_handler(void **gash);
#define HFG_INIT hfg_init
#define HFG_TASK hfg_handler
#else
#define HFG_INIT NULL
#define HFG_DEINIT NULL
#define HFG_TASK NULL
#endif

#if !defined(EXCLUDE_HF_MODULE)
extern void hf_init(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void hf_deinit(void **gash);
#define HF_DEINIT hf_deinit
#else
#define HF_DEINIT NULL
#endif
extern void hf_handler(void **gash);
#define HF_INIT hf_init
#define HF_TASK hf_handler
#else
#define HF_INIT NULL
#define HF_DEINIT NULL
#define HF_TASK NULL
#endif

#if !defined(EXCLUDE_PAC_MODULE)
extern void PacInit(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void PacDeinit(void **gash);
#define PAC_DEINIT PacDeinit
#else
#define PAC_DEINIT NULL
#endif
extern void PacHandler(void **gash);
#define PAC_INIT PacInit
#define PAC_TASK PacHandler
#else
#define PAC_INIT NULL
#define PAC_DEINIT NULL
#define PAC_TASK NULL
#endif

#if !defined(EXCLUDE_PAS_MODULE)
extern void	PasInit(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void	PasDeinit(void **gash);
#define PAS_DEINIT PasDeinit
#else
#define PAS_DEINIT NULL
#endif
extern void PasHandler(void **gash);
#define PAS_INIT PasInit
#define PAS_TASK PasHandler
#else
#define PAS_INIT NULL
#define PAS_DEINIT NULL
#define PAS_TASK NULL
#endif

#if !defined(EXCLUDE_TCS_MODULE)
extern void tcs_init(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void tcs_deinit(void **gash);
#define TCS_DEINIT tcs_deinit
#else
#define TCS_DEINIT NULL
#endif
extern void tcs_handler(void **gash);
#define TCS_INIT tcs_init
#define TCS_TASK tcs_handler
#else
#define TCS_INIT NULL
#define TCS_DEINIT NULL
#define TCS_TASK NULL
#endif

#if !defined(EXCLUDE_FTS_MODULE)
extern void FtsInit(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void FtsDeinit(void **gash);
#define FTS_DEINIT FtsDeinit
#else
#define FTS_DEINIT NULL
#endif
extern void FtsHandler(void **gash);
#define FTS_INIT FtsInit
#define FTS_TASK FtsHandler
#else
#define FTS_INIT NULL
#define FTS_DEINIT NULL
#define FTS_TASK NULL
#endif

#if !defined(EXCLUDE_BNEP_MODULE)
extern void BnepInit(void **gash);
extern void BslInit(void **gash);
extern void ProxyBslInit(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void BnepDeinit(void **gash);
extern void BslDeinit(void **gash);
extern void ProxyBslDeinit(void **gash);
#define BNEP_DEINIT BnepDeinit
#define BSL_DEINIT BslDeinit
#define PROXY_BSL_DEINIT ProxyBslDeinit
#else
#define BNEP_DEINIT NULL
#define BSL_DEINIT NULL
#define PROXY_BSL_DEINIT NULL
#endif
extern void BnepHandler(void **gash);
extern void BslHandler(void **gash);
extern void ProxyBslHandler(void **gash);
#define BNEP_INIT BnepInit
#define BNEP_TASK BnepHandler
#define BSL_INIT BslInit
#define BSL_TASK BslHandler
#define PROXY_BSL_INIT ProxyBslInit
#define PROXY_BSL_TASK ProxyBslHandler
#else
#define BNEP_INIT NULL
#define BNEP_DEINIT NULL
#define BNEP_TASK NULL
#define BSL_INIT NULL
#define BSL_DEINIT NULL
#define BSL_TASK NULL
#define PROXY_BSL_INIT NULL
#define PROXY_BSL_DEINIT NULL
#define PROXY_BSL_TASK NULL
#endif

#if !defined(EXCLUDE_FTS_MODULE) || !defined(EXCLUDE_LAPC_MODULE) || !defined(EXCLUDE_BNEP_MODULE) || !defined(EXCLUDE_DUNC_MODULE)
extern void IpInit(void ** gash);
extern void IpHandler(void ** gash);
extern void IcmpInit(void ** gash);
extern void IcmpHandler(void ** gash);
extern void UdpInit(void ** gash);
extern void UdpHandler(void ** gash);
extern void DhcpInit(void ** gash);
extern void DhcpHandler(void ** gash);
extern void TftpInit(void ** gash);
extern void TftpHandler(void ** gash);
#define IP_INIT IpInit
#define IP_DEINIT NULL
#define IP_TASK IpHandler
#define ICMP_INIT IcmpInit
#define ICMP_DEINIT NULL
#define ICMP_TASK IcmpHandler
#define UDP_INIT UdpInit
#define UDP_DEINIT NULL
#define UDP_TASK UdpHandler
#define DHCP_INIT DhcpInit
#define DHCP_DEINIT NULL
#define DHCP_TASK DhcpHandler
#define TFTP_INIT TftpInit
#define TFTP_DEINIT NULL
#define TFTP_TASK TftpHandler
#else
#define IP_INIT NULL
#define IP_DEINIT NULL
#define IP_TASK NULL
#define ICMP_INIT NULL
#define ICMP_DEINIT NULL
#define ICMP_TASK NULL
#define UDP_INIT NULL
#define UDP_DEINIT NULL
#define UDP_TASK NULL
#define DHCP_INIT NULL
#define DHCP_DEINIT NULL
#define DHCP_TASK NULL
#define TFTP_INIT NULL
#define TFTP_DEINIT NULL
#define TFTP_TASK NULL
#endif

#if !defined(EXCLUDE_BIPC_MODULE)
extern void    BipcInit(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void    BipcDeinit(void **gash);
#define BIPC_DEINIT BipcDeinit
#else
#define BIPC_DEINIT NULL
#endif
extern void BipcHandler(void **gash);
#define BIPC_INIT BipcInit
#define BIPC_TASK BipcHandler
#else
#define BIPC_INIT NULL
#define BIPC_DEINIT NULL
#define BIPC_TASK NULL
#endif

#if !defined(EXCLUDE_BIPS_MODULE)
extern void BipsInit(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void BipsDeinit(void **gash);
#define BIPS_DEINIT BipsDeinit
#else
#define BIPS_DEINIT NULL
#endif
extern void BipsHandler(void **gash);
#define BIPS_INIT BipsInit
#define BIPS_TASK BipsHandler
#else
#define BIPS_INIT NULL
#define BIPS_DEINIT NULL
#define BIPS_TASK NULL
#endif

#if !defined(EXCLUDE_FTC_MODULE)
extern void FtcInit(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void FtcDeinit(void **gash);
#define FTC_DEINIT FtcDeinit
#else
#define FTC_DEINIT NULL
#endif
extern void FtcHandler(void **gash);
#define FTC_INIT FtcInit
#define FTC_TASK FtcHandler
#else
#define FTC_INIT NULL
#define FTC_DEINIT NULL
#define FTC_TASK NULL
#endif

#if !defined(EXCLUDE_LAPC_MODULE)
extern void LapcInit(void ** gash);
#ifdef ENABLE_SHUTDOWN
extern void LapcDeinit(void ** gash);
#define LAPC_DEINIT LapcDeinit
#else
#define LAPC_DEINIT NULL
#endif
extern void LapcHandler(void ** gash);
#define LAPC_INIT LapcInit
#define LAPC_TASK LapcHandler
#else
#define LAPC_INIT NULL
#define LAPC_DEINIT NULL
#define LAPC_TASK NULL
#endif

#if !defined(EXCLUDE_PPP_MODULE) || !defined(EXCLUDE_LAPC_MODULE)
extern void PppInit(void ** gash);
#ifdef ENABLE_SHUTDOWN
extern void PppDeinit(void ** gash);
#define PPP_DEINIT PppDeinit
#else
#define PPP_DEINIT NULL
#endif
extern void PppHandler(void ** gash);
#define PPP_INIT PppInit
#define PPP_TASK PppHandler
#else
#define PPP_INIT NULL
#define PPP_DEINIT NULL
#define PPP_TASK NULL
#endif

#if !defined(EXCLUDE_BPP_MODULE)
extern void BppInit(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void BppDeinit(void **gash);
#define BPP_DEINIT BppDeinit
#else
#define BPP_DEINIT NULL
#endif
extern void BppHandler(void **gash);
#define BPP_INIT BppInit
#define BPP_TASK BppHandler
#else
#define BPP_INIT NULL
#define BPP_DEINIT NULL
#define BPP_TASK NULL
#endif

#if !defined(EXCLUDE_AV_MODULE)
extern void avInit(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void avDeinit(void **gash);
#define AV_DEINIT avDeinit
#else
#define AV_DEINIT NULL
#endif
extern void avHandler(void **gash);
#define AV_INIT avInit
#define AV_TASK avHandler
#else
#define AV_INIT NULL
#define AV_DEINIT NULL
#define AV_TASK NULL
#endif

#if !defined(EXCLUDE_AVRCP_MODULE)
extern void avrcpInit(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void avrcpDeinit(void **gash);
#define AVRCP_DEINIT avrcpDeinit
#else
#define AVRCP_DEINIT NULL
#endif
extern void avrcpHandler(void **gash);
#define AVRCP_INIT avrcpInit
#define AVRCP_TASK avrcpHandler
#else
#define AVRCP_INIT NULL
#define AVRCP_DEINIT NULL
#define AVRCP_TASK NULL
#endif

#if !defined(EXCLUDE_SAPS_MODULE)
extern void saps_init(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void saps_deinit(void **gash);
#define SAPS_DEINIT saps_deinit
#else
#define SAPS_DEINIT NULL
#endif
extern void saps_handler(void **gash);
#define SAPS_INIT saps_init
#define SAPS_TASK saps_handler
#else
#define SAPS_INIT NULL
#define SAPS_DEINIT NULL
#define SAPS_TASK NULL
#endif

#if !defined(EXCLUDE_SAPC_MODULE)
extern void sapc_init(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void sapc_deinit(void **gash);
#define SAPC_DEINIT sapc_deinit
#else
#define SAPC_DEINIT NULL
#endif
extern void sapc_handler(void **gash);
#define SAPC_INIT sapc_init
#define SAPC_TASK sapc_handler
#else
#define SAPC_INIT NULL
#define SAPC_DEINIT NULL
#define SAPC_TASK NULL
#endif

extern void SdInit(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void SdDeinit(void **gash);
#define SD_DEINIT SdDeinit
#else
#define SD_DEINIT NULL
#endif
extern void SdHandler(void **gash);
#define SD_INIT SdInit
#define SD_TASK SdHandler

#if !defined(EXCLUDE_BPPS_MODULE)
extern void BppsInit(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void BppsDeinit(void **gash);
#define BPPS_DEINIT BppsDeinit
#else
#define BPPS_DEINIT NULL
#endif
extern void BppsHandler(void **gash);
#define BPPS_INIT BppsInit
#define BPPS_TASK BppsHandler
#else
#define BPPS_INIT NULL
#define BPPS_DEINIT NULL
#define BPPS_TASK NULL
#endif

#if !defined(EXCLUDE_HCRP_MODULE)
extern void hcrpInit(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void hcrpDeinit(void **gash);
#define HCRP_DEINIT            hcrpDeinit
#else
#define HCRP_DEINIT            NULL
#endif
extern void hcrpHandler(void **gash);
#define HCRP_INIT hcrpInit
#define HCRP_TASK hcrpHandler
#else
#define HCRP_INIT NULL
#define HCRP_DEINIT NULL
#define HCRP_TASK NULL
#endif

#if !defined(EXCLUDE_TEST_MODULE)
extern void init_test(void ** gash);
#ifdef ENABLE_SHUTDOWN
extern void deinit_test(void ** gash);
#define TEST_DEINIT deinit_test
#else
#define TEST_DEINIT NULL
#endif
extern void test_handler(void ** gash);
#define TEST_INIT init_test
#define TEST_TASK test_handler
#else
#define TEST_INIT NULL
#define TEST_TASK NULL
#endif

#if !defined(EXCLUDE_HIDH_MODULE)
extern void hidh_init(void **gash);
extern void hidh_user_init(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void hidh_deinit(void **gash);
extern void hidh_user_deinit(void **gash);
#define HIDH_DEINIT hidh_deinit
#define HIDH_USER_DEINIT hidh_user_deinit
#else
#define HIDH_DEINIT NULL
#define HIDH_USER_DEINIT NULL
#endif
extern void hidh_handler(void **gash);
extern void hidh_user_handler(void **gash);
#define HIDH_INIT hidh_init
#define HIDH_USER_INIT hidh_user_init

#define HIDH_TASK hidh_handler
#define HIDH_USER_TASK hidh_user_handler
#else
#define HIDH_INIT NULL
#define HIDH_USER_INIT NULL
#define HIDH_DEINIT NULL
#define HIDH_USER_DEINIT NULL
#define HIDH_TASK NULL
#define HIDH_USER_TASK NULL
#endif

extern void extra_task_init(void **gash);
#ifdef ENABLE_SHUTDOWN
extern void extra_task_deinit(void **gash);
#define EXTRA_TASK_DEINIT extra_task_deinit
#else
#define EXTRA_TASK_DEINIT NULL
#endif
extern void extra_task_handler(void **gash);
#define EXTRA_TASK_INIT extra_task_init
#define EXTRA_TASK extra_task_handler

#if !defined(EXCLUDE_JSR82_MODULE)
extern void jsr82_init(void **gash);
#define JSR82_INIT jsr82_init
#ifdef ENABLE_SHUTDOWN
extern void jsr82_deinit(void **gash);
#define JSR82_DEINIT jsr82_deinit
#else
#define JSR82_DEINIT NULL
#endif
extern void jsr82_handler(void **gash);
#define JSR82_TASK jsr82_handler
#else
#define JSR82_INIT NULL
#define JSR82_TASK NULL
#define JSR82_DEINIT NULL
#endif

#if !defined(EXCLUDE_HIDD_MODULE)
extern void HiddInit(void **gash);
#define HIDD_INIT HiddInit
extern void HiddHandler(void **gash);
#define HIDD_TASK HiddHandler
#ifdef ENABLE_SHUTDOWN
extern void HiddDeinit(void **gash);
#define HIDD_DEINIT HiddDeinit
#else
#define HIDD_DEINIT NULL
#endif
#else
#define HIDD_INIT NULL
#define HIDD_TASK NULL
#define HIDD_DEINIT NULL
#endif

/* Queue definitions */
#define DM_IFACEQUEUE               (0)
#define DM_ACLQUEUE                 (1)
#define DM_HCIQUEUE                 (2)
#define L2CAP_IFACEQUEUE            (3)
#define L2CAP_PROXY_IFACEQUEUE      (4)
#define L2CAP_DMQUEUE               (5)
#define RFCOMM_IFACEQUEUE           (6)
#define RFCOMM_PROXY_IFACEQUEUE     (7)
#define RFCOMM_L2CAPQUEUE           (8)
#define RFCOMM_DMQUEUE              (9)
#define BCCMD_IFACEQUEUE           (10)
#define SDP_L2CAPQUEUE             (11)
#define SDP_IFACEQUEUE             (12)
#define CM_IFACEQUEUE              (13)
#define SC_IFACEQUEUE              (14)
#define DG_IFACEQUEUE              (15)
#define AT_IFACEQUEUE              (16)
#define AG_IFACEQUEUE              (17)
#define OPS_IFACEQUEUE             (18)
#define OPC_IFACEQUEUE             (19)
#define SYNCS_IFACEQUEUE           (20)
#define IWU_IFACEQUEUE             (21)
#define SPP_IFACEQUEUE             (22)
#define FAX_IFACEQUEUE             (23)
#define HFG_IFACEQUEUE             (24)
#define HF_IFACEQUEUE              (25)
#define TCS_IFACEQUEUE             (26)
#define FTS_IFACEQUEUE             (27)
#define BNEP_QUEUE                 (28)
#define BSL_IFACEQUEUE             (29)
#define IP_QUEUE                   (30)
#define ICMP_QUEUE                 (31)
#define UDP_QUEUE                  (32)
#define DHCP_QUEUE                 (33)
#define TFTP_QUEUE                 (34)
#define BIPC_IFACEQUEUE            (35)
#define FTC_IFACEQUEUE             (36)
#define LAPC_IFACEQUEUE            (37)
#define PPP_QUEUE                  (38)
#define BPP_IFACEQUEUE             (39)
#define AV_IFACEQUEUE              (40)
#define AVRCP_IFACEQUEUE           (41)
#define SAPS_IFACEQUEUE            (42)
#define SAPC_IFACEQUEUE            (43)
#define SD_IFACEQUEUE              (44)
#define HIDH_IFACEQUEUE            (45)
#define PROXY_BSL_IFACEQUEUE       (46)
#define BPPS_IFACEQUEUE            (47)
#define HCRP_IFACEQUEUE            (48)
#define BIPS_IFACEQUEUE            (49)
#define TESTQUEUE                  (50)
#define EXTRA_IFACEQUEUE           (51)
#define SMLC_IFACEQUEUE            (52)
#define SMLS_IFACEQUEUE            (53)
#define DUNC_IFACEQUEUE            (54)
#define JSR82_IFACEQUEUE           (55)
#define HIDD_IFACEQUEUE            (56)
#define FM_IFACEQUEUE              (57)
#define HQ_IFACEQUEUE              (58)
#define PAC_IFACEQUEUE             (59)
#define PAS_IFACEQUEUE             (60)
#define NUMBER_OF_TASKS             61

#define LAPC_QUEUE LAPC_IFACEQUEUE

/* Following line is an example of an external task queue definition (modify/remove as necessary) */
#define EXTERNAL_APP_QUEUE    (0x0000 | EXTERNAL_TASK_MASK)

#ifdef __cplusplus
}
#endif

#endif
