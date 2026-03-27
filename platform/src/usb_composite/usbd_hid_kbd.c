/**
 * @file usbd_hid_kbd.c
 * @brief USB HID Boot Keyboard class driver implementation.
 *
 * Registers as interface 2, using interrupt IN endpoint 0x83.
 * State is stored in a static struct so it survives pClassData pointer
 * swaps by the composite dispatcher.
 *
 * HID report descriptor: Standard Boot Protocol keyboard (45 bytes).
 */

#include "usbd_hid_kbd.h"

#include <string.h>
#include "usbd_def.h"
#include "usbd_ctlreq.h"

/* ── HID class constants ─────────────────────────────────────────────── */
#define HID_DESCRIPTOR_TYPE         0x21U
#define HID_REPORT_DESCRIPTOR_TYPE  0x22U
#define HID_REQ_SET_PROTOCOL        0x0BU
#define HID_REQ_GET_PROTOCOL        0x03U
#define HID_REQ_SET_IDLE            0x0AU
#define HID_REQ_GET_IDLE            0x02U
#define HID_REQ_SET_REPORT          0x09U

/* ── Static state (no heap allocation) ───────────────────────────────── */
static USBD_HID_KBD_HandleTypeDef s_hid_state;

/* ── HID keyboard report descriptor ─────────────────────────────────── */
/* Standard Boot Protocol keyboard, 45 bytes */
static const uint8_t HID_KeyboardReportDesc[] = {
    0x05U, 0x01U,   /* UsagePage(Generic Desktop)           */
    0x09U, 0x06U,   /* Usage(Keyboard)                      */
    0xA1U, 0x01U,   /* Collection(Application)              */
    /* Modifier keys — 8 bits, 1 field each */
    0x05U, 0x07U,   /*   UsagePage(Keyboard/Keypad)         */
    0x19U, 0xE0U,   /*   UsageMinimum(Left Control = 0xE0)  */
    0x29U, 0xE7U,   /*   UsageMaximum(Right GUI   = 0xE7)   */
    0x15U, 0x00U,   /*   LogicalMinimum(0)                  */
    0x25U, 0x01U,   /*   LogicalMaximum(1)                  */
    0x75U, 0x01U,   /*   ReportSize(1)                      */
    0x95U, 0x08U,   /*   ReportCount(8)                     */
    0x81U, 0x02U,   /*   Input(Data, Variable, Absolute)    */
    /* Reserved byte */
    0x95U, 0x01U,   /*   ReportCount(1)                     */
    0x75U, 0x08U,   /*   ReportSize(8)                      */
    0x81U, 0x03U,   /*   Input(Constant)                    */
    /* LED output (5 bits + 3-bit pad) */
    0x95U, 0x05U,   /*   ReportCount(5)                     */
    0x75U, 0x01U,   /*   ReportSize(1)                      */
    0x05U, 0x08U,   /*   UsagePage(LEDs)                    */
    0x19U, 0x01U,   /*   UsageMinimum(Num Lock)             */
    0x29U, 0x05U,   /*   UsageMaximum(Kana)                 */
    0x91U, 0x02U,   /*   Output(Data, Variable, Absolute)   */
    0x95U, 0x01U,   /*   ReportCount(1)                     */
    0x75U, 0x03U,   /*   ReportSize(3)                      */
    0x91U, 0x03U,   /*   Output(Constant)                   */
    /* Key array — 6 simultaneous keys */
    0x95U, 0x06U,   /*   ReportCount(6)                     */
    0x75U, 0x08U,   /*   ReportSize(8)                      */
    0x15U, 0x00U,   /*   LogicalMinimum(0)                  */
    0x25U, 0x65U,   /*   LogicalMaximum(101)                */
    0x05U, 0x07U,   /*   UsagePage(Keyboard/Keypad)         */
    0x19U, 0x00U,   /*   UsageMinimum(0)                    */
    0x29U, 0x65U,   /*   UsageMaximum(101)                  */
    0x81U, 0x00U,   /*   Input(Data, Array)                 */
    0xC0U           /* EndCollection                        */
};

#define HID_REPORT_DESC_SIZE  sizeof(HID_KeyboardReportDesc)

/* Per-interface HID descriptor, embedded inside the config descriptor */
static const uint8_t HID_KBD_Desc[] = {
    0x09U,                          /* bLength                 */
    HID_DESCRIPTOR_TYPE,            /* bDescriptorType (HID)   */
    0x11U, 0x01U,                   /* bcdHID 1.11             */
    0x00U,                          /* bCountryCode            */
    0x01U,                          /* bNumDescriptors         */
    HID_REPORT_DESCRIPTOR_TYPE,     /* bDescriptorType (Report)*/
    LOBYTE(HID_REPORT_DESC_SIZE),
    HIBYTE(HID_REPORT_DESC_SIZE)
};

/* ── Class driver callbacks ───────────────────────────────────────────── */

static uint8_t USBD_HID_KBD_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    (void)cfgidx;

    USBD_LL_OpenEP(pdev, HID_EPIN_ADDR, USBD_EP_TYPE_INTR, HID_EPIN_SIZE);
    pdev->ep_in[HID_EPIN_ADDR & 0x0FU].is_used = 1U;
    pdev->ep_in[HID_EPIN_ADDR & 0x0FU].bInterval = HID_FS_BINTERVAL;

    memset(&s_hid_state, 0, sizeof(s_hid_state));
    s_hid_state.state = HID_KBD_IDLE;

    /* Expose state via pClassData so compositor can save it */
    pdev->pClassData = &s_hid_state;

    return (uint8_t)USBD_OK;
}

