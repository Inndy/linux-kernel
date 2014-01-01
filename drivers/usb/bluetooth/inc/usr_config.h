#ifndef _USR_CONFIG_H
#define _USR_CONFIG_H
/****************************************************************************

                (c) Cambridge Silicon Radio Ltd, 2006

                All rights reserved

FILE:           usr_config.h

DESCRIPTION:    This file contains settings that may be changed at compile time
                for different defines used throughout the CPL components.
                A number of the defines are assigned default values if not otherwise
                set from the compile environment.

REVISION:       $Revision: #9 $

****************************************************************************/

#include "bchs_tasks.h"
#include "sched/types.h"
#include "bluetooth.h"
#include "hci.h"
#include "dm_prim.h"
#include "rfcomm_prim.h"

#ifdef __cplusplus
extern "C" {
#endif

/*--------------------------------------------------------------------------
 * Defines for Service Discovery (SD) module
 *--------------------------------------------------------------------------*/

/* Number of entries the SD module will allocate for its cache during
 * initialization. Remember that it can be changed during runtime by
 * using the "SdRegisterReqSend" call */
#define DEFAULT_NUM_OF_ENTRIES_IN_SD_CACHE                      (5)

/* Time in milliseconds the SD will inquire for new devices before
 * requesting names. A value of 0 will result in reading of a remote
 * name to be performed momentarily after the detection of an unknown
 * device. A value larger than 0 will result in devices beeing
 * buffered for the chosen duration. After this duration, remote names
 * will be read from the devices in the buffer starting with the
 * device with the highest RSSI. Please refer to the documentation for
 * further details */
#define SD_BUFFER_INQ_TO                                        (0)

/* Time-out in milliseconds for reading the name of a remote device */
#define SD_READ_REMOTE_NAME_TO                                  (5000)

/*--------------------------------------------------------------------------
 * Defines for Class Of Device (COD) and service/friendly names
 *--------------------------------------------------------------------------*/

#define MINOR_DEVICE_CLASS_FILTER   0x0000FC
#define MAJOR_DEVICE_CLASS_FILTER   0x001F00

/* Use to define the Major and Minor device class fields of the Class
 * of Device. */
#define MAJOR_MINOR_DEVICE_CLASS               0x000208 /* (PHONE_MAJOR_DEVICE_MASK + CORDLESS_MINOR_DEVICE_MASK) */

/* Max name length of the service name for a registered service */
#ifndef MAX_SERVICE_NAME_LEN
#define MAX_SERVICE_NAME_LEN                                    (50)
#endif


/* This is the maximum amount of chars to be passed between the
 * profile layer and the application for the remote name if the name
 * is shorter than the max length, the name must be zero terminated.
 * If a name longer than 40 chars (20 words) is sent to the chip during
 * bootstrap, the name will be ignored. Max length for the frindly name
 * is defined as 248 octets in HCI, but this will be limited in BCHS
 * by this define.
 */
#ifndef MAX_FRIENDLY_NAME_LEN
#define MAX_FRIENDLY_NAME_LEN                                   (40)
#endif


/*--------------------------------------------------------------------------
  Defines for BCHS L2CAP connection
  ----------------------------------------------------------------------------*/
/* Define the number of simultaneously L2CAP connections, that can be
 * used at the same time. */
#define MAX_NUM_OF_SIMULTANIOUS_L2CAP_CONNECTIONS               (6)

/* For L2CAP the frame size must be >= 48 bytes. It is recomented,
 * due to speed optimization, that the frame  size is set to 672 */
#define PROFILE_L2CA_DEFAULT_MAX_FRAME_SIZE                     (672)

/*--------------------------------------------------------------------------
  Defines for BCHS RFCOMM connection
  ----------------------------------------------------------------------------*/

/* Define the number of simultaneously RFCOMM connections, that can be
 * used at the same time. */
#define MAX_NUMBER_OF_SIMULTANIOUS_CONNECTIONS                  (4)

/* For the RFCOMM build the frame size must be within 23 - 720.
 * It is recomented, due to speed optimization, that the frame
 * size is set to 330 unless STACK_ON_HOST is defined */
#ifdef STACK_ON_HOST
#ifndef BLUESTACK_2P0
#define PROFILE_DEFAULT_MAX_FRAME_SIZE                          (330)
#else
#define PROFILE_DEFAULT_MAX_FRAME_SIZE                          (1012)
#endif
#else
#define PROFILE_DEFAULT_MAX_FRAME_SIZE                          (330)
#endif

/* Defines how many data frames each RFCOMM connection in the CM is
 * able to accept before its buffer is fill up. The value between 1-255.
 * Setting this value low can result in a lower bandrate */
#define CM_INIT_CREDIT                                          (25)

/* Port negotation defines */
#define DEFAULT_PORT_SPEED                                      (PORT_SPEED_115200)
#define DEFAULT_DATA_BITS                                       (DATA_BITS_8)
#define DEFAULT_STOP_BITS                                       (STOP_BITS_ONE)
#define DEFAULT_PARITY                                          (PARITY_ON)
#define DEFAULT_PARITY_TYPE                                     (PARITY_TYPE_EVEN)
#define DEFAULT_FLOW_CTRL_MASK                                  (FLC_RTR_INPUT)
#define DEFAULT_XON                                             (XON_CHAR_DEFAULT)
#define DEFAULT_XOFF                                            (XOFF_CHAR_DEFAULT)
#define DEFAULT_PARAMETER_MASK                                  (0X3FFF)


/*--------------------------------------------------------------------------
  Defines for BCHS voice parameter
  ----------------------------------------------------------------------------*/
/* Voice setting parameter controls all the various settings for voice
 * connection. These settings apply to all voice connection, and
 * CANNOT be set for individual voice connections. The voice setting
 * parameter controls the configuration for voice connections : Input
 * coding, input data format, input sample size and air coding.
 *
 * In order to set the voice setting parameters, the user need to pick
 * one of each configuration parameter and OR them together. The
 * result must is the value of VOICE_SETTING.
 *
 * E.g VOICE_SETTING set to : *
 *   INPUTCODING_LINEAR | INPUTDATAFORMAT_2COMPLEMENT |
 *   INPUTSAMPLESIZE_16BIT | AIRCODING_CVSD)
 * means that the input coding is linear, the input data format is 2's
 * complement, the input sample size is 16 bit, and the aircoding is
 * CVSD */

/* Defines Input Coding */
#define INPUTCODING_LINEAR                                      0x0000
#define INPUTCODING_my_LAW                                      0x0100
#define INPUTCODING_A_LAW                                       0x0200

/* Defines input data format */
#define INPUTDATAFORMAT_1COMPLEMENT                             0x0000
#define INPUTDATAFORMAT_2COMPLEMENT                             0x0040
#define INPUTDATAFORMAT_SIGNMAGNITUDE                           0x0080

/* Defines input sample size */
#define INPUTSAMPLESIZE_8BIT                                    0x0000
#define INPUTSAMPLESIZE_16BIT                                   0x0020

/* Defines air Coding */
#define AIRCODING_CVSD                                          0x0000
#define AIRCODING_my_LAW                                        0x0001
#define AIRCODING_A_LAW                                         0x0002
#define AIRCODING_TRANSPARENT_DATA                              0x0003

/* This is the default air-coding voice setting */
#define VOICE_SETTING_AIR                                       (AIRCODING_CVSD)

/* This is the default input-coding voice setting */
#define VOICE_SETTING_INPUT                                     (INPUTCODING_LINEAR | \
                                                                 INPUTDATAFORMAT_2COMPLEMENT | \
                                                                 INPUTSAMPLESIZE_16BIT )

