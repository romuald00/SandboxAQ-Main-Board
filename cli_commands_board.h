/*
 * cli_commands_db.h
 *
 *  Copyright Nuvation Research Corporation 2018-2024. All Rights Reserved.
 *  Created on: Oct 27, 2023
 *      Author: jzhang
 */

#ifndef APP_INC_CLI_COMMANDS_DB_H_
#define APP_INC_CLI_COMMANDS_DB_H_

#include "cli/cli.h"
#include "dbCommTask.h"
#include "dbTriggerTask.h"
#include "ddsTrigTask.h"
#include "realTimeClock.h"

int16_t testCommand(CLI *hCli, int argc, char *argv[]);
int16_t spiCliCmd(CLI *hCli, int argc, char *argv[]);
int16_t adcCommand(CLI *hCli, int argc, char *argv[]);
int16_t ipCliCommand(CLI *hCli, int argc, char *argv[]);
int16_t resetCmd(CLI *hCli, int argc, char *argv[]);
int16_t gpioCommand(CLI *hCli, int argc, char *argv[]);
int16_t fanCtrlCliCmd(CLI *hCli, int argc, char *argv[]);

#define NUM_BOARD_CMDS 10
#define BOARD_CMDS                                                                                                     \
    {"spi",                                                                                                            \
     "Display spi Information\r\n",                                                                                    \
     "\tstats - display spi stats\r\n"                                                                                 \
     "\tclear - clear spi stats\r\n"                                                                                   \
     "\tsend -  send a test packet to wake the spi bus\r\n"                                                            \
     "\ttxcrcerrorcnt - <spi> <cnt> on this spi send consecutive crc errors\r\n"                                       \
     "\tmapBoardToDbg6 <board>  - maps board SPI CS to DBG_6 pin\r\n",                                                 \
     spiCliCmd},                                                                                                       \
        {"adc",                                                                                                        \
         "Restart ADC continuous conversions or get set adc periodic\r\nparameters in one shot mode",                  \
         "\tstart - start ADC conversions\n\r"                                                                         \
         "\tstop - stop ADC conversions\n\r"                                                                           \
         "\tget_duty -  get duty\r\n"                                                                                  \
         "\tget_frequency -  get frequency\r\n"                                                                        \
         "\tset_duty <value %>-  set duty\r\n"                                                                         \
         "\tset_frequency <value_usec> -  set frequency\r\n",                                                          \
         adcCommand},                                                                                                  \
        {"ddsTrigger",                                                                                                 \
         "start and stop the dds wave generation",                                                                     \
         "\tstart - start wave generation"                                                                             \
         "\tstop - stop wave generation",                                                                              \
         ddsTrigCliCmd},                                                                                               \
        {"ddsClock",                                                                                                   \
         "Restart DDS clock feed",                                                                                     \
         "\tstart - start DDS Clock\r\n"                                                                               \
         "\tstop - stop DDS Clock\r\n"                                                                                 \
         "\tget_duty -  get duty\r\n"                                                                                  \
         "\tget_frequency -  get frequency\r\n"                                                                        \
         "\tset_duty <value %>-  set duty\r\n"                                                                         \
         "\tset_frequency <x100Hz> - set frequency 500 = 5Hz\r\n",                                                     \
         ddsClockCliCmd},                                                                                              \
        {"dbproc",                                                                                                     \
         "Test and stats for daughterboard communication tasks",                                                       \
         "\tstats <dbId all|[0-47]> - display stats for daughter board tasks\r\n"                                      \
         "\tclear <dbId all|[0-47]> - reset daughter board tasks stats\r\n"                                            \
         "\tsend <dbId [0-47]> <register> - read the ADC register at \r\n\t\taddr <register>\r\n"                      \
         "\tloopback_mb <dbId [0-47]> <value> - loop back from Main Board\r\n\t\ta set data over UDP stream\r\n"       \
         "\tloopback_delay <dbId [0-47]> <offset> - delay the algorithm by\r\n\t\toffset ticks for data over UDP "     \
         "stream\r\n"                                                                                                  \
         "\tenable <dbId [0-47]> <0|1> - 0 disable or 1 enable daughter board proc"                                    \
         "\tgpioen - show mask of Daughter boards toggle gpio DBG5 pin\r\n"                                            \
         "\tgpioen <dbId [0-47]> <0|1> -  disable (0) or enable gpio toggle\r\n"                                       \
         "\tfillEthPkt <true|false> - fill provision ethernet pkt, if true IMU\r\n\t\tdata is quaternion else euler",  \
         dbProcCliCmd},                                                                                                \
        {"dbtrigger",                                                                                                  \
         "Display settings for the daughter board trigger tasks\r\n",                                                  \
         "\tinterval - display trigger interval in usec\r\n"                                                           \
         "\tinterval <value_us> - set trigger interval\r\n"                                                            \
         "\tmask - 2 32bit values show the trigger mask where lower 24 bits\r\n\t\trepresent enabled boards \r\n",     \
         dbTriggerCliCmd},                                                                                             \
        {"time",                                                                                                       \
         "Set or Display the calendar time\r\n",                                                                       \
         "\tget Display the calendar time\r\n"                                                                         \
         "\tset \"<YYYY MM DD HH:MM:SS>\" - set the day and time\r\n",                                                 \
         rtcCliCmd},                                                                                                   \
        {"reset",                                                                                                      \
         "Reset mainboard",                                                                                            \
         "\treset (write|read) delay <value> - Write time to wait before reset\r\n"                                    \
         "\treset (write|read) trigger - Read trigger register or set trigger to true to reset MB\r\n",                \
         resetCmd},                                                                                                    \
        {"ip",                                                                                                         \
         "Set or Display ip information and port new values take\r\n"                                                  \
         "effect on next boot\r\n",                                                                                    \
         "\tget_ip Display address/netmask/gateway\r\n"                                                                \
         "\tset_ip <x.x.x.x> <y.y.y.y> <z.z.z.z>\r\n"                                                                  \
         "\t     set ip-address  netmask   gateway of the system\r\n"                                                  \
         "\tget_http Display the HTTP webserver port\r\n"                                                              \
         "\tset_http <UINT> set the http port\r\n"                                                                     \
         "\tget_udptxport Display this systems udp tx port\r\n"                                                        \
         "\tset_udptxport <UINT> set the systems udp tx port\r\n"                                                      \
         "\tget_udpclient Display the address and port of the UDP Destination server\r\n"                              \
         "\tset_udpclient <x.x.x.x> <y> - Set the ip address and port of the UDP Server\r\n",                          \
         ipCliCommand},                                                                                                \
        {"gpio",                                                                                                       \
         "commands for gpio set and get",                                                                              \
         "\tlist - list all the gpio pin names\r\n"                                                                    \
         "\tsetPin <port> <pin> <state> - set state for pin example A5 port=[a,b,c,d,e], pin=[0-15], state=0|1\r\n"    \
         "\tgetPin <port> <pin> - get value of pin at port=[a,b,c,d,e], pin=[0-15]\r\n",                               \
         gpioCommand},                                                                                                 \
        {"fan",                                                                                                        \
         "commands for fan control",                                                                                   \
         "\tread <hex addr> - read 8bit value at register, addr 0-5b\r\n"                                              \
         "\twrite <hex addr> <hex value> - write 8bit value to addr\r\n"                                               \
         "\ttemperature - read temperature in C\r\n"                                                                   \
         "\ttach <fan> - read tachometer for fan<0|1> in RPM\r\n"                                                      \
         "\tgetSpeed - read the current speed setting 0-100%\r\n"                                                      \
         "\tgetManual - boolean read the current manual setting\r\n"                                                   \
         "\tsetSpeed <0-100> - read the current speed setting 0-100%\r\n"                                              \
         "\tsetManual <0|1> - boolean 0=manual setting disabled, 1=required for setSpeed to take effect\r\n",          \
         fanCtrlCliCmd},

#endif /* APP_INC_CLI_COMMANDS_DB_H_ */
