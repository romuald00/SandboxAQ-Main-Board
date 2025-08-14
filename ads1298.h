/*
 * ads1298.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Nov 14, 2023
 *      Author: jzhang
 */

#ifndef LIB_INC_ADS1298_H_
#define LIB_INC_ADS1298_H_

#include "cmsis_os.h"
#include "registerParams.h"
#include "saqTarget.h"

#include <stdbool.h>

typedef struct {
    union {
        uint32_t comp_threshold_MPERC;
        uint32_t current_nA;
        uint32_t samplePerSec;
    };
    uint8_t bitSetting;
} bitSettingForValue_t;

#define ADS1298_VREF 2500.0
#define ADS1298_FULL_SCALE 0x7FFFFF

#define ADS1298_REG_ADDR_SIZE_BYTES 2
#define ADS1298_DATA_SIZE_BYTES 1
#define ADS1298_CMD_BYTES 1

// ADC commands
#define ADS1298_WAKEUP 0x02
#define ADS1298_STANDBY 0x04
#define ADS1298_RESET 0x06
#define ADS1298_START 0x08
#define ADS1298_STOP 0x0A
#define ADS1298_RDATAC 0x10
#define ADS1298_SDATAC 0x11
#define ADS1298_RDATA 0x12

#define ADS1298_REG_READ_ADDR_CMD 0b00100000
#define ADS1298_REG_WRITE_ADDR_CMD 0b01000000
#define ADS1298_REG_ADDR_MASK 0x1F
#define ADS1298_NUM_REG_MASK 0x1F

// ADC registers
#define ADS1298_NUM_REGISTERS 26 // 0x1A

// 9.6.1.1 ID REGISTER Address 0x00
#define IDCTRL_ADDR 0x00
#define IDCTRL_ADS1298_FAMILY 4
#define IDCTRL_ADS_FAMILY_OFFSET 5
#define IDCTRL_8CHAN 2
#define IDCTRL_ADS_CHANNEL_OFFSET 0
#define IDCTRL_ADS1298_RO                                                                                              \
    ((IDCTRL_ADS1298_FAMILY << IDCTRL_ADS_FAMILY_OFFSET) | (IDCTRL_8CHAN << IDCTRL_ADS_CHANNEL_OFFSET))

// 9.6.1.2 CONFIG1 Address 0x01h reset = 0x06
#define CFGREG1_ADDR 0x01
// CFGREG1 Bit settings:
// bit [7]
#define CFGREG1_HIGH_RESOLUTION 0x80
#define CFGREG1_MULTI_READBACK 0x40
// bit[3:0]
#define CFGREG1_32KSPS_HR 0x00
#define CFGREG1_16KSPS_HR 0x01
#define CFGREG1_8KSPS_HR 0x02
#define CFGREG1_4KSPS_HR 0x03
#define CFGREG1_2KSPS_HR 0x04
#define CFGREG1_1KSPS_HR 0x05
#define CFGREG1_500SPS_HR 0x06
#define CFGREG1_DONT_USE 0x07

#define CFGREG1_CLEAR_SAMPLERATE(x) (x = x & (~(7 << 0)))
#define CFGREG1_SET_SAMPLERATE(x, y)                                                                                   \
    assert(y < CFGREG1_DONT_USE);                                                                                      \
    CFGREG1_CLEAR_SAMPLERATE(x);                                                                                       \
    x = x | (y << 0)

// CFGREG2
// 9.6.1.3 CONFIG2 Address 0x01h reset = 0x80
#define CFGREG2_ADDR 0x02
#define CFGREG2_RESVD_BIT7_SET 0x80

