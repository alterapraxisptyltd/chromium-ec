/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "clock.h"
#include "common.h"
#include "config.h"
#include "console.h"
#include "gpio.h"
#include "hooks.h"
#include "link_defs.h"
#include "registers.h"
#include "task.h"
#include "timer.h"
#include "util.h"
#include "usb.h"

/* Console output macro */
#define CPRINTF(format, args...) cprintf(CC_USB, format, ## args)

/* USB Standard Device Descriptor */
static const struct usb_device_descriptor dev_desc = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200, /* v2.00 */
	.bDeviceClass = USB_CLASS_PER_INTERFACE,
	.bDeviceSubClass = 0x00,
	.bDeviceProtocol = 0x00,
	.bMaxPacketSize0 = USB_MAX_PACKET_SIZE,
	.idVendor = USB_VID_GOOGLE,
	.idProduct = CONFIG_USB_PID,
	.bcdDevice = 0x0200, /* 2.00 */
	.iManufacturer = USB_STR_VENDOR,
	.iProduct = USB_STR_PRODUCT,
	.iSerialNumber = USB_STR_VERSION,
	.bNumConfigurations = 1
};

/* USB Configuration Descriptor */
const struct usb_config_descriptor USB_CONF_DESC(conf) = {
	.bLength = USB_DT_CONFIG_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0x0BAD, /* no of returned bytes, set at runtime */
	.bNumInterfaces = USB_IFACE_COUNT,
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0x80, /* bus powered */
	.bMaxPower = 250, /* MaxPower 500 mA */
};

const uint8_t usb_string_desc[] = {
	4, /* Descriptor size */
	USB_DT_STRING,
	0x09, 0x04 /* LangID = 0x0409: U.S. English */
};

/* Endpoint table in USB controller RAM */
struct stm32_endpoint btable_ep[USB_EP_COUNT]
	__attribute__((section(".usb_ram.btable")));
/* Control endpoint (EP0) buffers */
static usb_uint ep0_buf_tx[USB_MAX_PACKET_SIZE / 2] __usb_ram;
static usb_uint ep0_buf_rx[USB_MAX_PACKET_SIZE / 2] __usb_ram;

static int set_addr;

/* Requests on the control endpoint (aka EP0) */
static void ep0_rx(void)
{
	uint16_t req = ep0_buf_rx[0]; /* bRequestType | bRequest */

	/* interface specific requests */
	if ((req & USB_RECIP_MASK) == USB_RECIP_INTERFACE) {
		uint8_t iface = ep0_buf_rx[1] & 0xff;
		if (iface < USB_IFACE_COUNT)
			usb_iface_request[iface](ep0_buf_rx, ep0_buf_tx);
		return;
	}

	/* TODO check setup bit ? */
	if (req == (USB_DIR_IN | (USB_REQ_GET_DESCRIPTOR << 8))) {
		uint8_t type = ep0_buf_rx[1] >> 8;
		uint8_t idx = ep0_buf_rx[1] & 0xff;
		const uint8_t *str_desc;

		switch (type) {
		case USB_DT_DEVICE: /* Setup : Get device descriptor */
			memcpy_usbram(ep0_buf_tx, (void *)&dev_desc,
					 sizeof(dev_desc));
			btable_ep[0].tx_count =  sizeof(dev_desc);
			STM32_TOGGLE_EP(0, EP_TX_RX_MASK, EP_TX_RX_VALID,
				  EP_STATUS_OUT /*null OUT transaction */);
			break;
		case USB_DT_CONFIGURATION: /* Setup : Get configuration desc */
			memcpy_usbram(ep0_buf_tx, __usb_desc,
					 USB_DESC_SIZE);
			/* set the real descriptor size */
			ep0_buf_tx[1] = USB_DESC_SIZE;
			btable_ep[0].tx_count = MIN(ep0_buf_rx[3],
					 USB_DESC_SIZE);
			STM32_TOGGLE_EP(0, EP_TX_RX_MASK, EP_TX_RX_VALID,
				  EP_STATUS_OUT /*null OUT transaction */);
			break;
		case USB_DT_STRING: /* Setup : Get string descriptor */
			if (idx >= USB_STR_COUNT) {
				/* The string does not exist : STALL */
				STM32_TOGGLE_EP(0, EP_TX_RX_MASK,
					  EP_RX_VALID | EP_TX_STALL, 0);
				return; /* don't remove the STALL */
			}
			str_desc = usb_strings[idx];
			memcpy_usbram(ep0_buf_tx, str_desc, str_desc[0]);
			btable_ep[0].tx_count = MIN(ep0_buf_rx[3], str_desc[0]);
			STM32_TOGGLE_EP(0, EP_TX_RX_MASK, EP_TX_RX_VALID,
				  EP_STATUS_OUT /*null OUT transaction */);
			break;
		case USB_DT_DEVICE_QUALIFIER: /* Get device qualifier desc */
			/* Not high speed : STALL next IN used as handshake */
			STM32_TOGGLE_EP(0, EP_TX_RX_MASK,
					EP_RX_VALID | EP_TX_STALL, 0);
			break;
		default: /* unhandled descriptor */
			goto unknown_req;
		}
	} else if (req == (USB_DIR_IN | (USB_REQ_GET_STATUS << 8))) {
		uint16_t zero = 0;
		/* Get status */
		memcpy_usbram(ep0_buf_tx, (void *)&zero, 2);
		btable_ep[0].tx_count = 2;
		STM32_TOGGLE_EP(0, EP_TX_RX_MASK, EP_TX_RX_VALID,
			  EP_STATUS_OUT /*null OUT transaction */);
	} else if ((req & 0xff) == USB_DIR_OUT) {
		switch (req >> 8) {
		case USB_REQ_SET_ADDRESS:
			/* set the address after we got IN packet handshake */
			set_addr = ep0_buf_rx[1] & 0xff;
			/* need null IN transaction -> TX Valid */
			STM32_TOGGLE_EP(0, EP_TX_RX_MASK, EP_TX_RX_VALID, 0);
			break;
		case USB_REQ_SET_CONFIGURATION:
			/* uint8_t cfg = ep0_buf_rx[1] & 0xff; */
			/* null IN for handshake */
			btable_ep[0].tx_count = 0;
			STM32_TOGGLE_EP(0, EP_TX_RX_MASK, EP_TX_RX_VALID, 0);
			break;
		default: /* unhandled request */
			goto unknown_req;
		}

	} else {
		goto unknown_req;
	}

	return;
unknown_req:
	STM32_TOGGLE_EP(0, EP_TX_RX_MASK, EP_RX_VALID | EP_TX_STALL, 0);
}

