#include "Common.h"

#include <libopencm3/cm3/common.h>
#include <libopencm3/cm3/vector.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/efm32/wdog.h>
#include <libopencm3/efm32/gpio.h>
#include <libopencm3/efm32/cmu.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define VENDOR_ID                 0x1209    /* pid.code */
#define PRODUCT_ID                0x70b1    /* Assigned to Tomu project */
#define DEVICE_VER                0x0101    /* Program version */

bool g_usbd_is_connected = false;
usbd_device *g_usbd_dev = 0;

static const struct usb_device_descriptor dev = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = USB_CLASS_CDC,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	.idVendor = VENDOR_ID,
	.idProduct = PRODUCT_ID,
	.bcdDevice = DEVICE_VER,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

/*
 * This notification endpoint isn't implemented. According to CDC spec its
 * optional, but its absence causes a NULL pointer dereference in Linux
 * cdc_acm driver.
 */
static const struct usb_endpoint_descriptor comm_endp[] = {{
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x83,
	.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
	.wMaxPacketSize = 16,
	.bInterval = 255,
}};

static const struct usb_endpoint_descriptor data_endp[] = {{
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x01,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 1,
}, {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x82,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 1,
}};

static const struct {
	struct usb_cdc_header_descriptor header;
	struct usb_cdc_call_management_descriptor call_mgmt;
	struct usb_cdc_acm_descriptor acm;
	struct usb_cdc_union_descriptor cdc_union;
} __attribute__((packed)) cdcacm_functional_descriptors = {
	.header = {
		.bFunctionLength = sizeof(struct usb_cdc_header_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_HEADER,
		.bcdCDC = 0x0110,
	},
	.call_mgmt = {
		.bFunctionLength =
			sizeof(struct usb_cdc_call_management_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
		.bmCapabilities = 0,
		.bDataInterface = 1,
	},
	.acm = {
		.bFunctionLength = sizeof(struct usb_cdc_acm_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_ACM,
		.bmCapabilities = 0,
	},
	.cdc_union = {
		.bFunctionLength = sizeof(struct usb_cdc_union_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_UNION,
		.bControlInterface = 0,
		.bSubordinateInterface0 = 1,
	 }
};

static const struct usb_interface_descriptor comm_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 1,
	.bInterfaceClass = USB_CLASS_CDC,
	.bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
	.bInterfaceProtocol = USB_CDC_PROTOCOL_AT,
	.iInterface = 0,

	.endpoint = comm_endp,

	.extra = &cdcacm_functional_descriptors,
	.extralen = sizeof(cdcacm_functional_descriptors)
}};

static const struct usb_interface_descriptor data_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 1,
	.bAlternateSetting = 0,
	.bNumEndpoints = 2,
	.bInterfaceClass = USB_CLASS_DATA,
	.bInterfaceSubClass = 0,
	.bInterfaceProtocol = 0,
	.iInterface = 0,

	.endpoint = data_endp,
}};

static const struct usb_interface ifaces[] = {{
	.num_altsetting = 1,
	.altsetting = comm_iface,
}, {
	.num_altsetting = 1,
	.altsetting = data_iface,
}};

static const struct usb_config_descriptor config = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 2,
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0x80,
	.bMaxPower = 0x32,

	.interface = ifaces,
};

static const char *usb_strings[] = {
	"Tomu",
	"HDMI CEC DEMO",
	"DEMO DEVICE",
};

/* Buffer to be used for control requests. */
uint8_t usbd_control_buffer[128];

static enum usbd_request_return_codes cdcacm_control_request(usbd_device *usbd_dev, struct usb_setup_data *req, uint8_t **buf,
                uint16_t *len, void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req))
{
        (void)complete;
        (void)buf;
        (void)usbd_dev;

        switch(req->bRequest) {
        case USB_CDC_REQ_SET_CONTROL_LINE_STATE: {
        g_usbd_is_connected = req->wValue & 1; /* Check RTS bit */
                return USBD_REQ_HANDLED;
                }
        case USB_CDC_REQ_SET_LINE_CODING: 
                if(*len < sizeof(struct usb_cdc_line_coding))
                        return 0;

                return USBD_REQ_HANDLED;
        }
        return 0;
}

/* Simple callback that echoes whatever is sent */
static void cdcacm_data_rx_cb(usbd_device *usbd_dev, uint8_t ep)
{
   (void)ep;
}

static void cdcacm_set_config(usbd_device *usbd_dev, uint16_t wValue)
{
        (void)wValue;

        usbd_ep_setup(usbd_dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64, cdcacm_data_rx_cb);
        usbd_ep_setup(usbd_dev, 0x82, USB_ENDPOINT_ATTR_BULK, 64, 0);
        usbd_ep_setup(usbd_dev, 0x83, USB_ENDPOINT_ATTR_INTERRUPT, 16, 0);

        usbd_register_control_callback(
                                usbd_dev,
                                USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
                                USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
                                cdcacm_control_request);
}


static void usb_puts(char *s) {
    if(g_usbd_is_connected) {
        usbd_ep_write_packet(g_usbd_dev, 0x82, s, strnlen(s, 64));
    }
}

void common_init(void) {
   /* Make sure the vector table is relocated correctly (after the Tomu bootloader) */
   SCB_VTOR = 0x4000;

   /* Disable the watchdog that the bootloader started. */
   WDOG_CTRL = 0;

   /* Configure the USB core & stack */
   g_usbd_dev = usbd_init(&efm32hg_usb_driver, &dev, &config, usb_strings, 3, usbd_control_buffer, sizeof(usbd_control_buffer));
   usbd_register_set_config_callback(g_usbd_dev, cdcacm_set_config);

   /* Enable USB IRQs */
   nvic_enable_irq(NVIC_USB_IRQ);
}

void DbgPrint(const char* fmt, ...)
{
   char FormatBuffer[128]; 
   va_list args;
   va_start(args, fmt);
   vsprintf(FormatBuffer, fmt, args);

//   char c;
//   char* addr = FormatBuffer;

   usb_puts(FormatBuffer);
/*
   while ((c = *addr++))
   {
      Serial.print(c);    
   }
*/
}