// 9.6.1.4 CONFIG3 Address 0x03h reset = 0x40
#define CFGREG3_ADDR 0x03
#define CFGREG3_BUFFER_REFEENCE_OFF (0 << 7)
#define CFGREG3_BUFFER_REFEENCE_ON (1 << 7)
#define CFGREG3_RESVD6 (1 << 6)
#define CFGREG3_REF_2V4 (0 << 5)
#define CFGREG3_RLD_MEAS_OPEN (0 << 4)
#define CFGREG3_RLDREF_INTERNAL (1 << 3)
#define CFGREG3_PWR_RLD_ENABLED (1 << 2)
#define CFGREG3_RLD_LOFF_SENS_DISABLE (0 << 1)
#define CFGREG3_RLD_LOFF_SENS_ENABLE (1 << 1)
#define CFGREG3_RLD_STAT_NOT_CONNECTED (1 << 0)

// bit[7] + reserved bit
#define CFGREG3_INTERNAL_REF 0xc0

#ifdef ADS1298_CREATE_TABLE_INSTANCE
const bitSettingForValue_t cfg1HRSampleRate[] = {
    {.bitSetting = CFGREG1_32KSPS_HR, .samplePerSec = 32000},
    {.bitSetting = CFGREG1_16KSPS_HR, .samplePerSec = 16000},
    {.bitSetting = CFGREG1_8KSPS_HR, .samplePerSec = 8000},
    {.bitSetting = CFGREG1_4KSPS_HR, .samplePerSec = 4000},
    {.bitSetting = CFGREG1_2KSPS_HR, .samplePerSec = 2000},
    {.bitSetting = CFGREG1_1KSPS_HR, .samplePerSec = 1000},
    {.bitSetting = CFGREG1_500SPS_HR, .samplePerSec = 500},
};

const bitSettingForValue_t loffCompPositiveSide[] = {
    {.bitSetting = 0, .comp_threshold_MPERC = 95000},
    {.bitSetting = 1, .comp_threshold_MPERC = 92500},
    {.bitSetting = 2, .comp_threshold_MPERC = 90000},
    {.bitSetting = 3, .comp_threshold_MPERC = 87500},
    {.bitSetting = 4, .comp_threshold_MPERC = 85000},
    {.bitSetting = 5, .comp_threshold_MPERC = 80000},
    {.bitSetting = 6, .comp_threshold_MPERC = 75000},
    {.bitSetting = 7, .comp_threshold_MPERC = 70000},
};

const bitSettingForValue_t loffCompNegativeSide[] = {
    {.bitSetting = 0, .comp_threshold_MPERC = 5000},
    {.bitSetting = 1, .comp_threshold_MPERC = 7500},
    {.bitSetting = 2, .comp_threshold_MPERC = 10000},
    {.bitSetting = 3, .comp_threshold_MPERC = 12500},
    {.bitSetting = 4, .comp_threshold_MPERC = 15000},
    {.bitSetting = 5, .comp_threshold_MPERC = 20000},
    {.bitSetting = 6, .comp_threshold_MPERC = 25000},
    {.bitSetting = 7, .comp_threshold_MPERC = 30000},
};

const bitSettingForValue_t loffIleadCurrentMagnitude[] = {
    {.bitSetting = 0, .current_nA = 6},
    {.bitSetting = 1, .current_nA = 12},
    {.bitSetting = 2, .current_nA = 18},
    {.bitSetting = 3, .current_nA = 24},
};
#endif

// 9.6.1.5 LOFF (lead off control register) Address 0x04h reset = 0x40
#define LOFF_ADDR 0x04
#define LOFFREG_COMP_CLEAR(X) (X &= (~(0x7 << 5)))
#define LOFFREG_COMP_SETTING(BITVALUE) (BITVALUE << 5)
#define LOFFREG_COMP_GET(BITVALUE) (BITVALUE >> 5)
#define LOFFREG_VLEAD_OFF_CURRENT (0 << 4)
#define LOFFREG_VLEAD_OFF_VOLTAGE (1 << 4)
#define LOFFREG_ILEAD_OFF_SET(BITVALUE) (BITVALUE << 2)
#define LOFFREG_ILEAD_OFF_GET(reg)                                                                                     \
    reg = reg & (3 << 2);                                                                                              \
    reg = (reg >> 2)
#define LOFFREG_FLEAD_OFF_DC_DETECTION (3 << 0)
#define LOFFREG_FLEAD_OFF_AC_DETECTION (1 << 0)
// 9.6.1.6 CHnSET Address 0x05 - 0x0ch reset = 0x00

