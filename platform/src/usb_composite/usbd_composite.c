/**
 * @file usbd_composite.c
 * @brief Composite USB device dispatcher — routes ST middleware callbacks
 *        to the correct sub-class driver based on interface and endpoint.
 *
 * pClassData swap protocol:
 *   ST's CDC and HID class drivers read pdev->pClassData directly.  In a
 *   composite device pClassData cannot point to two things at once.  This
 *   module saves each sub-class's allocated state pointer after Init() and
 *   swaps it in before every per-sub-class callback invocation.  The pointer
 *   is left pointing at the last sub-class after Init so that callbacks fired
 *   from IRQ context (DataIn/DataOut) receive their correct state.
 */

#include "usbd_composite.h"

#include <string.h>

/* ── Internal state ─────────────────────────────────────────────────────── */

static const usbd_sub_class_t *s_classes[USBD_COMPOSITE_MAX_CLASSES];
static void                   *s_class_data[USBD_COMPOSITE_MAX_CLASSES]; /* per-class pClassData */
static uint8_t                  s_num_classes;
static uint8_t                  s_ep0_owner;    /* class index that owns current EP0 Rx transfer */

/* ── Registration ───────────────────────────────────────────────────────── */

void usbd_composite_register(const usbd_sub_class_t *entry)
{
    if (s_num_classes < USBD_COMPOSITE_MAX_CLASSES) {
        s_classes[s_num_classes] = entry;
        s_num_classes++;
    }
}

void *usbd_composite_get_state(uint8_t class_index)
{
    if (class_index < s_num_classes) {
        return s_class_data[class_index];
    }
    return NULL;
}

/* ── Helpers ─────────────────────────────────────────────────────────────  */

/**
 * @brief Find which sub-class owns the given interface number.
 * @return class index, or 0xFF if not found.
 */
static uint8_t find_class_by_itf(uint8_t itf)
{
    for (uint8_t i = 0U; i < s_num_classes; i++) {
        if ((itf >= s_classes[i]->first_itf) &&
            (itf <  s_classes[i]->first_itf + s_classes[i]->itf_count)) {
            return i;
        }
    }
    return 0xFFU;
}

/**
 * @brief Find which sub-class owns the given IN endpoint address.
 * @return class index, or 0xFF if not found.
 */
static uint8_t find_class_by_ep_in(uint8_t ep_addr)
{
    for (uint8_t i = 0U; i < s_num_classes; i++) {
        for (uint8_t j = 0U; j < USBD_COMPOSITE_MAX_EPS; j++) {
            if (s_classes[i]->ep_in[j] == 0U) {
                break;
            }
            if (s_classes[i]->ep_in[j] == ep_addr) {
                return i;
            }
        }
    }
    return 0xFFU;
}

/**
 * @brief Find which sub-class owns the given OUT endpoint address.
 * @return class index, or 0xFF if not found.
 */
static uint8_t find_class_by_ep_out(uint8_t ep_addr)
{
    for (uint8_t i = 0U; i < s_num_classes; i++) {
        for (uint8_t j = 0U; j < USBD_COMPOSITE_MAX_EPS; j++) {
            if (s_classes[i]->ep_out[j] == 0U) {
                break;
            }
            if (s_classes[i]->ep_out[j] == ep_addr) {
                return i;
            }
        }
    }
    return 0xFFU;
}

/**
 * @brief Swap pUserData + pClassData for sub-class i and invoke callback.
 */
#define COMPOSITE_DISPATCH(pdev, idx, expr)          \
    do {                                             \
        (pdev)->pUserData  = (void *)(uintptr_t)s_classes[(idx)]->user_data; \
        (pdev)->pClassData = s_class_data[(idx)];    \
        (expr);                                      \
    } while (0)

/* ── USBD_ClassTypeDef callbacks ──────────────────────────────────────────  */

static uint8_t Composite_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    for (uint8_t i = 0U; i < s_num_classes; i++) {
        pdev->pUserData  = (void *)(uintptr_t)s_classes[i]->user_data;
        pdev->pClassData = NULL;

        if (s_classes[i]->cls->Init != NULL) {
            if (s_classes[i]->cls->Init(pdev, cfgidx) != (uint8_t)USBD_OK) {
                return (uint8_t)USBD_FAIL;
            }
        }
        /* Save whatever the sub-class Init() allocated into pClassData */
        s_class_data[i] = pdev->pClassData;
    }
    return (uint8_t)USBD_OK;
}

static uint8_t Composite_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
    for (uint8_t i = 0U; i < s_num_classes; i++) {
        if (s_classes[i]->cls->DeInit != NULL) {
            COMPOSITE_DISPATCH(pdev, i, s_classes[i]->cls->DeInit(pdev, cfgidx));
        }
    }
    return (uint8_t)USBD_OK;
}

