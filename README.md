# sync-frames

## 简介

本工程生成两个库文件，libsyncframe.so和libv4l2enc.so。<br>
libsyncframe.so提供获取同步帧接口，支持单个摄像头数据的获取，数据源格式为NV12和NV21。<br>
libv4l2enc.so提供jpg编码接口，支持NV12和NV21格式编码成jpg格式。<br>

### 在Manhattun目录下编译
```
#make sync-frames
```

### 生成文件说明

提供库中函数测试程序，驱动双摄提取同步帧、重置camera队列缓冲区、驱动单摄获取单帧的demo。
生成test, get_syncframe, reset_syncframe, get_monoframe, osd_syncframe_client可执行文件。<br>

#### test :

对双目摄像头库中所有函数进行测试，test/test.c为示例。<br>

##### 设备要求

双目摄像头<br>

##### 输入参数
```
Parameter 1 : width 1280/640
Parameter 2 : height 720/480
Parameter 3 : format 0 : V4L2_PIX_FMT_NV21 | 1 : V4L2_PIX_FMT_NV12
Parameter 4 : bcount 3/4/5
Parameter 5 : get the number of frames (the max is bcount-1)
Parameter 6 : open two cameras or one camera 1/0
Parameter 7 : The name of one of the two cameras
Parameter 8 : The name of one of the two cameras
Parameter 9 : The name of one camera
Parameter 10 :The name of  encode device
```

##### 运行命令示例
```
#syncframe_test 640 480 0 3 1 1 /dev/video0 /dev/video3 0 /dev/video1
```
<br>

#### get_syncframe :

双目摄像头提取同步帧demo，对提取同步帧API的调用，并把帧数据写入文件，example/syncframe.c为示例。<br>

##### 设备要求

双目摄像头<br>

##### 运行命令
```
#get_syncframe
```
<br>

#### reset_syncframe :

libsyncframe.so支持提取同步帧时重置camera缓存的数据，example/reset_syncframe.c 为调用示例。<br>

##### 设备要求

双目摄像头<br>

##### 运行命令
```
#reset_syncframe
```
<br>

#### get_monoframe :

驱动单摄取帧demo，syncframe库支持驱动单个摄像头取帧操作，并把帧数据写入文件，example/monoframe.c 为调用示例。<br>

##### 设备要求

双目摄像头<br>

##### 运行命令
```
#get_monoframe
```
<br>

#### osd_syncframe_client :

获取同步帧并在LCD上显示，串口输出同步帧时间差，example/osd_syncframe_client.cpp 为调用示例。<br>

##### 设备要求

双目摄像头，LCD屏。<br>

##### 运行条件

编译osd，运行osdServer后，在adb shell中运行osd_syncframe_client，运行成功后，LCD屏显示获取到的同步帧，串口输出同步帧时间差。

minicom端
```
#osdServer
```
adb shell端
```
#osd_syncframe_client
```
<br>

## libsyncframe.so库接口说明

### 结构体

```c
struct buf_info{//单帧信息结构体
    void *camera_buf;//帧数据
    __u32 index;//帧索引
}；
    
struct cameras_buf{//同步帧信息结构体
    struct buf_info Rcamera;//右摄像头帧信息
    struct buf_info Lcamera;//左摄像头帧信息
    float time_diff;//同步帧时间差
}；
    
struct config{//用户设置
    bool video_flag;//是否取同步帧标志
    
    char *camera_r_name;//右摄像头设备节点
    char *camera_l_name;//左摄像头设备节点
    char *camera_name;//单摄时，摄像头设备节点
    
    __u32 width;//数据帧的宽
    __u32 height;//数据帧的高
    __u23 pixfmt;//数据帧的格式
    __u32 buffer_size;//一帧的大小
    __u32 bcount;//申请buffer数量
    
    void *camera;   
}
```

### 库宏

```c
#define SYNCFRAME       true
#define MONOFRAME       false
```

### 库函数

#### 摄像头初始化函数

`int cameras_init(bool video_flag, struct config *config);`

###### 参数说明：

bool video_flag : SYNCFRAME(开启双摄) 或者 MONOFRAME（开启单摄）<br>
struct config : 用户设置结构体。<br>

##### 函数功能：

camera init函数，init成功返回0，失败返回-1。<br><br>

#### 提取同步帧函数

`struct cameras_buf *dqbuf_syncframe(struct config *config);`

##### 参数说明：

struct cameras_buf : 同步帧信息结构体<br>
struct config : 用户设置结构体<br>

##### 函数功能：

提取同步帧，成功返回同步帧信息，错误返回NULL。<br><br>

#### 释放同步帧函数

`void qbuf_syncframe(struct config *config, struct cameras_buf *syncframe);`

##### 参数说明：

struct config : 用户设置结构体<br>
struct cameras_buf : 同步帧信息结构体<br>

##### 函数功能：

释放当前同步帧,把buffer重新放回输入队列。<br><br>

#### 重置队列缓存帧数据函数

`void reset_buf(struct config *config);`

##### 参数说明：

struct config : 用户设置结构体<br>

##### 函数功能：

更新队列中的buffer，保证buffer的实时性。在处理同步帧时间过长，导致下一个缓存中的同步帧失去实时性的时候使用。<br><br>

#### 获取单摄像头的帧数据

`struct buf_info *get_monoframe(struct config *config);`

##### 参数说明：

struct buf_info : 单帧buffer信息结构体。<br>
struct config : 用户设置结构体。<br>

##### 函数功能：

获取单帧摄像头的数据，成功返回单身信息结构体指针，失败返回NULL。<br><br>

#### 释放单帧函数

`void qbuf_monoframe(struct config *config, struct buf_info *monoframe);`

##### 参数说明：

struct config : 用户设置结构体<br>
struct buf_info : 单帧信息结构体<br>

##### 函数功能：

把单个摄像头的buffer放回输入队列。<br><br>

#### uninit函数

`void uninit_video(strcut config *config)`

##### 参数说明：

struct config : 用户设置结构体。<br>

##### 函数功能：

关流，关闭设备节点，取消内存映射。<br><br>

## libv4l2enc.so库接口说明

### 结构体
```c
struct encode_config {                              
    char *name;//编码设备节点
    __u32 width;//帧宽度
    __u32 height;//帧高度
    __u32 pixfmt;//帧格式
    void *input_buf;//编码输入buffer指针
    void *output_buf;//编码输出buffer指针
    void *encode;//
}; 
```

### 库函数

#### 编码初始化函数

`int jpeg_enc_init(struct encode_config *encode_config)`

##### 参数说明：

struct encode_config ：用户设置结构体<br>

##### 函数功能：

初始化编码设备节点，初始化成功返回0，失败返回-1。<br><br>


#### 编码函数

`int jpeg_enc_start(struct encode_config *encode_config)`

##### 参数说明：

struct encode_config ：用户设置结构体<br>

##### 函数功能：

对V4L2_PIX_FMT_NV21和V4L2_PIX_FMT_NV12格式编码成jpg格式。编码成功返回编码后数据大小，失败返回-1。<br><br>


#### 停止编码函数

`int jpeg_enc_stop(struct encode_config *encode_config);`

##### 参数说明：

struct encode_config ：用户设置结构体<br>

##### 函数功能：

停止编码，关闭设备节点，释放占用资源。成功返回0，失败返回-1。<br>