static void ep0_tx(void)
{
	if (set_addr) {
		STM32_USB_DADDR = set_addr | 0x80;
		set_addr = 0;
		CPRINTF("SETAD %02x\n", STM32_USB_DADDR);
	}

	STM32_TOGGLE_EP(0, EP_TX_MASK, EP_TX_VALID, 0);
}

static void ep0_reset(void)
{
	STM32_USB_EP(0) = (1 << 9) /* control EP */ |
			  (2 << 4) /* TX NAK */ |
			  (3 << 12) /* RX VALID */;

	btable_ep[0].tx_addr = usb_sram_addr(ep0_buf_tx);
	btable_ep[0].rx_addr = usb_sram_addr(ep0_buf_rx);
	btable_ep[0].rx_count = 0x8000 | ((USB_MAX_PACKET_SIZE/32-1) << 10);
	btable_ep[0].tx_count = 0;
}
USB_DECLARE_EP(0, ep0_tx, ep0_rx, ep0_reset);

static void usb_reset(void)
{
	int ep;

	for (ep = 0; ep < USB_EP_COUNT; ep++)
		usb_ep_reset[ep]();

	/*
	 * set the default address : 0
	 * as we are not configured yet
	 */
	STM32_USB_DADDR = 0 | 0x80;
	CPRINTF("RST EP0 %04x\n", STM32_USB_EP(0));
}

void usb_interrupt(void)
{
	uint16_t status = STM32_USB_ISTR;

	if ((status & (1 << 10)))
		usb_reset();

	if (status & (1 << 15)) {
		int ep = status & 0x000f;
		if (ep < USB_EP_COUNT) {
			if (status & 0x0010)
				usb_ep_rx[ep]();
			else
				usb_ep_tx[ep]();
		}
		/* TODO: do it in a USB task */
		/* task_set_event(, 1 << ep_task); */
	}

	/* ack interrupts */
	STM32_USB_ISTR = 0;
}
DECLARE_IRQ(STM32_IRQ_USB_LP, usb_interrupt, 1);

static void usb_init(void)
{
	/* Enable USB device clock. */
	STM32_RCC_APB1ENR |= STM32_RCC_PB1_USB;

	/* we need a proper 48MHz clock */
	clock_enable_module(MODULE_USB, 1);

	/* power on sequence */

	/* keep FRES (USB reset) and remove PDWN (power down) */
	STM32_USB_CNTR = 0x01;
	udelay(1); /* startup time */
	/* reset FRES and keep interrupts masked */
	STM32_USB_CNTR = 0x00;
	/* clear pending interrupts */
	STM32_USB_ISTR = 0;

	/* set descriptors table offset in dedicated SRAM */
	STM32_USB_BTABLE = 0;

	/* EXTI18 is USB wake up interrupt */
	/* STM32_EXTI_RTSR |= 1 << 18; */
	/* STM32_EXTI_IMR |= 1 << 18; */

	/* Enable interrupt handlers */
	task_enable_irq(STM32_IRQ_USB_LP);
	/* set interrupts mask : reset/correct tranfer/errors */
	STM32_USB_CNTR = 0xe400;

	/* set pull-up on DP for FS mode */
#ifdef CHIP_VARIANT_STM32L15X
	STM32_SYSCFG_PMC |= 1;
#elif defined(CHIP_FAMILY_STM32F0)
	STM32_USB_BCDR |= 1 << 15 /* DPPU */;
#else
	/* hardwired or regular GPIO on other platforms */
#endif

	CPRINTF("USB init done\n");
}
DECLARE_HOOK(HOOK_INIT, usb_init, HOOK_PRIO_DEFAULT);