#define ADS1298_CHSET_BASE_ADDR 0x05
#define CHXREG_ADDR(X) (ADS1298_CHSET_BASE_ADDR + X)

// bit[6:4] Gain
#define CHN_GAIN(BIT) (BIT << 4)
#define CHNSET_GAIN_6 0x00
#define CHNSET_GAIN_1 0x1
#define CHNSET_GAIN_2 0x2
#define CHNSET_GAIN_3 0x3
#define CHNSET_GAIN_4 0x4
#define CHNSET_GAIN_8 0x5
#define CHNSET_GAIN_12 0x6

// bit[2:0]
#define CHNSET_INPUT_SHORT 0x01
#define CHNSET_INPUT_NORMAL 0x00

// 9.6.1.7 RLD_SENSP 0x0D
#define STARTING_INP_CHAN 1 // Channel numbering starts at 1 but bit index is 0
#define RLDSENSP_ADDR 0x0D
#define RLD_CHAN_CONNECTED(X) (1 << X)
#define RLD_LL_CHAN (3 - 1)
#define RLD_LA_CHAN (2 - 1)

// 9.6.1.8 RLD_SENSN 0x0E
#define RLDSENSN_ADDR 0x0E
#define RLD_RA_CHAN (2 - 1)

// 9.6.1.9 LOFF_SENSP READ ONLY
#define LOFF_SENSP_ADDR 0x0F

// 9.6.1.10 LOFF_SENSP  READ ONLY
#define LOFF_SENSN_ADDR 0x10

// 9.6.1.11 LOFF_FLIP
#define LOFF_FLIP_ADDR 0x11

// 9.6.1.12 LOFF_STATEP
#define LOFF_STATEP_ADDR 0x12
#define LOFF_STATEP_ENABLE_ALL 0xFF

// 9.6.1.13 LOFF_STATEN
#define LOFF_STATEN_ADDR 0x13
#define LOFF_RA_CN2 (2 - 1)
#define LOFF_RA_CN3 (3 - 1)
#define LOFF_STATE_EN_CH(X) (1 << X)

// 9.6.1.14 GPIO_SETTING
#define GPIO_SETTINGS_ADDR 0x14
#define GPIO_REG_CONFIG 0x01 // GPIO1 is input, all others are output
#define GPIO_CH_INPUT(X) (1 << X)
#define GPIO_CH_OUTPUT(X) (0 << X)
#define GPIO1_OFFSET 0
#define GPIO2_OFFSET 1
#define PACE_RESET_OUTP (1 << 5)
#define PACE_DETECT(value) value &(1 << 4) ? 1 : 0

// 9.6.1.15 PACE Register
#define PD_PACE_SETTINGS_ADDR 0x15
#define PD_PACE_BUFFER_DISABLE 0
#define PD_PACE_BUFFER_ENABLE (1 << 0)
#define PD_PACE_ODD_CHANNEL_ECGV3 (1 << 1)
#define PD_PACE_EVEN_CHANNEL_ECGV8 (3 << 3)

// 9.6.1.16 Respitory control register
#define RESP_CTRL_ADDR 0x16

// 9.6.1.17 CONFIG4 Address 0x17h reset = 0x00
#define CFGREG4_ADDR 0x17

// 9.6.1.18 WCT1 Address 0x18h reset = 0x00
#define WCT1_ADDR 0x18
#define WCTA_CONNECT_IN2P (2 << 0) // bits 010  CH2 Pos
#define WCTA_ENABLE (1 << 3)

// 9.6.1.18 WCT1 Address 0x18h reset = 0x00
#define WCT2_ADDR 0x19
#define WCTB_CONNECT_IN2N (3 << 3) // bits 011  CH2 Neg
#define WCTB_ENABLE (1 << 6)
#define WCTC_CONNECT_IN3P (4 << 0) // bits 100  CH3 Pos
#define WCTC_ENABLE (1 << 7)