/* The default combined voice-setting. This will be written to the
 * chip when BCHS boots. Settings can after that only be changed if
 * the specific profile allows it, and only if there are no active SCO
 * connections. */
#define VOICE_SETTING                                           (VOICE_SETTING_AIR | VOICE_SETTING_INPUT)


/*--------------------------------------------------------------------------
 * Defines for BCHS SCO/SYNC connection
 *--------------------------------------------------------------------------*/

/* Define the max number of simultaneously sync connections */
#define MAX_NUM_OF_SIMULTANIOUS_SYNC_CONNECTIONS                (2)

/* Default values for the accept_connect_sco for Handsfree
 * Gateway. Note that CVSD encoding is the only air-coding allowed for
 * HFG, so we use that and OR the default voice setting input
 * coding */
#define SCO_DEFAULT_ACCEPT_TX_BANDWIDTH                         (0xFFFFFFFF)
#define SCO_DEFAULT_ACCEPT_RX_BANDWIDTH                         (0xFFFFFFFF)
#define SCO_DEFAULT_ACCEPT_MAX_LATENCY                          (0xFFFF)
#define SCO_DEFAULT_ACCEPT_VOICE_SETTINGS                       (AIRCODING_CVSD | VOICE_SETTING_INPUT)
#define SCO_DEFAULT_ACCEPT_RE_TX_EFFORT                         (0xFF)
/*
   3M EDR eSCO packets cannot be used on BC4 with CVSD.
   This value can be changed to 0xFC3F if using BC5 or
   newer, or if another aircoding is used.
*/
#define SCO_DEFAULT_ACCEPT_AUDIO_QUALITY (0xFEBF)


