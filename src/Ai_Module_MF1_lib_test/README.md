# 深圳矽速科技`MF1`模块二次开发SDK文档

### 如果有任何的需求或者BUG,请发邮件至**support@sipeed.com**


## 工程目录说明

| | |
|-|-|
|.  |   |
|├── board.c            | 板子外设的初始化等  |
|├── board.h||
|├── build  | 编译目录  |
|├── driver |  板子外设驱动目录,包含lcd,flash,sd_card等 |
|├── face_lib   | 人脸识别库,以及一些回调操作  |
|├── lib    | 一些使用到的第三方库  |
|├── main.c |  主函数 |
|├── network    |esp8285网络,目前暂未支持|
|├── project.cmake  |   |
|├── README.md  |   |
|├── system_config.h    | 系统配置  |
|├── uart_recv  |  串口协议接受处理 |
|└── ui |  ui操作 |

## 人脸识别库API说明

### 人脸识别API
```C
uint8_t face_lib_init_module(void);
```
初始化人脸识别库,返回值为0的时候模块才可正常使用

```C
uint8_t face_lib_regisiter_callback(face_lib_callback_t *cfg_cb);
```
注册人脸识别过程中一些事件回调函数,请参考例程中已经实现的函数

```C
void face_lib_run(face_recognition_cfg_t *cfg);
```
进行人脸识别,需要摄像头是正常的,用户可以设置一些配置信息

### 串口协议库API

```C
uint8_t protocol_send_init_done(void);
```
模块初始化完成

```C
void protocol_prase(unsigned char *protocol_buf);
```
进行协议数据的解析,目前仅支持内置一些的指令,之后用户可添加自定义指令

```C
void protocol_cal_pic_fea(uint8_t *jpeg_buf, uint32_t jpeg_len);
```
接收完图片之后进行图片特征值的计算

```C
uint8_t protocol_send_cal_pic_result(uint8_t code, char *msg, float feature[FEATURE_DIMENSION], uint8_t *uid, float prob);
```
发送计算图片特征值结果

```C
uint8_t protocol_send_face_info(face_obj_t *obj,
                                float score, uint8_t uid[UID_LEN], float feature[FEATURE_DIMENSION],
                                uint32_t total, uint32_t current, uint64_t *time);
```
识别到人脸之后,发送人脸信息,具体的使用方法请参考例程

### FLASH读写API

```C
w25qxx_status_t w25qxx_init(uint8_t spi_index, uint8_t spi_ss, uint32_t rate);
w25qxx_status_t w25qxx_read_id(uint8_t *manuf_id, uint8_t *device_id);
w25qxx_status_t w25qxx_write_data(uint32_t addr, uint8_t *data_buf, uint32_t length);
w25qxx_status_t w25qxx_read_data(uint32_t addr, uint8_t *data_buf, uint32_t length);
w25qxx_status_t my_w25qxx_read_data(uint32_t addr, uint8_t *data_buf, uint32_t length, w25qxx_read_t mode);
```

对板载的16MByte FLAHS的读写API

### CAMERA操作API

```C
int dvp_irq(void *ctx);
```
DVP中断回调,在Board.c中进行注册

```C
int gc0328_init(uint16_t time); //sleep time in ms
```
GC0328初始化,在Board.c中调用
  
time表示初始化的时候延时的函数,之后会去掉