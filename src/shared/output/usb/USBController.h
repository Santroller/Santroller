#pragma once
/* Defines: */
/** \name Endpoint Direction Masks */
/**@{*/
/** Endpoint direction mask, for masking against endpoint addresses to retrieve
 * the endpoint's direction for comparing with the \c ENDPOINT_DIR_* masks.
 */
#define ENDPOINT_DIR_MASK 0x80

/** Endpoint address direction mask for an OUT direction (Host to Device)
 * endpoint. This may be ORed with the index of the address within a device to
 * obtain the full endpoint address.
 */
#define ENDPOINT_DIR_OUT 0x00

/** Endpoint address direction mask for an IN direction (Device to Host)
 * endpoint. This may be ORed with the index of the address within a device to
 * obtain the full endpoint address.
 */
#define ENDPOINT_DIR_IN 0x80
/**@}*/

/** \name Pipe Direction Masks */
/**@{*/
/** Pipe direction mask, for masking against pipe addresses to retrieve the
 * pipe's direction for comparing with the \c PIPE_DIR_* masks.
 */
#define PIPE_DIR_MASK 0x80

/** Endpoint address direction mask for an OUT direction (Host to Device)
 * endpoint. This may be ORed with the index of the address within a device to
 * obtain the full endpoint address.
 */
#define PIPE_DIR_OUT 0x00

/** Endpoint address direction mask for an IN direction (Device to Host)
 * endpoint. This may be ORed with the index of the address within a device to
 * obtain the full endpoint address.
 */
#define PIPE_DIR_IN 0x80
/**@}*/

/** \name Endpoint/Pipe Type Masks */
/**@{*/
/** Mask for determining the type of an endpoint from an endpoint descriptor.
 * This should then be compared with the \c EP_TYPE_* masks to determine the
 * exact type of the endpoint.
 */
#define EP_TYPE_MASK 0x03

/** Mask for a CONTROL type endpoint or pipe.
 *
 *  \note See \ref Group_EndpointManagement and \ref Group_PipeManagement for
 * endpoint/pipe functions.
 */
#define EP_TYPE_CONTROL 0x00

/** Mask for an ISOCHRONOUS type endpoint or pipe.
 *
 *  \note See \ref Group_EndpointManagement and \ref Group_PipeManagement for
 * endpoint/pipe functions.
 */
#define EP_TYPE_ISOCHRONOUS 0x01

/** Mask for a BULK type endpoint or pipe.
 *
 *  \note See \ref Group_EndpointManagement and \ref Group_PipeManagement for
 * endpoint/pipe functions.
 */
#define EP_TYPE_BULK 0x02

/** Mask for an INTERRUPT type endpoint or pipe.
 *
 *  \note See \ref Group_EndpointManagement and \ref Group_PipeManagement for
 * endpoint/pipe functions.
 */
#define EP_TYPE_INTERRUPT 0x03