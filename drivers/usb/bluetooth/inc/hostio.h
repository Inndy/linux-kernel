/******************************************************************************
FILE
	hostio.h  -  the hostio subsystem

CONTAINS
	hostio_find_fromhost_buffer  -  locate BUFFER for a connection
	hostio_enable_tohost_data  -  enable transmission on a connection

DESCRIPTION
	Hostio is responsible for communications between the host and on-chip
	software using one of a number of different physical interfaces.  It
	carries fifteen multiplexed protocols, each of which has a number of
	configurable parameters.
*/

#ifndef __HOSTIO_H__
#define __HOSTIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
NAME
	hostio_find_fromhost_buffer  -  find appropriate buf for payload data

RETURNS
	A pointer to a BUFFER, or NULL in case of error (caused, for example,
	by an invalid connection handle being supplied).
*/

extern BUFFER *hostio_find_fromhost_buffer(uint16 hci_data_connection_handle);

/****************************************************************************
NAME
	hostio_enable_tohost_data  -  enable transmission on a connection

RETURNS
	TRUE if any buffers were enabled otherwise FALSE.
*/

extern bool hostio_enable_tohost_data( uint16 hci_data_connection_handle );

#ifdef __cplusplus
}
#endif

#endif	/* __HOSTIO_H__ */
