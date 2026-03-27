/**
 * @file usbd_conf_l0.c
 * @brief STM32L0 USB FS HAL glue for CDC + HID composite device.
 *
 * Key differences from the F4 version:
 *   - USB instance is `USB` (not `USB_OTG_FS`; no FIFO concept)
 *   - Packet memory (PMA) is configured via HAL_PCDEx_PMAConfig()
 *   - USB clock sourced from HSI48, synchronised by CRS to USB-SOF
 *   - PA11/PA12 are dedicated D−/D+ pins: no GPIO alternate-function config
 *
 * PMA layout (512 B total; BTABLE occupies bytes 0x000–0x03F):
 *   EP0 OUT            : offset 0x040, 64 B
 *   EP0 IN             : offset 0x080, 64 B
 *   CDC DATA OUT 0x01  : offset 0x0C0, 64 B
 *   CDC DATA IN  0x81  : offset 0x100, 64 B
 *   CDC CMD  IN  0x82  : offset 0x140,  8 B
 *   HID  IN      0x83  : offset 0x148,  8 B  — total used: 336 B of 512 B
 */

#include "stm32l0xx_hal.h"
#include "board.h"

#include "usbd_core.h"
#include "usbd_def.h"
#include "usbd_cdc.h"

extern USBD_HandleTypeDef hUsbDeviceFS;
extern PCD_HandleTypeDef  hpcd_USB;

/* ── USB clock: HSI48 + CRS ──────────────────────────────────────────── */

static void USB_Clock_Config(void)
{
    RCC_OscInitTypeDef       osc  = {0};
    RCC_PeriphCLKInitTypeDef pclk = {0};
    RCC_CRSInitTypeDef       crs  = {0};

    /* Enable HSI48 oscillator */
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSI48;
    osc.HSI48State     = RCC_HSI48_ON;
    HAL_RCC_OscConfig(&osc);

    /* Route HSI48 to the USB peripheral */
    pclk.PeriphClockSelection = RCC_PERIPHCLK_USB;
    pclk.UsbClockSelection    = RCC_USBCLKSOURCE_HSI48;
    HAL_RCCEx_PeriphCLKConfig(&pclk);

    /* CRS: synchronise HSI48 to USB-SOF for frequency accuracy */
    __HAL_RCC_CRS_CLK_ENABLE();
    crs.Prescaler             = RCC_CRS_SYNC_DIV1;
    crs.Source                = RCC_CRS_SYNC_SOURCE_USB;
    crs.Polarity              = RCC_CRS_SYNC_POLARITY_RISING;
    crs.ReloadValue           = __HAL_RCC_CRS_RELOADVALUE_CALCULATE(48000000U, 1000U);
    crs.ErrorLimitValue       = RCC_CRS_ERRORLIMIT_DEFAULT;
    crs.HSI48CalibrationValue = RCC_CRS_HSI48CALIBRATION_DEFAULT;
    HAL_RCCEx_CRSConfig(&crs);
}

/* ── MSP callbacks ───────────────────────────────────────────────────── */

void HAL_PCD_MspInit(PCD_HandleTypeDef *hpcd)
{
    if (hpcd->Instance == USB) {
        USB_Clock_Config();
        __HAL_RCC_USB_CLK_ENABLE();
        HAL_NVIC_SetPriority(BOARD_USB_IRQn, BOARD_USB_IRQ_PRIORITY, 0U);
        HAL_NVIC_EnableIRQ(BOARD_USB_IRQn);
    }
}

void HAL_PCD_MspDeInit(PCD_HandleTypeDef *hpcd)
{
    if (hpcd->Instance == USB) {
        HAL_NVIC_DisableIRQ(BOARD_USB_IRQn);
        __HAL_RCC_USB_CLK_DISABLE();
    }
}

/* ── USBD LL implementation ───────────────────────────────────────────── */

