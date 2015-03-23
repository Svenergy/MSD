#include "adc_spi.h"

void adc_spi_setup(void){
	SPI_CFG_T spiCfg;
	SPI_DELAY_CONFIG_T spiDelayCfg;

	Chip_SPI_Init(LPC_SPI1);

	spiCfg.ClkDiv = SystemCoreClock/36000000L-1; // 36MHz SPI clock, T_h = T_l = 13.5ns
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
