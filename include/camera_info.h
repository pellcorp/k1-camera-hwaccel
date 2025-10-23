#ifndef _CAMERA_INFO_H_
#define _CAMERA_INFO_H_

#include <stdbool.h>
#include <linux/videodev2.h>

struct camera {
    int fd;
    char *name;
    void *bufs[VIDEO_MAX_FRAME];
    __u32 bcount;
    volatile __u32 index;

    volatile long long count_offset;
    volatile long long count;
    volatile long long last_count;

    volatile int handle_buf_flag;
    volatile bool thread_flag;
    pthread_t id;

    __u32 select_timeout_flag;
};

struct camera_info {

    struct camera camera_r;
    struct camera camera_l;
    struct camera camera;
    __u32 width;
    __u32 height;
    __u32 pixfmt;
    int buffer_size;

    __u32 threshold;
    __u32 count_1s;
};

#endif
