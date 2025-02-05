#ifndef AVDEVICE_AVDEVICE_H
#define AVDEVICE_AVDEVICE_H

#include "version_major.h"
#ifndef HAVE_AV_CONFIG_H
/* 当作为ffmpeg构建的一部分包含时,仅包含主版本号
 * 以避免不必要的重建。当外部包含时,保持包含
 * 完整版本信息。 */
#include "version.h"
#endif

/**
 * @file
 * @ingroup lavd
 * libavdevice API主头文件
 */

/**
 * @defgroup lavd libavdevice
 * 特殊设备复用/解复用库。
 *
 * Libavdevice是@ref libavf "libavformat"的补充库。它
 * 提供各种"特殊"的平台特定复用器和解复用器,例如用于
 * 抓取设备、音频捕获和播放等。因此,libavdevice中的
 * (解)复用器属于AVFMT_NOFILE类型(它们使用自己的I/O函数)。
 * 传递给avformat_open_input()的文件名通常不是指向实际存在
 * 的文件,而是具有一些特殊的设备特定含义 - 例如对于xcbgrab
 * 它是显示名称。
 *
 * 要使用libavdevice,只需调用avdevice_register_all()来注册所有
 * 已编译的复用器和解复用器。它们都使用标准的libavformat API。
 *
 * @{
 */

#include "libavutil/log.h"
#include "libavutil/opt.h"
#include "libavutil/dict.h"
#include "libavformat/avformat.h"

/**
 * 返回LIBAVDEVICE_VERSION_INT常量。
 */
unsigned avdevice_version(void);

/**
 * 返回libavdevice的编译时配置。
 */
const char *avdevice_configuration(void);

/**
 * 返回libavdevice的许可证。
 */
const char *avdevice_license(void);

/**
 * 初始化libavdevice并注册所有输入和输出设备。
 */
void avdevice_register_all(void);

/**
 * 音频输入设备迭代器。
 *
 * 如果d为NULL,返回第一个注册的输入音频/视频设备,
 * 如果d不为NULL,返回d之后的下一个注册的输入音频/视频设备
 * 如果d是最后一个则返回NULL。
 */
const AVInputFormat *av_input_audio_device_next(const AVInputFormat  *d);

/**
 * 视频输入设备迭代器。
 *
 * 如果d为NULL,返回第一个注册的输入音频/视频设备,
 * 如果d不为NULL,返回d之后的下一个注册的输入音频/视频设备
 * 如果d是最后一个则返回NULL。
 */
const AVInputFormat *av_input_video_device_next(const AVInputFormat  *d);

/**
 * 音频输出设备迭代器。
 *
 * 如果d为NULL,返回第一个注册的输出音频/视频设备,
 * 如果d不为NULL,返回d之后的下一个注册的输出音频/视频设备
 * 如果d是最后一个则返回NULL。
 */
const AVOutputFormat *av_output_audio_device_next(const AVOutputFormat *d);

/**
 * 视频输出设备迭代器。
 *
 * 如果d为NULL,返回第一个注册的输出音频/视频设备,
 * 如果d不为NULL,返回d之后的下一个注册的输出音频/视频设备
 * 如果d是最后一个则返回NULL。
 */
const AVOutputFormat *av_output_video_device_next(const AVOutputFormat *d);

typedef struct AVDeviceRect {
    int x;      /**< 左上角x坐标 */
    int y;      /**< 左上角y坐标 */
    int width;  /**< 宽度 */
    int height; /**< 高度 */
} AVDeviceRect;

/**
 * avdevice_app_to_dev_control_message()使用的消息类型。
 */
enum AVAppToDevMessageType {
    /**
     * 空消息。
     */
    AV_APP_TO_DEV_NONE = MKBETAG('N','O','N','E'),

    /**
     * 窗口大小改变消息。
     *
     * 每当应用程序改变设备渲染窗口的大小时,
     * 都会向设备发送消息。
     * 创建窗口后也应立即发送消息。
     *
     * data: AVDeviceRect: 新窗口大小。
     */
    AV_APP_TO_DEV_WINDOW_SIZE = MKBETAG('G','E','O','M'),

    /**
     * 重绘请求消息。
     *
     * 当窗口需要重绘时向设备发送消息。
     *
     * data: AVDeviceRect: 需要重绘的区域。
     *       NULL: 需要重绘整个区域。
     */
    AV_APP_TO_DEV_WINDOW_REPAINT = MKBETAG('R','E','P','A'),

