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

/*****************************************************************************
 *
 * Filename:
 * ---------
 *    usbacm_adap.c
 *
 * Project:
 * --------
 *   Maui_Software
 *
 * Description:
 * ------------
 *   This file implements usb adaption layer for UART API
 *
 * Author:
 * -------
 *  Jensen Hu
 *
 *****************************************************************************/
#ifdef MTK_USB_DEMO_ENABLED

#define __USB_COM_PORT_ENABLE__
#ifdef __USB_COM_PORT_ENABLE__

#include <string.h>
#include "usbacm_drv.h"
#include "usbacm_adap.h"
#include "usb_custom.h"
#include "usb_custom_def.h"
#include "bmd.h"
#include "syslog.h"
#include "usb.h"

#ifdef MTK_PORT_SERVICE_ENABLE
#include "serial_port.h"
#include "serial_port_uart.h"
#endif

/* Exception flag */
extern kal_uint8 INT_Exception_Enter;


/* functions*/
void USB2UART_ClrRxBuffer(uint8_t port);
kal_uint16 USB2UART_GetRxAvail(uint8_t port);
kal_uint16 USB2UART_GetBufAvail(BUFFER_INFO *buf_info);
kal_uint16 USB2UART_GetBytes(uint8_t port, kal_uint8 *buffaddr, kal_uint16 length);
void USB2UART_ClrTxBuffer(uint8_t port);
kal_uint16 USB2UART_GetTxRoomLeft(uint8_t port);
kal_uint16 USB2UART_PutBytes(uint8_t port, kal_uint8 *buffaddr, kal_uint16 length);
void USB2UART_PutData_Polling(uint8_t port, kal_uint8 *Buffaddr, kal_uint16 Length);
extern void USB2UART_Tx_DMA_Callback(uint8_t port);
extern void USB2UART_Send_Intr_Pending(kal_uint32 usb_port);

void USB2UART_init(void);


static kal_bool USB2UART_Check_Config(kal_uint32 usb_port)
{
    if ((g_UsbACM[usb_port].txpipe == NULL) || (USB2UARTPort[usb_port].initialized == KAL_FALSE) || (USB_Get_Device_State() != DEVSTATE_CONFIG))
	{
        LOG_E(hal, "USB2UART_Check_Config, txpipe=0x%x, init=%d, state=%d\n", g_UsbACM[usb_port].txpipe, USB2UARTPort[usb_port].initialized, USB_Get_Device_State());
        return KAL_FALSE;
    }
    else
    {
        return KAL_TRUE;
    }
}


/* Initialize USB2UART setting, called when driver initialize, no matter user select as UART or not */
void USB2UART_init(void)
{
    kal_uint32 usb_port;

#ifdef MTK_PORT_SERVICE_ENABLE
    /* Initialize USB port value */
    USB_PORT[SERIAL_PORT_DEV_USB_COM1] = 0;

    /* Initialize USB port value */
    USB_PORT[SERIAL_PORT_DEV_USB_COM2] = 1;
#else
    USB_PORT[0] = 0;
    USB_PORT[1] = 1;
#endif


    /* Initialize USB port value */
    for (usb_port = 0; usb_port < MAX_USB_PORT_NUM; usb_port++) {

        g_UsbACM[usb_port].send_Txilm = KAL_FALSE;
        g_UsbACM[usb_port].send_Rxilm = KAL_TRUE;

        g_UsbACM[usb_port].config_send_Txilm = KAL_FALSE;
        USB2UARTPort[usb_port].tx_cb = USB2UART_Dafault_Tx_Callback;
        USB2UARTPort[usb_port].rx_cb = USB2UART_Dafault_Rx_Callback;
    }

}

/* Clear Tx Ring buffer */
void USB2UART_Clear_Tx_Buffer(uint8_t port)
{
    USB2UART_ClrTxBuffer(port);
}

/* Clear RX buffer */
void USB2UART_Clear_Rx_Buffer(uint8_t port)
{
    USB2UART_ClrRxBuffer(port);
}


/************************************************************
	UART driver  functions
*************************************************************/

/* Clear RX buffer */
void USB2UART_ClrRxBuffer(uint8_t port)
{
    kal_uint32 usb_port = USB_PORT[port];

    NVIC_DisableIRQ(USB20_IRQn);//IRQMask(IRQ_USB_CODE);
    /* Clear ring buffer */
    USB2UARTPort[usb_port].Rx_Buffer.Write = 0;
    USB2UARTPort[usb_port].Rx_Buffer.Read = 0;
    g_UsbACM[usb_port].send_Rxilm = KAL_TRUE;

    /* Clear hardware FIFO if current status is CDC ACM */
    if (USB2UART_Check_Config(usb_port) == KAL_TRUE) {
        /* Clear RX FIFO */
        USB_Acm_Rx_ClrFifo(port);
        /* Clear interrupt mask variable */
        /* Open intr */
//		USB_Set_UnMask_Irq(KAL_TRUE);
        USB_UnMask_COM_Intr(port);

    }
    NVIC_EnableIRQ(USB20_IRQn);//USB_IRQ_Unmask();
}