#define CFGREG4_SINGLE_SHOT (1 << 3)
#define CFGREG4_CONTINUOUS (0 << 3)
#define CFGREG4_WCT_TO_RLD (1 << 2)
#define CFGREG4_LOFF_COMPAR_EN (1 << 1)

#define ADS1298_CONV_STATUS_SIZE_BYTES 3
#define ADS1298_CONV_DATA_SIZE_BYTES 3
#define ADS1298_NUM_CHANNELS 8
#define ADS1298_CONV_DATA_TOTAL_SIZE_BYTES                                                                             \
    (ADS1298_CONV_STATUS_SIZE_BYTES + ADS1298_CONV_DATA_SIZE_BYTES * ADS1298_NUM_CHANNELS)

// ADC common values, used for ECG12
// Max physical register address of ADS1298 is 0x19
typedef enum {
    ADS1298_LEAD_OFF_FREQ_TYPE = 0x20,
    ADS1209_LEAD_OFF_NEGATIVE_STATE,
    ADS1209_LEAD_OFF_POSITIVE_STATE,
    ADS1298_ILEAD_OFF_CURRENT_TYPE,
    ADS1298_LEAD_OFF_DETECTION_MODE,
    ADS1298_LEAD_OFF_COMPARATOR_THRESHOLD,
    ADS1298_RLD_SENSE_FN_ENABLE,
    ADS1298_RLD_CONNECT_STATUS,
    ADS1298_PACE_DETECT_RESET,
    ADS1298_PACE_DETECT_GET,
} ADS1298_ADDL_REGISTER_ADDR;

typedef enum {
    // The following will return a single bit representing the status of the desired value
    STATUSBIT_GPIO4,
    STATUSBIT_GPIO5,
    STATUSBIT_GPIO6,
    STATUSBIT_GPIO7,
    STATUSBIT_ECG_V1,
    STATUSBIT_ECG_V2,
    STATUSBIT_ECG_V3,
    STATUSBIT_ECG_V4,
    STATUSBIT_ECG_V5,
    STATUSBIT_ECG_V6,
    STATUSBIT_ECG_LA,
    STATUSBIT_ECG_LL,
    STATUSBIT_ECG_RA,
    STATUSBIT_ECG_MAX,
    STATUSBIT_GPIO = STATUSBIT_ECG_MAX, // The following will return a 4 bit value representing GPIO4-7
    STATUSBIT_IN_NEG, // Will return 8 bit value representing the state of all 8 Negative inputs bit[1,2]=ECG_RA
    STATUSBIT_IN_POS, // Will return 8 bit value representing the state of all 8 Positive inputs
                      // bit[0....7] = [ECG_V6, ECG_LA, ECG_LL, ECG_V2, ECG_V3, ECG_V4, ECG_V5, ECG_V1]
    STATUSBIT_MAX
} ADS1298_STATUSBIT_e;

#define BITS_IN_BYTE 8

// STAT 24 bits:   <1100><INP:8><INN:8><GPIO7:4>
#define GPIO7_OFFSET 3
#define GPIO6_OFFSET 2
#define GPIO5_OFFSET 1
#define GPIO4_OFFSET 0

// data offset
#define INN_POSITION 4
#define IN1N 0
#define IN2N 1
#define IN3N 2
#define IN4N 3
#define IN5N 4
#define IN6N 5
#define IN7N 6
#define IN8N 7

#define INP_POSITION 12
#define IN1P 0
#define IN2P 1
#define IN3P 2
#define IN4P 3
#define IN5P 4
#define IN6P 5
#define IN7P 6
#define IN8P 7

// ECG data name and channel location
#define ECG_V6 IN1P
#define ECG_LA IN2P
#define ECG_RA IN2N
#define ECG_LL IN3P
#define ECG_RA2 IN3N // RA and RA2 are the same signal going to both IN2N and IN3N
#define ECG_V2 IN4P
#define ECG_V3 IN5P
#define ECG_V4 IN6P
#define ECG_V5 IN7P
#define ECG_V1 IN8P

