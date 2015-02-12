#include "adc_spi.h"

static int32_t last_conv_time = 0;

void adc_spi_setup(void){
	SPI_CFG_T spiCfg;
	SPI_DELAY_CONFIG_T spiDelayCfg;

	Chip_SPI_Init(LPC_SPI1);

	spiCfg.ClkDiv = SystemCoreClock/18000000L-1; // 18MHz SPI clock, T_h = T_l = 27ns
	spiCfg.Mode = SPI_MODE_MASTER;
	spiCfg.ClockMode = SPI_CLOCK_MODE0;
	spiCfg.DataOrder = SPI_DATA_MSB_FIRST;
	spiCfg.SSELPol = SPI_CFG_SPOL0_LO;
	Chip_SPI_SetConfig(LPC_SPI1, &spiCfg);

	spiDelayCfg.PreDelay = 0; // may need to be set to 1 if read errors occur
	spiDelayCfg.PostDelay = 0;
	spiDelayCfg.FrameDelay = 0;
	spiDelayCfg.TransferDelay = 0;
	Chip_SPI_DelayConfig(LPC_SPI1, &spiDelayCfg);

	Chip_SPI_Enable(LPC_SPI1);
}

static uint16_t SPI_Transfer(uint16_t data) {
  while(~LPC_SPI1->STAT & SPI_STAT_TXRDY){};
  LPC_SPI0->TXDATCTL = SPI_TXDATCTL_LEN(16-1) | SPI_TXDATCTL_EOT | SPI_TXCTL_ASSERT_SSEL0 | data;
  while(~LPC_SPI1->STAT & SPI_STAT_RXRDY){};
  return LPC_SPI1->RXDAT;
}

uint16_t adc_read(uint16_t config){
	// Ensure >5us between conversions
	int32_t now;

	do{
		now = DWT_Get();
	}while(now - last_conv_time < 360); // 5us is 360 cycles at 72Mhz
	last_conv_time = now;

	// SPI transfer and return data
	return SPI_Transfer(config);
}