/* Get available bytes in RX buffer */
kal_uint16 USB2UART_GetRxAvail(uint8_t port)
{
    kal_uint32 usb_port = USB_PORT[port];
    kal_uint16 real_count;
    kal_uint32 savedMask;


    savedMask = SaveAndSetIRQMask();
    Buf_GetBytesAvail(&(USB2UARTPort[usb_port].Rx_Buffer), real_count);
    RestoreIRQMask(savedMask);
    return real_count;
}

/* Get available bytes in RX buffer */
kal_uint16 USB2UART_GetBufAvail(BUFFER_INFO *buf_info)
{
//	kal_uint32 usb_port = USB_PORT[port];
    kal_uint16 real_count;

    Buf_GetBytesAvail(buf_info, real_count);

    return real_count;
}

/* Get bytes from RX buffer, parameter status shows escape and break status
     return value is the actually get bytes */
kal_uint16 USB2UART_GetBytes(uint8_t port, kal_uint8 *buffaddr, kal_uint16 length)
{
    kal_uint32 usb_port = USB_PORT[port];
    kal_uint16 real_count;
    kal_uint16 RoomLeft;
    kal_uint32 savedMask;
    kal_int32 remain;
    BUFFER_INFO *rx_buf_info = &(USB2UARTPort[usb_port].Rx_Buffer);

    /* Return directly if not match conditions */
    if (USB2UART_Check_Config(usb_port) == KAL_FALSE) {
        return 0;
    }

    /* Determine real data count */
    /* Note that the area to determine send_Rxilm must also contain in critical section.
       Otherwise if USB HISR activated before send_Rxilm setting as true,
       this message will be lost */
    savedMask = SaveAndSetIRQMask();
    real_count = USB2UART_GetBufAvail(rx_buf_info);
    if (real_count >= length) {
        real_count = length;
    } else {
        g_UsbACM[usb_port].send_Rxilm = KAL_TRUE;  /*After this time get byte, buffer will be empty */
    }

    RestoreIRQMask(savedMask);

    if (real_count != 0) {
        remain = (BRead(rx_buf_info) + real_count) - BLength(rx_buf_info);

        if (remain < 0) {
            /* dest, src, len */
            kal_mem_cpy(buffaddr, BuffRead(rx_buf_info), real_count);
            BRead(rx_buf_info) += real_count;
        } else {
            kal_mem_cpy(buffaddr, BuffRead(rx_buf_info), real_count - remain);
            kal_mem_cpy((kal_uint8 *)(buffaddr + real_count - remain), BStartAddr(rx_buf_info), remain);
            BRead(rx_buf_info) = remain;
        }
    }

    NVIC_DisableIRQ(USB20_IRQn);//IRQMask(IRQ_USB_CODE);
    RoomLeft = USB2UART_GetBuffRoomLeft(rx_buf_info);

    if (RoomLeft >= HAL_USB_MAX_PACKET_SIZE_ENDPOINT_BULK_FULL_SPEED) {
        //USB_UnMask_COM_Intr(port);
        NVIC_EnableIRQ(USB20_IRQn);//USB_IRQ_Unmask();
    }

    return real_count;
}


/* Clear TX buffer */
void USB2UART_ClrTxBuffer(uint8_t port)
{
    kal_uint32 usb_port = USB_PORT[port];
    kal_uint32 savedMask;

    /* Stop DMA channel if current status is CDC ACM */
    if (USB2UART_Check_Config(usb_port) == KAL_TRUE) {
        hal_usb_stop_dma_channel(g_UsbACM[usb_port].txpipe->byEP, HAL_USB_EP_DIRECTION_TX);
        g_UsbACM[usb_port].setup_dma = KAL_FALSE;
        if (g_UsbACM[usb_port].put_start == KAL_TRUE) {
            g_UsbACM[usb_port].dmaf_setmember |= 0x20;
        }
    }
    savedMask = SaveAndSetIRQMask();
    USB2UARTPort[usb_port].Tx_Buffer.Write = 0;
    USB2UARTPort[usb_port].Tx_Buffer.Read = 0;

    {
        g_UsbACM[usb_port].send_Txilm = KAL_TRUE;
    }

    RestoreIRQMask(savedMask);

}


/*Get the left bytes for buffer */
kal_uint16 USB2UART_GetBuffRoomLeft(BUFFER_INFO *buf_info)
{
    kal_uint16 real_count;
    Buf_GetRoomLeft(buf_info, real_count);
    return real_count;
}
/*Get the left bytes for TX buffer */
kal_uint16 USB2UART_GetTxRoomLeft(uint8_t port)
{
    kal_uint32 usb_port = USB_PORT[port];
    kal_uint16 real_count;
    kal_uint32  savedMask;


    savedMask = SaveAndSetIRQMask();
    real_count = USB2UART_GetBuffRoomLeft(&(USB2UARTPort[usb_port].Tx_Buffer));
    RestoreIRQMask(savedMask);
    return real_count;
}