#define ESCO_DEFAULT_CONNECT_TX_BANDWIDTH                       (8000)
#define ESCO_DEFAULT_CONNECT_RX_BANDWIDTH                       (8000)
#define ESCO_DEFAULT_CONNECT_MAX_LATENCY                        (0x000A)
#define ESCO_DEFAULT_CONNECT_VOICE_SETTINGS                     (AIRCODING_CVSD | VOICE_SETTING_INPUT)
#define ESCO_DEFAULT_CONNECT_RE_TX_EFFORT                       (HCI_ESCO_POWER_SAVING_RETX)
/*
   3M EDR eSCO packets cannot be used on BC4 with CVSD.
   This value can be changed to 0x003F if using BC5 or
   newer, or if another aircoding is used.
*/
#define ESCO_DEFAULT_CONNECT_AUDIO_QUALITY (0x02BF)

/* Default packet type for accept of incoming SCO and eSCO (S1, S2 ans S3) */
#define COMPLETE_SCO_DEFAULT_ACCEPT_AUDIO_QUALITY               (0x038F) /* HV1 + HV2 + HV3 + EV3 + not 3-EV3 + not 2-EV5 + not 3-EV5 */

/*----------------------------------------------------------------------------
 * Defines for JSR82
 *----------------------------------------------------------------------------*/

/* This is the max number of bytes the JSR82 layer will accept from
 * the SD in one SD_READ_TRUSTED_DEVICES_IND/CFM signal */
#define JSR82_MAX_BYTES_IN_SD_CACHE_READ                        (2000)

/* This is the time in seconds that the JSR-82 layer uses for the
 * inquiry procedure */
#define JSR82_INQ_TIME                                          (11)

/* Size in bytes of the circular buffer for JSR-82 RFCOMM connections.
 * Should be greater than PROFILE_DEFAULT_MAX_FRAME_SIZE */
#define JSR82_RFC_BUFFER_SIZE                                   ((uint32_t)(5*PROFILE_DEFAULT_MAX_FRAME_SIZE))

/* Max number of packets to store in the receive buffer for a JSR-82
 * L2CAP connection. If more than this is received, the oldest packets
 * will be dropped */
#define JSR82_MAX_UNPROCESSED_DATA_PACKETS                      ((uint32_t)25)

/* The maximum number of dynamic range PSMs that may be allocated to the JSR82 task */
#define JSR82_MAX_ALLOCATED_PSMS                                (16)


