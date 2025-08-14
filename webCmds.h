/*
 * webCmds.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Feb 28, 2024
 *      Author: jzhang
 */

#ifndef WEBSERVER_INC_WEBCMDS_H_
#define WEBSERVER_INC_WEBCMDS_H_

#include "peripherals/MB_handleADC.h"
#include "peripherals/MB_handleDDS.h"
#include "peripherals/MB_handleDac.h"
#include "peripherals/MB_handleFan.h"
#include "peripherals/MB_handleIMU.h"
#include "peripherals/MB_handleLED.h"
#include "peripherals/MB_handleLogs.h"
#include "peripherals/MB_handlePwrCtrl.h"
#include "peripherals/MB_handleReg.h"
#include "realTimeClock.h"
#include "webCliMisc.h"
#include "webCmdHandler.h"
#include <peripherals/MB_handleADC.h>

typedef webResponse_tp (*WEB_COMMAND_FUNCTION)(const char *jsonStr, const int strLen);

typedef struct {
    char *cmdString;
    char *description;
    char *helpText;
    WEB_COMMAND_FUNCTION function;
} WEB_COMMAND;

/**
 * @fn webHelpCmd
 *
 * @brief Handler for web API help command
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webHelpCmd(const char *jsonStr, int strLen);

/**
 * @fn webUnrecognizedCmd
 *
 * @brief Handler for when the command is not recognized
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webUnrecognizedCmd(const char *jsonStr, int strLen);

/**
 * @fn webCnc
 *
 * @brief Handler for web API Command and control request
 *
 * @param[in] const char *jsonStr: Pointer to JSON body
 * @param[in] int strLen: JSON body string length
 *
 * @return webResponse_tp: Updated JSON structure
 **/
webResponse_tp webCnc(const char *jsonStr, int strLen);

#define NUM_WEB_COMMANDS 111

