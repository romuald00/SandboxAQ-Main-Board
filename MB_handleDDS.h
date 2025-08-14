/*
 * MB_handleDDS.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Dec 8, 2023
 *      Author: jzhang
 */

#ifndef APP_INC_PERIPHERALS_MB_HANDLEDDS_H_
#define APP_INC_PERIPHERALS_MB_HANDLEDDS_H_

#include "mongooseHandler.h"
#include <stdbool.h>

#define MAX_DDS_OUTPUT 4
#define MAX_COIL_DDS_DEVICE_ID 6
#define MAX_DDS_PHASE_MDEG 360000

/**
 * @fn webDDSAmpGet
 *
 * @brief Handler for web API get DDS amplitude command.
 *
 * @note Coil version handles different peripheral ids then MCG
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webDDSAmpGet(const char *jsonStr, int strLen);
webResponse_tp webCoilDDSAmpGet(const char *jsonStr, int strLen);

/**
 * @fn webDDSAmpSet
 *
 * @brief Handler for web API set DDS amplitude command.
 *
 * @note Coil version handles different peripheral ids then MCG
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webDDSAmpSet(const char *jsonStr, int strLen);
webResponse_tp webCoilDDSAmpSet(const char *jsonStr, int strLen);

/**
 * @fn webDDSFreqGet
 *
 * @brief Handler for web API get DDS frequency command.
 * @note Coil version handles different peripheral ids then MCG
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webDDSFreqGet(const char *jsonStr, int strLen);
webResponse_tp webCoilDDSFreqGet(const char *jsonStr, int strLen);

/**
 * @fn webDDSFreqSet
 *
 * @brief Handler for web API set DDS frequency command.
 * @note Coil version handles different peripheral ids then MCG
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webDDSFreqSet(const char *jsonStr, int strLen);
webResponse_tp webCoilDDSFreqSet(const char *jsonStr, int strLen);

/**
 * @fn webDDSPhaseGet
 *
 * @brief Handler for web API get DDS Phase command.
 * @note Coil version handles different peripheral ids then MCG
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webDDSPhaseGet(const char *jsonStr, int strLen);
webResponse_tp webCoilDDSPhaseGet(const char *jsonStr, int strLen);

/**
 * @fn webDDSPhaseSet
 *
 * @brief Handler for web API set DDS Phase command.
 * @note Coil version handles different peripheral ids then MCG
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webDDSPhaseSet(const char *jsonStr, int strLen);
webResponse_tp webCoilDDSPhaseSet(const char *jsonStr, int strLen);

/**
 * @fn webDDSStart
 *
 * @brief Handler for web API start wave generation.
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webDDSStart(const char *jsonStr, int strLen);

/**
 * @fn webDDSStop
 *
 * @brief Handler for web API stop wave generation.
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webDDSStop(const char *jsonStr, int strLen);

/**
 * @fn testCoilDDSValue
 *
 * @brief Test DDS SLOT value from web API call.
 *
 * @param[in] webResponse_tp p_webResponse: Web response
 * @param[in] int slot: dds id to write to (between 0-5)
 *
 * @return webResponse_tp: Updated JSON structure
 **/
bool testCoilDDSValue(webResponse_tp p_webResponse, int slot);

/**
 * @fn testDacValue
 *
 * @brief Test DAC value from web API call.
 *
 * @param[in] webResponse_tp p_webResponse: Web response
 * @param[in] int dac: DADC to write to (between 0-3)
 *
 * @return webResponse_tp: Updated JSON structure
 **/
bool testDacValue(webResponse_tp p_webResponse, int dac);

/**
 * @fn testWaveformValue
 *
 * @brief Test DAC value from web API call.
 *
 * @param[in] webResponse_tp p_webResponse: Web response
 * @param[in] int waveform: waveform value
 *
 * @return webResponse_tp: Updated JSON structure
 **/
bool testWaveformValue(webResponse_tp p_webResponse, int waveform);

/**
 * @fn testPhaseValue
 *
 * @brief Test phase value from web API call.
 *
 * @param[in] webResponse_tp p_webResponse: Web response
 * @param[in] int phase: Phase in degrees
 *
 * @return webResponse_tp: Updated JSON structure
 **/
bool testPhaseValue(webResponse_tp p_webResponse, int phase);

/**
 * @fn testAmplitudeValue
 *
 * @brief Test amplitude value from web API call.
 *
 * @param[in] webResponse_tp p_webResponse: Web response
 * @param[in] int amplitude: Amplitude in percentage
 *
 * @return webResponse_tp: Updated JSON structure
 **/