/*--------------------------------------------------------------------------
 * Defines for BCHS TCS profile manager
 *--------------------------------------------------------------------------*/
/* 0x12 - 0x1000, recommended value max 480 slots = 300 ms */
#define TCS_PAGE_SCAN_INTERVAL                                  (480)

/* window   0x12 - 0x1000, default 0x12 */
#define TCS_PAGE_SCAN_WINDOW                                    (0x12)

/* max number of calls to handle simultaneously */
#define TCS_MAX_NUMBER_OF_CALLS                                 (0x03)

/* minimum value according to ctp profile */
#define TCS_CTP_MTU_SIZE                                        (171)

/* minimum value is 3 */
#define TCS_ICP_MTU_SIZE                                        (127)

/* Max time in betw. reconnect attempt for default GW */
#define LINK_GW_RECONNECT_MAX                                   (60)

/* Inc timer for reconnect (attempt * value) */
#define LINK_GW_RECONNECT_INC                                   (2)

/* Initiate link low power modes if no call setup received and link not parked for ctp within */
#define TCS_TIMER_CTP_LINK_SUPERVISION                          ((TcsTimer_t) (15))

/* Release link if no call setup received link not parked for ctp within */
#define TCS_TIMER_CTP_LINK_SUPERVISION_DISCONNECT               ((TcsTimer_t) (75))

/* Try to set the link in low power mode if no data received within this time for new connections (GW side) */

/* Time in seconds in between consequetive re-connect from the TL
 * towards the GW upon disconnect or failed connect attempts.  Value
 * of zero means no connect or reconnect. TCS will connect upon a
 * setup request */
#define TCS_TIMER_CTP_LINK_SUPERVISION_GW                       ((TcsTimer_t) (5))


/*--------------------------------------------------------------------------
 * Defines for BCHS BNEP connection
 *--------------------------------------------------------------------------*/
/* Defines the maximum number of packets allowed on queue for each
 * bnep connection */
#define MAX_BSL_QUEUE_LENGTH                                    (8)
#define MAX_BSL_LOCAL_MULTICAST_QUEUE_LENGTH                    (8)


/*--------------------------------------------------------------------------
 * Defines for BCHS discovery
 *--------------------------------------------------------------------------*/
/* Defines the maximum number of different applications and/or
 * profiles that will request dicovery (perform inquiry) */
#define NUM_OF_DIFF_APP_PROFILES_REQUEST_DISCOVERY              (7)

/* Define the size of the discovery filter for each instance defined
 * by NUM_OF_DIFF_APP_PROFILES_REQUEST_DISCOVERY */
#define BD_ADDR_FILTER_BUFFER_SIZE                              (11)


/*--------------------------------------------------------------------------
 * Defines for BCHS security
 *--------------------------------------------------------------------------*/

/* Set the default sec level to mode 2 (service invoked) */
#define DEFAULT_SEC_MODE                                        SEC_MODE2_SERVICE

/* Set the default encryption mode for security mode 3. Please note
 * that this value is only used for security mode 3.  It can any one
 * of:
 * - HCI_ENC_MODE_OFF
 * - HCI_ENC_MODE_PT_TO_PT
 * - HCI_ENC_MODE_PT_TO_PT_AND_BCAST */
#define DEFAULT_ENC_MODE3                                       HCI_ENC_MODE_PT_TO_PT

/* The default application layer above the SC profile manager. Used if
 * no other handle is given when the function is activated */
#define APPLICATION_SECURITY_HANDLER                            TESTQUEUE

/* Set the time window in which the user has to respond to a 
 * SC_PASSKEY_IND, if the user fails to respond within this time
 * the SC will automatically respond the passkey indication with 
 * error. The time is given in mSec and must not be higher than 
 * 29000 mSec. Default 28000 mSec. */
#define SC_PASSKEY_RESPONSE_TIMEOUT                 28000

/*--------------------------------------------------------------------------
 * Defines for BCHS page scan
 *--------------------------------------------------------------------------*/