USBD_StatusTypeDef USBD_LL_Init(USBD_HandleTypeDef *pdev)
{
    hpcd_USB.Instance               = USB;
    hpcd_USB.Init.dev_endpoints     = 8U;
    hpcd_USB.Init.speed             = PCD_SPEED_FULL;
    hpcd_USB.Init.ep0_mps           = 0x40U;
    hpcd_USB.Init.phy_itface        = PCD_PHY_EMBEDDED;
    hpcd_USB.Init.Sof_enable        = DISABLE;
    hpcd_USB.Init.low_power_enable  = DISABLE;
    hpcd_USB.Init.lpm_enable        = DISABLE;

    hpcd_USB.pData = pdev;
    pdev->pData    = &hpcd_USB;

    if (HAL_PCD_Init(&hpcd_USB) != HAL_OK) {
        return USBD_FAIL;
    }

    /* Assign a PMA buffer region to each endpoint address */
    HAL_PCDEx_PMAConfig(&hpcd_USB, 0x00U, PCD_SNG_BUF, 0x040U); /* EP0 OUT       */
    HAL_PCDEx_PMAConfig(&hpcd_USB, 0x80U, PCD_SNG_BUF, 0x080U); /* EP0 IN        */
    HAL_PCDEx_PMAConfig(&hpcd_USB, 0x01U, PCD_SNG_BUF, 0x0C0U); /* CDC DATA OUT  */
    HAL_PCDEx_PMAConfig(&hpcd_USB, 0x81U, PCD_SNG_BUF, 0x100U); /* CDC DATA IN   */
    HAL_PCDEx_PMAConfig(&hpcd_USB, 0x82U, PCD_SNG_BUF, 0x140U); /* CDC CMD  IN   */
    HAL_PCDEx_PMAConfig(&hpcd_USB, 0x83U, PCD_SNG_BUF, 0x148U); /* HID      IN   */

    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_DeInit(USBD_HandleTypeDef *pdev)
{
    HAL_PCD_DeInit(pdev->pData);
    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_Start(USBD_HandleTypeDef *pdev)
{
    HAL_PCD_Start(pdev->pData);
    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_Stop(USBD_HandleTypeDef *pdev)
{
    HAL_PCD_Stop(pdev->pData);
    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_OpenEP(USBD_HandleTypeDef *pdev,
                                   uint8_t ep_addr, uint8_t ep_type,
                                   uint16_t ep_mps)
{
    HAL_PCD_EP_Open(pdev->pData, ep_addr, ep_mps, ep_type);
    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_CloseEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    HAL_PCD_EP_Close(pdev->pData, ep_addr);
    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_FlushEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    HAL_PCD_EP_Flush(pdev->pData, ep_addr);
    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_StallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    HAL_PCD_EP_SetStall(pdev->pData, ep_addr);
    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_ClearStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    HAL_PCD_EP_ClrStall(pdev->pData, ep_addr);
    return USBD_OK;
}

uint8_t USBD_LL_IsStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    PCD_HandleTypeDef *hpcd = (PCD_HandleTypeDef *)pdev->pData;
    if ((ep_addr & 0x80U) != 0U) {
        return hpcd->IN_ep[ep_addr & 0x7FU].is_stall;
    }
    return hpcd->OUT_ep[ep_addr & 0x7FU].is_stall;
}

USBD_StatusTypeDef USBD_LL_SetUSBAddress(USBD_HandleTypeDef *pdev, uint8_t dev_addr)
{
    HAL_PCD_SetAddress(pdev->pData, dev_addr);
    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_Transmit(USBD_HandleTypeDef *pdev,
                                     uint8_t ep_addr, uint8_t *pbuf,
                                     uint32_t size)
{
    HAL_PCD_EP_Transmit(pdev->pData, ep_addr, pbuf, (uint16_t)size);
    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_PrepareReceive(USBD_HandleTypeDef *pdev,
                                          uint8_t ep_addr, uint8_t *pbuf,
                                          uint32_t size)
{
    HAL_PCD_EP_Receive(pdev->pData, ep_addr, pbuf, (uint16_t)size);
    return USBD_OK;
}

uint32_t USBD_LL_GetRxDataSize(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    return HAL_PCD_EP_GetRxCount(pdev->pData, ep_addr);
}

void USBD_LL_Delay(uint32_t delay)
{
    HAL_Delay(delay);
}

/* ── PCD HAL callbacks → USBD core ───────────────────────────────────── */

void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_LL_SetupStage((USBD_HandleTypeDef *)hpcd->pData, (uint8_t *)hpcd->Setup);
}

void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_DataOutStage((USBD_HandleTypeDef *)hpcd->pData,
                         epnum, hpcd->OUT_ep[epnum].xfer_buff);
}

void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_DataInStage((USBD_HandleTypeDef *)hpcd->pData,
                        epnum, hpcd->IN_ep[epnum].xfer_buff);
}

void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_LL_SOF((USBD_HandleTypeDef *)hpcd->pData);
}

void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_LL_SetSpeed((USBD_HandleTypeDef *)hpcd->pData, USBD_SPEED_FULL);
    USBD_LL_Reset((USBD_HandleTypeDef *)hpcd->pData);
}

void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_LL_Suspend((USBD_HandleTypeDef *)hpcd->pData);
}

void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_LL_Resume((USBD_HandleTypeDef *)hpcd->pData);
}

void HAL_PCD_ISOOUTIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_IsoOUTIncomplete((USBD_HandleTypeDef *)hpcd->pData, epnum);
}

void HAL_PCD_ISOINIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_IsoINIncomplete((USBD_HandleTypeDef *)hpcd->pData, epnum);
}

void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_LL_DevConnected((USBD_HandleTypeDef *)hpcd->pData);
}

void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_LL_DevDisconnected((USBD_HandleTypeDef *)hpcd->pData);
}
