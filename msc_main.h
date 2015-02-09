/*
 * msc_main.h
 *
 *  Original Code Created on: 2014.07.04
 *      Author: Kestutis Bivainis
 *
 *  Created on: 2014.02.08
 *  	Author: Kyle Smith 2014.02.08
 */

#ifndef __MSC_MAIN_H_
#define __MSC_MAIN_H_

#include <string.h>

#include "board.h"
#include "sd_spi.h"
#include "msc_usb.h"
#include "app_usbd_cfg.h"

typedef enum {
	MSC_OK,
	MSC_USB_ERR,
} MSC_ERR;

const  USBD_API_T *g_pUsbApi;

extern SD_CardInfo cardinfo;

void USB_IRQHandler(void);

void msc_stop(void);

/* Start up MSC */
MSC_ERR msc_init(void);

#endif /* __MSC_MAIN_H_ */