static const WEB_COMMAND webCommandList[NUM_WEB_COMMANDS] = {
    {"/dac/compensation/set",
     "Set output voltage for compensation DAC",
     "destination, slot, uid, voltage_mv",
     webDacCompSet},
    {"/dac/compensation/get", "Get output voltage for compensation DAC", "destination, slot, uid", webDacCompGet},
    {"/dac/excitation/set",
     "Set output voltage for excitation DAC",
     "destination, slot, uid, voltage_mv",
     webDacExcSet},
    {"/dac/excitation/get", "Get output voltage for excitation DAC", "destination, slot, uid", webDacExcGet},

    {"/dds/amplitude/set",
     "Set wave amplitude for DDS (0-100000 m%)",
     "destination, slot, uid, dac, amplitude",
     webDDSAmpSet},
    {"/dds/amplitude/get", "Get wave amplitude for DDS (0-100000 m%)", "destination, slot, uid, dac", webDDSAmpGet},
    {"/dds/frequency/set", "Set wave frequency for DDS (hz)", "destination, slot, uid, dac, frequency", webDDSFreqSet},
    {"/dds/frequency/get", "Set wave frequency for DDS (hz)", "destination, slot, uid, dac", webDDSFreqGet},
    {"/dds/phase/set",
     "Set wave phase for DDS (0-360000 millidegrees)",
     "destination, slot, uid, dac, phase_mDeg",
     webDDSPhaseSet},
    {"/dds/waveform/set",
     "Set waveform for DAC, 0=CHIRP, 1=ECG, 2=SINE",
     "destination, slot, uid, dac, waveform(0-2)",
     webDDSWaveformSet},
    {"/dds/waveform/get",
     "Get waveform for DAC, 0=CHIRP, 1=ECG, 2=SINE",
     "destination, slot, uid, dac",
     webDDSWaveformGet},
    {"/dds/waveform/elongate/set",
     "Set the elongation of the CHIRP or ECG waveform, 1=shortest, 16=longest",
     "destination, slot, uid, elongate(1-16)",
     webDDSWaveformELONGATESet},
    {"/dds/waveform/elongate/get",
     "Get waveform elongation value",
     "destination, slot, uid",
     webDDSWaveformELONGATEGet},
    {"/dds/waveform/interval/set",
     "Set the interval between waveforms, only applies to CHIRP or ECG waveforms, 1=shortest, 16=longest",
     "destination, slot, uid, interval(1-16)",
     webDDSWaveformINTERVALSet},
    {"/dds/waveform/interval/get", "Get waveform interval value", "destination, slot, uid", webDDSWaveformINTERVALGet},
    {"/dds/waveform/phase/set",
     "Set the interval between waveforms, only applies to CHIRP or ECG waveforms, 1=shortest, 16=longest",
     "destination, slot, uid, interval(1-16)",
     webDDSWaveformDELAYSet},
    {"/dds/waveform/phase/get",
     "Get waveform phase value, value is number of DAC periods to wait before starting",
     "destination, slot, uid",
     webDDSWaveformDELAYGet},
    {"/dds/phase/get", "Set wave phase for DDS (0-360000 millidegrees)", "destination, slot, uid, dac", webDDSPhaseGet},
    {"/dds/start", "Start pattern generation", "no parameters", webDDSStart},
    {"/dds/stop", "Stop pattern generation", "no parameters", webDDSStop},
    {"/coil_dds/amplitude/set",
     "Set wave amplitude for DDS (0-100000 m%)",
     "destination, slot, uid, amplitude",
     webCoilDDSAmpSet},
    {"/coil_dds/amplitude/get", "Get wave amplitude for DDS (0-100000 m%)", "destination, slot, uid", webCoilDDSAmpGet},
    {"/coil_dds/frequency/set",
     "Set wave frequency for DDS (hz)",
     "destination, slot, uid, frequency",
     webCoilDDSFreqSet},
    {"/coil_dds/frequency/get", "Set wave frequency for DDS (hz)", "destination, slot, uid", webCoilDDSFreqGet},
    {"/coil_dds/phase/set",
     "Set wave phase for DDS (0-360000 millidegrees)",
     "destination, slot, uid, phase_mDeg",
     webCoilDDSPhaseSet},
    {"/coil_dds/phase/get",
     "Set wave phase for DDS (0-360000 millidegrees)",
     "destination, slot, uid",
     webCoilDDSPhaseGet},
    {"/coil_dds/waveform/set",
     "Set waveform for DAC, waveform 0=CHIRP, 1=ECG, 2=SINE",
     "destination, slot, uid, dac, waveform(0-2)",
     webCoilDDSWaveformSet},
    {"/coil_dds/waveform/get",
     "Get waveform for DAC, return waveform 0=CHIRP, 1=ECG, 2=SINE",
     "destination, slot, uid, dac",
     webCoilDDSWaveformGet},
    {"/coil_dds/waveform/elongate/set",
     "Set the elongation of the CHIRP or ECG waveform, 1=shortest, 16=longest",
     "destination, slot, uid, elongate(1-16)",
     webCoilDDSWaveformELONGATESet},
    {"/coil_dds/waveform/elongate/get",
     "Get waveform elongation value",
     "destination, slot, uid",
     webCoilDDSWaveformELONGATEGet},
    {"/coil_dds/waveform/interval/set",
     "Set the interval between waveforms, only applies to CHIRP or ECG waveforms, 1=shortest, 16=longest",
     "destination, slot, uid, interval(1-16)",
     webCoilDDSWaveformINTERVALSet},
    {"/coil_dds/waveform/interval/get",
     "Get waveform interval value",
     "destination, slot, uid",
     webCoilDDSWaveformINTERVALGet},
    {"/coil_dds/waveform/phase/set",
     "Set the interval between waveforms, only applies to CHIRP or ECG waveforms, 1=shortest, 16=longest",
     "destination, slot, uid, phase(1-16)",
     webCoilDDSWaveformDELAYSet},
    {"/coil_dds/waveform/phase/get",
     "Get waveform phase value, value is number of DAC periods to wait before starting",
     "destination, slot, uid",
     webCoilDDSWaveformDELAYGet},

    {"/adc/start", "Start ADC conversions", "no parameters", webAdcStart},
    {"/adc/stop", "Stop ADC conversions", "no parameters", webAdcStop},
    {"/adc/frequency/set", "Set ADC data rate frequency (hz)", "frequency (hz)", webAdcSetFreq},
    {"/adc/frequency/get", "Get ADC data rate frequency (hz)", "no parameters", webAdcGetFreq},
    {"/adc/duty/set", "Set ADC data duty (%)", "duty (%)", webAdcSetDuty},
    {"/adc/duty/get", "Get ADC data duty (%)", "no parameters", webAdcGetDuty},
    {"/led_state/set",
     "Set on time and off time for on board LED",
     "destination, led <GREEN|RED>, state <0|1>, uid",
     webLedStateSet},
    {"/led_state/get",
     "Get on time and off time for on board LED",
     "destination, led <GREEN|RED>, uid",
     webLedStateGet},
    {"/stream", "Upgrade http to websocket", "no parameters", NULL}, // unimplemented
    {"/date/set", "Set the date and time", "year, month, day, hour24, minute, second", webTimeSet},
    {"/date/get", "Get the date and time", "no parameters", webTimeGet},
    {"/reboot", "Reboot a daughterboard", "reboot (bool), destination (db_id | 254 for main board)", webReboot},
    {"/ip/set", "Set the IP", "ip, netmask, gateway", webSetIp},
    {"/ip/get", "Get the IP", "no parameters", webGetIp},
    {"/udp/client/port/set", "Set UDP client tx port", "port", webTxUdpPortSet},
    {"/udp/client/port/get", "Get UDP client tx port", "no parameters", webTxUdpPortGet},
    {"/udp/server/set", "Set UDP destination server ip:port", "ip, port", webUdpServerIpSet},
    {"/udp/server/get", "Get UDP destination server ip:port", "no parameters", webUdpServerIpGet},
    {"/tcp/client/port/set", "Set tcp client listening port", "port", webTcpClientPortSet},
    {"/tcp/client/port/get", "Get tcp client listening port", "no parameters", webTcpClientPortGet},
    {"/stream/ipType/set", "Set streaming protocol", "ipType [IPTYPE_UDP | IPTYPE_TCP]", webClientTxIpDataTypeSet},
    {"/stream/ipType/get", "Get streaming protocol", "No Parameters", webClientTxIpDataTypeGet},
    {"/db_proc/loopback/set",
     "Set loopback enable on main board",
     "destination, loopback (bool), offset",
     webDbProcLoopbackSet},
    {"/db_proc/loopback/get", "Get loopback enabled on main board", "destination", webDbProcLoopbackGet},
    {"/db_proc/spi/interval/set", "Set SPI interval rate (us)", "spi_interval_us", webDbProcSpiIntervalSet},
    {"/db_proc/spi/interval/get", "Get SPI interval rate (us)", "no parameters", webDbProcSpiIntervalGet},
    {"/client/interval/set", "Set streaming interval rate (ms)", "stream_interval_ms", webUdpStreamIntervalSet},
    {"/client/interval/get", "Get streaming interval rate (ms)", "no parameters", webUdpStreamIntervalGet},
    {"/db_loopback/set",
     "Set loopback enable for specified daughter board",
     "destination, loopback (bool), uid",
     webLoopbackSetDb},
    {"/db_loopback/get", "Get loopback enable for specified daughter board", "destination, uid", webLoopbackGetDb},
    {"/db_loopback/interval/set",
     "Set streaming interval rate for daughter board (us)",
     "destination, loopback_interval_us, uid",
     webLoopbackIntervalSetDb},
    {"/db_loopback/interval/get",
     "Get streaming interval rate for daughter board (us)",
     "destination, uid",
     webLoopbackIntervalGetDb},
    {"/db_mfg/serialnumber/get", "Get db_mfg serial number string", "destination, uid", webSerialNumberGetDb},
    {"/db_mfg/serialnumber/set",
     "Set the sensor board serial number string, mfg_write_en must be true",
     "destination, uid, serialNumber_str",
     webSerialNumberSetDb},
    {"/db_mfg/hw_rev/get", "Get db_mfg sensor board hardware version", "destination, uid", webHwRevGetDb},
    {"/db_mfg/hw_rev/set",
     "Set the sensor board hardware version, mfg_write_en must be true",
     "destination, uid, hw_version",
     webHwRevSetDb},
    {"/db_mfg/hw_type/get", "Get sensor board type", "destination, uid", webHwTypeGetDb},
    {"/db_mfg/hw_type/set",
     "Set the sensor board type, mfg_write_en must be true",
     "destination, uid, hw_type",
     webHwTypeSetDb},
    {"/mfg_write_en/get", "get the value of mfg_write_en", "uid", webMfgWriteEnGet},
    {"/mfg_write_en/set", "Set the value of mfg_write_en", "uid, mfg_write_en", webMfgWriteEnSet},
    {"/imu/cmd", "Send a IMU command", "uid, destination, imuIdx, imu command as a string", webImuCmd},
    {"/sensor_configure/set",
     "Set the configuration for a slot, requires reboot for get to see change",
     "uid, destination [0-23,all], type [BOARDTYPE_MCG|BOARDTYPE_ECG|BOARDTYPE_IMU_COIL|BOARDTYPE_EMPTY]",
     webSensorConfigureSet},
    {"/sensor_configure/get",
     "Get the run time configuration for a slot",
     "uid, destination [0-23,all]",
     webSensorConfigureGet},
    {"/sensor_configure/status/get", "Return the status of all boards", "uid", webSensorConfigureStatusGet},
    {"/system_status/get", "Return the system error status", "uid", webSystemStatusGet},
    {"/echo", "Send a simple JSON command back", "echo", NULL},
    {"/cnc", "Send a CNC command", "destination, peripheral, command, address, value, uid", webCnc},
    {"/fan/speed/get", "Get the Fan speed as a percentage", "uid", webFanSpeedGet},
    {"/fan/speed/set", "Get the Fan speed as a percentage", "uid, speed [0-100]", webFanSpeedSet},
    {"/fan/tachometer/get", "Get the current Fan speed in RPM of the fan [0|1]", "uid fan", webFanTachometerGet},
    {"/fan/temperature/get", "Get the current temperature in C", "uid", webFanTemperatureGet},
    {"/fan/register/get", "Get the register value at address 8bit", "uid, address", webFanRegisterGet},
    {"/fan/register/set", "Set the register value 8bit at address 8bit", "uid, address, value", webFanRegisterSet},
    {"/debug/log", "Retrieve debug log from destination", "uid, destination", webDebugLogGet},
    {"/debug/clearlog", "CLear the debug log contents at destination", "uid, destination", webDebugLogClear},
    {"/debug/db/stats",
     "Return the spi stats from destination",
     "uid, destination, type [spi_db,watchdog,task,stack,heap,system,reglist]",
     webDebugStats},
    {"/debug/mb/stats",
     "Return the spi stats from the main board",
     "uid, type [spi_1,spi_2,spi_3,dbproc_0,...,dbproc_23,watchdog,task,stack,heap,system,reglist]",
     webDebugStatsMb},
    {"/power/set", "Set power mode on or low", "uid, on [true|false]", webPowerSet},
    {"/power/get", "Get power mode on or low", "uid", webPowerGet},
    {"/dbg/setTargetToEmpty",
     "uid, target",
     "for each target value found in SENSOR_BOARD_xx register change it to empty (7)",
     webCorrectTargetToEmpty},
    {"/dbg/setTargetToNewValue",
     "uid, target, newvalue",
     "for each target value found in SENSOR_BOARD_xx register change it to newValue",
     webCorrectTargetToNewValue},

    {"/ecg12/rightLegDriveConnectedStatus/get",
     "uid, destination",
     "for target get Right leg Drive connect status. See ADS1298 Reg 0x3 bit 0.",
     webRLDriveConnectedStatusGet},
    {"/ecg12/rightLegDriveFunctionEnable/set",
     "uid, destination, enable[true|false]",
     "for target set Right leg Drive sense function enable state. See ADS1298 Reg 0x3 bit 1.",
     webRLDriveFunctionSet},
    {"/ecg12/rightLegDriveFunctionEnable/get",
     "uid, destination",
     "for target get Right leg Drive sense function enable state. See ADS1298 Reg 0x3 bit 1.",
     webRLDriveFunctionGet},
    {"/ecg12/leadOffComparatorThreshold/set",
     "uid, destination, threshold[95000 - 70000 m%]",
     "for target set lead off comparator threshold using positive side will be bin to closest allowed value. See "
     "ADS1298 Reg 0x4 bit 7:5.",
     webLeadOffComparatorThresholdSet},
    {"/ecg12/leadOffComparatorThreshold/get",
     "uid, destination",
     "for target get lead off comparator threshold value. See ADS1298 Reg 0x4 bit 7:5",
     webLeadOffComparatorThresholdGet},
    {"/ecg12/leadOffModeDetection/set",
     "uid, destination, mode[0,1]",
     "for target set lead off detection mode 0=current, 1=resistor. See ADS1298 Reg 0x4 bit 4.",
     webLeadOffDetectionModeSet},
    {"/ecg12/leadOffModeDetection/get",
     "uid, destination",
     "for target get lead off detection mode 0=current, 1=resistor. See ADS1298 Reg 0x4 bit 4",
     webLeadOffDetectionModeGet},
    {"/ecg12/ileadOffCurrentMagnitude/set",
     "uid, destination, magnitude[6,12,18,24]",
     "for target set value of Lead Off current magnitude setting, values (6,12,18,24)nA will select value closest to "
     "allowed values. See ADS1298 Reg 0x4 bit 3:2.",
     webIleadOffCurrentSet},
    {"/ecg12/ileadOffCurrentMagnitude/get",
     "uid, destination",
     "for target get value of Lead Off current magnitude setting in nA. See ADS1298 Reg 0x4 bit 3:2.",
     webIleadOffCurrentGet},
    {"/ecg12/leadOffFrequency/set",
     "uid, destination, type[1,3]",
     "for target set value of Lead Off Frequency setting, value must 1(AC) or 3(DC). See ADS1298 Reg 0x4 bit 1:0.",
     webEcg12FleadOffFreqSet},
    {"/ecg12/leadOffFrequency/get",
     "uid, destination",
     "for target get value of Lead Off Frequency setting, value must 1(AC) or 3(DC). See ADS1298 Reg 0x4 bit 1:0.",
     webEcg12FleadOffFreqGet},
    {"/ecg12/leadOffPositiveState/get",
     "uid, destination",
     "Lead off state for negative states. See ADS1298 Reg 0x12.",
     webLeadOffPositiveStateGet},
    {"/ecg12/leadOffNegativeState/get",
     "uid, destination",
     "Lead off state for negative states. See ADS1298 Reg 0x11.",
     webLeadOffNegativeStateGet},
    {"/ecg12/leadConnectionStates/get",
     "uid, destination",
     "from destination get the Connection lead states",
     webLeadConnectionStatesGet},
    {"/ecg12/paceDetect/reset", "uid, destination", "reset the pace detect", webPaceDetectReset},
    {"/ecg12/paceDetect/get", "uid, destination", "get the pace detect", webPaceDetectGet},
};

#endif /* WEBSERVER_INC_WEBCMDS_H_ */
