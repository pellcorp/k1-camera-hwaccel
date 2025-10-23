#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <linux/media.h>
#include <linux/videodev2.h>

#include <v4l2.h>
#include <v4l2-jpegenc.h>

/******************
 * Encode function
 ******************/

static bool type_is_output(__u32 type)
{
    switch (type) {
        case V4L2_BUF_TYPE_VIDEO_OUTPUT:
        case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
            return true;

        default:
            return false;
    }
}

static int query_capabilities(int fd, __u32 *capabilities)
{
    struct v4l2_capability capability;
    int rc;

    memset(&capability, 0, sizeof(capability));

    rc = ioctl(fd, VIDIOC_QUERYCAP, &capability);
    if (rc < 0)
        return -1;

    if (capabilities != NULL) {
        if ((capability.capabilities & V4L2_CAP_DEVICE_CAPS) != 0)
            *capabilities = capability.device_caps;
        else
            *capabilities = capability.capabilities;
    }

    return 0;
}

bool video_engine_capabilities_test(int fd,
        __u32 capabilities_required)
{
    __u32 capabilities;
    int rc;

    rc = query_capabilities(fd, &capabilities);
    if (rc < 0) {
        fprintf(stderr, "Unable to query video capabilities: %s\n",
                strerror(errno));
        return false;
    }

    if ((capabilities & capabilities_required) != capabilities_required)
        return false;

    return true;
}

static int codec_dst_format(enum codec_type type)
{
    switch (type) {
        case CODEC_TYPE_H264:
            return V4L2_PIX_FMT_H264;
        case CODEC_TYPE_JPEG:
            return V4L2_PIX_FMT_JPEG;
        default:
            fprintf(stderr, "Invalid format type\n");
            return -1;
    }
}

static int query_buffer(int fd, __u32 index, __u32 *lengths, __u32 *offsets, __u32 buffers_count, __u32 type)
{
    struct v4l2_plane planes[buffers_count];
    struct v4l2_buffer buffer;
    __u32 i;
    int rc;

    memset(planes, 0, sizeof(planes));
    memset(&buffer, 0, sizeof(buffer));

    buffer.type = type;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.index = index;
    buffer.length = buffers_count;
    buffer.m.planes = planes;

    rc = ioctl(fd, VIDIOC_QUERYBUF, &buffer);
    if (rc < 0) {
        fprintf(stderr, "Unable to query buffer: %s\n",
                strerror(errno));
        return -1;
    }

    if (type_is_mplane(type)) {
        if (lengths != NULL)
            for (i = 0; i < buffer.length; i++)
                lengths[i] = buffer.m.planes[i].length;

        if (offsets != NULL)
            for (i = 0; i < buffer.length; i++)
                offsets[i] = buffer.m.planes[i].m.mem_offset;
    } else {
        if (lengths != NULL)
            lengths[0] = buffer.length;

        if (offsets != NULL)
            offsets[0] = buffer.m.offset;
    }

    return 0;
}

