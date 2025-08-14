/**
 * @file
 * Function scope exception handling macros for C.
 *
 * See test_try_catch.c for an example.
 *
 * Copyright Nuvation Research Corporation 2013-2017. All Rights Reserved.
 * www.nuvation.com
 */

#ifndef NUVC_TRY_CATCH_H_
#define NUVC_TRY_CATCH_H_

#include "log.h"

/**
 * @brief Create a TRY code block that may #RAISE errors.
 *
 * Only a single TRY block is permitted per function. Any #RAISE statements
 * must be placed within a TRY block. A #FINALLY block must be created for
 * each TRY block.
 */
#define TRY                                                                                                            \
    goto try;                                                                                                          \
    try:

/**
 * @brief Raise a named error.
 *
 * Must be called within a #TRY block.
 *
 * @param errorName The name of the error to raise. There
 * must be an associated #CATCH(errorName) block
 * with matching name.
 */
#define RAISE(errorName)                                                                                               \
    {                                                                                                                  \
        LOG_ERR("Error '" #errorName "' raised.");                                                                     \
        goto errorName;                                                                                                \
    }

/**
 * @brief Catch a named error.
 *
 * Catches any errors raised with #RAISE(errorName).
 *
 * @param errorName The name of the error to catch.
 */
#define CATCH(errorName)                                                                                               \
    goto finally;                                                                                                      \
    errorName:

/**
 * @brief Create a FINALLY code block to perform final actions.
 *
 * Each #TRY block must have an associated FINALLY block. Code in the
 * FINALLY block is executed last, regardless of whether or not  an error
 * was rased.
 */
#define FINALLY                                                                                                        \
    finally:

#endif /* NUVC_TRY_CATCH_H_ */
