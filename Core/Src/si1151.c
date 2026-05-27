#include "si1151.h"

/* ── Helpers internes ─────────────────────────────────────────────────────── */

static HAL_StatusTypeDef si1151_write(I2C_HandleTypeDef *hi2c,
                                       uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    return HAL_I2C_Master_Transmit(hi2c, SI1151_ADDR, buf, 2, HAL_MAX_DELAY);
}

static HAL_StatusTypeDef si1151_read(I2C_HandleTypeDef *hi2c,
                                      uint8_t reg, uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef ret;
    ret = HAL_I2C_Master_Transmit(hi2c, SI1151_ADDR, &reg, 1, HAL_MAX_DELAY);
    if (ret != HAL_OK) return ret;
    return HAL_I2C_Master_Receive(hi2c, SI1151_ADDR, data, len, HAL_MAX_DELAY);
}

/**
 * @brief Lit RESPONSE0 et retourne le CMD_CTR (4 bits bas).
 */
static uint8_t si1151_get_ctr(I2C_HandleTypeDef *hi2c)
{
    uint8_t r = 0;
    si1151_read(hi2c, SI1151_REG_RESPONSE0, &r, 1);
    return r & SI1151_RESP0_CTR_MASK;
}

/**
 * @brief Attend que CMD_CTR ait incrémenté (commande acceptée).
 *        old_ctr : valeur du compteur AVANT l'envoi de la commande.
 *        Timeout 100 ms.
 */
static HAL_StatusTypeDef si1151_wait_cmd(I2C_HandleTypeDef *hi2c, uint8_t old_ctr)
{
    uint8_t resp;
    uint32_t t0 = HAL_GetTick();
    do {
        if (si1151_read(hi2c, SI1151_REG_RESPONSE0, &resp, 1) != HAL_OK)
            return HAL_ERROR;
        if (resp & SI1151_RESP0_CMD_ERR)
            return HAL_ERROR;   /* commande rejetée par le capteur */
        if ((HAL_GetTick() - t0) > 100)
            return HAL_TIMEOUT;
    } while ((resp & SI1151_RESP0_CTR_MASK) == old_ctr);
    return HAL_OK;
}

/**
 * @brief Écrit un paramètre dans la RAM interne du SI1151.
 *        Procédure datasheet §5.3.1 :
 *          1. Lire RESPONSE0 et mémoriser CMD_CTR.
 *          2. Écrire valeur dans HOSTIN0.
 *          3. Écrire (PARAM_SET | param) dans COMMAND.
 *          4. Attendre que CMD_CTR ait incrémenté.
 */
static HAL_StatusTypeDef si1151_write_param(I2C_HandleTypeDef *hi2c,
                                             uint8_t param, uint8_t val)
{
    HAL_StatusTypeDef ret;
    uint8_t old_ctr = si1151_get_ctr(hi2c);

    ret = si1151_write(hi2c, SI1151_REG_HOSTIN0, val);
    if (ret != HAL_OK) return ret;

    ret = si1151_write(hi2c, SI1151_REG_COMMAND, SI1151_PARAM_SET | param);
    if (ret != HAL_OK) return ret;

    return si1151_wait_cmd(hi2c, old_ctr);
}

/**
 * @brief Envoie une commande simple (sans argument) et attend la confirmation.
 */
static HAL_StatusTypeDef si1151_send_cmd(I2C_HandleTypeDef *hi2c, uint8_t cmd)
{
    uint8_t old_ctr = si1151_get_ctr(hi2c);
    HAL_StatusTypeDef ret = si1151_write(hi2c, SI1151_REG_COMMAND, cmd);
    if (ret != HAL_OK) return ret;
    return si1151_wait_cmd(hi2c, old_ctr);
}

/* ── API publique ─────────────────────────────────────────────────────────── */

