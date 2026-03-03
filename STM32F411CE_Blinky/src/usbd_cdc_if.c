#include "usbd_cdc_if.h"

#include <string.h>

#include "stm32f4xx_hal.h"
#include "usb_device.h"

#define APP_RX_DATA_SIZE  512U
#define APP_TX_DATA_SIZE  256U

extern USBD_HandleTypeDef hUsbDeviceFS;

static uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];
static uint8_t UserTxBufferFS[APP_TX_DATA_SIZE];

static volatile uint16_t cdc_rx_head = 0U;
static volatile uint16_t cdc_rx_tail = 0U;
static uint8_t cdc_rx_ring[APP_RX_DATA_SIZE];

static int8_t CDC_Init_FS(void);
static int8_t CDC_DeInit_FS(void);
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t *pbuf, uint16_t length);
static int8_t CDC_Receive_FS(uint8_t *buf, uint32_t *len);

USBD_CDC_ItfTypeDef USBD_Interface_fops_FS = {
    CDC_Init_FS,
    CDC_DeInit_FS,
    CDC_Control_FS,
    CDC_Receive_FS
};

static int8_t CDC_Init_FS(void)
{
    USBD_CDC_SetTxBuffer(&hUsbDeviceFS, UserTxBufferFS, 0U);
    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, UserRxBufferFS);
    return (USBD_CDC_ReceivePacket(&hUsbDeviceFS) == USBD_OK) ? (int8_t)USBD_OK : (int8_t)USBD_FAIL;
}

static int8_t CDC_DeInit_FS(void)
{
    return (int8_t)USBD_OK;
}

static int8_t CDC_Control_FS(uint8_t cmd, uint8_t *pbuf, uint16_t length)
{
    (void)cmd;
    (void)pbuf;
    (void)length;
    return (int8_t)USBD_OK;
}

static int8_t CDC_Receive_FS(uint8_t *buf, uint32_t *len)
{
    uint32_t index;

    if ((buf != NULL) && (len != NULL))
    {
        for (index = 0U; index < *len; ++index)
        {
            uint16_t next_head = (uint16_t)((cdc_rx_head + 1U) % APP_RX_DATA_SIZE);
            if (next_head == cdc_rx_tail)
            {
                break;
            }

            cdc_rx_ring[cdc_rx_head] = buf[index];
            cdc_rx_head = next_head;
        }
    }

    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, UserRxBufferFS);
    (void)USBD_CDC_ReceivePacket(&hUsbDeviceFS);
    return (int8_t)USBD_OK;
}

uint8_t UsbCdc_IsConfigured(void)
{
    return (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED) ? 1U : 0U;
}

int32_t UsbCdc_ReadByte(uint8_t *byte_out)
{
    if (byte_out == NULL)
    {
        return -1;
    }

    if (cdc_rx_head == cdc_rx_tail)
    {
        return 0;
    }

    *byte_out = cdc_rx_ring[cdc_rx_tail];
    cdc_rx_tail = (uint16_t)((cdc_rx_tail + 1U) % APP_RX_DATA_SIZE);
    return 1;
}

int32_t UsbCdc_Write(const uint8_t *data, uint16_t length)
{
    USBD_CDC_HandleTypeDef *hcdc;
    uint32_t start;
    uint16_t tx_length;

    if ((data == NULL) || (length == 0U))
    {
        return -1;
    }

    if (!UsbCdc_IsConfigured())
    {
        return 0;
    }

    hcdc = (USBD_CDC_HandleTypeDef *)hUsbDeviceFS.pClassData;
    if (hcdc == NULL)
    {
        return -1;
    }

    start = HAL_GetTick();
    while (hcdc->TxState != 0U)
    {
        if ((HAL_GetTick() - start) > 25U)
        {
            return 0;
        }
    }

    tx_length = (length > APP_TX_DATA_SIZE) ? APP_TX_DATA_SIZE : length;
    memcpy(UserTxBufferFS, data, tx_length);

    USBD_CDC_SetTxBuffer(&hUsbDeviceFS, UserTxBufferFS, tx_length);
    if (USBD_CDC_TransmitPacket(&hUsbDeviceFS) != USBD_OK)
    {
        return 0;
    }

    return tx_length;
}