#ifdef CREATE_LEAD_LOOKUP_TABLES
const char *lookupPositiveLeads[BITS_IN_BYTE] = {
    "ECG_V6", "ECG_LA", "ECG_LL", "ECG_V2", "ECG_V3", "ECG_V4", "ECG_V5", "ECG_V1"};
const char *lookupNegativeLeads[BITS_IN_BYTE] = {
    "INP1_NC", "ECG_RA", "ECG_RA_3", "INP4_NC", "INP5_NC", "INP6_NC", "INP7_NC", "INP8_NC"};

const uint32_t STATUS_BIT_LOCATION_MAP[STATUSBIT_ECG_MAX] = {
    [STATUSBIT_GPIO4] = 1 << GPIO4_OFFSET,
    [STATUSBIT_GPIO5] = 1 << GPIO5_OFFSET,
    [STATUSBIT_GPIO6] = 1 << GPIO6_OFFSET,
    [STATUSBIT_GPIO7] = 1 << GPIO7_OFFSET,
    [STATUSBIT_ECG_V1] = (1 << ECG_V1) << INP_POSITION,
    [STATUSBIT_ECG_V2] = (1 << ECG_V2) << INP_POSITION,
    [STATUSBIT_ECG_V3] = (1 << ECG_V3) << INP_POSITION,
    [STATUSBIT_ECG_V4] = (1 << ECG_V4) << INP_POSITION,
    [STATUSBIT_ECG_V5] = (1 << ECG_V5) << INP_POSITION,
    [STATUSBIT_ECG_V6] = (1 << ECG_V6) << INP_POSITION,
    [STATUSBIT_ECG_LA] = (1 << ECG_LA) << INP_POSITION,
    [STATUSBIT_ECG_LL] = (1 << ECG_LL) << INP_POSITION,
    [STATUSBIT_ECG_RA] = (1 << ECG_RA) << INN_POSITION,
};

#endif

extern const char *lookupPositiveLeads[BITS_IN_BYTE];
extern const char *lookupNegativeLeads[BITS_IN_BYTE];

/**
 * @struct cncInfo_t
 * This structure is used to communicate with Daughter Board procs on the main board and the
 * main board and daughter board CNC tasks.
 * @Note: This structure is available from an osPool so use the osPoolAlloc and osPoolFree methods
 * to get instances to pass to other tasks.
 */
typedef struct adcData {
    uint32_t status24;
    uint32_t channelData[ADS1298_NUM_CHANNELS];
} adcData_t, *adcData_tp;

/**
 * @fn dacInit
 *
 * @brief Initialize DAC
 *
 * @return osStatus
 **/
osStatus ads1298Init(void);

/**
 * @fn ads1298ReadRegister
 *
 * @brief Read registers from the ads1298
 *
 * @param[in] uint8_t addr: 5 bit value of address to read
 * @param[in] uint8_t numRegisters: Number of registers to read
 * @param[in] uint8_t *pData: 8bit data to read from ads1298
 *
 * @return osStatus
 **/
osStatus ads1298ReadRegister(uint8_t addr, uint8_t numRegisters, uint8_t *pData);

/**
 * @fn ads1298WriteRegister
 *
 * @brief Write registers in ads1298
 *
 * @param[in] uint8_t addr: 5 bit value of address to read
 * @param[in] uint8_t numRegisters: Number of registers to read
 * @param[in] uint8_t *pData: 8bit data to write to ads1298
 *
 * @return osStatus
 **/
osStatus ads1298WriteRegister(uint8_t addr, uint8_t numRegisters, const uint8_t *pdata);

/**
 * @fn ads1298ReadData
 *
 * @brief Read conversion data from ADC
 *
 * @param[out] adcData_tp *data: Data read from ADC
 * @param[out] statusReg: statusReg bits read from the ADC
 *
 * @return osStatus
 **/
osStatus ads1298ReadData(adc24Reading_t adcData[NUMBER_OF_SENSOR_READINGS], uint32_t *statusReg);

