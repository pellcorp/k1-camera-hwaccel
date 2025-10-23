#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <syncframe.h>
//*******************************************
//    get monoframe
//*******************************************
int main()
{
    struct config config;

    char name[32] = "monocamera.yuv";

    config.width = 1280;
    config.height = 720;
    config.pixfmt = V4L2_PIX_FMT_NV21;
    config.camera_r_name = NULL;
    config.camera_l_name = NULL;
    config.camera_name = "/dev/video0";// "/dev/video0" or "/dev/video3" The monocular camera device node name. When you get syncframe, don't use it.
    config.bcount = 3;//The = minimum buf you can request

    struct buf_info *monoframe = NULL;
    if (cameras_init(MONOFRAME, &config) < 0) {
        fprintf(stderr, "cameras_init failed!\n");
        return 0;
    }
    
    //get monoframe, save yuv.
    FILE *monocamera_file = fopen(name, "w");
    for (int i = 0; i < 3; i++) {

        while ((monoframe = dqbuf_monoframe(&config)) != NULL ) {
            printf("get monoframe failed\n");
            printf("restart get monoframe\n");
        }

        fwrite(monoframe->camera_buf, config.buffer_size, 1, monocamera_file);


        qbuf_monoframe(&config, monoframe);
    }

    fclose(monocamera_file);
    uninit_video(&config);
    
    return 0;
}