static uint8_t Composite_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
    uint8_t recipient = (uint8_t)(req->bmRequest & 0x1FU);

    if (recipient == USBD_REQ_RECIPIENT_INTERFACE) {
        uint8_t itf = (uint8_t)(req->wIndex & 0xFFU);
        uint8_t idx = find_class_by_itf(itf);
        if (idx == 0xFFU) {
            USBD_CtlError(pdev, req);
            return (uint8_t)USBD_FAIL;
        }
        s_ep0_owner = idx;
        uint8_t ret;
        COMPOSITE_DISPATCH(pdev, idx, ret = s_classes[idx]->cls->Setup(pdev, req));
        return ret;
    }

    if (recipient == USBD_REQ_RECIPIENT_ENDPOINT) {
        uint8_t ep = (uint8_t)(req->wIndex & 0xFFU);
        uint8_t idx;
        if ((ep & 0x80U) != 0U) {
            idx = find_class_by_ep_in(ep);
        } else {
            idx = find_class_by_ep_out(ep);
        }
        if (idx == 0xFFU) {
            USBD_CtlError(pdev, req);
            return (uint8_t)USBD_FAIL;
        }
        uint8_t ret;
        COMPOSITE_DISPATCH(pdev, idx, ret = s_classes[idx]->cls->Setup(pdev, req));
        return ret;
    }

    /* Device-recipient requests: broadcast to all sub-classes */
    for (uint8_t i = 0U; i < s_num_classes; i++) {
        if (s_classes[i]->cls->Setup != NULL) {
            uint8_t ret;
            COMPOSITE_DISPATCH(pdev, i, ret = s_classes[i]->cls->Setup(pdev, req));
            if (ret != (uint8_t)USBD_OK) {
                return ret;
            }
        }
    }
    return (uint8_t)USBD_OK;
}

static uint8_t Composite_EP0_TxSent(USBD_HandleTypeDef *pdev)
{
    if (s_ep0_owner < s_num_classes && s_classes[s_ep0_owner]->cls->EP0_TxSent != NULL) {
        uint8_t ret;
        COMPOSITE_DISPATCH(pdev, s_ep0_owner, ret = s_classes[s_ep0_owner]->cls->EP0_TxSent(pdev));
        return ret;
    }
    return (uint8_t)USBD_OK;
}

static uint8_t Composite_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
    if (s_ep0_owner < s_num_classes && s_classes[s_ep0_owner]->cls->EP0_RxReady != NULL) {
        uint8_t ret;
        COMPOSITE_DISPATCH(pdev, s_ep0_owner, ret = s_classes[s_ep0_owner]->cls->EP0_RxReady(pdev));
        return ret;
    }
    return (uint8_t)USBD_OK;
}

static uint8_t Composite_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    uint8_t ep_addr = (uint8_t)(epnum | 0x80U);
    uint8_t idx     = find_class_by_ep_in(ep_addr);
    if (idx == 0xFFU) {
        return (uint8_t)USBD_OK; /* ep not owned by any class — ignore */
    }
    if (s_classes[idx]->cls->DataIn == NULL) {
        return (uint8_t)USBD_OK;
    }
    uint8_t ret;
    COMPOSITE_DISPATCH(pdev, idx, ret = s_classes[idx]->cls->DataIn(pdev, epnum));
    return ret;
}

static uint8_t Composite_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
    uint8_t idx = find_class_by_ep_out(epnum);
    if (idx == 0xFFU) {
        return (uint8_t)USBD_OK;
    }
    if (s_classes[idx]->cls->DataOut == NULL) {
        return (uint8_t)USBD_OK;
    }
    uint8_t ret;
    COMPOSITE_DISPATCH(pdev, idx, ret = s_classes[idx]->cls->DataOut(pdev, epnum));
    return ret;
}

/* Configuration descriptor is supplied by the concrete descriptor module. */
extern uint8_t *usbd_cdc_hid_get_fs_config_desc(uint16_t *length);

USBD_ClassTypeDef USBD_Composite = {
    .Init                          = Composite_Init,
    .DeInit                        = Composite_DeInit,
    .Setup                         = Composite_Setup,
    .EP0_TxSent                    = Composite_EP0_TxSent,
    .EP0_RxReady                   = Composite_EP0_RxReady,
    .DataIn                        = Composite_DataIn,
    .DataOut                       = Composite_DataOut,
    .SOF                           = NULL,
    .IsoINIncomplete               = NULL,
    .IsoOUTIncomplete              = NULL,
    .GetHSConfigDescriptor         = usbd_cdc_hid_get_fs_config_desc,
    .GetFSConfigDescriptor         = usbd_cdc_hid_get_fs_config_desc,
    .GetOtherSpeedConfigDescriptor = usbd_cdc_hid_get_fs_config_desc,
    .GetDeviceQualifierDescriptor  = NULL,
};
