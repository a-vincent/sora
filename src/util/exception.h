#ifndef UTIL_EXCEPTION_H_
#define UTIL_EXCEPTION_H_
/*
 * Exceptions handling macros and prototypes for ANSI C[89]9.
 *
 * Programmed in 2001 by Aymeric Vincent. Public domain.
 *
 */

#include <setjmp.h>

#if (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 5))
#define NORETURN __attribute__ ((noreturn))
#else
#define NORETURN
#endif

#define EXCEPTION_DEFINE(exc, par) \
    struct exception_class EXCEPTION__COMPUTE_NAME(exc) = {		\
	#exc,								\
	&EXCEPTION__COMPUTE_NAME(par)					\
    }

#define EXCEPTION_DECLARE(exc) \
    extern struct exception_class EXCEPTION__COMPUTE_NAME(exc)

#define EXCEPTION_CATCH(exc, instruction)				\
do {									\
    struct exception__context exception__local_context;			\
    exception__local_context.catcher = &EXCEPTION__COMPUTE_NAME(exc);	\
    exception__local_context.parent = exception__top_context;		\
    exception__top_context = &exception__local_context;			\
    if (sigsetjmp(exception__local_context.environment, 0) == 0) {	\
	instruction;							\
	exception__has_been_raised = 0;					\
    } else								\
	exception__has_been_raised = 1;					\
    exception__top_context = exception__top_context->parent;		\
} while (/* CONSTCOND */ 0)

extern const void *exception_get_arg(void);
extern const char *exception_get_name(void);

#define EXCEPTION_IS_RAISED(exc) \
    exception__is_raised(&EXCEPTION__COMPUTE_NAME(exc))

#define EXCEPTION_RAISE(exc, arg) \
    exception__raise(&EXCEPTION__COMPUTE_NAME(exc), arg) /* NOTREACHED */

#define EXCEPTION_RAISE_SAME() exception__raise_same() /* NOTREACHED */

/*
 * Everything below this point is PRIVATE to the exception module.
 */
typedef struct exception_class *exception__;

struct exception_class {
    const char *name;
    struct exception_class *parent;
};

struct exception__context {
    struct exception__context *parent;
    sigjmp_buf environment;
    exception__ catcher;
};

extern int exception__is_raised(exception__);
extern void exception__raise(exception__, const void *) NORETURN;
extern void exception__raise_same(void) NORETURN;

extern int exception__has_been_raised;
extern struct exception__context *exception__top_context;

#define EXCEPTION__COMPUTE_NAME(exc) exception__name_ ## exc

EXCEPTION_DECLARE(any);
EXCEPTION_DECLARE(runtime_error);
EXCEPTION_DECLARE(overflow_error);	/* inherits from runtime_error */
EXCEPTION_DECLARE(logic_error);

#endif /* UTIL_EXCEPTION_H_ */
