/************************************************************************
* Library to read samples from AD7682 analog to digital converter
* http://www.analog.com/static/imported-files/data_sheets/AD7682_7689.pdf
* Kyle Smith
* 2-11-2015
*
* Read mode is read after conversion (RAC) without a busy indicator
* See "General Timing Without a Busy Indicator" and Fig.37 on p.26
************************************************************************/

#ifndef __ADC_SPI_
#define __ADC_SPI_

#include "board.h"
#include "delay.h"

//CFG register bit definitions
typedef enum {
	ADC_RB	 =  2, // CFG readback, 0 = read back, 1 = do not read back
	ADC_SEQ	 =  3, // Channel Sequencer
	ADC_REF	 =  5, // Reference select
	ADC_BW	 =  8, // Select Low-Pass BW, 0 = 1/4 BW, 1 = full BW
	ADC_IN	 =  9, // Input channel selection
	ADC_INCC = 12, // Input channel config
	ADC_CFG	 = 15, // 0 = keep config, 1 = overwrite config
};

void adc_spi_setup(void);

uint16_t adc_read(uint16_t config);

#endif /* __ADC_SPI_ */