/* Define the Page Scan interval: 0x12 - 0x1000, default 0x800 */
#define PAGESCANINTERVAL                                        0x800

/* Define the Page Scan window: 0x12 - 0x1000, default 0x12 */
#define PAGESCANWINDOW                                          0x12

/* Define the timeout window before page request fails.
 * Sec = PAGE_TIMEOUT * 0.000625 */
#define PAGE_TIMEOUT                                            (24000)


/*--------------------------------------------------------------------------
 * Defines for the Link Supervision timeout
 *--------------------------------------------------------------------------*/

/* Define the default link supervision timeout. Range is 0x0001 -
 * 0xFFFF and default is 0x7D00 (32000 ~ 20 seconds).  time_in_seconds
 * = DEFAULT_LINK_SUPERVISION_TIMEOUT * 0.000625 0x0000 means infinite
 * link supervision timeout. */
#define DEFAULT_LINK_SUPERVISION_TIMEOUT                        (32000)


/*--------------------------------------------------------------------------
 * Defines for BCHS Page Scan Type and Inquiry Scan Type
 *--------------------------------------------------------------------------*/

/* Use HCI_SCAN_TYPE_LEGACY for standard scan.
 * HCI_SCAN_TYPE_INTERLACED for interlaced scan.
 *
 * Please note that the drawbacks of using HCI_SCAN_TYPE_INTERLACED
 * scan is that it uses more power than HCI_SCAN_TYPE_LEGACY. The
 * advantage of using HCI_SCAN_TYPE_INTERLACED is that a peer device
 * faster can make inquiries and connect to this
 * device. */
#define PAGE_SCAN_TYPE                                          HCI_SCAN_TYPE_LEGACY
#define INQUIRY_SCAN_TYPE                                       HCI_SCAN_TYPE_LEGACY


/*--------------------------------------------------------------------------
 * Defines for BCHS OBEX parameter
 *--------------------------------------------------------------------------*/

/* The obex max packet size received from the peer side will be this
 * value - 1024 dec.  Please notice that the largest acceptable packet
 * size value for IrOBEX is 0xFFFF, and the minimum value is
 * 0x00FF. */
#define MAX_OBEX_SIGNAL_LENGTH                                  16384
#define MIN_OBEX_PACKET_LENGTH                                  255
#define MAX_PACKET_SIZE_SERVER                                  (uint16_t)(MAX_OBEX_SIGNAL_LENGTH)


/*--------------------------------------------------------------------------
 * Defines for FM Radio
 *--------------------------------------------------------------------------*/

/* FM_EU_US_BND uses european / U.S. FM band (87.5 - 108 MHz)
 * FM_JAPAN_BND uses japanese FM band (76 - 90 MHz) */
#define FM_DEFAULT_BAND                                         FM_EU_US_BAND

/* RDS_SYSTEM uses the european standard
 * RBDS_SYSTEM uses the U.S. standard */
#define FM_DEFAULT_RDS_SYSTEM                                   FM_RDS_SYSTEM

/* Choose an IF limit in the interval [1:255] */
#define FM_DEFAULT_IF_LIMIT                                     ((uint8_t) 150)

/* Choose an RSSI limit in the interval [1:255] */
#define FM_DEFAULT_RSSI_LIMIT                                   ((uint8_t) 150)

/* Choose a search level (RSSI) in the interval [1:255] */
#define FM_DEFAULT_SEARCH_LEVEL                                 ((uint8_t) 150)


/*--------------------------------------------------------------------------
 * Defines for BCSP
 *--------------------------------------------------------------------------*/
#define BCSP_RETRANSMISSION_TIMER (250000)

/*--------------------------------------------------------------------------
 * Defines for the Linux port
 *--------------------------------------------------------------------------*/

/* Setup whihc PCM SLOT the AV2ALSA device is allocated to */
#define AV2ALSA_PCM_SLOT                                        (0)



#ifdef __cplusplus
}
#endif

#endif