static int queue_buffer(int fd, __u32 type,
        uint64_t ts, __u32 index, __u32 *sizes, __u32 buffers_count)
{
    struct v4l2_plane planes[buffers_count];
    struct v4l2_buffer buffer;
    __u32 i;
    int rc;

    memset(planes, 0, sizeof(planes));
    memset(&buffer, 0, sizeof(buffer));

    buffer.type = type;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.index = index;
    buffer.length = buffers_count;
    buffer.m.planes = planes;

    for (i = 0; i < buffers_count; i++)
        if (type_is_mplane(type))
            buffer.m.planes[i].bytesused = sizes[i];
        else
            buffer.bytesused = sizes[0];

    buffer.timestamp.tv_usec = ts / 1000;
    buffer.timestamp.tv_sec = ts / 1000000000ULL;

    rc = ioctl(fd, VIDIOC_QBUF, &buffer);
    if (rc < 0) {
        fprintf(stderr, "Unable to queue buffer: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

static int dequeue_buffer(int fd, __u32 type, __u32 *index,
        __u32 buffers_count, bool *error, __u32 *bytesused)
{
    struct v4l2_plane planes[buffers_count];
    struct v4l2_buffer buffer;
    int rc;

    memset(planes, 0, sizeof(planes));
    memset(&buffer, 0, sizeof(buffer));

    buffer.type = type;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.length = buffers_count;
    buffer.m.planes = planes;

    rc = ioctl(fd, VIDIOC_DQBUF, &buffer);
    if (rc < 0) {
        fprintf(stderr, "Unable to dequeue buffer: %s, type: %d\n",
                strerror(errno), type);
        return -1;
    }

    if(bytesused) {
        *bytesused = buffer.m.planes[0].bytesused;
    }

    *index = buffer.index;

    if (error != NULL)
        *error = !!(buffer.flags & V4L2_BUF_FLAG_ERROR);

    return 0;
}

#ifdef EXPORT_BUFFER
static int export_buffer(int fd, __u32 type, __u32 index,
        __u32 flags, int *export_fds,
        __u32 export_fds_count)
{
    struct v4l2_exportbuffer exportbuffer;
    __u32 i;
    int rc;

    for (i = 0; i < export_fds_count; i++) {
        memset(&exportbuffer, 0, sizeof(exportbuffer));
        exportbuffer.type = type;
        exportbuffer.index = index;
        exportbuffer.plane = i;
        exportbuffer.flags = flags;

        rc = ioctl(fd, VIDIOC_EXPBUF, &exportbuffer);
        if (rc < 0) {
            fprintf(stderr, "Unable to export buffer: %s\n",
                    strerror(errno));
            return -1;
        }

        export_fds[i] = exportbuffer.fd;
    }

    return 0;
}
#endif

static int set_stream(int fd, __u32 type, bool enable)
{
    enum v4l2_buf_type buf_type = type;
    int rc;

    rc = ioctl(fd, enable ? VIDIOC_STREAMON : VIDIOC_STREAMOFF,
            &buf_type);
    if (rc < 0) {
        fprintf(stderr, "Unable to %sable stream: %s\n",
                enable ? "en" : "dis", strerror(errno));
        return -1;
    }

    return 0;
}

int save_memcpy_jpeg_bs(void *buf, struct video_buffer *buffers, __u32 bytesused)
{
    memcpy(buf, buffers->destination_map[0], bytesused);
    return 0;
}

int save_write_jpeg_bs(int fd, struct video_buffer *buffer, __u32 bytesused)
{
    write(fd, buffer->destination_map[0], bytesused);
    return 0;
}

struct encode *set_enc_config(struct encode_config *encode_config)
{
    struct encode *encode = (struct encode *)malloc(sizeof(struct encode));
    encode->name = encode_config->name;
    encode->width = (encode_config->width + 15) / 16 * 16;
    encode->height = encode_config->height;
    encode->pixfmt = encode_config->pixfmt;
    encode->buf_count = 1;
    encode->v4l2_buffers_count = 2;
    encode->buffers_count = 1;
    encode->planes_count = 2;

    return encode;

}

int jpeg_enc_init(struct encode_config *encode_config)
{
    struct encode *encode = NULL;
    struct setformat setformat;
    struct video_buffer *buffers = (struct video_buffer *)malloc(sizeof(struct video_buffer));

    encode = set_enc_config(encode_config);

    __u32 source_map_offsets[VIDEO_MAX_PLANES];
    __u32 destination_map_offsets[VIDEO_MAX_PLANES];
    __u32 destination_sizes[VIDEO_MAX_PLANES];
    __u32 destination_planes_count;
    __u32 export_fds_count;

    int rc;
    bool mplane = true;

    if ((encode->fd = open(encode->name, O_RDWR)) < 0) {
        fprintf(stderr, "Failed to open %s: %s\n", encode->name, strerror(errno));
        goto error;
    }

    setformat.width = encode->width;
    setformat.height = encode->height;
    setformat.pixfmt = encode->pixfmt;
    setformat.type = mplane ? V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE :
        V4L2_BUF_TYPE_VIDEO_OUTPUT;

    rc = try_format(encode->fd, &setformat);
    if (rc < 0) {
        fprintf(stderr, "try format error\n");
        goto error;
    }

    rc = video_engine_capabilities_test(encode->fd, V4L2_CAP_STREAMING);
    if (!rc) {
        fprintf(stderr, "Missing required driver streaming capability\n");
        goto error;
    }

    encode->output_type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    encode->capture_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    /* set OUTPUT format */
    setformat.type = encode->output_type;
    rc = set_format(encode->fd, &setformat);
    if (rc < 0) {
        fprintf(stderr, "Unable to set source format\n");
        goto error;
    }

    /* set CAPTUREPUT format */
    setformat.pixfmt = codec_dst_format(CODEC_TYPE_JPEG);
    setformat.type = encode->capture_type;
    rc = set_format(encode->fd, &setformat);
    if (rc < 0) {
        fprintf(stderr, "Unable to set destination format\n");
        goto error;
    }

    destination_planes_count = 1;	/*h264*/

    /* output Buffer. */
    if (reqbufs(encode->fd, encode->buf_count, encode->output_type) < 0) {
        fprintf(stderr, "reqbuf failed.\n");
        goto error;
    }

    for (int i = 0; i < encode->buf_count; i++) {

        rc = query_buffer(encode->fd, i, buffers->source_map_lengths, source_map_offsets, encode->planes_count, encode->output_type);
        if (rc < 0) {
            fprintf(stderr, "Unable to request source buffer\n");
            goto error;
        }

        for (int j = 0; j < encode->v4l2_buffers_count; j++) {
            buffers->source_map[j] = mmap(NULL,
                    buffers->source_map_lengths[j],
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    encode->fd,
                    source_map_offsets[j]);

            if (buffers->source_map[j] == MAP_FAILED) {
                fprintf(stderr, "Unable to map source buffer\n");
                goto error;
            }

            buffers->source_data[j] = buffers->source_map[j];
            buffers->source_sizes[j] = buffers->source_map_lengths[j];

        }

        buffers->source_planes_count = encode->planes_count;
        buffers->source_buffers_count = buffers->source_planes_count;
    }


    /* capture Buffer. */
    if (reqbufs(encode->fd, encode->buf_count, encode->capture_type) < 0) {
        fprintf(stderr, "Unable to create destination buffers\n");
        goto error;
    }

    for (int i = 0; i < encode->buf_count; i++) {

        rc = query_buffer(encode->fd, i, buffers->destination_map_lengths, destination_map_offsets, destination_planes_count, encode->capture_type);
        if (rc < 0) {
            fprintf(stderr, "Unable to request destination buffer\n");
            goto error;
        }

        for(int j = 0; j < destination_planes_count; j++) {
            buffers->destination_map[j] = mmap(NULL,
                    buffers->destination_map_lengths[j],
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    encode->fd,
                    destination_map_offsets[j]);
            if(buffers->destination_map[j] == MAP_FAILED) {
                fprintf(stderr,	"Unable to map destination buffer\n");
                goto error;
            }

            buffers->destination_data[j] = buffers->destination_map[j];
            buffers->destination_sizes[j] = buffers->destination_map_lengths[j];
        }

        buffers->destination_planes_count = destination_planes_count;
        buffers->destination_buffers_count = destination_planes_count;

        export_fds_count = encode->v4l2_buffers_count;

        for (int j = 0; j < export_fds_count; j++)
            buffers->export_fds[j] = -1;

#ifdef EXPORT_BUFFER
        rc = export_buffer(encode->fd, encode->capture_type, i, O_RDONLY,
                buffers->export_fds, export_fds_count);
        if (rc < 0) {
            fprintf(stderr,
                    "Unable to export destination buffer\n");
            goto error;
        }
#endif
    }

    rc = set_stream(encode->fd, encode->output_type, true);
    if (rc < 0) {
        fprintf(stderr, "Unable to enable source stream\n");
        goto error;
    }

    rc = set_stream(encode->fd, encode->capture_type, true);
    if (rc < 0) {
        fprintf(stderr, "Unable to enable destination stream\n");
        goto error;
    }

    rc = 0;

    encode->buffers = buffers;
    encode_config->encode = encode;

    goto complete;

error:
    close(encode->fd);
    return -1;
complete:
    return rc;
}

int video_engine_encode(int fd, __u32 index,
        struct video_buffer *buffers,
        struct encode *encode,void *output_buf)
{
    struct timeval tv = { 0, 300000 };
    fd_set except_fds;
    bool source_error, destination_error;
    int rc;
    int ret;


    rc = queue_buffer(fd, encode->output_type, 0, index,
            buffers->source_sizes, buffers->source_planes_count);
    if(rc < 0) {
        fprintf(stderr, "Unable to queue source buffer\n");
        return -1;
    }
    rc = queue_buffer(fd, encode->capture_type, 0, index,
            buffers->destination_sizes,
            buffers->destination_planes_count);
    if (rc < 0) {
        fprintf(stderr, "Unable to queue destination buffer\n");
        return -1;
    }

    int out_index = 0;
    FD_ZERO(&except_fds);
    FD_SET(fd,&except_fds);

    rc = select(fd + 1,&except_fds,NULL,NULL,&tv);
    if(-1 == rc)
    {
	    perror("Fail to select");
	    exit(EXIT_FAILURE);
    }
    if(0 == rc)
    {
	    fprintf(stderr,"select Timeout\n");
	    exit(EXIT_FAILURE);
    }

    rc = dequeue_buffer(fd, encode->output_type, &out_index,
            buffers->source_buffers_count, &source_error, NULL);
    if (rc < 0) {
        fprintf(stderr, "Unable to dequeue source buffer\n");
        return -1;
    }

    int cap_index = 0;
    __u32 bytesused = 0;
    rc = dequeue_buffer(fd, encode->capture_type, &cap_index,
            buffers->destination_buffers_count,
            &destination_error, &bytesused);
    if(rc < 0) {
        fprintf(stderr, "Unable to dequeue destination buffer\n");
        return -1;
    }

    save_memcpy_jpeg_bs(output_buf, buffers, bytesused);

    return bytesused;
}

int jpeg_enc_start(struct encode_config *encode_config)
{
    struct encode *encode = encode_config->encode;
    struct video_buffer *buffers = encode->buffers;

    encode->input_buf = encode_config->input_buf;
    encode->output_buf = encode_config->output_buf;
    __u32 v4l2_index = 0;
    __u32 i;
    int fd = -1;

    __u32 bytesused = 0;
    unsigned int enc_linesize = encode->width;

    unsigned char *dst = buffers->source_map[0];
    unsigned char *src = encode->input_buf;

    unsigned int copy_linesize = encode->width > encode_config->width ? encode_config->width : encode->width;

//    printf("copy_linesize: %d, encode->width: %d, encode_config->width: %d\n", copy_linesize, encode->width, encode_config->width);
    /*Y*/
    for(i = 0; i < encode_config->height; i++) {
	memcpy(dst, src, copy_linesize);

	dst += encode->width;		//codec stride.
	src += encode_config->width;	//input buffer's width
    }

    /*UV*/

    dst = buffers->source_map[1];
    src = encode->input_buf + encode_config->width * encode_config->height;
    for(i = 0; i < encode_config->height / 2; i++) {
	    memcpy(dst, src, copy_linesize);

	dst += encode->width;
	src += encode_config->width;
    }

    /*ENCODE*/
    bytesused = video_engine_encode(encode->fd, v4l2_index, buffers,
            encode, encode->output_buf);
    if(bytesused <= 0) {
        fprintf(stderr, "Unable to encode \n");
    }

    return bytesused;
}

int video_engine_stop(int fd, struct video_buffer *buffers,
        __u32 buffers_count, __u32 output_type, __u32 capture_type)
{
    __u32 i, j;
    int rc;

    rc = set_stream(fd, output_type, false);
    if (rc < 0) {
        fprintf(stderr, "Unable to enable source stream\n");
        return -1;
    }

    rc = set_stream(fd, capture_type, false);
    if (rc < 0) {
        fprintf(stderr, "Unable to enable destination stream\n");
        return -1;
    }

    for (i = 0; i < buffers_count; i++) {

        for(j = 0; j < buffers[i].source_buffers_count; j++) {
            if(buffers[i].source_map[j] == NULL)
                break;

            munmap(buffers[i].source_map[j],
                    buffers[i].source_map_lengths[j]);
        }

        for (j = 0; j < buffers[i].destination_buffers_count; j++) {
            if (buffers[i].destination_map[j] == NULL)
                break;

            munmap(buffers[i].destination_map[j],
                    buffers[i].destination_map_lengths[j]);

            if (buffers[i].export_fds[j] >= 0)
                close(buffers[i].export_fds[j]);
        }

        for (j = 0; j < buffers[i].destination_buffers_count; j++) {
            if (buffers[i].export_fds[j] < 0)
                break;

            close(buffers[i].export_fds[j]);
        }
    }
    close(fd);
    return 0;
}

int jpeg_enc_stop(struct encode_config *encode_config)
{
    struct encode *encode = encode_config->encode;
    struct video_buffer *buffers = encode->buffers;
    int rc = 0;

    rc = video_engine_stop(encode->fd, buffers, encode->buffers_count,
            encode->output_type, encode->capture_type);
    if (rc < 0) {
        fprintf(stderr, "Unable to stop video engine\n");
        close(encode->fd);
    }
    free(encode_config->encode);
    return rc;
}