static uint8_t USBD_HID_KBD_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    (void)cfgidx;
    USBD_LL_CloseEP(pdev, HID_EPIN_ADDR);
    pdev->ep_in[HID_EPIN_ADDR & 0x0FU].is_used = 0U;
    return (uint8_t)USBD_OK;
}

static uint8_t USBD_HID_KBD_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    USBD_HID_KBD_HandleTypeDef *hhid = (USBD_HID_KBD_HandleTypeDef *)pdev->pClassData;
    uint16_t len;
    const uint8_t *pbuf;
    uint16_t status = 0U;

    switch (req->bmRequest & USB_REQ_TYPE_MASK) {

    case USB_REQ_TYPE_CLASS:
        switch (req->bRequest) {
        case HID_REQ_SET_PROTOCOL:
            hhid->protocol = (uint32_t)(req->wValue);
            break;
        case HID_REQ_GET_PROTOCOL:
            USBD_CtlSendData(pdev, (uint8_t *)&hhid->protocol, 1U);
            break;
        case HID_REQ_SET_IDLE:
            hhid->idle_state = (uint32_t)(req->wValue >> 8);
            break;
        case HID_REQ_GET_IDLE:
            USBD_CtlSendData(pdev, (uint8_t *)&hhid->idle_state, 1U);
            break;
        default:
            USBD_CtlError(pdev, req);
            return (uint8_t)USBD_FAIL;
        }
        break;

    case USB_REQ_TYPE_STANDARD:
        switch (req->bRequest) {
        case USB_REQ_GET_STATUS:
            USBD_CtlSendData(pdev, (uint8_t *)&status, 2U);
            break;

        case USB_REQ_GET_DESCRIPTOR:
            if ((req->wValue >> 8) == HID_REPORT_DESCRIPTOR_TYPE) {
                pbuf = HID_KeyboardReportDesc;
                len  = (uint16_t)MIN((uint32_t)HID_REPORT_DESC_SIZE, (uint32_t)req->wLength);
                USBD_CtlSendData(pdev, (uint8_t *)(uintptr_t)pbuf, len);
            } else if ((req->wValue >> 8) == HID_DESCRIPTOR_TYPE) {
                pbuf = HID_KBD_Desc;
                len  = (uint16_t)MIN((uint32_t)sizeof(HID_KBD_Desc), (uint32_t)req->wLength);
                USBD_CtlSendData(pdev, (uint8_t *)(uintptr_t)pbuf, len);
            } else {
                USBD_CtlError(pdev, req);
                return (uint8_t)USBD_FAIL;
            }
            break;

        case USB_REQ_GET_INTERFACE:
            USBD_CtlSendData(pdev, (uint8_t *)&hhid->alt_setting, 1U);
            break;

        case USB_REQ_SET_INTERFACE:
            hhid->alt_setting = (uint32_t)(req->wValue);
            break;

        case USB_REQ_CLEAR_FEATURE:
            break;

        default:
            USBD_CtlError(pdev, req);
            return (uint8_t)USBD_FAIL;
        }
        break;

    default:
        USBD_CtlError(pdev, req);
        return (uint8_t)USBD_FAIL;
    }

    return (uint8_t)USBD_OK;
}

static uint8_t USBD_HID_KBD_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    (void)epnum;
    USBD_HID_KBD_HandleTypeDef *hhid = (USBD_HID_KBD_HandleTypeDef *)pdev->pClassData;

    /* Transmission complete — mark idle so the next report can be queued */
    hhid->state = HID_KBD_IDLE;
    return (uint8_t)USBD_OK;
}

/* GetFSConfigDescriptor is not needed — composite descriptor is built by
   usbd_cdc_hid_desc.c and the vtable pointer in USBD_Composite is used. */
static uint8_t *USBD_HID_KBD_GetConfigDesc(uint16_t *length)
{
    /* Should not be called directly when using composite descriptor */
    *length = 0U;
    return NULL;
}

uint8_t USBD_HID_KBD_SendReport(USBD_HandleTypeDef *pdev,
                                uint8_t *report,
                                uint16_t len)
{
    USBD_HID_KBD_HandleTypeDef *hhid = (USBD_HID_KBD_HandleTypeDef *)pdev->pClassData;

    if (hhid == NULL) {
        return (uint8_t)USBD_FAIL;
    }

    if ((pdev->dev_state == USBD_STATE_CONFIGURED) && (hhid->state == HID_KBD_IDLE)) {
        hhid->state = HID_KBD_BUSY;
        USBD_LL_Transmit(pdev, HID_EPIN_ADDR, report, len);
        return (uint8_t)USBD_OK;
    }
    return (uint8_t)USBD_BUSY;
}

USBD_ClassTypeDef USBD_HID_Keyboard = {
    .Init                          = USBD_HID_KBD_Init,
    .DeInit                        = USBD_HID_KBD_DeInit,
    .Setup                         = USBD_HID_KBD_Setup,
    .EP0_TxSent                    = NULL,
    .EP0_RxReady                   = NULL,
    .DataIn                        = USBD_HID_KBD_DataIn,
    .DataOut                       = NULL,
    .SOF                           = NULL,
    .IsoINIncomplete               = NULL,
    .IsoOUTIncomplete              = NULL,
    .GetHSConfigDescriptor         = USBD_HID_KBD_GetConfigDesc,
    .GetFSConfigDescriptor         = USBD_HID_KBD_GetConfigDesc,
    .GetOtherSpeedConfigDescriptor = USBD_HID_KBD_GetConfigDesc,
    .GetDeviceQualifierDescriptor  = NULL,
};
