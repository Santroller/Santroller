#include <stdint.h>
/** Convenience macro to easily create \ref USB_Descriptor_String_t instances from a wide character string.
			 *
			 *  \note This macro is for little-endian systems only.
			 *
			 *  \param[in] String  String to initialize a USB String Descriptor structure with.
			 */
#define USB_STRING_DESCRIPTOR(String)                                                                                             \
    {                                                                                                                             \
        .bLength = sizeof(uint8_t) + sizeof(uint8_t) + (sizeof(String) - 2), .bDescriptorType = USB_DESCRIPTOR_STRING, .UnicodeString = String \
    }

/** Convenience macro to easily create \ref USB_Descriptor_String_t instances from an array of characters.
			 *
			 *  \param[in] ...  Characters to initialize a USB String Descriptor structure with.
			 */
#define USB_STRING_DESCRIPTOR_ARRAY(...)                                                                                                                \
    {                                                                                                                                                   \
        .bLength = sizeof(uint8_t) + sizeof(uint8_t)  + sizeof((uint16_t[]){__VA_ARGS__}), .bDescriptorType = USB_DESCRIPTOR_STRING, .UnicodeString = { __VA_ARGS__ } \
    }

/** Macro to encode a given major/minor/revision version number into Binary Coded Decimal format for descriptor
			 *  fields requiring BCD encoding, such as the USB version number in the standard device descriptor.
			 *
			 *  \note This value is automatically converted into Little Endian, suitable for direct use inside device
			 *        descriptors on all architectures without endianness conversion macros.
			 *
			 *  \param[in]  Major     Major version number to encode.
			 *  \param[in]  Minor     Minor version number to encode.
			 *  \param[in]  Revision  Revision version number to encode.
			 */
#define VERSION_BCD(Major, Minor, Revision) \
    (((Major & 0xFF) << 8) |     \
                ((Minor & 0x0F) << 4) |     \
                (Revision & 0x0F))

/** Macro to calculate the power value for the configuration descriptor, from a given number of milliamperes.
			 *
			 *  \param[in] mA  Maximum number of milliamps the device consumes when the given configuration is selected.
			 */
#define USB_CONFIG_POWER_MA(mA) ((mA) >> 1)

/** Indicates that a given descriptor does not exist in the device. This can be used inside descriptors
			 *  for string descriptor indexes, or may be use as a return value for GetDescriptor when the specified
			 *  descriptor does not exist.
			 */
#define NO_DESCRIPTOR 0

#define HID_DESCRIPTOR_REPORT  0x22
#define AUDIO_DESCRIPTOR_CSInterface 0x24 /**< Audio class specific Interface functional descriptor. */
#define AUDIO_DESCRIPTOR_CSEndpoint 0x25
/** String language ID for the English language. Should be used in \ref USB_Descriptor_String_t descriptors
			 *  to indicate that the English language is supported by the device in its string descriptors.
			 */
#define LANGUAGE_ID_ENG 0x0409


/** \name USB Configuration Descriptor Attribute Masks */
/**@{*/
/** Mask for the reserved bit in the Configuration Descriptor's \c ConfigAttributes field, which must be always
			 *  set on all USB devices for historical purposes.
			 */
#define USB_CONFIG_TATTR_RESERVED 0x80

/** Can be masked with other configuration descriptor attributes for a \ref USB_Descriptor_Configuration_Header_t
			 *  descriptor's \c ConfigAttributes value to indicate that the specified configuration can draw its power
			 *  from the device's own power source, instead of drawing it from the USB host.
			 *
			 *  Note that the host will probe this dynamically - the device should report its current power state via the
			 *  \ref USB_Device_CurrentlySelfPowered global variable.
			 */
#define USB_CONFIG_TATTR_SELFPOWERED 0x40

/** Can be masked with other configuration descriptor attributes for a \ref USB_Descriptor_Configuration_Header_t
			 *  descriptor's \c ConfigAttributes value to indicate that the specified configuration supports the
			 *  remote wakeup feature of the USB standard, allowing a suspended USB device to wake up the host upon
			 *  request.
			 *
			 *  If set, the host will dynamically enable and disable remote wakeup support, indicated via the
			 *  \ref USB_Device_RemoteWakeupEnabled global variable. To initiate a remote wakeup of the host (when allowed)
			 *  see \ref USB_Device_RemoteWakeupEnabled().
			 */
#define USB_CONFIG_TATTR_REMOTEWAKEUP 0x20
/**@}*/

/** \name Endpoint Descriptor Attribute Masks */
/**@{*/
/** Can be masked with other endpoint descriptor attributes for a \ref USB_Descriptor_Endpoint_t descriptor's
			 *  \c Attributes value to indicate that the specified endpoint is not synchronized.
			 *
			 *  \see The USB specification for more details on the possible Endpoint attributes.
			 */
#define ENDPOINT_TATTR_NO_SYNC (0 << 2)

/** Can be masked with other endpoint descriptor attributes for a \ref USB_Descriptor_Endpoint_t descriptor's
			 *  \c Attributes value to indicate that the specified endpoint is asynchronous.
			 *
			 *  \see The USB specification for more details on the possible Endpoint attributes.
			 */