    /**
     * 请求暂停/播放。
     *
     * 应用程序请求暂停/取消暂停播放。
     * 主要用于具有内部缓冲区的设备。
     * 默认情况下设备不会暂停。
     *
     * data: NULL
     */
    AV_APP_TO_DEV_PAUSE        = MKBETAG('P', 'A', 'U', ' '),
    AV_APP_TO_DEV_PLAY         = MKBETAG('P', 'L', 'A', 'Y'),
    AV_APP_TO_DEV_TOGGLE_PAUSE = MKBETAG('P', 'A', 'U', 'T'),

    /**
     * 音量控制消息。
     *
     * 设置音量级别。根据设备的不同,音量可能是按流
     * 或系统范围更改。如果可能,期望按流更改音量。
     *
     * data: double: 新音量范围为0.0 - 1.0。
     */
    AV_APP_TO_DEV_SET_VOLUME = MKBETAG('S', 'V', 'O', 'L'),

    /**
     * 静音控制消息。
     *
     * 更改静音状态。根据设备的不同,静音状态可能是按流
     * 或系统范围更改。如果可能,期望按流更改静音状态。
     *
     * data: NULL。
     */
    AV_APP_TO_DEV_MUTE        = MKBETAG(' ', 'M', 'U', 'T'),
    AV_APP_TO_DEV_UNMUTE      = MKBETAG('U', 'M', 'U', 'T'),
    AV_APP_TO_DEV_TOGGLE_MUTE = MKBETAG('T', 'M', 'U', 'T'),

    /**
     * 获取音量/静音消息。
     *
     * 强制设备分别发送AV_DEV_TO_APP_VOLUME_LEVEL_CHANGED或
     * AV_DEV_TO_APP_MUTE_STATE_CHANGED命令。
     *
     * data: NULL。
     */
    AV_APP_TO_DEV_GET_VOLUME = MKBETAG('G', 'V', 'O', 'L'),
    AV_APP_TO_DEV_GET_MUTE   = MKBETAG('G', 'M', 'U', 'T'),
};

/**
 * avdevice_dev_to_app_control_message()使用的消息类型。
 */
enum AVDevToAppMessageType {
    /**
     * 空消息。
     */
    AV_DEV_TO_APP_NONE = MKBETAG('N','O','N','E'),

    /**
     * 创建窗口缓冲区消息。
     *
     * 设备请求创建窗口缓冲区。具体含义取决于设备
     * 和应用程序。在渲染第一帧之前发送消息,所有一次性
     * 初始化都应在此处完成。
     * 应用程序可以忽略首选的窗口缓冲区大小。
     *
     * @note: 应用程序必须通过AV_APP_TO_DEV_WINDOW_SIZE消息
     *        通知窗口缓冲区大小。
     *
     * data: AVDeviceRect: 窗口缓冲区的首选大小。
     *       NULL: 窗口缓冲区没有首选大小。
     */
    AV_DEV_TO_APP_CREATE_WINDOW_BUFFER = MKBETAG('B','C','R','E'),

    /**
     * 准备窗口缓冲区消息。
     *
     * 设备请求准备用于渲染的窗口缓冲区。
     * 具体含义取决于设备和应用程序。
     * 在渲染每一帧之前发送消息。
     *
     * data: NULL。
     */
    AV_DEV_TO_APP_PREPARE_WINDOW_BUFFER = MKBETAG('B','P','R','E'),

    /**
     * 显示窗口缓冲区消息。
     *
     * 设备请求显示窗口缓冲区。
     * 当新帧准备好显示时发送消息。
     * 通常需要在此消息的处理程序中交换缓冲区。
     *
     * data: NULL。
     */
    AV_DEV_TO_APP_DISPLAY_WINDOW_BUFFER = MKBETAG('B','D','I','S'),

    /**
     * 销毁窗口缓冲区消息。
     *
     * 设备请求销毁窗口缓冲区。
     * 当设备即将被销毁且不再需要窗口缓冲区时
     * 发送消息。
     *
     * data: NULL。
     */
    AV_DEV_TO_APP_DESTROY_WINDOW_BUFFER = MKBETAG('B','D','E','S'),

    /**
     * 缓冲区满状态消息。
     *
     * 设备发出缓冲区溢出/下溢信号。
     *
     * data: NULL。
     */
    AV_DEV_TO_APP_BUFFER_OVERFLOW = MKBETAG('B','O','F','L'),
    AV_DEV_TO_APP_BUFFER_UNDERFLOW = MKBETAG('B','U','F','L'),

    /**
     * 缓冲区可读/可写。
     *
     * 设备通知缓冲区可读/可写。
     * 如果可能,设备会通知可以读/写多少字节。
     *
     * @warning 当可读/写字节数发生变化时,设备可能不会通知。
     *
     * data: int64_t: 可读/写的字节数。
     *       NULL: 可读/写的字节数未知。
     */
    AV_DEV_TO_APP_BUFFER_READABLE = MKBETAG('B','R','D',' '),
    AV_DEV_TO_APP_BUFFER_WRITABLE = MKBETAG('B','W','R',' '),

