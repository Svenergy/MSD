/*
 * msc_main.c
 *
 *  Original Code Created on: 2014.07.04
 *      Author: Kestutis Bivainis
 *
 *  Modified by Kyle Smith 2014.02.08
 */

#include "msc_main.h"

static USBD_HANDLE_T g_hUsb;

void USB_IRQHandler(void) {
  USBD_API->hw->ISR(g_hUsb);
}

// Make USB device invisible on the USB bus
void msc_stop(void){
  USBD_API->hw->Connect(g_hUsb, 0);
}

// Start up MSC without test outputs
MSC_ERR msc_init(void){
  USBD_API_INIT_PARAM_T usb_param;
  USB_CORE_DESCS_T desc;
  USBD_MSC_INIT_PARAM_T msc_param;

  ErrorCode_t ret = LPC_OK;
  USB_INTERFACE_DESCRIPTOR* pIntfDesc;

  /* enable clocks */
  Chip_USB_Init();

  /* initialize USBD ROM API pointer. */
  g_pUsbApi = (const USBD_API_T *) LPC_ROM_API->pUSBD;

  /* initialize call back structures */
  memset((void *) &usb_param, 0, sizeof(USBD_API_INIT_PARAM_T));
  usb_param.usb_reg_base = LPC_USB0_BASE;
  /*  WORKAROUND for artf44835 ROM driver BUG:
	  Code clearing STALL bits in endpoint reset routine corrupts memory area
	  next to the endpoint control data. For example When EP0, EP1_IN, EP1_OUT,
	  EP2_IN are used we need to specify 3 here. But as a workaround for this
	  issue specify 4. So that extra EPs control structure acts as padding buffer
	  to avoid data corruption. Corruption of padding memory doesn't affect the
	  stack/program behaviour.
  */
  usb_param.max_num_ep = 2 + 1;
  usb_param.mem_base = USB_STACK_MEM_BASE;
  usb_param.mem_size = USB_STACK_MEM_SIZE;

  /* Set the USB descriptors */
  desc.device_desc = (uint8_t *) USB_DeviceDescriptor;
  desc.string_desc = (uint8_t *) USB_StringDescriptor;

  /* Note, to pass USBCV test full-speed only devices should have both
   * descriptor arrays point to same location and device_qualifier set
   * to 0.
   */
  desc.high_speed_desc = USB_FsConfigDescriptor;
  desc.full_speed_desc = USB_FsConfigDescriptor;
  desc.device_qualifier = 0;

  /* USB Initialization */
  ret = USBD_API->hw->Init(&g_hUsb, &desc, &usb_param);
  if(ret == LPC_OK) {

	memset((void *) &msc_param, 0, sizeof(USBD_MSC_INIT_PARAM_T));
	msc_param.mem_base = usb_param.mem_base;
	msc_param.mem_size = usb_param.mem_size;
	msc_param.InquiryStr = InquiryStr;
	msc_param.BlockCount = cardinfo.CardCapacity/SD_BLOCKSIZE;
	msc_param.BlockSize = SD_BLOCKSIZE;

	// Attempting to accept cards >4gb, doesn't allow read/write beyond 4gb, may not be necessary for reading up to the full 4gb mark
	if(cardinfo.CardCapacity > 0xFFFFFFFF){
		msc_param.MemorySize = 0xFFFFFFFF;
	}else{
		msc_param.MemorySize = cardinfo.CardCapacity;
	}

	pIntfDesc = (USB_INTERFACE_DESCRIPTOR*)((uint32_t)desc.high_speed_desc + USB_CONFIGURATION_DESC_SIZE);

	// check we are referencing to the proper interface descriptor
	if((pIntfDesc == 0) || (pIntfDesc->bInterfaceClass != USB_DEVICE_CLASS_STORAGE) || (pIntfDesc->bInterfaceSubClass != MSC_SUBCLASS_SCSI)) {
	  return MSC_USB_ERR;
	}

	msc_param.intf_desc = (uint8_t*)pIntfDesc;

	// user defined functions
	msc_param.MSC_Write = MSC_Write;
	msc_param.MSC_Read = MSC_Read;
	msc_param.MSC_Verify = MSC_Verify;

	ret = USBD_API->msc->init(g_hUsb, &msc_param);

	if(ret == LPC_OK) {
	  // enable USB interrupts
	  NVIC_EnableIRQ(USB0_IRQn);
	  // now connect
	  USBD_API->hw->Connect(g_hUsb, 1);
	}
	else {
	  return MSC_USB_ERR;
	}
  }
  else {
	return MSC_USB_ERR;
  }
  return MSC_OK;
}
