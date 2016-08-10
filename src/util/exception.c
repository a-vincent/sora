/*
 * Exceptions handling functions for ANSI C[89]9.
 *
 * Programmed in 2001 by Aymeric Vincent. Public domain.
 *
 */

#include <util/exception.h>

#include <stdlib.h>

struct exception_class EXCEPTION__COMPUTE_NAME(any) = { "any", NULL };

EXCEPTION_DEFINE(runtime_error, any);
EXCEPTION_DEFINE(overflow_error, runtime_error);
EXCEPTION_DEFINE(logic_error, any);

/*
 * Exception__has_been_raised is set to something different than 0 iff an
 * EXCEPTION_RAISE() occurred during the last call to EXCEPTION_CATCH()
 */
int exception__has_been_raised = 0;
struct exception__context *exception__top_context = NULL;

static const void *current_arg;
static exception__ current_exception;

const void *
exception_get_arg(void) {
    return current_arg;
}

const char *
exception_get_name(void) {
    return current_exception->name;
}

static int
is_caught_by(exception__ raised, exception__ catcher) {
    return catcher == &EXCEPTION__COMPUTE_NAME(any) || catcher == raised ||
	(raised->parent != NULL && is_caught_by(raised->parent, catcher));
}

int
exception__is_raised(exception__ exc) {
    return exception__has_been_raised && is_caught_by(current_exception, exc);
}

void
exception__raise(exception__ exc, const void *arg) {
    current_exception = exc;
    current_arg = arg;

    while (exception__top_context != NULL &&
	    !is_caught_by(exc, exception__top_context->catcher))
	exception__top_context = exception__top_context->parent;

    if (exception__top_context == NULL)
	abort();

    siglongjmp(exception__top_context->environment, 1);
}

void
exception__raise_same(void) {
    exception__raise(current_exception, current_arg);
}
