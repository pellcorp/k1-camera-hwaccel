#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <getopt.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>

#include <syncframe.h>
#include <v4l2.h>

#define LEFT	1
#define RIGHT	0

#define DQBUF           2
#define UPDATE_BUF      3
#define RESET_BUFS      4

struct buf_info *dqbuf_monoframe(struct config *config) {
    struct camera_info *camera = (struct camera_info *)(config->camera);
    struct buf_info *monoframe = (struct buf_info*) malloc(sizeof(struct buf_info));
    if (get_frame_from_camera(&camera->camera) < 0) {
        fprintf(stderr, "update buf failed\n");

        return NULL;
    }
    monoframe->camera_buf = camera->camera.bufs[camera->camera.index];
    monoframe->index = camera->camera.index;

    return monoframe;
}

void qbuf_monoframe(struct config *config, struct buf_info *monoframe) {
    struct camera_info *camera = (struct camera_info *)(config->camera);

    qbuf(camera->camera.fd, monoframe->index);

    if (monoframe != NULL) {
        free(monoframe);
        monoframe = NULL;
    }
}

int buf_update(struct camera *camera) {
    if (get_frame_from_camera(camera) < 0) {

        return -1;
    }

    camera->count = *((__u32 *)camera->bufs[camera->index] + 1) + camera->count_offset;
    if (camera->count < camera->last_count) {
        camera->count_offset += 0xFFFFFFFF;
        camera->count = *((__u32 *)camera->bufs[camera->index] + 1) + camera->count_offset;
    }
    //	printf("camera->name = %s, camera->index = %d\n", camera->name, camera->index);
    //	printf("camera->last_count = %lld\n", camera->last_count);
    //	printf("camera->count = %lld\n", camera->count);
    //	printf("camera difference = %lld\n\n\n", camera->count - camera->last_count);
    camera->last_count = camera->count;

    return 0;
}

int return_buf(struct camera *camera) {
    if (qbuf(camera->fd, camera->index) < 0) {
        fprintf(stderr, "return buf failed\n");

        return -1;
    }

    return 0;
}

void *camera_thread(void *arg) {
    struct camera *camera = (struct camera*)arg;

    while(camera->thread_flag) {
        switch (camera->handle_buf_flag) {
            case DQBUF:
                if (buf_update(camera) < 0) {
                    camera->select_timeout_flag++;
                    if (camera->select_timeout_flag > 10) {
                        camera->thread_flag = false;
                        camera->handle_buf_flag = 0;
                        fprintf(stderr, "%s dqbuf failed\n", camera->name);
                        fprintf(stdout, "restart dqbuf\n");
                    }
                    break;
                }
                camera->handle_buf_flag = 0;
                camera->select_timeout_flag = 0;
                break;
            case UPDATE_BUF:
                if (return_buf(camera) < 0) {
                    fprintf(stderr, "%s return buf failed\n", camera->name);
                    break;
                }
                if (buf_update(camera) < 0) {
                    camera->handle_buf_flag = DQBUF;
                    break;
                }
                camera->handle_buf_flag = 0;
                camera->select_timeout_flag = 0;
                break;
            case RESET_BUFS:
                if (buf_update(camera) < 0) {
                    camera->handle_buf_flag = DQBUF;
                    break;
                }
                if (return_buf(camera) < 0) {
                    fprintf(stderr, "%s return buf failed\n", camera->name);
                    break;
                }
                camera->handle_buf_flag = 0;
                camera->select_timeout_flag = 0;
                break;
            default :
                break;
        }
    }
}

struct cameras_buf *dqbuf_syncframe(struct config *config) {
    struct camera_info *camera = (struct camera_info *)(config->camera);
    long long Difference = 0;
    int times = 0;
    bool the_new = 0;

    camera->camera_r.handle_buf_flag = DQBUF;
    camera->camera_l.handle_buf_flag = DQBUF;
    while (camera->camera_r.handle_buf_flag || camera->camera_l.handle_buf_flag);

