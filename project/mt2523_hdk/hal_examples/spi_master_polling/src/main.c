/* Copyright Statement:
 *
 * (C) 2005-2016  MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its licensors.
 * Without the prior written permission of MediaTek and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) MediaTek Software
 * if you have agreed to and been bound by the applicable license agreement with
 * MediaTek ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* hal includes */
#include "hal.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

#ifdef __GNUC__
int __io_putchar(int ch)
#else
int fputc(int ch, FILE *f)
#endif
{
    /* Place your implementation of fputc here */
    /* e.g. write a character to the HAL_UART_2 one at a time */
    hal_uart_put_char(HAL_UART_2, ch);
    return ch;
}

/**
*@brief Configure and initialize UART hardware initialization for logging.
*@param None.
*@return None.
*/
static void plain_log_uart_init(void)
{
    hal_uart_config_t uart_config;
    /* gpio config for uart2 */
    hal_gpio_init(HAL_GPIO_0);
    hal_gpio_init(HAL_GPIO_1);
    hal_pinmux_set_function(HAL_GPIO_0, HAL_GPIO_0_U2RXD);
    hal_pinmux_set_function(HAL_GPIO_1, HAL_GPIO_1_U2TXD);

    /* COM port settings */
    uart_config.baudrate = HAL_UART_BAUDRATE_115200;
    uart_config.word_length = HAL_UART_WORD_LENGTH_8;
    uart_config.stop_bit = HAL_UART_STOP_BIT_1;
    uart_config.parity = HAL_UART_PARITY_NONE;
    hal_uart_init(HAL_UART_2, &uart_config);
}

/**
*@brief Configure and initialize the systerm clock. In this example, we invoke hal_clock_init to initialize clock driver and clock gates.
*@param None.
*@return None.
*/
static void SystemClock_Config(void)
{
    hal_clock_init();
}

/**
*@brief  Initialize the periperal driver in this function. In this example, we initialize UART, Flash, and NVIC drivers.
*@param None.
*@return None.
*/
static void prvSetupHardware(void)
{
    /* Peripherals initialization */
    plain_log_uart_init();
    hal_flash_init();
    hal_nvic_init();
}

/**
*@brief  Example of SPI master send data with polling mode.
*@param None.
*@return None.
*/
static void spim_send_data_polling_example()
{
    uint8_t send_data[2] = {0x7E, 0x55};
    uint32_t frequency = 1000000;
    hal_spi_master_config_t spi_config;

    printf("---spim_example begin---\r\n");

    /*Step1: Init GPIO, set GPIO pinmux(if EPT tool hasn't been used to configure the related pinmux).*/
    hal_gpio_init(HAL_GPIO_25);
    hal_gpio_init(HAL_GPIO_26);
    hal_gpio_init(HAL_GPIO_27);
    hal_gpio_init(HAL_GPIO_28);

    /* Call hal_pinmux_set_function() to set GPIO pinmux*/
    hal_pinmux_set_function(HAL_GPIO_25, HAL_GPIO_25_MA_SPI3_A_CS);
    hal_pinmux_set_function(HAL_GPIO_26, HAL_GPIO_26_MA_SPI3_A_SCK);
    hal_pinmux_set_function(HAL_GPIO_27, HAL_GPIO_27_MA_SPI3_A_MOSI);
    hal_pinmux_set_function(HAL_GPIO_28, HAL_GPIO_28_MA_SPI3_A_MISO);

    /*Step2: Init SPI master and send data to lave. */
    spi_config.bit_order = HAL_SPI_MASTER_LSB_FIRST;
    spi_config.slave_port = HAL_SPI_MASTER_SLAVE_0;
    spi_config.clock_frequency = frequency;
    spi_config.phase = HAL_SPI_MASTER_CLOCK_PHASE0;
    spi_config.polarity = HAL_SPI_MASTER_CLOCK_POLARITY0;
    if (HAL_SPI_MASTER_STATUS_OK != hal_spi_master_init(HAL_SPI_MASTER_3, &spi_config)) {
        printf("\r\nSPI master init fail\r\n");
        return;
    }

    if (HAL_SPI_MASTER_STATUS_OK != hal_spi_master_send_polling(HAL_SPI_MASTER_3, send_data, 2)) {
        printf("hal_spi_master_send_polling fail\n");
        return;
    }

    /*Step3: Deinit spi master and GPIO.*/
    if (HAL_SPI_MASTER_STATUS_OK != hal_spi_master_deinit(HAL_SPI_MASTER_3)) {
        printf("hal_spi_master_deinit fail\n");
        return;
    }
    hal_gpio_deinit(HAL_GPIO_25);
    hal_gpio_deinit(HAL_GPIO_26);
    hal_gpio_deinit(HAL_GPIO_27);
    hal_gpio_deinit(HAL_GPIO_28);

    printf("---spim_example end---\r\n");
}

int main(void)
{
    /* SystemClock Config */
    SystemClock_Config();

    SystemCoreClockUpdate();

    /* Configure the hardware ready to run the test. */
    prvSetupHardware();

    /* enable I,F bits */
    __enable_irq();
    __enable_fault_irq();

    /* Add your application code here */
    printf("\r\n\r\n");/* The output UART used by log_hal_info is set by log_uart_init() */
    printf("welcome to main()\r\n");
    printf("pls add your own code from here\r\n");
    printf("\r\n\r\n");

    spim_send_data_polling_example();

    while (1);
}

