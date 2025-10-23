#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <v4l2-jpegenc.h>
#include <syncframe.h>

int main(int argc, char **argv)
{
    int width, height, format_flag, bcount, frame_num, frame_flag, format;
    char *camera_l_name = NULL;
    char *camera_r_name = NULL;
    char *camera_name = NULL;
    char *encode_name = NULL;

    if (argc >= 11) {
        width = atoi(argv[1]);
        height = atoi(argv[2]);
        format_flag = atoi(argv[3]);
        bcount = atoi(argv[4]);
        frame_num = atoi(argv[5]);
        frame_flag = atoi(argv[6]);
        camera_l_name = argv[7];
        camera_r_name = argv[8];
        camera_name = argv[9];
        encode_name = argv[10];
    } else {
        printf("Parameter 1 : width 1280/640\n");
        printf("Parameter 2 : height 720/480\n");
        printf("Parameter 3 : format 0 : V4L2_PIX_FMT_NV21 | 1 : V4L2_PIX_FMT_NV12\n");
        printf("Parameter 4 : bcount 3/4/5\n");
        printf("Parameter 5 : get the number of frames (the max is bcount-1)\n");
        printf("Parameter 6 : open two cameras or one camera 1/0\n");
        printf("Parameter 7 : The name of one of the two cameras\n");
        printf("Parameter 8 : The name of one of the two cameras\n");
        printf("Parameter 9 : The name of one camera\n");
        printf("Parameter 10 :The name of  encode device\n");

        return 0;
    }

    switch (format_flag) {
        case 0:
            format = V4L2_PIX_FMT_NV21;
            break;
        case 1:
            format = V4L2_PIX_FMT_NV12;
            break;
        default :
            printf("format set fault\n");
            return -1;
    }

    struct config config;
    struct encode_config encode_config;

    __u32 length;
    int frame_size;
    char name[32];

    config.width = width;
    config.height = height;
    config.pixfmt = format;
    config.camera_r_name = camera_r_name;
    config.camera_l_name = camera_l_name;
    config.camera_name = camera_name;
    config.bcount = bcount;

    encode_config.width = width;
    encode_config.height = height;
    encode_config.pixfmt = format;
    encode_config.name = encode_name;


    if (frame_flag) {
        struct cameras_buf *syncframe[frame_num];
        for (int i = 0; i < frame_num; i++)
            syncframe[i] = NULL;

        if (cameras_init(SYNCFRAME, &config) < 0) {
            fprintf(stderr, "cameras_init failed!\n");
            return 0;
        }

        if (jpeg_enc_init(&encode_config) < 0) {
            jpeg_enc_stop(&encode_config);
            fprintf(stderr, "jpeg_enc_init failed\n");
        }

        void *output_buf = malloc(config.buffer_size);
        encode_config.output_buf = output_buf;

        for (__u32 i = 0; i < 10; i++) {
            printf("get syncframe times = %d\n", i);

            for (int j = 0; j < frame_num; j++) {

                if ((syncframe[j] = dqbuf_syncframe(&config)) == NULL ) {
                    fprintf(stderr, "get syncframe failed\n");
                    return 0;
                }

                encode_config.input_buf = syncframe[j]->Rcamera.camera_buf;
                frame_size = jpeg_enc_start(&encode_config);
                //		   sprintf(name, "%d%s%d", i, "videoR.jpg", j);
                //		   FILE *video0_fd = fopen(name, "w");
                //		   fwrite(output_buf, frame_size, 1, video0_fd);

                encode_config.input_buf = syncframe[j]->Lcamera.camera_buf;
                frame_size = jpeg_enc_start(&encode_config);
                //		   sprintf(name, "%d%s%d", i, "videoL.jpg", j);
                //		   FILE *video1_fd = fopen(name, "w");
                //		   fwrite(output_buf, frame_size, 1, video1_fd);

                //		   fclose(video0_fd);
                //		   fclose(video1_fd);
            }


            for (int z = 0; z < frame_num; z++)
                qbuf_syncframe(&config, syncframe[z]);

            reset_buf(&config);
        }

    } else {

        struct buf_info *camera_buf[frame_num];

        for (int i = 0; i < frame_num; i++) {
            camera_buf[i] = NULL;
        }
        if (cameras_init(MONOFRAME, &config) < 0) {
            fprintf(stderr, "cameras_init failed!\n");
            return 0;
        }

        struct camera_info *camera = (struct camera_info *)(config.camera);

        if (jpeg_enc_init(&encode_config) < 0) {
            jpeg_enc_stop(&encode_config);
            fprintf(stderr, "jpeg_enc_init failed\n");
        }

        void *output_buf = malloc(config.buffer_size);
        encode_config.output_buf = output_buf;

        for (__u32 i = 0; i < 10; i++) {
            printf("get monoframe times = %d\n", i);

            for (int j = 0; j < frame_num; j++) {

                if ((camera_buf[j] = dqbuf_monoframe(&config)) == NULL ) {
                    fprintf(stderr, "get monoframe failed\n");
                    return 0;
                }

                encode_config.input_buf = camera_buf[j]->camera_buf;
                frame_size = jpeg_enc_start(&encode_config);
                //				sprintf(name, "%d%s%d", i, "video.jpg", j);
                //				FILE *video_fd = fopen(name, "w");
                //				fwrite(output_buf, frame_size, 1, video_fd);
            }
            for (int z = 0; z < frame_num; z++) {
                qbuf_monoframe(&config, camera_buf[z]);
            }
        }

        //		fclose(video_fd);
    }

    jpeg_enc_stop(&encode_config);
    uninit_video(&config);
    printf("demo test success\n");

    return 0;
}
