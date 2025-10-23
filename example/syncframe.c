#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <syncframe.h>

//***********************************************
//     get syncframe
//***********************************************
int main(int argc, char **argv)
{
    struct config config;

    config.width = 1280;    //1280 | 640
    config.height = 720;    //720  | 480
    config.pixfmt = V4L2_PIX_FMT_NV21;  //V4L2_PIX_FMT_NV21 | V4L2_PIX_FMT_NV12
    config.camera_r_name = "/dev/video3";//The right camera device node name
    config.camera_l_name = "/dev/video0";//The left camera device node name
    config.camera_name = NULL;//The monocular camera device node name. When you get syncframe, don't use it.
    config.bcount = 3;

    struct cameras_buf *syncframe = NULL;

    if (cameras_init(SYNCFRAME, &config) < 0) {
        fprintf(stderr, "cameras_init failed!\n");
        return 0;
    }

    //get syncframe, save it.
    char name_l[32] = "camera_l.yuv";
    char name_r[32] = "camera_r.yuv";
    FILE *camera_l_file = fopen(name_l, "w");
    FILE *camera_r_file = fopen(name_r, "w");

    for (int i = 0; i < 3; i++) {

        if ((syncframe = dqbuf_syncframe(&config)) == NULL ) {
            fprintf(stderr, "get syncframe failed\n");
            return 0;
        }

        fwrite(syncframe->Rcamera.camera_buf, config.buffer_size, 1, camera_r_file);
        fwrite(syncframe->Lcamera.camera_buf, config.buffer_size, 1, camera_l_file);

        printf("the time difference = %0.6f ms\n", syncframe->time_diff);

        qbuf_syncframe(&config, syncframe);
    }

    fclose(camera_r_file);
    fclose(camera_l_file);

    uninit_video(&config);

    return 0;
}