    while (times < 100) {
        times++;

        if (!(camera->camera_r.thread_flag && camera->camera_l.thread_flag)) {
            fprintf(stderr, "dqbuf timeout, thread shutdown\n");

            return NULL;
        }

        if ((Difference = camera->camera_l.count - camera->camera_r.count) > 0) {
            the_new = LEFT;
        } else {
            the_new = RIGHT;
            Difference = llabs(Difference);
        }

        if (Difference > camera->threshold) {
            if(the_new) {
                camera->camera_r.handle_buf_flag = UPDATE_BUF;
                camera->camera_l.handle_buf_flag = 0;
                while (camera->camera_r.handle_buf_flag || camera->camera_l.handle_buf_flag) {
                    if (camera->camera_r.select_timeout_flag > 2) {
                        camera->camera_l.handle_buf_flag = UPDATE_BUF;
                        while(camera->camera_l.handle_buf_flag);
                    }
                }
            } else {
                camera->camera_l.handle_buf_flag = UPDATE_BUF;
                camera->camera_r.handle_buf_flag = 0;
                while (camera->camera_r.handle_buf_flag || camera->camera_l.handle_buf_flag) {
                    if (camera->camera_l.select_timeout_flag > 2) {
                        camera->camera_r.handle_buf_flag = UPDATE_BUF;
                        while(camera->camera_r.handle_buf_flag);
                    }
                }
            }
        } else {
            struct cameras_buf *syncframe = (struct cameras_buf*) malloc(sizeof(struct cameras_buf));
            syncframe->Rcamera.camera_buf = camera->camera_r.bufs[camera->camera_r.index];
            syncframe->Lcamera.camera_buf = camera->camera_l.bufs[camera->camera_l.index];
            syncframe->Rcamera.index = camera->camera_r.index;
            syncframe->Lcamera.index = camera->camera_l.index;
            if ((camera->camera_r.count_offset > 0xFFFFFFFF) && (camera->camera_l.count_offset > 0xFFFFFFFF)) {
                camera->camera_r.count_offset = 0;
                camera->camera_l.count_offset = 0;
            }
            syncframe->time_diff = ((float)Difference / (float)camera->count_1s) * 1000.0;

            return syncframe;
        }
    }

    return NULL;
}

void reset_buf(struct config *config){
    struct camera_info *camera = (struct camera_info *)(config->camera);

    for (int i = 0; i < config->bcount; i++) {
        camera->camera_r.handle_buf_flag = RESET_BUFS;
        camera->camera_l.handle_buf_flag = RESET_BUFS;
        while (camera->camera_r.handle_buf_flag || camera->camera_l.handle_buf_flag);
    }
}

void qbuf_syncframe(struct config *config, struct cameras_buf *syncframe) {
    struct camera_info *camera = (struct camera_info *)(config->camera);

    qbuf(camera->camera_l.fd, syncframe->Lcamera.index);
    qbuf(camera->camera_r.fd, syncframe->Rcamera.index);

    if (syncframe != NULL) {
        free(syncframe);
        syncframe = NULL;
    }
}


