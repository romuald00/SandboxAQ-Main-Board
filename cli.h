/**
 *  @file
 *  Portable Command Line Interface Library
 *
 *  Header file for the portable command line interface.
 *
 *  Copyright Nuvation Research Corporation 2006-2018. All Rights Reserved.
 *  www.nuvation.com
 */

#ifndef NUVC_CLI_H_
#define NUVC_CLI_H_

#include "ringbuffer.h"
#include <inttypes.h>

#define MIN_CLI_BUFFER_LOG2SIZE 9
#define MIN_CLI_BUFFER_SIZE (1 << MIN_CLI_BUFFER_LOG2SIZE)

/** The maximum number of arguments a command can have. */
#define CLI_MAX_COMMAND_ARGUMENTS 32

#define CLI_MAX_COMMAND_HISTORY 5

#define CLI_RESULT_OFFSET (0)

#define COMMAND_IS_EXECUTING 0
#define ECHO_IS_SUPPRESSED 1
#define BREAK_AFTER_COMMAND 2
#define INTERPRETING_ESC_STAGE_1 3
#define INTERPRETING_ESC_STAGE_2 4

#define CLI_SET_BOOL(hCli, bit) ((hCli)->globalBools |= (1 << (bit)))
#define CLI_CLEAR_BOOL(hCli, bit) ((hCli)->globalBools &= ~(1 << (bit)))
#define CLI_CHECK_BOOL(hCli, bit) (((hCli)->globalBools >> (bit)) & 1)

/** The result code from executing a CLI API function. */
typedef enum {
    /** The function completed successfully. */
    CLI_RESULT_SUCCESS = 0,

    /** One of the arguments to the function was an illegal value. */
    CLI_RESULT_BAD_ARGUMENT = CLI_RESULT_OFFSET - 1,

    /** Too many arguments were passed to a CLI function. */
    CLI_RESULT_TOO_MANY_ARGUMENTS = CLI_RESULT_OFFSET - 2,

    /** Could not find matching command in command list */
    CLI_RESULT_UNRECOGNIZED_COMMAND = CLI_RESULT_OFFSET - 3,

    /** No data is available to read */
    CLI_RESULT_NO_DATA = CLI_RESULT_OFFSET - 4,

    /** Command encountered an internal error */
    CLI_RESULT_ERROR = CLI_RESULT_OFFSET - 5,

    CLI_RESULT_LAST_RESULT = CLI_RESULT_OFFSET - 6,
} CLI_RESULT;

/** A defined type for the command line interface definition structure. */
typedef struct _cli CLI;

/**
 * This signature type defines the callback when new command comes in.
 */
typedef void (*CommandReadyCallback)(CLI *hCli);

/** The type of a command function to be registered with the CLI. */
typedef int16_t (*CLI_COMMAND_FUNCTION)(CLI *hCli, int argc, char *argv[]);

/**
 * A structure containing the registration information for a command to be
 * used in the Command Line Interface.
 */
typedef struct {
    /**
     * The name of the command.
     *
     * The name cannot be NULL and should not contain any whitespace.
     */
    char *name;

    /**
     * Description for the command.
     *
     * If no description, leave NULL.
     */
    char *description;

    /**
     * Help text for the command.
     *
     * If no help text, leave NULL.
     */
    char *helpText;

    /**
     * A pointer to the function to call when the command is executed.
     *
     * If the pointer is NULL the command will be treated as NO OPERATION.
     */
    CLI_COMMAND_FUNCTION function;
} CLI_COMMAND;

/** The command line interface definition structure. */
struct _cli {
    /** The number of commands in the commandList. */
    int numCommands;

    /** A pointer to the array of command definitions. */
    const CLI_COMMAND *commandList;

    /** The prompt string to display. */
    char prompt[8];

    SMALL_RINGBUFFER_TYPE(MIN_CLI_BUFFER_LOG2SIZE) stdinBuffer;
    int cliUart;
    volatile uint8_t globalBools;
    int8_t currentHistoryBuffer;
    int8_t maxHistoryBuffer;
    int8_t lastViewedHistoryBuffer;
    uint8_t upLevels;
    uint8_t *inputBuffer;
    uint8_t *historyBuffers;
    uint16_t inputBufferIndex;
    uint16_t historyBufferIndexes[CLI_MAX_COMMAND_HISTORY];
    CommandReadyCallback commandReadyCallback;

    int cliArgc;
    char *argv[CLI_MAX_COMMAND_ARGUMENTS];
};

