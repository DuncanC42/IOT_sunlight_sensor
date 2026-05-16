#include "si1145.h"

/* ── Helpers internes ─────────────────────────────────────────────────────── */

static HAL_StatusTypeDef si1145_write(I2C_HandleTypeDef *hi2c,
                                       uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    return HAL_I2C_Master_Transmit(hi2c, SI1145_ADDR, buf, 2, HAL_MAX_DELAY);
}

static HAL_StatusTypeDef si1145_read(I2C_HandleTypeDef *hi2c,
                                      uint8_t reg, uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef ret;
    ret = HAL_I2C_Master_Transmit(hi2c, SI1145_ADDR, &reg, 1, HAL_MAX_DELAY);
    if (ret != HAL_OK) return ret;
    return HAL_I2C_Master_Receive(hi2c, SI1145_ADDR, data, len, HAL_MAX_DELAY);
}

/**
 * @brief Écrit un paramètre dans la RAM interne du SI1145.
 *        Procédure : écrire la valeur dans PARAM_WR,
 *        puis envoyer la commande PARAM_SET | adresse_param.
 */
static HAL_StatusTypeDef si1145_write_param(I2C_HandleTypeDef *hi2c,
                                             uint8_t param, uint8_t val)
{
    HAL_StatusTypeDef ret;

    ret = si1145_write(hi2c, SI1145_REG_PARAM_WR, val);
    if (ret != HAL_OK) return ret;

    ret = si1145_write(hi2c, SI1145_REG_COMMAND, SI1145_PARAM_SET | param);
    if (ret != HAL_OK) return ret;

    /* Petite attente pour que le RESPONSE se mette à jour */
    HAL_Delay(1);
    return HAL_OK;
}

/* ── API publique ─────────────────────────────────────────────────────────── */

HAL_StatusTypeDef SI1145_Init(I2C_HandleTypeDef *hi2c)
{
    HAL_StatusTypeDef ret;
    uint8_t part_id = 0;

    /* 1. Vérifier la présence du capteur : PART_ID doit valoir 0x45 */
    ret = si1145_read(hi2c, SI1145_REG_PART_ID, &part_id, 1);
    if (ret != HAL_OK || part_id != 0x45) return HAL_ERROR;

    /* 2. Reset logiciel */
    ret = si1145_write(hi2c, SI1145_REG_COMMAND, SI1145_CMD_RESET);
    if (ret != HAL_OK) return ret;
    HAL_Delay(10);

    /* 3. HW_KEY : valeur obligatoire pour déverrouiller les écritures */
    ret = si1145_write(hi2c, SI1145_REG_HW_KEY, 0x17);
    if (ret != HAL_OK) return ret;

    /* 4. Coefficients UV (valeurs de référence issues de la datasheet SI1145) */
    ret = si1145_write(hi2c, SI1145_REG_UCOEF0, 0x29); if (ret != HAL_OK) return ret;
    ret = si1145_write(hi2c, SI1145_REG_UCOEF1, 0x89); if (ret != HAL_OK) return ret;
    ret = si1145_write(hi2c, SI1145_REG_UCOEF2, 0x02); if (ret != HAL_OK) return ret;
    ret = si1145_write(hi2c, SI1145_REG_UCOEF3, 0x00); if (ret != HAL_OK) return ret;

    /* 5. CHLIST : activer visible + IR + UV */
    ret = si1145_write_param(hi2c, SI1145_PARAM_CHLIST,
                             SI1145_CHLIST_EN_UV |
                             SI1145_CHLIST_EN_ALS_IR |
                             SI1145_CHLIST_EN_ALS_VIS);
    if (ret != HAL_OK) return ret;

    /* 6. Gain ADC visible et IR : gain=0 (×1) pour éviter la saturation en plein soleil */
    ret = si1145_write_param(hi2c, SI1145_PARAM_ALS_VIS_ADC_GAIN, 0x00);
    if (ret != HAL_OK) return ret;
    ret = si1145_write_param(hi2c, SI1145_PARAM_ALS_IR_ADC_GAIN, 0x00);
    if (ret != HAL_OK) return ret;

    /* 7. MISC visible : high signal range (bit 5) pour plein soleil */
    ret = si1145_write_param(hi2c, SI1145_PARAM_ALS_VIS_ADC_MISC, 0x20);
    if (ret != HAL_OK) return ret;

    /* 8. Taux de mesure automatique : 0xFF00 ≈ 255 × 31.25 µs ≈ 8 ms entre mesures */
    ret = si1145_write(hi2c, SI1145_REG_MEAS_RATE0, 0xFF); if (ret != HAL_OK) return ret;
    ret = si1145_write(hi2c, SI1145_REG_MEAS_RATE1, 0x00); if (ret != HAL_OK) return ret;

    /* 9. Lancer le mode ALS automatique (mesures continues) */
    ret = si1145_write(hi2c, SI1145_REG_COMMAND, SI1145_CMD_ALS_AUTO);
    if (ret != HAL_OK) return ret;

    HAL_Delay(10); /* laisser la première mesure se terminer */
    return HAL_OK;
}

HAL_StatusTypeDef SI1145_Read(I2C_HandleTypeDef *hi2c, SI1145_Data *data)
{
    HAL_StatusTypeDef ret;
    uint8_t buf[2];

    /* Visible */
    ret = si1145_read(hi2c, SI1145_REG_ALS_VIS_DATA0, buf, 2);
    if (ret != HAL_OK) return ret;
    data->visible = (uint16_t)(buf[0] | (buf[1] << 8));

    /* Infrarouge */
    ret = si1145_read(hi2c, SI1145_REG_ALS_IR_DATA0, buf, 2);
    if (ret != HAL_OK) return ret;
    data->infrared = (uint16_t)(buf[0] | (buf[1] << 8));

    /* UV index (×100 dans le registre) */
    ret = si1145_read(hi2c, SI1145_REG_UV_INDEX0, buf, 2);
    if (ret != HAL_OK) return ret;
    data->uv_index = (uint16_t)(buf[0] | (buf[1] << 8));

    return HAL_OK;
}