#define ENDPOINT_TATTR_ASYNC (1 << 2)

/** Can be masked with other endpoint descriptor attributes for a \ref USB_Descriptor_Endpoint_t descriptor's
			 *  \c Attributes value to indicate that the specified endpoint is adaptive.
			 *
			 *  \see The USB specification for more details on the possible Endpoint attributes.
			 */
#define ENDPOINT_TATTR_ADAPTIVE (2 << 2)

/** Can be masked with other endpoint descriptor attributes for a \ref USB_Descriptor_Endpoint_t descriptor's
			 *  \c Attributes value to indicate that the specified endpoint is synchronized.
			 *
			 *  \see The USB specification for more details on the possible Endpoint attributes.
			 */
#define ENDPOINT_TATTR_SYNC (3 << 2)
/**@}*/

/** \name Endpoint Descriptor Usage Masks */
/**@{*/
/** Can be masked with other endpoint descriptor attributes for a \ref USB_Descriptor_Endpoint_t descriptor's
			 *  \c Attributes value to indicate that the specified endpoint is used for data transfers.
			 *
			 *  \see The USB specification for more details on the possible Endpoint usage attributes.
			 */
#define ENDPOINT_USAGE_DATA (0 << 4)

/** Can be masked with other endpoint descriptor attributes for a \ref USB_Descriptor_Endpoint_t descriptor's
			 *  \c Attributes value to indicate that the specified endpoint is used for feedback.
			 *
			 *  \see The USB specification for more details on the possible Endpoint usage attributes.
			 */
#define ENDPOINT_USAGE_FEEDBACK (1 << 4)

/** Can be masked with other endpoint descriptor attributes for a \ref USB_Descriptor_Endpoint_t descriptor's
			 *  \c Attributes value to indicate that the specified endpoint is used for implicit feedback.
			 *
			 *  \see The USB specification for more details on the possible Endpoint usage attributes.
			 */
#define ENDPOINT_USAGE_IMPLICIT_FEEDBACK (2 << 4)


/** Enum for possible Class, Subclass and Protocol values of device and interface descriptors. */
enum USB_Descriptor_ClassSubclassProtocol_t {
    USB_CSCP_NoDeviceClass = 0x00,          /**< Descriptor Class value indicating that the device does not belong
				                                         *   to a particular class at the device level.
				                                         */
    USB_CSCP_NoDeviceSubclass = 0x00,       /**< Descriptor Subclass value indicating that the device does not belong
				                                         *   to a particular subclass at the device level.
				                                         */
    USB_CSCP_NoDeviceProtocol = 0x00,       /**< Descriptor Protocol value indicating that the device does not belong
				                                         *   to a particular protocol at the device level.
				                                         */
    USB_CSCP_VendorSpecificClass = 0xFF,    /**< Descriptor Class value indicating that the device/interface belongs
				                                         *   to a vendor specific class.
				                                         */
    USB_CSCP_VendorSpecificSubclass = 0xFF, /**< Descriptor Subclass value indicating that the device/interface belongs
				                                         *   to a vendor specific subclass.
				                                         */
    USB_CSCP_VendorSpecificProtocol = 0xFF, /**< Descriptor Protocol value indicating that the device/interface belongs
				                                         *   to a vendor specific protocol.
				                                         */
    USB_CSCP_IADDeviceClass = 0xEF,         /**< Descriptor Class value indicating that the device belongs to the
				                                         *   Interface Association Descriptor class.
				                                         */
    USB_CSCP_IADDeviceSubclass = 0x02,      /**< Descriptor Subclass value indicating that the device belongs to the
				                                         *   Interface Association Descriptor subclass.
				                                         */
    USB_CSCP_IADDeviceProtocol = 0x01,      /**< Descriptor Protocol value indicating that the device belongs to the
				                                         *   Interface Association Descriptor protocol.
				                                         */
};


/* Type Defines: */
/** Enum for possible Class, Subclass and Protocol values of device and interface descriptors relating to the HID
		 *  device class.
		 */
enum THID_Descriptor_ClassSubclassProtocol_t {
    HID_CSCP_HIDClass = 0x03,             /**< Descriptor Class value indicating that the device or interface
			                                       *   belongs to the HID class.
			                                       */
    HID_CSCP_NonBootSubclass = 0x00,      /**< Descriptor Subclass value indicating that the device or interface
			                                       *   does not implement a HID boot protocol.
			                                       */
    HID_CSCP_BootSubclass = 0x01,         /**< Descriptor Subclass value indicating that the device or interface
			                                       *   implements a HID boot protocol.
			                                       */
    HID_CSCP_NonBootProtocol = 0x00,      /**< Descriptor Protocol value indicating that the device or interface
			                                       *   does not belong to a HID boot protocol.
			                                       */
    HID_CSCP_KeyboardBootProtocol = 0x01, /**< Descriptor Protocol value indicating that the device or interface
			                                       *   belongs to the Keyboard HID boot protocol.
			                                       */
    HID_CSCP_MouseBootProtocol = 0x02,    /**< Descriptor Protocol value indicating that the device or interface
			                                       *   belongs to the Mouse HID boot protocol.
			                                       */
};