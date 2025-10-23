#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <inttypes.h>
#include <getopt.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>

#include <v4l2.h>

int qbuf(int fd, __u32 buf_index)
{
    struct v4l2_buffer buf;

    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = buf_index;
    if (ioctl(fd, VIDIOC_QBUF, &buf)) {
        fprintf(stderr, "\n\nQBUF FAILED %s \n", strerror(errno));
        return -1;
    }
    return 0;
}

bool type_is_mplane(__u32 type)
{
    switch (type) {
        case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
        case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
            return true;

        default:
            return false;
    }
}

void setup_format(struct v4l2_format *format, struct setformat *setformat)
{
    __u32 sizeimage;

    memset(format, 0, sizeof(*format));
    format->type = setformat->type;

    if(setformat->pixfmt == V4L2_PIX_FMT_H264 || setformat->pixfmt == V4L2_PIX_FMT_JPEG) {
        sizeimage = setformat->width * setformat->height * 2;	/*压缩后的数据不可能超过原始帧大小.*/
    } else {
        sizeimage = 0;
    }

    if (type_is_mplane(setformat->type)) {
        format->fmt.pix_mp.width = setformat->width;
        format->fmt.pix_mp.height = setformat->height;
        format->fmt.pix_mp.plane_fmt[0].sizeimage = sizeimage;
        format->fmt.pix_mp.pixelformat = setformat->pixfmt;
    } else {
        format->fmt.pix.width = setformat->width;
        format->fmt.pix.height = setformat->height;
        format->fmt.pix.pixelformat = setformat->pixfmt;
    }
}

int try_format(int fd, struct setformat *setformat)
{
    struct v4l2_format format;
    int rc;

    setup_format(&format, setformat);

    rc = ioctl(fd, VIDIOC_TRY_FMT, &format);
    if (rc < 0) {
        fprintf(stderr, "Unable to try format for type %d: %s\n", setformat->type,
                strerror(errno));
        return -1;
    }
    return 0;
}

int set_format(int fd, struct setformat *setformat)
{
    int rc;
    struct v4l2_format vfmt;

    if (try_format(fd, setformat))
        return -1;

    setup_format(&vfmt, setformat);

    rc = ioctl(fd, VIDIOC_S_FMT, &vfmt);
    if (rc < 0) {
        fprintf(stderr, "Unable to set format for type %d: %s\n", setformat->type,
                strerror(errno));
        return -1;
    }
    return 0;
}

int reqbufs(int fd, __u32 buf_count, __u32 type)
{
    struct v4l2_requestbuffers reqbufs;
    int err;

    memset(&reqbufs, 0, sizeof(reqbufs));
    reqbufs.count = buf_count;
    reqbufs.type = type;
    reqbufs.memory = V4L2_MEMORY_MMAP;
    err = ioctl(fd, VIDIOC_REQBUFS, &reqbufs);
    if (err >= 0) {
        return reqbufs.count;
    }

    return -1;
}

int do_setup_cap_buffers(struct camera *camera)
{
    struct v4l2_buffer buf;
    for (__u32 i = 0; i < camera->bcount; i++) {

        memset(&buf, 0, sizeof(buf));

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(camera->fd, VIDIOC_QUERYBUF, &buf))
            return -1;

        if (buf.memory == V4L2_MEMORY_MMAP) {
            camera->bufs[i] = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, camera->fd, buf.m.offset);

            if (camera->bufs[i] == MAP_FAILED) {
                fprintf(stderr, "mmap failed\n");
                return -1;
            }
        }
        qbuf(camera->fd, i);
    }
    return buf.length;
}

static int read_frame(struct camera *camera)
{
    int ret;
    struct v4l2_buffer buf;
    int fd = camera->fd;
    memset(&buf, 0, sizeof(buf));

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    for (;;) {
        ret = ioctl(fd, VIDIOC_DQBUF, &buf);
        camera->index = buf.index;
        if (ret < 0) {
            fprintf(stderr, "%s: failed : %s\n", "VICIOC_DQBUF", strerror(errno));
            return -1;
        }
        if (!(buf.flags & V4L2_BUF_FLAG_ERROR)) {
            break;
        }
    }
    return 0;
}

int get_frame_from_camera(struct camera *camera)
{
    int r;

    while(1) {
        fd_set read_fds;
#if 0
        fd_set exception_fds;
#endif
        struct timeval timeout = { 1, 50000 };

#if 0
        FD_ZERO(&exception_fds);
        FD_SET(camera->fd, &exception_fds);
#endif
        FD_ZERO(&read_fds);
        FD_SET(camera->fd, &read_fds);
        r = select(camera->fd + 1, &read_fds, NULL, NULL, &timeout);
        if (r == -1) {
            if (EINTR == errno)
                continue;
            fprintf(stderr, "select error: %s\n",strerror(errno));
            return -1;
        }

        if (r == 0) {
            fprintf(stderr, "select timeout\n");
            return -1;
        }

        if (FD_ISSET(camera->fd, &read_fds)) {
            if (read_frame(camera) < 0) {
                return -1;
            } else {
                break;
            }
        }
    }
    return 0;
}

