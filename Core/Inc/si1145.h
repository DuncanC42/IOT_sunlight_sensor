#ifndef SI1145_H
#define SI1145_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

/* I2C address (fixe, non modifiable sur le SI1145) */
#define SI1145_ADDR         (0x60 << 1)  /* HAL attend l'adresse shiftée */

/* ── Registres principaux ─────────────────────────────────────────────────── */
#define SI1145_REG_PART_ID      0x00
#define SI1145_REG_HW_KEY       0x07
#define SI1145_REG_MEAS_RATE0   0x08
#define SI1145_REG_MEAS_RATE1   0x09
#define SI1145_REG_UCOEF0       0x13
#define SI1145_REG_UCOEF1       0x14
#define SI1145_REG_UCOEF2       0x15
#define SI1145_REG_UCOEF3       0x16
#define SI1145_REG_PARAM_WR     0x17
#define SI1145_REG_COMMAND      0x18
#define SI1145_REG_RESPONSE     0x20
#define SI1145_REG_IRQ_STATUS   0x21
#define SI1145_REG_ALS_VIS_DATA0 0x22
#define SI1145_REG_ALS_VIS_DATA1 0x23
#define SI1145_REG_ALS_IR_DATA0  0x24
#define SI1145_REG_ALS_IR_DATA1  0x25
#define SI1145_REG_UV_INDEX0    0x2C
#define SI1145_REG_UV_INDEX1    0x2D

/* ── RAM paramètres (accès via PARAM_WR + commande PARAM_SET) ─────────────── */
#define SI1145_PARAM_CHLIST         0x01
#define SI1145_PARAM_ALS_VIS_ADC_MISC 0x12
#define SI1145_PARAM_ALS_IR_ADC_MISC  0x1F
#define SI1145_PARAM_ALS_VIS_ADC_GAIN 0x11
#define SI1145_PARAM_ALS_IR_ADC_GAIN  0x1E

/* ── Commandes ────────────────────────────────────────────────────────────── */
#define SI1145_CMD_RESET        0x01
#define SI1145_CMD_ALS_FORCE    0x06
#define SI1145_CMD_ALS_AUTO     0x0E
#define SI1145_PARAM_SET        0xA0  /* OR avec l'adresse du paramètre */

/* ── Bits CHLIST ──────────────────────────────────────────────────────────── */
#define SI1145_CHLIST_EN_UV     0x80
#define SI1145_CHLIST_EN_AUX    0x40
#define SI1145_CHLIST_EN_ALS_IR 0x20
#define SI1145_CHLIST_EN_ALS_VIS 0x10

/* ── Structure résultat ───────────────────────────────────────────────────── */
typedef struct {
    uint16_t visible;   /* lumière visible (ADC raw) */
    uint16_t infrared;  /* infrarouge (ADC raw)      */
    uint16_t uv_index;  /* UV index × 100 (ex: 125 = UV index 1.25) */
} SI1145_Data;

/* ── API publique ─────────────────────────────────────────────────────────── */

/**
 * @brief  Initialise le SI1145 (reset + config ALS + UV en mode auto).
 * @param  hi2c  Pointeur vers le handle I2C (ex: &hi2c1)
 * @retval HAL_OK si succès, HAL_ERROR sinon
 */
HAL_StatusTypeDef SI1145_Init(I2C_HandleTypeDef *hi2c);

/**
 * @brief  Lit les valeurs visible, IR et UV index.
 *         Le capteur doit être initialisé et en mode auto.
 * @param  hi2c  Pointeur vers le handle I2C
 * @param  data  Pointeur vers la structure résultat
 * @retval HAL_OK si succès, HAL_ERROR sinon
 */
HAL_StatusTypeDef SI1145_Read(I2C_HandleTypeDef *hi2c, SI1145_Data *data);

#endif /* SI1145_H */
