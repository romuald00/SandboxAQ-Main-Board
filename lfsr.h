/**
 * @file
 * API definition for a 16 bit galois LFSR.
 *
 * Copyright Nuvation Research Corporation 2012-2017. All Rights Reserved.
 * www.nuvation.com
 */

#ifndef NUVC_LFSR_H_
#define NUVC_LFSR_H_

#define INITIAL_LFSR_SEED UINT16_C(0)

/**
 * Fills the provided data buffer with the output of a 16 bit Galois LFSR
 * using the specified seed value.
 *
 * The first time this function is called, the seed should be
 * INITIAL_LFSR_SEED.
 * Afterwards, the LFSR state returned from a previous call can be used as the
 * seed to continue sequence generation from the previous end point.
 *
 * @param[in] data  points to the buffer to fill
 * @param[in] size  size of the input buffer in bytes
 * @param[in] lfsr  seed value for calculating the lfsr output
 * @return          the current state of the lfsr
 */
extern uint16_t Lfsr16(uint8_t *data, size_t size, uint16_t lfsr);

#endif /* NUVC_LFSR_H_ */
