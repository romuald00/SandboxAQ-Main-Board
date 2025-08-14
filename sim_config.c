/**
 * @file
 * Implementation of simulated configuration functions.
 *
 * Copyright Nuvation Research Corporation 2014-2018. All Rights Reserved.
 * www.nuvation.com
 */

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sim/sim_config.h"

static const char *const default_argv[] = {"sim"};

static struct {
    int argc;
    const char *const *argv;
} args = {
    .argc = 1,
    .argv = default_argv,
};

static pthread_mutex_t parse_mutex = PTHREAD_MUTEX_INITIALIZER;

void SimStoreArgs(int argc, const char *const *argv) {
    args.argc = argc;
    args.argv = argv;
}

void SimGetArgs(int *argc, const char *const **argv) {
    assert(argc != NULL);
    assert(argv != NULL);
    *argc = args.argc;
    *argv = args.argv;
}

void SimParseArgs(SimArgCallback callback, const char *options) {
    int c;
    char **fakeArgv = malloc(args.argc * sizeof(char *));

    pthread_mutex_lock(&parse_mutex);

    /* We need a copy of argv because getopt changes the order. */
    assert(fakeArgv != NULL);
    memcpy(fakeArgv, args.argv, args.argc * sizeof(char *));

    optind = 1;

    assert(callback != NULL);
    assert(options != NULL);

    while ((c = getopt(args.argc, fakeArgv, options)) != -1) {

        if (c == '?')
            continue;

        assert(callback(c, optarg) == 0);
    }

    pthread_mutex_unlock(&parse_mutex);

    free(fakeArgv);
}