bool testAmplitudeValue(webResponse_tp p_webResponse, int amplitude);

/**
 * @fn testFrequencyValue
 *
 * @brief Test destination value from web API call.
 *
 * @param[in] webResponse_tp p_webResponse: Web response
 * @param[in] int frequency: Frequency in kHz
 *
 * @return webResponse_tp: Updated JSON structure
 **/
bool testFrequencyValue(webResponse_tp p_webResponse, int frequency);

/**
 * @fn testWaveformParameterValue
 *
 * @brief Test waveformParameter value from web API call is within parameters .
 *
 * @param[in] webResponse_tp p_webResponse: Web response
 * @param[in] int waveformParameter: must fall within 1-16
 *
 * @return webResponse_tp: Updated JSON structure
 **/
bool testWaveformParameterValue(webResponse_tp p_webResponse, int waveformParameter);

/**
 * @fn webDDSWaveformSet
 *
 * @brief Handler for web API to set the type of DAC output.
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webDDSWaveformSet(const char *jsonStr, int strLen);

/**
 * @fn webDDSWaveformGet
 *
 * @brief Handler for web API to get the waveform of the DAC.
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webDDSWaveformGet(const char *jsonStr, int strLen);

/**
 * @fn webDDSWaveformELONGATESet
 *
 * @brief Handler for web API to set the duration of the chirp or ecg waveform.
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webDDSWaveformELONGATESet(const char *jsonStr, int strLen);

/**
 * @fn webCoilDDSWaveformELONGATESet
 *
 * @brief Handler for web API to set the duration of the chirp or ecg waveform.
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webCoilDDSWaveformELONGATESet(const char *jsonStr, int strLen);

/**
 * @fn webDDSWaveformELONGATEGet
 *
 * @brief Handler for web API to get the duration of the chirp or ecg waveform.
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webDDSWaveformELONGATEGet(const char *jsonStr, int strLen);

/**
 * @fn webCoilDDSWaveformELONGATEGet
 *
 * @brief Handler for web API to get the duration of the chirp or ecg waveform.
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webCoilDDSWaveformELONGATEGet(const char *jsonStr, int strLen);

/**
 * @fn webDDSWaveformINTERVALSet
 *
 * @brief Handler for web API to set the time between each chirp or ecg waveform.
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webDDSWaveformINTERVALSet(const char *jsonStr, int strLen);

/**
 * @fn webCoilDDSWaveformINTERVALSet
 *
 * @brief Handler for web API to set the time between each chirp or ecg waveform.
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webCoilDDSWaveformINTERVALSet(const char *jsonStr, int strLen);

/**
 * @fn webDDSWaveformINTERVALGet
 *
 * @brief Handler for web API to get the time between each chirp or ecg waveform.
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webDDSWaveformINTERVALGet(const char *jsonStr, int strLen);

/**
 * @fn webCoilDDSWaveformINTERVALGet
 *
 * @brief Handler for web API to get the time between each chirp or ecg waveform.
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webCoilDDSWaveformINTERVALGet(const char *jsonStr, int strLen);

/**
 * @fn webDDSWaveformDELAYSet
 *
 * @brief Handler for web API to set the time offset for the first chirp or ecg waveform.
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webDDSWaveformDELAYSet(const char *jsonStr, int strLen);

/**
 * @fn webCoilDDSWaveformDELAYSet
 *
 * @brief Handler for web API to set the time offset for the first chirp or ecg waveform.
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webCoilDDSWaveformDELAYSet(const char *jsonStr, int strLen);

/**
 * @fn webDDSWaveformDELAYGet
 *
 * @brief Handler for web API to get the time offset for the first chirp or ecg waveform.
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webDDSWaveformDELAYGet(const char *jsonStr, int strLen);

/**
 * @fn webCoilDDSWaveformDELAYGet
 *
 * @brief Handler for web API to get the time offset for the first chirp or ecg waveform.
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webCoilDDSWaveformDELAYGet(const char *jsonStr, int strLen);

/**
 * @fn webCoilDDSWaveformSet
 *
 * @brief Handler for web API to set the waveform of the DAC.
 *
 * @note: special case of the webDDSWaveformGet, it has only one dac
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webCoilDDSWaveformSet(const char *jsonStr, int strLen);

/**
 * @fn webCoilDDSWaveformGet
 *
 * @brief Handler for web API to get the waveform of the DAC.
 *
 * @note: special case of the webDDSWaveformGet, it has only one dac
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webCoilDDSWaveformGet(const char *jsonStr, int strLen);

#endif /* APP_INC_PERIPHERALS_MB_HANDLEDDS_H_ */
