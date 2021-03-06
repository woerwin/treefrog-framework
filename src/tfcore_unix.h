#ifndef TFCORE_UNIX_H
#define TFCORE_UNIX_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

#ifndef Q_OS_UNIX
# error "tfcore_unix.h included on a non-Unix system"
#endif

#define EINTR_LOOP(var, cmd)                    \
    do {                                        \
        var = cmd;                              \
    } while (var == -1 && errno == EINTR)


static inline int tf_close(int fd)
{
    register int ret;
    EINTR_LOOP(ret, ::close(fd));
    return ret;
}

#undef TF_CLOSE
#define TF_CLOSE tf_close

#endif // TFCORE_UNIX_H