HAL_StatusTypeDef SI1151_Init(I2C_HandleTypeDef *hi2c)
{
    HAL_StatusTypeDef ret;
    uint8_t part_id = 0;

    /* 1. Vérifier la présence du capteur : PART_ID doit valoir 0x51 */
    ret = si1151_read(hi2c, SI1151_REG_PART_ID, &part_id, 1);
    if (ret != HAL_OK || part_id != 0x51) return HAL_ERROR;

    /* 2. Reset logiciel — après reset RESPONSE0 = 0x0F (CMD_CTR=1111) */
    ret = si1151_write(hi2c, SI1151_REG_COMMAND, SI1151_CMD_RESET_SW);
    if (ret != HAL_OK) return ret;
    HAL_Delay(25);

    /* 3. Remettre CMD_CTR à 0 pour avoir une base connue */
    ret = si1151_write(hi2c, SI1151_REG_COMMAND, SI1151_CMD_RESET_CTR);
    if (ret != HAL_OK) return ret;
    HAL_Delay(5);

    /* 4. Canal 0 : visible (white photodiode, ADCMUX=0x0B) */
    ret = si1151_write_param(hi2c, SI1151_PARAM_ADCCONFIG0, SI1151_ADCMUX_WHITE);
    if (ret != HAL_OK) return ret;

    /* gain ×1, HSIG=0 (indoor) — mettre SI1151_HSIG si plein soleil */
    ret = si1151_write_param(hi2c, SI1151_PARAM_ADCSENS0, 0x00);
    if (ret != HAL_OK) return ret;

    /* sortie 16 bits, pas de seuil */
    ret = si1151_write_param(hi2c, SI1151_PARAM_ADCPOST0, 0x00);
    if (ret != HAL_OK) return ret;

    /* COUNTER_INDEX=1 → utilise MEASCOUNT0 pour la période */
    ret = si1151_write_param(hi2c, SI1151_PARAM_MEASCONFIG0, 0x40);
    if (ret != HAL_OK) return ret;

    /* 5. Canal 1 : IR (small IR photodiode, ADCMUX=0x00) */
    ret = si1151_write_param(hi2c, SI1151_PARAM_ADCCONFIG1, SI1151_ADCMUX_SMALL_IR);
    if (ret != HAL_OK) return ret;

    ret = si1151_write_param(hi2c, SI1151_PARAM_ADCSENS1, 0x00);
    if (ret != HAL_OK) return ret;

    ret = si1151_write_param(hi2c, SI1151_PARAM_ADCPOST1, 0x00);
    if (ret != HAL_OK) return ret;

    /* COUNTER_INDEX=1 → même période que canal 0 */
    ret = si1151_write_param(hi2c, SI1151_PARAM_MEASCONFIG1, 0x40);
    if (ret != HAL_OK) return ret;

    /* 6. CHAN_LIST : activer canal 0 (visible) + canal 1 (IR) */
    ret = si1151_write_param(hi2c, SI1151_PARAM_CHAN_LIST,
                             SI1151_CHAN0_EN | SI1151_CHAN1_EN);
    if (ret != HAL_OK) return ret;

    /* 7. Taux de mesure : MEASRATE × 800 µs = période de base
     *    MEASCOUNT0 × MEASRATE × 800 µs = période effective par canal
     *    MEASRATE=1, MEASCOUNT0=10 → 10 × 1 × 800 µs = 8 ms */
    ret = si1151_write_param(hi2c, SI1151_PARAM_MEASRATE_H, 0x00);
    if (ret != HAL_OK) return ret;
    ret = si1151_write_param(hi2c, SI1151_PARAM_MEASRATE_L, 0x01);
    if (ret != HAL_OK) return ret;
    ret = si1151_write_param(hi2c, SI1151_PARAM_MEASCOUNT0, 10);
    if (ret != HAL_OK) return ret;

    /* 8. Démarrer le mode autonome (START) */
    ret = si1151_send_cmd(hi2c, SI1151_CMD_START);
    if (ret != HAL_OK) return ret;

    /* Laisser quelques mesures se terminer */
    HAL_Delay(50);
    return HAL_OK;
}

HAL_StatusTypeDef SI1151_Read(I2C_HandleTypeDef *hi2c, SI1151_Data *data)
{
    uint8_t irq, buf[2];

    /* Attendre que les deux canaux aient une mesure fraîche */
    uint32_t t0 = HAL_GetTick();
    do {
        if (si1151_read(hi2c, SI1151_REG_IRQ_STATUS, &irq, 1) != HAL_OK)
            return HAL_ERROR;
        if ((HAL_GetTick() - t0) > 200) return HAL_TIMEOUT;
    } while ((irq & 0x03) != 0x03);  /* attend bits 0 ET 1 */

    if (si1151_read(hi2c, SI1151_REG_HOSTOUT0, buf, 2) != HAL_OK) return HAL_ERROR;
    data->visible = (uint16_t)(buf[0] | (buf[1] << 8));

    if (si1151_read(hi2c, SI1151_REG_HOSTOUT2, buf, 2) != HAL_OK) return HAL_ERROR;
    data->infrared = (uint16_t)(buf[0] | (buf[1] << 8));

    return HAL_OK;
}