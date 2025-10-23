#ifndef _V4L2_H_
#define _V4L2_H_

#include <camera_info.h>

struct setformat {
    __u32 width;
    __u32 height;
    __u32 pixfmt;
    __u32 type;
};

int qbuf(int fd, __u32 buf_index);
int try_format(int fd, struct setformat *setformat);
int set_format(int fd, struct setformat *setformat);
int reqbufs(int fd, __u32 buf_count, __u32 type);
int do_setup_cap_buffers(struct camera *camera);
int get_frame_from_camera(struct camera *camera);
bool type_is_mplane(__u32 type);


#endif
