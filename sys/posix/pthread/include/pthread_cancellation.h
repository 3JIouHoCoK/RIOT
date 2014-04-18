/**
 * @ingroup pthread
 */

#define PTHREAD_CANCEL_DISABLE 0
#define PTHREAD_CANCEL_ENABLE  1

#define PTHREAD_CANCEL_DEFERRED     0
#define PTHREAD_CANCEL_ASYNCHRONOUS 1

#define PTHREAD_CANCELED ((void *) -2)

/**
 * @brief        Cancellation point are not supported, yet.
 * @param[in]    state      Unused
 * @param[out]   oldstate   Unused
 * @returns      `-1`, this invocation fails
 */
int pthread_setcancelstate(int state, int *oldstate);

/**
 * @brief        Cancellation point are not supported, yet.
 * @param[in]    type      Unused
 * @param[out]   oldtype   Unused
 * @returns      `-1`, this invocation fails
 */
int pthread_setcanceltype(int type, int *oldtype);

/**
 * @brief        Tells a pthread that it should exit.
 * @note         Cancellation points are not supported, yet.
 * @details      A pthread `th` can call pthread_testcancel().
 *               If pthread_cancel(th) was called before, it will exit then.
 * @param[in]    th   Pthread to tell that it should cancel.
 * @returns      `-1`, this invocation fails
 */
int pthread_cancel(pthread_t th);

/**
 * @brief        Exit the current pthread if pthread_cancel() was called for this thread before.
 * @details      If pthread_cancel() called before, the current thread exits with with the code #PTHREAD_CANCELED.
 */
void pthread_testcancel(void);
