#ifndef SI1151_H
#define SI1151_H

#include "stm32f4xx_hal.h"

/* ── Adresse I2C (7-bit → shiftée pour le HAL STM32) ─────────────────────── */
#define SI1151_ADDR             (0x53 << 1)   /* 0xA6 */

/* ── Registres I2C ───────────────────────────────────────────────────────── */
#define SI1151_REG_PART_ID      0x00
#define SI1151_REG_HW_ID        0x01
#define SI1151_REG_REV_ID       0x02
#define SI1151_REG_HOSTIN0      0x0A   /* INPUT0 : valeur pour PARAM_SET     */
#define SI1151_REG_COMMAND      0x0B
#define SI1151_REG_IRQENABLE    0x0F
#define SI1151_REG_RESPONSE1    0x10   /* miroir du param écrit / lu          */
#define SI1151_REG_RESPONSE0    0x11   /* compteur de commandes + erreur      */
#define SI1151_REG_IRQ_STATUS   0x12
#define SI1151_REG_HOSTOUT0     0x13   /* données canal 0 LSB                */
#define SI1151_REG_HOSTOUT1     0x14   /* données canal 0 MSB                */
#define SI1151_REG_HOSTOUT2     0x15   /* données canal 1 LSB                */
#define SI1151_REG_HOSTOUT3     0x16   /* données canal 1 MSB                */

/* ── Commandes (COMMAND register) ───────────────────────────────────────── */
#define SI1151_CMD_RESET_CTR    0x00   /* remet CMD_CTR à 0                  */
#define SI1151_CMD_RESET_SW     0x01   /* reset logiciel complet             */
#define SI1151_CMD_FORCE        0x11   /* mesure forcée unique               */
#define SI1151_CMD_START        0x13   /* démarrage mode autonome            */
#define SI1151_CMD_PAUSE        0x12
#define SI1151_PARAM_SET        0x80   /* 0b10xxxxxx | param_addr            */
#define SI1151_PARAM_QUERY      0x40   /* 0b01xxxxxx | param_addr            */

/* ── Adresses dans la Parameter Table ───────────────────────────────────── */
#define SI1151_PARAM_CHAN_LIST      0x01
#define SI1151_PARAM_ADCCONFIG0    0x02   /* canal 0 : photodiode + décimation */
#define SI1151_PARAM_ADCSENS0      0x03   /* canal 0 : gain                    */
#define SI1151_PARAM_ADCPOST0      0x04   /* canal 0 : format sortie           */
#define SI1151_PARAM_MEASCONFIG0   0x05   /* canal 0 : counter index           */
#define SI1151_PARAM_ADCCONFIG1    0x06   /* canal 1                           */
#define SI1151_PARAM_ADCSENS1      0x07
#define SI1151_PARAM_ADCPOST1      0x08
#define SI1151_PARAM_MEASCONFIG1   0x09
#define SI1151_PARAM_MEASRATE_H    0x1A
#define SI1151_PARAM_MEASRATE_L    0x1B
#define SI1151_PARAM_MEASCOUNT0    0x1C   /* multiplicateur counter 0          */

/* ── Valeurs CHAN_LIST ───────────────────────────────────────────────────── */
#define SI1151_CHAN0_EN          (1 << 0)   /* canal 0 activé */
#define SI1151_CHAN1_EN          (1 << 1)   /* canal 1 activé */

/* ── Valeurs ADCCONFIG : ADCMUX ─────────────────────────────────────────── */
#define SI1151_ADCMUX_SMALL_IR   0x00   /* small IR photodiode  */
#define SI1151_ADCMUX_WHITE      0x0B   /* visible (white)      */

/* ── Bits ADCSENS ────────────────────────────────────────────────────────── */
#define SI1151_HSIG              (1 << 7)   /* high signal range (÷14.5)       */

/* ── Bits RESPONSE0 ──────────────────────────────────────────────────────── */
#define SI1151_RESP0_CMD_ERR     (1 << 4)
#define SI1151_RESP0_CTR_MASK    0x0F
#define SI1151_RESP0_RESET_VAL   0x0F   /* valeur après RESET_SW             */

/* ── Structure de données retournée ─────────────────────────────────────── */
typedef struct {
    uint16_t visible;    /* canal 0 : photodiode visible (white) */
    uint16_t infrared;   /* canal 1 : photodiode IR              */
} SI1151_Data;

/* ── API publique ────────────────────────────────────────────────────────── */
HAL_StatusTypeDef SI1151_Init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef SI1151_Read(I2C_HandleTypeDef *hi2c, SI1151_Data *data);

#endif /* SI1151_H */