/**
 * @fn ads1298SendCmd
 *
 * @brief Send one byte command to ads1298
 *
 * @param[in] uint8_t cmd: 8bit command to send
 *
 * @return osStatus
 **/
osStatus ads1298SendCmd(uint8_t cmd);

/**
 * @fn adcConvertToV
 *
 * @brief Convert 16bit ADC value to voltage
 *
 * @param[in]     uint16_t adcValue: 16bit value from ADC
 * @param[in]     float *voltage_mv: voltage in mV
 *
 * @return osStatus
 **/
osStatus adcConvertToV(uint32_t adcValue, float *voltage_mv);

/**
 * @fn adcTaskInit
 *
 * @brief initialize the ADC Task.
 *
 * @param[in]     priority thread priority
 * @param[in]     stackSize  thread stack size
 **/
void adcTaskInit(int priority, int stackSize);

/**
 * @fn ads1298_sampleRateSet
 *
 * @brief Change the sampling rate use values use power (2^x) *500 where x=0...6
 * @note there are set rates and the function will find the closes
 *
 * @param[in]     rate_SPS: sample rate per second to use min 500 max 32000
 *
 * @return osStatus osOK on success
 **/
osStatus ads1298_sampleRateSet(uint32_t rate_SPS);

/**
 * @fn printSampleRates
 *
 * @brief write the allowed values to the buffer space separated
 *
 * @param[out]  char *buffer: location to write value to
 * @param[in]   uint32_t sizeofBuffer: max size of the buffer
 *
 * @return number of characters written to buffer
 **/
int printSampleRates(char *buffer, uint32_t sizeofBuffer);

/**
 * @fn ads1298_RLDLeadOff_Connected
 *
 * @brief Read the CFG3 RLDDrive Connected state bit 0
 *
 * @param[out]     bool *connected: fill with boolean state of connected bit
 *
 * @return osStatus osOK on success
 **/
osStatus ads1298_RLDLeadOff_Connected(bool *connected);

/**
 * @fn ads1298_RLDriveFunctionSet
 *
 * @brief Set the state of the Right leg drive enable bit, CFG3 BIT 1
 *
 * @param[in] bool enable: enable the right leg drive function.
 *
 * @return osStatus osOK on success
 **/

osStatus ads1298_RLDriveFunctionGet(bool *enable);

/**
 * @fn ads1298_RLDriveFunctionSet
 *
 * @brief Set the state of the Right leg drive enable bit, CFG3 BIT 1
 *
 * @param[in] bool enable: enable the right leg drive function.
 *
 * @return osStatus osOK on success
 **/
osStatus ads1298_RLDriveFunctionSet(bool enable);

/**
 * @fn ads1298_loffComparatorThresholdSet
 *
 * @brief Set the lead off detection threshold to a value
 * [70000...95000] on positive side with positive==TRUE
 * [5000...30000] on negative side
 *
 * @param[in] positive identify if the threshold value is postive or negative.
 * @param[in] compare_mperc: milli% threshold value
 *
 * @return osStatus osOK on success
 **/
osStatus ads1298_loffComparatorThresholdSet(bool positive, uint32_t compare_mperc);

/**
 * @fn ads1298_loffComparatorThresholdGet
 *
 * @brief Get the lead off detection threshold to a value
 * [70000...95000] on positive side
 * [5000...30000] on negative side
 *
 * @param[out] compare_mperc: milli% threshold value
 *
 * @return osStatus osOK on success
 **/
osStatus ads1298_loffComparatorThresholdGet(uint32_t *compare_mperc);

/**
 * @fn ads1298_leadOffDetectionModeSet
 *
 * @brief if true then set detection mode to voltage else current REG4 bit 4
 *
 * @param[in] bool voltage if true then set detection mode to voltage else current
 *
 * @return osStatus osOK on success
 **/
osStatus ads1298_leadOffDetectionModeSet(bool voltage);

/**
 * @fn ads1298_leadOffDetectionModeGet
 *
 * @brief Get the current lead off detection mode, TRUE=voltage else current
 *
 * @param[out] bool voltage type setting
 *
 * @return osStatus osOK on success
 **/
