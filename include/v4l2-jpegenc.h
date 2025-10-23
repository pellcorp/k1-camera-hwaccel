#ifndef _V4L2_JPEGENC_H_
#define _V4L2_JPEGENC_H_

#include <linux/videodev2.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define TS_REF_INDEX(index) ((index) * 1000)
#define INDEX_REF_TS(ts) ((ts) / 1000)

struct encode_config {
    char *name;
    __u32 width;
    __u32 height;
    __u32 pixfmt;

    void *input_buf;
    void *output_buf;

    void *encode;
};

struct encode {
    char *name;
    int fd;
    __u32 width;
    __u32 height;
    __u32 pixfmt;

    __u32 planes_count;
    __u32 capture_type;
    __u32 output_type;
    __u32 v4l2_buffers_count;
    __u32 buffers_count;
    __u32 buf_count;

    void *input_buf;
    void *output_buf;

    void *buffers;
};

struct video_buffer {
    /*source*/
    void *source_map[VIDEO_MAX_PLANES];
    __u32 source_map_lengths[VIDEO_MAX_PLANES];
    void *source_data[VIDEO_MAX_PLANES];
    __u32 source_sizes[VIDEO_MAX_PLANES];		// unused?
    __u32 source_offsets[VIDEO_MAX_PLANES];		// unused.
    __u32 source_bytesperlines[VIDEO_MAX_PLANES];	// unused.
    __u32 source_planes_count;
    __u32 source_buffers_count;

    /*dst*/
    void *destination_map[VIDEO_MAX_PLANES];
    __u32 destination_map_lengths[VIDEO_MAX_PLANES];
    void *destination_data[VIDEO_MAX_PLANES];
    __u32 destination_sizes[VIDEO_MAX_PLANES];
    __u32 destination_offsets[VIDEO_MAX_PLANES];
    __u32 destination_bytesperlines[VIDEO_MAX_PLANES];
    __u32 destination_planes_count;
    __u32 destination_buffers_count;

    int export_fds[VIDEO_MAX_PLANES];
    int request_fd;

    void *output_buf;
};

enum codec_type {
    CODEC_TYPE_H264,
    CODEC_TYPE_JPEG
};

int jpeg_enc_init(struct encode_config *encode_config);
int jpeg_enc_start(struct encode_config *encode_config);
int jpeg_enc_stop(struct encode_config *encode_config);

#endif
