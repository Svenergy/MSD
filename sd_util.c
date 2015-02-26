#include "sd_util.h"

void sMemDump_8(uint8_t *ptr, uint32_t length){
	char buf[32];
	uint32_t i = 0;
	while(i < length){
		if (i % 16 == 0){
			sprintf(buf, "\n0x%08x | ", ptr + i);
			putLineUART(buf);
		}
		uint8_t value = *(ptr + i);
		sprintf(buf, "%02X ", value);
		putLineUART(buf);
		i++;
	}
}

void sMemDump_32(uint32_t *ptr, uint32_t length){
	char buf[32];
	uint32_t i = 0;
	length = length / 4;
	while(i < length){
		if (i % 4 == 0){
			sprintf(buf, "\n0x%08x | ", ptr + i);
			putLineUART(buf);
		}
		uint32_t value = *(ptr + i);
		sprintf(buf, "%08X ", value);
		putLineUART(buf);
		i++;
	}
}

// Print all SD card info
SD_ERROR sd_print_info(void) {
  SD_ERROR sderr;

  sderr=init_sd_spi(&cardinfo);

  switch(sderr) {
    case SD_OK:
      sprintf(buffer,"\r\n----->Card initialization OK\r\n");
    break;
    case ERROR_GO_IDLE_STATE_TIMEOUT:
      sprintf(buffer,"Error: ERROR_GO_IDLE_STATE_TIMEOUT\r\n");
    break;
    case ERROR_GO_IDLE_STATE_RESPONSE:
      sprintf(buffer,"Error: ERROR_GO_IDLE_STATE_RESPONSE:%d\r\n",response[0]);
    break;
    case ERROR_SEND_IF_COND_TIMEOUT:
      sprintf(buffer,"Error: ERROR_SEND_IF_COND_TIMEOUT\r\n");
    break;
    case ERROR_SEND_IF_COND_RESPONSE:
      sprintf(buffer,"Error: ERROR_SEND_IF_COND_RESPONSE:%02x%02x%02x%02x%02x\r\n",response[0],response[1],response[2],response[3],response[4]);
    break;
    case ERROR_READ_OCR_TIMEOUT:
      sprintf(buffer,"Error: ERROR_READ_OCR_TIMEOUT\r\n");
    break;
    case ERROR_READ_OCR_RESPONSE:
      sprintf(buffer,"Error: ERROR_READ_OCR_RESPONSE:%02x%02x%02x\r\n",response[0],response[1],response[2]);
    break;
    case ERROR_APP_CMD_TIMEOUT:
      sprintf(buffer,"Error: ERROR_APP_CMD_TIMEOUT\r\n");
    break;
    case ERROR_SD_SEND_OP_COND_TIMEOUT:
      sprintf(buffer,"Error: ERROR_SD_SEND_OP_COND_TIMEOUT\r\n");
    break;
    case ERROR_INIT_TIMEOUT:
      sprintf(buffer,"Error: ERROR_INIT_TIMEOUT\r\n");
    break;
    case ERROR_SEND_CID_TIMEOUT:
      sprintf(buffer,"Error: ERROR_SEND_CID_TIMEOUT\r\n");
    break;
    case ERROR_SEND_CID_TOKEN_TIMEOUT:
      sprintf(buffer,"Error: ERROR_SEND_CID_TOKEN_TIMEOUT\r\n");
    break;
    case ERROR_SEND_CSD_TIMEOUT:
      sprintf(buffer,"Error: ERROR_SEND_CSD_TIMEOUT\r\n");
    break;
    case ERROR_SEND_CSD_TOKEN_TIMEOUT:
      sprintf(buffer,"Error: ERROR_SEND_CSD_TOKEN_TIMEOUT\r\n");
    break;
    case ERROR_SEND_OP_COND_TIMEOUT:
      sprintf(buffer,"Error: ERROR_SEND_OP_COND_TIMEOUT\r\n");
    break;
    case ERROR_SEND_OP_COND_RESPONSE:
      sprintf(buffer,"Error: ERROR_SEND_OP_COND_RESPONSE\r\n");
    break;
    case SD_TIMEOUT:
      sprintf(buffer,"Error: SD_TIMEOUT\r\n");
    break;
    case ERROR_AQUIRE_LOCK_TIMEOUT:
	  sprintf(buffer,"Error: ERROR_AQUIRE_LOCK_TIMEOUT\r\n");
	break;
  }
  putLineUART(buffer);

  if(sderr!=SD_OK) {
    return sderr;
  }

  putLineUART("\r\n----------CARD INFO------------\r\n");

  switch(cardinfo.CardType){
    case SD_CARD_STD_CAPACITY_V1_1:sprintf(buffer,"Card Type: SD Card v1.1\r\n");break;
    case SD_CARD_STD_CAPACITY_V2_0:sprintf(buffer,"Card Type: SD Card v2.0\r\n");break;
    case SD_CARD_HIGH_CAPACITY:sprintf(buffer,"Card Type: SDHC Card\r\n");break;
    case MULTIMEDIA_CARD:sprintf(buffer,"Card Type: MMC Card\r\n");break;
  }
  putLineUART(buffer);

  sprintf(buffer,"ManufacturerID:%d",cardinfo.SD_cid.ManufacturerID);
  putLineUART(buffer);

  sprintf(buffer,"\r\nOEM_AppliID:%c%c",cardinfo.SD_cid.OEM_AppliID>>8,cardinfo.SD_cid.OEM_AppliID&0x00FF);
  putLineUART(buffer);

  sprintf(buffer,"\r\nProdName:%c%c%c%c%c",cardinfo.SD_cid.ProdName1>>24,(cardinfo.SD_cid.ProdName1&0x00FF0000)>>16,
       (cardinfo.SD_cid.ProdName1&0x0000FF00)>>8,cardinfo.SD_cid.ProdName1&0x000000FF,cardinfo.SD_cid.ProdName2);
  putLineUART(buffer);
  if(cardinfo.CardType==MULTIMEDIA_CARD) {
    sprintf(buffer,"%c",cardinfo.SD_cid.Reserved1);
    putLineUART(buffer);
  }

  sprintf(buffer,"\r\nProdRev:%d.%d",cardinfo.SD_cid.ProdRev>>4,cardinfo.SD_cid.ProdRev&0x0F);
  putLineUART(buffer);

  sprintf(buffer,"\r\nProdSN:0x%08X",cardinfo.SD_cid.ProdSN);
  putLineUART(buffer);

  if(cardinfo.CardType==MULTIMEDIA_CARD) {
    sprintf(buffer,"\r\nManufactDate:%04d-%02d",(cardinfo.SD_cid.ManufactDate>>4)+1997,cardinfo.SD_cid.ManufactDate&0x000F);
  }
  else {
    sprintf(buffer,"\r\nManufactDate:%04d-%02d",(cardinfo.SD_cid.ManufactDate>>4)+2000,cardinfo.SD_cid.ManufactDate&0x000F);
  }
  putLineUART(buffer);

  sprintf(buffer,"\r\nCapacity:%lld MB",cardinfo.CardCapacity/1048576);
  putLineUART(buffer);

  sprintf(buffer,"\r\nBlock Size:%d bytes",cardinfo.CardBlockSize);
  putLineUART(buffer);

  sprintf(buffer,"\r\nCSDStruct:%d",cardinfo.SD_csd.CSDStruct);
  putLineUART(buffer);
  sprintf(buffer,"\r\nSysSpecVersion:%d",cardinfo.SD_csd.SysSpecVersion);
  putLineUART(buffer);
  sprintf(buffer,"\r\nTAAC:0x%x(",cardinfo.SD_csd.TAAC);
  putLineUART(buffer);
  switch(cardinfo.SD_csd.TAAC>>3){
    case 0:sprintf(buffer,"reserved");break;
    case 1:sprintf(buffer,"1.0");break;
    case 2:sprintf(buffer,"1.2");break;
    case 3:sprintf(buffer,"1.3");break;
    case 4:sprintf(buffer,"1.5");break;
    case 5:sprintf(buffer,"2.0");break;
    case 6:sprintf(buffer,"2.5");break;
    case 7:sprintf(buffer,"3.0");break;
    case 8:sprintf(buffer,"3.5");break;
    case 9:sprintf(buffer,"4.0");break;
    case 10:sprintf(buffer,"4.5");break;
    case 11:sprintf(buffer,"5.0");break;
    case 12:sprintf(buffer,"5.5");break;
    case 13:sprintf(buffer,"6.0");break;
    case 14:sprintf(buffer,"7.0");break;
    case 15:sprintf(buffer,"8.0");break;
  }
  putLineUART(buffer);
  switch(cardinfo.SD_csd.TAAC&0x07){
    case 0:sprintf(buffer," x 1ns)");break;
    case 1:sprintf(buffer," x 10ns)");break;
    case 2:sprintf(buffer," x 100ns)");break;
    case 3:sprintf(buffer," x 1us)");break;
    case 4:sprintf(buffer," x 10us)");break;
    case 5:sprintf(buffer," x 100us)");break;
    case 6:sprintf(buffer," x 1ms)");break;
    case 7:sprintf(buffer," x 10ms)");break;
  }
  putLineUART(buffer);
  sprintf(buffer,"\r\nNSAC:%d",cardinfo.SD_csd.NSAC);
  putLineUART(buffer);
  sprintf(buffer,"\r\nMaxBusClkFrec:0x%x(",cardinfo.SD_csd.MaxBusClkFrec);
  putLineUART(buffer);
  switch((cardinfo.SD_csd.MaxBusClkFrec&0x78)>>3) {
    case 0:sprintf(buffer,"reserved");break;
    case 1:sprintf(buffer,"1.0");break;
    case 2:sprintf(buffer,"1.2");break;
    case 3:sprintf(buffer,"1.3");break;
    case 4:sprintf(buffer,"1.5");break;
    case 5:sprintf(buffer,"2.0");break;
    case 6:sprintf(buffer,"2.5");break;
    case 7:sprintf(buffer,"3.0");break;
    case 8:sprintf(buffer,"3.5");break;
    case 9:sprintf(buffer,"4.0");break;
    case 10:sprintf(buffer,"4.5");break;
    case 11:sprintf(buffer,"5.0");break;
    case 12:sprintf(buffer,"5.5");break;
    case 13:sprintf(buffer,"6.0");break;
    case 14:sprintf(buffer,"7.0");break;
    case 15:sprintf(buffer,"8.0");break;
  }
  putLineUART(buffer);
  switch(cardinfo.SD_csd.MaxBusClkFrec&0x03){
    case 0:sprintf(buffer," x 100kbit/s)");break;
    case 1:sprintf(buffer," x 1Mbit/s");break;
    case 2:sprintf(buffer," x 10Mbit/s)");break;
    case 3:sprintf(buffer," x 100Mbit/s)");break;
    default:sprintf(buffer," reserved)");break;
  }
  putLineUART(buffer);
  sprintf(buffer,"\r\nCardComdClasses:0x%x",cardinfo.SD_csd.CardComdClasses);
  putLineUART(buffer);
  sprintf(buffer,"\r\nRdBlockLen:%d(%db)",cardinfo.SD_csd.RdBlockLen,1<<cardinfo.SD_csd.RdBlockLen);
  putLineUART(buffer);
  sprintf(buffer,"\r\nPartBlockRead:%d(",cardinfo.SD_csd.PartBlockRead);
  putLineUART(buffer);
  if(cardinfo.SD_csd.PartBlockRead==0) {
    sprintf(buffer,"no)");
  }
  else {
    sprintf(buffer,"yes)");
  }
  putLineUART(buffer);
  sprintf(buffer,"\r\nWrBlockMisalign:%d(",cardinfo.SD_csd.WrBlockMisalign);
  putLineUART(buffer);
  if(cardinfo.SD_csd.WrBlockMisalign==0) {
    sprintf(buffer,"no)");
  }
  else {
    sprintf(buffer,"yes)");
  }
  putLineUART(buffer);
  sprintf(buffer,"\r\nRdBlockMisalign:%d(",cardinfo.SD_csd.RdBlockMisalign);
  putLineUART(buffer);
  if(cardinfo.SD_csd.RdBlockMisalign==0) {
    sprintf(buffer,"no)");
  }
  else {
    sprintf(buffer,"yes)");
  }
  putLineUART(buffer);
  sprintf(buffer,"\r\nDSRImpl:%d(",cardinfo.SD_csd.DSRImpl);
  putLineUART(buffer);
  if(cardinfo.SD_csd.DSRImpl==0) {
    sprintf(buffer,"no)");
  }
  else {
    sprintf(buffer,"yes)");
  }
  putLineUART(buffer);

  if(cardinfo.CardType!=SD_CARD_HIGH_CAPACITY) {
    sprintf(buffer,"\r\nMaxRdCurrentVDDMin:%d(",cardinfo.SD_csd.MaxRdCurrentVDDMin);
    putLineUART(buffer);
    switch(cardinfo.SD_csd.MaxRdCurrentVDDMin){
      case 0:sprintf(buffer,"0.5mA)");break;
      case 1:sprintf(buffer,"1mA)");break;
      case 2:sprintf(buffer,"5mA)");break;
      case 3:sprintf(buffer,"10mA)");break;
      case 4:sprintf(buffer,"25mA)");break;
      case 5:sprintf(buffer,"35mA)");break;
      case 6:sprintf(buffer,"60mA)");break;
      case 7:sprintf(buffer,"100mA)");break;
    }
    putLineUART(buffer);
    sprintf(buffer,"\r\nMaxRdCurrentVDDMax:%d(",cardinfo.SD_csd.MaxRdCurrentVDDMax);
    putLineUART(buffer);
    switch(cardinfo.SD_csd.MaxRdCurrentVDDMax){
      case 0:sprintf(buffer,"1mA)");break;
      case 1:sprintf(buffer,"5mA)");break;
      case 2:sprintf(buffer,"10mA)");break;
      case 3:sprintf(buffer,"25mA)");break;
      case 4:sprintf(buffer,"35mA)");break;
      case 5:sprintf(buffer,"45mA)");break;
      case 6:sprintf(buffer,"80mA)");break;
      case 7:sprintf(buffer,"200mA)");break;
    }
    putLineUART(buffer);
    sprintf(buffer,"\r\nMaxWrCurrentVDDMin:%d(",cardinfo.SD_csd.MaxWrCurrentVDDMin);
    putLineUART(buffer);
    switch(cardinfo.SD_csd.MaxWrCurrentVDDMin){
      case 0:sprintf(buffer,"0.5mA)");break;
      case 1:sprintf(buffer,"1mA)");break;
      case 2:sprintf(buffer,"5mA)");break;
      case 3:sprintf(buffer,"10mA)");break;
      case 4:sprintf(buffer,"25mA)");break;
      case 5:sprintf(buffer,"35mA)");break;
      case 6:sprintf(buffer,"60mA)");break;
      case 7:sprintf(buffer,"100mA)");break;
    }
    putLineUART(buffer);
    sprintf(buffer,"\r\nMaxWrCurrentVDDMax:%d(",cardinfo.SD_csd.MaxWrCurrentVDDMax);
    putLineUART(buffer);
    switch(cardinfo.SD_csd.MaxWrCurrentVDDMax){
      case 0:sprintf(buffer,"1mA)");break;
      case 1:sprintf(buffer,"5mA)");break;
      case 2:sprintf(buffer,"10mA)");break;
      case 3:sprintf(buffer,"25mA)");break;
      case 4:sprintf(buffer,"35mA)");break;
      case 5:sprintf(buffer,"45mA)");break;
      case 6:sprintf(buffer,"80mA)");break;
      case 7:sprintf(buffer,"200mA)");break;
    }
    putLineUART(buffer);
  }
  sprintf(buffer,"\r\nEraseGrSize:%d",cardinfo.SD_csd.EraseGrSize);
  putLineUART(buffer);
  sprintf(buffer,"\r\nEraseGrMul:%d",cardinfo.SD_csd.EraseGrMul);
  putLineUART(buffer);
  sprintf(buffer,"\r\nWrProtectGrSize:%d",cardinfo.SD_csd.WrProtectGrSize);
  putLineUART(buffer);
  sprintf(buffer,"\r\nWrProtectGrEnable:%d(",cardinfo.SD_csd.WrProtectGrEnable);
  putLineUART(buffer);
  if(cardinfo.SD_csd.WrProtectGrEnable==0) {
    sprintf(buffer,"no)");
  }
  else {
    sprintf(buffer,"yes)");
  }
  putLineUART(buffer);
  sprintf(buffer,"\r\nWrSpeedFact:%d(x%d)",cardinfo.SD_csd.WrSpeedFact,1<<cardinfo.SD_csd.WrSpeedFact);
  putLineUART(buffer);
  sprintf(buffer,"\r\nMaxWrBlockLen:%d(%db)",cardinfo.SD_csd.MaxWrBlockLen,1<<cardinfo.SD_csd.MaxWrBlockLen);
  putLineUART(buffer);
  sprintf(buffer,"\r\nWriteBlockPaPartial:%d(",cardinfo.SD_csd.WriteBlockPaPartial);
  putLineUART(buffer);
  if(cardinfo.SD_csd.WriteBlockPaPartial==0) {
    sprintf(buffer,"no)");
  }
  else {
    sprintf(buffer,"yes)");
  }
  putLineUART(buffer);
  sprintf(buffer,"\r\nContentProtectAppli:%d",cardinfo.SD_csd.ContentProtectAppli);
  putLineUART(buffer);
  sprintf(buffer,"\r\nFileFormatGroup:%d",cardinfo.SD_csd.FileFormatGroup);
  putLineUART(buffer);
  sprintf(buffer,"\r\nCopyFlag:%d(",cardinfo.SD_csd.CopyFlag);
  putLineUART(buffer);
  if(cardinfo.SD_csd.CopyFlag==0) {
    sprintf(buffer,"original)");
  }
  else {
    sprintf(buffer,"copied)");
  }
  putLineUART(buffer);
  sprintf(buffer,"\r\nPermWrProtect:%d(",cardinfo.SD_csd.PermWrProtect);
  putLineUART(buffer);
  if(cardinfo.SD_csd.PermWrProtect==0) {
    sprintf(buffer,"no)");
  }
  else {
    sprintf(buffer,"yes)");
  }
  putLineUART(buffer);
  sprintf(buffer,"\r\nTempWrProtect:%d(",cardinfo.SD_csd.TempWrProtect);
  putLineUART(buffer);
  if(cardinfo.SD_csd.TempWrProtect==0) {
    sprintf(buffer,"no)");
  }
  else {
    sprintf(buffer,"yes)");
  }
  putLineUART(buffer);
  sprintf(buffer,"\r\nFileFormat:%d",cardinfo.SD_csd.FileFormat);
  putLineUART(buffer);
  return SD_OK;
}

// Run a speed test reading the first 1000 blocks
void sd_speed_test(void){
  uint32_t t1,t2,i;

  putLineUART("\r\n\r\n------SPI READ SPEED TEST----------");

  t1=DWT_Get();
  for(i=0;i<1000;i+=1) {
    sd_read_block(i,bf);
  }
  t2=DWT_Get();

  sprintf(buffer,"\r\nRead speed - Total:%d bytes, Time:%d ms, Avg:%d kbytes/sec\r\n",
     SD_BLOCKSIZE*1000,(t2-t1)/(SystemCoreClock/1000),((SD_BLOCKSIZE*1000))/((t2-t1)/(SystemCoreClock/1000)));
  putLineUART(buffer);

}