osStatus ads1298_leadOffDetectionModeGet(bool *voltage);

/**
 * @fn ads1298_ileadOffCurrentSet
 *
 * @brief Find the nearest allowed current threshold value to the value passed in in mA and set the
 * register 4 and bits 3:2
 * [6,12,18,24] nA
 *
 * @param[in] value_nA: value to set the current threshold to for off lead detection
 *
 * @return osStatus osOK on success
 **/
osStatus ads1298_ileadOffCurrentSet(uint32_t value_nA);

/**
 * @fn ads1298_ileadOffCurrentGet
 *
 * @brief Get the current setting of the current threshold detect in nA
 *
 * @param[out] *value_nA: location to write value to
 *
 * @return osStatus osOK on success
 **/
osStatus ads1298_ileadOffCurrentGet(uint32_t *value_nA);

/**
 * @fn ads1298_fleadOffFreqSet
 *
 * @brief set the lead off detect frequency, value must be either 1(AC) or 3 (DC)
 *
 *
 * @param[in] freqSetting: value to set detect frequency
 *
 *
 * @return osStatus osOK on success
 **/
osStatus ads1298_fleadOffFreqSet(uint32_t freqSetting);

/**
 * @fn ads1298_fleadOffFreqGet
 *
 * @brief get the lead off detect frequency, value will be either 1(AC) or 3 (DC)
 *
 *
 * @param[out] *freqSetting: value read, will be either 1(AC) or 3(DC)
 *
 *
 * @return osStatus osOK on success
 **/
osStatus ads1298_fleadOffFreqGet(uint32_t *freqSetting);

/**
 * @fn ads1298_leadOffPositiveStateGet
 *
 * @brief get the bit state of the Lead connections off = 1
 *
 * @param[out] *stateBit: holds bit values
 *
 * @return osStatus osOK on success
 **/
osStatus ads1298_leadOffPositiveStateGet(uint32_t *stateBit);

/**
 * @fn ads1298_leadOffNegativeStateGet
 *
 * @brief get the bit state of the Lead connections off = 1
 *
 * @param[out] *stateBit: holds bit values
 *
 * @return osStatus osOK on success
 **/
osStatus ads1298_leadOffNegativeStateGet(uint32_t *stateBit);

/**
 * @fn ads1298GetConnectionStatus
 *
 * @brief get the status bit or all bits based on statusID. newDataReading true will result
 * in getting a new result. A False setting will use the last TRUE reading.
 *
 * @note The idea is to set newDataReading to TRUE and then on subsequent calls use FALSE until
 *       all required data is taken and then repeat.
 *
 * @param[in] statusID: identify which value(s) to return
 * @param[in] newDataReading: if true store a new reading for use in subsequent calls, if set to FALSE
 *
 * @return the identified bit(s) values
 **/
uint8_t ads1298GetConnectionStatus(ADS1298_STATUSBIT_e statusID, bool newDataReading);

/**
 * @fn adc1298RightLegConnected
 *
 * @brief get the value of the RightLegConnected state from CFG3 register.
 *
 *
 * @param[in] registerInfo: register info to fill in. 0= not connected, 1=connected

 *
 * @return the success =osOK, else error.
 **/
RETURN_CODE adc1298RightLegConnected(registerInfo_tp registerInfo);

/**
 * @fn ads1298_PaceDetectReset
 *
 * @brief Reset the Pace detect circuit
 *
 * @return the success =RETURN_OK, else error.
 **/
RETURN_CODE ads1298_PaceDetectReset(void);

/**
 * @fn ads1298_PaceDetectGet
 *
 * @brief get the status of the Pace Detect value. Note it is latching so must
 * call ads1298_PaceDetectReset before calling this function
 *
 * @param[out] bool: bvalue state of the Pace Detect circuit
 *
 * @return the success =RETURN_OK, else error.
 **/
RETURN_CODE ads1298_PaceDetectGet(bool *bvalue);
#endif /* LIB_INC_ADS1298_H_ */