/* Put bytes to tx buffer, return value is the actually put out bytes */
kal_uint16 USB2UART_PutBytes(uint8_t port, kal_uint8 *buffaddr, kal_uint16 length)
{
    kal_uint32 usb_port = USB_PORT[port];
    kal_uint16  real_count;
    kal_uint32  savedMask;
    kal_bool  setup_dma = KAL_FALSE;
    kal_int32 	remain;
    BUFFER_INFO 	*tx_info = &(USB2UARTPort[usb_port].Tx_Buffer);

    /* Return directly if not match condition */
    if (USB2UART_Check_Config(usb_port) == KAL_FALSE) {
        if ((USB2UARTPort[usb_port].initialized == KAL_TRUE) && (USB_Get_Device_State() != DEVSTATE_CONFIG)) {
            g_UsbACM[usb_port].config_send_Txilm = KAL_TRUE;  /* for PC set config later then can issue the first message */
        }
        LOG_I(hal, "USB2UART_PutBytes, enumeration not ready");
        return 0;
    }

    /* The same issue as USB2UART_GetBytes()
       The area to determine send_Txilm must also contain in critical section.
       Otherwise if DMA callback activated before send_Txilm setting as true,
       this message will be lost */
    savedMask = SaveAndSetIRQMask();
    real_count = USB2UART_GetBuffRoomLeft(tx_info);
    if ((real_count == 0) && (g_UsbACM[usb_port].dma_txcb_just_done == KAL_TRUE)) {
        LOG_I(hal, "ASSERT");
    }

    g_UsbACM[usb_port].dma_txcb_just_done = KAL_FALSE;

    /* determine real sent data count */
    if (real_count > length) {
        real_count = length;
    } else {
        g_UsbACM[usb_port].send_Txilm = KAL_TRUE;  /*After this time put bytes, buffer will be full */
        g_UsbACM[usb_port].config_send_Txilm = KAL_TRUE; /* if be reseted, then it can issue the message waited for*/
    }
    RestoreIRQMask(savedMask);

    if (real_count != 0) {
        remain = (BWrite(tx_info) + real_count) - BLength(tx_info);

        if (remain < 0) {
            /* dest, src, len */
            kal_mem_cpy(BuffWrite(tx_info), buffaddr, real_count);
            BWrite(tx_info) += real_count;
        } else {
            kal_mem_cpy(BuffWrite(tx_info), buffaddr, real_count - remain);
            kal_mem_cpy(BStartAddr(tx_info), (kal_uint8 *)(buffaddr + real_count - remain), remain);
            BWrite(tx_info) = remain;
        }
    }

    savedMask = SaveAndSetIRQMask();
    /* In case USB is plugged out just before this critical section */
    if (g_UsbACM[usb_port].txpipe != NULL) {
        if (g_UsbACM[usb_port].setup_dma == KAL_FALSE) {
            g_UsbACM[usb_port].setup_dma = KAL_TRUE;
            setup_dma = KAL_TRUE;
            g_UsbACM[usb_port].put_start = KAL_TRUE;
            g_UsbACM[usb_port].dmaf_setmember = 0;
        }
    }
    RestoreIRQMask(savedMask);

    if (setup_dma == KAL_TRUE) {		
		if (USB2UART_Check_Config(usb_port) == KAL_FALSE) {
			if ((USB2UARTPort[usb_port].initialized == KAL_TRUE) && (USB_Get_Device_State() != DEVSTATE_CONFIG)) {
				g_UsbACM[usb_port].config_send_Txilm = KAL_TRUE;  /* for PC set config later then can issue the first message */
			}		
			LOG_E(hal, "USB2UART_PutBytes, enumeration not ready, stop dma transfer then return error\n");
			return 0;
		}

        USB2UART_DMATransmit(port);
    }
    g_UsbACM[usb_port].put_start = KAL_FALSE;
    g_UsbACM[usb_port].dmaf_setmember = 0;
    return real_count;
}

/* This function is only used for retrive exception log*/
void USB2UART_PutData_Polling(uint8_t port, kal_uint8 *Buffaddr, kal_uint16 Length)
{
    kal_uint32 usb_port = USB_PORT[port];

    hal_usb_write_endpoint_fifo(g_UsbACM[usb_port].txpipe->byEP, Length, Buffaddr);

    hal_usb_set_endpoint_tx_ready(g_UsbACM[usb_port].txpipe->byEP);

    while (!hal_usb_is_endpoint_tx_empty(g_UsbACM[usb_port].txpipe->byEP));

}

#endif /*__USB_COM_PORT_ENABLE__*/

#endif //MTK_USB_DEMO_ENABLED

