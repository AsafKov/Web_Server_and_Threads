//
// Created by student on 6/11/22.
//

#ifndef THREADS_SERVERREQUEST_H
#define THREADS_SERVERREQUEST_H

typedef struct ServerRequest {
    int fd;
    struct timeval arrival_interval;
    struct timeval dispatch_interval;
} ServerRequest;
#endif //THREADS_SERVERREQUEST_H
