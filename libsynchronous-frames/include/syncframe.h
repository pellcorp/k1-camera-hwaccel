#ifndef _SYNCFRAME_H_
#define _SYNCFRAME_H_
#include <asm/types.h>
#include <stdbool.h>
#include <linux/videodev2.h>

#ifdef __cplusplus
extern "C" {
#endif
#define SYNCFRAME       true
#define MONOFRAME       false

struct buf_info{
    void *camera_buf;
    __u32 index;
};

struct cameras_buf {
    struct buf_info Rcamera;
    struct buf_info Lcamera;
    float time_diff;
};

struct config {//
    bool video_flag;

    char *camera_r_name;
    char *camera_l_name;
    char *camera_name;

    __u32 width;
    __u32 height;
    __u32 pixfmt;
    __u32 buffer_size;
    __u32 bcount;

    void *camera;

};

int cameras_init(bool video_flag, struct config *config);
struct cameras_buf *dqbuf_syncframe(struct config *config);
void qbuf_syncframe(struct config *config, struct cameras_buf *syncframe);
struct buf_info *dqbuf_monoframe(struct config *config);
void qbuf_monoframe(struct config *config, struct buf_info *monoframe);
void reset_buf(struct config *config);
void uninit_video(struct config *config);

#ifdef __cplusplus
}
#endif
#endif