    /**
     * 静音状态改变消息。
     *
     * 设备通知静音状态已改变。
     *
     * data: int: 0表示非静音状态,非零表示静音状态。
     */
    AV_DEV_TO_APP_MUTE_STATE_CHANGED = MKBETAG('C','M','U','T'),

    /**
     * 音量级别改变消息。
     *
     * 设备通知音量级别已改变。
     *
     * data: double: 新音量范围为0.0 - 1.0。
     */
    AV_DEV_TO_APP_VOLUME_LEVEL_CHANGED = MKBETAG('C','V','O','L'),
};

/**
 * 从应用程序向设备发送控制消息。
 *
 * @param s         设备上下文。
 * @param type      消息类型。
 * @param data      消息数据。具体类型取决于消息类型。
 * @param data_size 消息数据大小。
 * @return >= 0表示成功,负值表示错误。
 *         AVERROR(ENOSYS)表示设备未实现消息处理程序。
 */
int avdevice_app_to_dev_control_message(struct AVFormatContext *s,
                                        enum AVAppToDevMessageType type,
                                        void *data, size_t data_size);

/**
 * 从设备向应用程序发送控制消息。
 *
 * @param s         设备上下文。
 * @param type      消息类型。
 * @param data      消息数据。可以为NULL。
 * @param data_size 消息数据大小。
 * @return >= 0表示成功,负值表示错误。
 *         AVERROR(ENOSYS)表示应用程序未实现消息处理程序。
 */
int avdevice_dev_to_app_control_message(struct AVFormatContext *s,
                                        enum AVDevToAppMessageType type,
                                        void *data, size_t data_size);

/**
 * 描述设备基本参数的结构。
 */
typedef struct AVDeviceInfo {
    char *device_name;                   /**< 设备名称,格式取决于设备 */
    char *device_description;            /**< 人类友好的名称 */
    enum AVMediaType *media_types;       /**< 数组指示设备可以提供哪些媒体类型(如果有)。如果为null,则不能提供任何类型 */
    int nb_media_types;                  /**< media_types数组的长度,如果设备不能提供任何媒体类型则为0 */
} AVDeviceInfo;

/**
 * 设备列表。
 */
typedef struct AVDeviceInfoList {
    AVDeviceInfo **devices;              /**< 自动检测设备列表 */
    int nb_devices;                      /**< 自动检测设备数量 */
    int default_device;                  /**< 默认设备的索引,如果没有默认设备则为-1 */
} AVDeviceInfoList;

/**
 * 列出设备。
 *
 * 返回可用设备名称及其参数。
 *
 * @note: 某些设备可能接受无法自动检测的系统相关设备名称。
 *        不能假定此函数返回的列表始终是完整的。
 *
 * @param s                设备上下文。
 * @param[out] device_list 自动检测设备列表。
 * @return 自动检测设备数量,负值表示错误。
 */
int avdevice_list_devices(struct AVFormatContext *s, AVDeviceInfoList **device_list);

/**
 * 释放avdevice_list_devices()结果的便捷函数。
 *
 * @param device_list 要释放的设备列表。
 */
void avdevice_free_list_devices(AVDeviceInfoList **device_list);

/**
 * 列出设备。
 *
 * 返回可用设备名称及其参数。
 * 这些是avdevice_list_devices()的便捷包装器。
 * 设备上下文在内部分配和释放。
 *
 * @param device           设备格式。如果设置了设备名称,可以为NULL。
 * @param device_name      设备名称。如果设置了设备格式,可以为NULL。
 * @param device_options   填充了设备私有选项的AVDictionary。可以为NULL。
 *                         对于输出设备,相同的选项必须稍后传递给avformat_write_header(),
 *                         对于输入设备,必须传递给avformat_open_input(),或在任何其他
 *                         影响设备私有选项的地方传递。
 * @param[out] device_list 自动检测设备列表
 * @return 自动检测设备数量,负值表示错误。
 * @note 当两者都设置时,device参数优先于device_name。
 */
int avdevice_list_input_sources(const AVInputFormat *device, const char *device_name,
                                AVDictionary *device_options, AVDeviceInfoList **device_list);
int avdevice_list_output_sinks(const AVOutputFormat *device, const char *device_name,
                               AVDictionary *device_options, AVDeviceInfoList **device_list);

/**
 * @}
 */

#endif /* AVDEVICE_AVDEVICE_H */