typedef void (*InitMessageGenerator)(void);

/**
 * Initialize the CLI module. The UART driver needs to be intialized before
 * calling this function.
 *
 * @param[in]  hCli
 *      the current CLI instance
 * @param[in]  channel
 *      Which uart to attach to.
 * @param[in]  _inputBuffer
 *      Memory buffer to use for compiling the message.
 * @param[in]  _historyBuffers
 *      Memory buffers to use for command history.
 * @param[in]  callback
 *      CLI handler callback
 * @param[in]  initMsgs
 *      Routine to call to generate initial messages. If initMSgs is NULL,
 *      no keys are printed.
 */
void CliInit(CLI *hCli,
             int channel,
             int numCommands,
             const CLI_COMMAND *commandList,
             const char *prompt,
             uint_least8_t *_inputBuffer,
             uint_least8_t *_historyBuffers,
             CommandReadyCallback callback,
             InitMessageGenerator initMsgs);

/**
 * Callback to execute when a new character is received. Parses the character
 * according to the CLI protocol.
 *
 * @param[in] hCli
 *      the current CLI instance
 * @param[in] ch
 *      holds the character received from the UART module
 */
void CliCharacterReceivedCallback(CLI *hCli, uint_least8_t ch);

/**
 * Tells the CLI to prepare for new commands to come in.
 *
 * @param[in] hCli
 *      A pointer to the Command Line Interface (CLI) structure.
 */
void CliGetReadyForNewCommand(CLI *hCli);

/**
 * Returns the latest command.
 *
 * @param[in] hCli
 *      A pointer to the Command Line Interface (CLI) structure.
 *
 * @param[out] argc
 *      The number of tokens in the command line (standard C argc argument)
 *
 * @param[out] argv
 *      The tokens from the command line (standard C argv argument)
 *
 * @param[out] cmd
 *      A pointer to a pointer to the CLI_COMMAND structure being requested on
 *      the command line. The provided pointer is updated to point to the
 *      appropriate CLI_COMMAND structure if successful, or NULL if an error
 *      occurred.
 *
 * @return
 *      - CLI_RESULT_SUCCESS if parsing was successful.
 *      - CLI_RESULT_UNRECOGNIZED_COMMAND if parsing was unsuccessful.
 *      - CLI_RESULT_TOO_MANY_ARGUMENTS if CLI_MAX_COMMAND_ARGUMENTS was
 *        exceeeded.
 */
CLI_RESULT CliGetNewCommand(const CLI *hCli, const CLI_COMMAND **cmd, int *argc, char *argv[]);

/**
 * Reads a character from the UART.
 *
 * @param[in] hCli
 *      the current CLI instance
 *
 * @return
 *      negative if no data, otherwise the received char
 */
int CliReadChar(CLI *hCli);

/**
 * Writes the given character to the UART.
 *
 * @param[in] hCli
 *      the current CLI instance
 *
 * @param[in] ch
 *      The character to be written.
 */
void CliWriteChar(CLI *hCli, int ch);

/**
 * Writes the given string to the UART.
 *
 * @param[in] hCli
 *      the current CLI instance
 *
 * @param[in] string
 *      The string to write to the CLI's output function.
 */
void CliWriteString(CLI *hCli, const char *string);

/**
 * Parses commandBuffer into a list of arguments.
 *
 * @param[in,out] commandBuffer
 *      A pointer to the command buffer to be parsed.  This command buffer
 *      will be altered by removing quotes around parameters, expanding
 *      escaped quotes, tabs, bells, and newlines.
 *
 * @param[out] argc
 *      a pointer to an integer to hold the number of arguments parsed.
 *
 * @param[out] argv
 *      a pointer to the array of arguments parsed.
 *
 * @return
 *      - CLI_RESULT_SUCCESS if parsing was successful.
 *      - CLI_RESULT_TOO_MANY_ARGUMENTS if CLI_MAX_COMMAND_ARGUMENTS was
 *        exceeeded.
 */
CLI_RESULT CliParse(char *commandBuffer, int *argc, char *argv[]);

/**
 * Suppresses characters from being echoed on the CLI.
 *
 * This only applies to the command prompt. Commands will still print normally
 * during execution.
 */
void CliSuppressEcho(CLI *hCli);

/**
 * Re-enables characters to be echoed on the CLI.
 */
void CliEnableEcho(CLI *hCli);

#endif /* NUVC_CLI_H_ */