int camera_init(struct camera *camera, __u32 width, __u32 height, __u32 pixfmt) {
    struct v4l2_capability vcap;
    struct setformat setformat;

    memset(&setformat, 0, sizeof(setformat));
    camera->thread_flag = true;
    __u32 capabilities = 0;
    bool is_multiplanar = false;
    int length;

    if ((camera->fd = open(camera->name, O_RDWR)) < 0) {
        fprintf(stderr, "Failed to open %s: %s\n", camera->name, strerror(errno));
        return -1;
    }

    memset(&vcap, 0, sizeof(vcap));
    if (ioctl(camera->fd, VIDIOC_QUERYCAP, &vcap)) {
        fprintf(stderr, "%s: not a v4l2 node\n", camera->name);
        goto err;
    }

    capabilities = vcap.capabilities;
    if (capabilities & V4L2_CAP_DEVICE_CAPS)
        capabilities = vcap.device_caps;

    is_multiplanar = capabilities & (V4L2_CAP_VIDEO_CAPTURE_MPLANE |
            V4L2_CAP_VIDEO_M2M_MPLANE |
            V4L2_CAP_VIDEO_OUTPUT_MPLANE);
    setformat.type = is_multiplanar ? V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE :
        V4L2_BUF_TYPE_VIDEO_CAPTURE;
    setformat.width = width;
    setformat.height = height;
    setformat.pixfmt = pixfmt;
    if (set_format(camera->fd, &setformat) < 0) {
        goto err;
    }
    if ((camera->bcount = reqbufs(camera->fd, camera->bcount, setformat.type)) < 0)
        goto err;
    if ((length = do_setup_cap_buffers(camera)) < 0)
        goto err;

    return length;

err:
    close(camera->fd);
    return -1;
}

struct camera_info* set_config(struct config *config) {
    struct camera_info *camera = NULL;
    camera = (struct camera_info *)malloc(sizeof(struct camera_info));

    camera->camera_r.name = config->camera_r_name;
    camera->camera_r.bcount = config->bcount;

    camera->camera_l.name = config->camera_l_name;
    camera->camera_l.bcount = config->bcount;

    camera->camera.name = config->camera_name;
    camera->camera.bcount = config->bcount;

    camera->width = config->width;
    camera->height = config->height;
    camera->pixfmt = config->pixfmt;
    return camera;
}

__u32 get_streampram(int fd) {
    struct v4l2_streamparm parm;

    __u32 timeperframe;

    memset(&parm, 0, sizeof(parm));

    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_G_PARM, &parm) < 0) {
        fprintf(stdout, "get streamparm failed\n");
        return 0;
    }

    timeperframe = parm.parm.capture.timeperframe.denominator;

    return timeperframe;
}

__u32 get_isp_clk(char *dir, char *cmd) {
    FILE *fd;
    __u32 isp_clk;

    fd = fopen(dir, cmd);
    if (fd < 0) {
        fprintf(stderr, "get isp_clk failed\n");
        return 0;
    }
    fscanf(fd, "%d", &isp_clk);
    fclose(fd);

    return isp_clk;
}

__u32 get_count_period(char *dir, char *cmd) {
    FILE *fd;
    __u32 count_period;

    fd = fopen(dir, cmd);
    if (fd < 0) {
        fprintf(stderr, "get count_period failed\n");
        return 0;
    }
    fscanf(fd, "%d", &count_period);
    fclose(fd);

    return count_period;
}

int set_threshold(struct camera_info *camera) {
    __u32 isp_clk = 0;
    __u32 count_period = 0;
    __u32 timeperframe = 0;

    timeperframe = get_streampram(camera->camera_r.fd);
    if ((timeperframe != get_streampram(camera->camera_l.fd)) ||(timeperframe == 0)) {
        fprintf(stderr, "frame rate not match\n");
        return -1;
    }

    isp_clk = get_isp_clk("/sys/module/isp/parameters/isp_clk", "r");
    if (isp_clk == 0) {
        fprintf(stderr, "get isp_clk fault\n");
        return -1;
    }

    count_period = get_count_period("/sys/module/vic/parameters/count_period", "r");
    if (count_period == 0) {
        fprintf(stderr, "get count_period fault\n");
        return -1;
    }

    camera->count_1s = isp_clk / count_period;
    camera->threshold = (__u32)((((1000.0 / (float)timeperframe / 2.0) + 1) * (isp_clk / count_period)) / 1000);

    return 0;
}

int thread_create(struct camera_info *camera) {
    pthread_t Rcamera_id;
    pthread_t Lcamera_id;

    camera->camera_r.id = Rcamera_id;
    camera->camera_l.id = Lcamera_id;

    if (pthread_create(&camera->camera_r.id, NULL, camera_thread, &camera->camera_r) != 0) {
        fprintf(stderr, "camerar thread create failed\n");
        return -1;
    }

    if (pthread_create(&camera->camera_l.id, NULL, camera_thread, &camera->camera_l) != 0) {
        fprintf(stderr, "cameral thread create failed\n");
        return -1;
    }
    return 0;
}

