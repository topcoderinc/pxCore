#ifndef POLYFILL_H_
#define POLYFILL_H_

// A poly fill headers for mac os
// SOL_TCP is not defined on mac
#ifndef SOL_TCP
#define SOL_TCP IPPROTO_TCP
#endif

#if defined(__APPLE__)

// No pipe2 on mac, use pipe instead
// Not found an equivalent workaround for flags O_CLOEXEC
// So in mac, O_CLOEXEC will be ignored.
#define pipe2(pipefd, flags) \
	pipe(pipefd)

#endif

#endif