void uninit(struct camera *camera, bool streamon_flag, __u32 buffer_size) {
    __u32 type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (streamon_flag) {

        if (ioctl(camera->fd, VIDIOC_STREAMOFF, &type) < 0) {
            fprintf(stderr, "%s: failed : %s\n", "video VICIOC_STREAMOFF", strerror(errno));
        }
    }
    for(int i = 0; i < camera->bcount; i++) {
        munmap(camera->bufs[i], buffer_size); 
    }
    close(camera->fd);
    camera->fd = -1;
}

int cameras_init(bool video_flag, struct config *config) {
    config->video_flag = video_flag;
    struct camera_info *camera = NULL;

    bool streamon_flag_r = false;
    bool streamon_flag_l = false;

    __u32 type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (video_flag) {
        camera = set_config(config);

        if ((camera->buffer_size = camera_init(&camera->camera_r, camera->width, camera->height, camera->pixfmt)) < 0) {
            fprintf(stderr, "%s init failed \n", camera->camera_r.name);
            return -1;
        }
        if ((camera->buffer_size = camera_init(&camera->camera_l, camera->width, camera->height, camera->pixfmt)) < 0) {
            fprintf(stderr, "%s init failed \n", camera->camera_l.name);
            uninit(&camera->camera_r, streamon_flag_r, camera->buffer_size);
            return -1;
        }

        if (set_threshold(camera) < 0) {
            fprintf(stderr, "set threshold falied\n");
            goto err;
        }

        if (ioctl(camera->camera_r.fd, VIDIOC_STREAMON, &type) < 0) {
            fprintf(stderr, "%s VIDIOC_STREAMON failed\n", camera->camera_r.name);
            goto err;
        } else {
            streamon_flag_r = true;
        }
        if (ioctl(camera->camera_l.fd, VIDIOC_STREAMON, &type) < 0) {
            fprintf(stderr,"%s VIDIOC_STREAMON failed\n", camera->camera_l.name);
            goto err;
        } else {
            streamon_flag_l = true;
        }

        if (thread_create(camera) < 0) {
            fprintf(stderr, "thread create failed\n");
            goto err;
        }

    } else {
        camera = set_config(config);

        if ((camera->buffer_size = camera_init(&camera->camera, camera->width, camera->height, camera->pixfmt)) < 0) {
            fprintf(stderr, "%s init failed \n", camera->camera.name);
            return -1;
        }
        if (ioctl(camera->camera.fd, VIDIOC_STREAMON, &type) < 0) {
            fprintf(stderr, "%s VIDIOC_STREAMON failed\n", camera->camera.name);
            uninit(&camera->camera, false, camera->buffer_size);
            return -1;
        }
    }

    config->buffer_size = camera->buffer_size;
    config->camera = camera;

    return 0;

err:
    uninit(&camera->camera_l, streamon_flag_l, camera->buffer_size);
    uninit(&camera->camera_r, streamon_flag_r, camera->buffer_size);

    return -1;
}

void uninit_video(struct config *config) {
    struct camera_info *camera = (struct camera_info *)(config->camera);
    void *ret;

    if (config->video_flag) {
        camera->camera_r.thread_flag = false;
        camera->camera_l.thread_flag = false;

        pthread_join(camera->camera_r.id, &ret);
        pthread_join(camera->camera_l.id, &ret);

        uninit(&camera->camera_r, true, camera->buffer_size);
        uninit(&camera->camera_l, true, camera->buffer_size);
    } else {
        uninit(&camera->camera, true, camera->buffer_size);
    }
    if (camera != NULL) {
        free(camera);
        camera = NULL;
    }

}

