/*
 * AVCodec 公共 API
 */

#ifndef AVCODEC_CODEC_H
#define AVCODEC_CODEC_H

#include <stdint.h>

#include "libavutil/avutil.h"
#include "libavutil/hwcontext.h"
#include "libavutil/log.h"
#include "libavutil/pixfmt.h"
#include "libavutil/rational.h"
#include "libavutil/samplefmt.h"

#include "libavcodec/codec_id.h"
#include "libavcodec/version_major.h"

/**
 * @addtogroup lavc_core
 * @{
 */

/**
 * 解码器可以使用 draw_horiz_band 回调。
 */
#define AV_CODEC_CAP_DRAW_HORIZ_BAND     (1 <<  0)
/**
 * 编解码器使用 get_buffer() 或 get_encode_buffer() 来分配缓冲区并支持自定义分配器。
 * 如果未设置,它可能根本不使用 get_buffer() 或 get_encode_buffer(),或者
 * 使用假定缓冲区是由 avcodec_default_get_buffer2 或 avcodec_default_get_encode_buffer 分配的操作。
 */
#define AV_CODEC_CAP_DR1                 (1 <<  1)
/**
 * 编码器或解码器需要在结束时用 NULL 输入进行刷新,以给出完整和正确的输出。
 *
 * 注意:如果未设置此标志,则保证永远不会用 NULL 数据馈送编解码器。用户仍然可以向公共编码
 * 或解码函数发送 NULL 数据,但除非设置了此标志,否则 libavcodec 不会将其传递给编解码器。
 *
 * 解码器:
 * 解码器有非零延迟,需要在结束时用 avpkt->data=NULL, avpkt->size=0 进行馈送,
 * 直到解码器不再返回帧为止,以获取延迟的数据。
 *
 * 编码器:
 * 编码器需要在编码结束时用 NULL 数据进行馈送,直到编码器不再返回数据。
 *
 * 注意:对于实现 AVCodec.encode2() 函数的编码器,设置此标志也意味着编码器必须为每个输出包
 * 设置 pts 和 duration。如果未设置此标志,pts 和 duration 将由 libavcodec 从输入帧确定。
 */
#define AV_CODEC_CAP_DELAY               (1 <<  5)
/**
 * 编解码器可以接受大小较小的最后一帧。
 * 这可用于防止最后的音频样本被截断。
 */
#define AV_CODEC_CAP_SMALL_LAST_FRAME    (1 <<  6)

#if FF_API_SUBFRAMES
/**
 * 编解码器可以在每个 AVPacket 中输出多个帧
 * 通常解复用器一次返回一帧,不这样做的解复用器会连接到解析器以将它们返回的内容分割成适当的帧。
 * 此标志保留给非常罕见的编解码器类别,它们的比特流在没有耗时操作(如完全解码)的情况下无法分割成帧。
 * 因此,携带此类比特流的解复用器可能在一个数据包中返回多个帧。这有许多缺点,比如在许多情况下禁止流复制,
 * 因此它只应作为最后的手段考虑。
 */
#define AV_CODEC_CAP_SUBFRAMES           (1 <<  8)
#endif

/**
 * 编解码器是实验性的,因此相对于非实验性编码器会被避免使用
 */
#define AV_CODEC_CAP_EXPERIMENTAL        (1 <<  9)
/**
 * 编解码器应该填充通道配置和采样率,而不是容器
 */
#define AV_CODEC_CAP_CHANNEL_CONF        (1 << 10)
/**
 * 编解码器支持帧级多线程。
 */
#define AV_CODEC_CAP_FRAME_THREADS       (1 << 12)
/**
 * 编解码器支持基于切片(或分区)的多线程。
 */
#define AV_CODEC_CAP_SLICE_THREADS       (1 << 13)
/**
 * 编解码器支持在任何时候更改参数。
 */
#define AV_CODEC_CAP_PARAM_CHANGE        (1 << 14)
/**
 * 编解码器支持通过切片级或帧级多线程以外的方法进行多线程。
 * 通常这标记了围绕支持多线程的外部库的包装器。
 */
#define AV_CODEC_CAP_OTHER_THREADS       (1 << 15)
/**
 * 音频编码器支持在每次调用中接收不同数量的样本。
 */
#define AV_CODEC_CAP_VARIABLE_FRAME_SIZE (1 << 16)
/**
 * 解码器不是探测的首选选择。
 * 这表明解码器不是探测的好选择。
 * 例如,它可能是启动成本高昂的硬件解码器,
 * 或者它可能根本不能提供关于流的很多有用信息。
 * 标记有此标志的解码器应该只作为探测的最后手段选择。
 */
#define AV_CODEC_CAP_AVOID_PROBING       (1 << 17)

/**
 * 编解码器由硬件实现支持。通常用于
 * 识别非硬件加速的硬件解码器。有关硬件加速的信息,请使用
 * avcodec_get_hw_config() 代替。
 */
#define AV_CODEC_CAP_HARDWARE            (1 << 18)

/**
 * 编解码器可能由硬件实现支持,但不
 * 一定。如果实现提供某种内部回退,则使用此标志
 * 代替 AV_CODEC_CAP_HARDWARE。
 */
#define AV_CODEC_CAP_HYBRID              (1 << 19)

/**
 * 此编码器可以重新排序来自输入 AVFrames 的用户不透明值,并将
 * 它们与相应的输出数据包一起返回。
 * @see AV_CODEC_FLAG_COPY_OPAQUE
 */
#define AV_CODEC_CAP_ENCODER_REORDERED_OPAQUE (1 << 20)

/**
 * 此编码器可以使用 avcodec_flush_buffers() 进行刷新。如果未设置此标志,
 * 必须关闭并重新打开编码器以确保没有帧
 * 保持挂起。
 */
#define AV_CODEC_CAP_ENCODER_FLUSH   (1 << 21)

/**
 * 编码器能够输出重建的帧数据,即解码编码比特流时
 * 会产生的原始帧。
 *
 * 重建帧输出由 AV_CODEC_FLAG_RECON_FRAME 标志启用。
 */
#define AV_CODEC_CAP_ENCODER_RECON_FRAME (1 << 22)

/**
 * AVProfile。
 */
typedef struct AVProfile {
    int profile;
    const char *name; ///< 配置文件的简短名称
} AVProfile;

/**
 * AVCodec。
 */
typedef struct AVCodec {
    /**
     * 编解码器实现的名称。
     * 该名称在编码器中和解码器中是全局唯一的(但编码器
     * 和解码器可以共享相同的名称)。
     * 这是从用户角度查找编解码器的主要方式。
     */
    const char *name;
    /**
     * 编解码器的描述性名称,旨在比 name 更易读。
     * 你应该使用 NULL_IF_CONFIG_SMALL() 宏来定义它。
     */
    const char *long_name;
    enum AVMediaType type;
    enum AVCodecID id;
    /**
     * 编解码器功能。
     * 参见 AV_CODEC_CAP_*
     */
    int capabilities;
    uint8_t max_lowres;                     ///< 解码器支持的最大 lowres 值
    const AVRational *supported_framerates; ///< 支持的帧率数组,如果任意则为 NULL,数组以 {0,0} 结束
    const enum AVPixelFormat *pix_fmts;     ///< 支持的像素格式数组,如果未知则为 NULL,数组以 -1 结束
    const int *supported_samplerates;       ///< 支持的音频采样率数组,如果未知则为 NULL,数组以 0 结束
    const enum AVSampleFormat *sample_fmts; ///< 支持的采样格式数组,如果未知则为 NULL,数组以 -1 结束
    const AVClass *priv_class;              ///< 私有上下文的 AVClass
    const AVProfile *profiles;              ///< 已识别的配置文件数组,如果未知则为 NULL,数组以 {AV_PROFILE_UNKNOWN} 结束

    /**
     * 编解码器实现的组名称。
     * 这是支持此编解码器的包装器的简短符号名称。包装器
     * 使用某种外部实现作为编解码器,例如
     * 外部库,或操作系统或硬件提供的编解码器实现。
     * 如果此字段为 NULL,则这是一个内置的、libavcodec 原生编解码器。
     * 如果非 NULL,在大多数情况下这将是 AVCodec.name 中的后缀
     * (通常 AVCodec.name 的形式为 "<codec_name>_<wrapper_name>")。
     */
    const char *wrapper_name;

    /**
     * 支持的通道布局数组,以零化布局结束。
     */
    const AVChannelLayout *ch_layouts;
} AVCodec;

/**
 * 遍历所有已注册的编解码器。
 *
 * @param opaque 一个指针,libavcodec 将在其中存储迭代状态。必须
 *               指向 NULL 以开始迭代。
 *
 * @return 下一个已注册的编解码器,或在迭代
 *         结束时返回 NULL
 */
const AVCodec *av_codec_iterate(void **opaque);

/**
 * 查找具有匹配编解码器 ID 的已注册解码器。
 *
 * @param id 请求的解码器的 AVCodecID
 * @return 如果找到则返回解码器,否则返回 NULL。
 */
const AVCodec *avcodec_find_decoder(enum AVCodecID id);

/**
 * 通过指定的名称查找已注册的解码器。
 *
 * @param name 请求的解码器的名称
 * @return 如果找到则返回解码器,否则返回 NULL。
 */
const AVCodec *avcodec_find_decoder_by_name(const char *name);

/**
 * 查找具有匹配编解码器 ID 的已注册编码器。
 *
 * @param id 请求的编码器的 AVCodecID
 * @return 如果找到则返回编码器,否则返回 NULL。
 */
const AVCodec *avcodec_find_encoder(enum AVCodecID id);

/**
 * 通过指定的名称查找已注册的编码器。
 *
 * @param name 请求的编码器的名称
 * @return 如果找到则返回编码器,否则返回 NULL。
 */
const AVCodec *avcodec_find_encoder_by_name(const char *name);
/**
 * @return 如果编解码器是编码器则返回非零数,否则返回零
 */
int av_codec_is_encoder(const AVCodec *codec);

/**
 * @return 如果编解码器是解码器则返回非零数,否则返回零
 */
int av_codec_is_decoder(const AVCodec *codec);

/**
 * 如果可用,返回指定配置文件的名称。
 *
 * @param codec 要搜索给定配置文件的编解码器
 * @param profile 请求名称的配置文件值
 * @return 如果找到则返回配置文件的名称,否则返回 NULL。
 */
const char *av_get_profile_name(const AVCodec *codec, int profile);

enum {
    /**
     * 编解码器通过 hw_device_ctx 接口支持此格式。
     *
     * 当选择此格式时,在调用 avcodec_open2() 之前,
     * AVCodecContext.hw_device_ctx 应该已设置为指定类型的设备。
     */
    AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX = 0x01,
    /**
     * 编解码器通过 hw_frames_ctx 接口支持此格式。
     *
     * 当为解码器选择此格式时,
     * 应在 get_format() 回调中将 AVCodecContext.hw_frames_ctx 设置为合适的帧上下文。
     * 帧上下文必须在指定类型的设备上创建。
     *
     * 当为编码器选择此格式时,
     * 在调用 avcodec_open2() 之前,应将 AVCodecContext.hw_frames_ctx 设置为将用于输入帧的上下文。
     */
    AV_CODEC_HW_CONFIG_METHOD_HW_FRAMES_CTX = 0x02,
    /**
     * 编解码器通过某些内部方法支持此格式。
     *
     * 可以选择此格式而无需任何额外配置 -
     * 不需要设备或帧上下文。
     */
    AV_CODEC_HW_CONFIG_METHOD_INTERNAL      = 0x04,
    /**
     * 编解码器通过某些特殊方法支持此格式。
     *
     * 需要额外的设置和/或函数调用。有关详细信息,请参阅
     * 特定编解码器的文档。(需要这种配置的方法已弃用,
     * 应优先使用其他方法。)
     */
    AV_CODEC_HW_CONFIG_METHOD_AD_HOC        = 0x08,
};

typedef struct AVCodecHWConfig {
    /**
     * 对于解码器,如果有合适的硬件,该解码器可能能够
     * 解码到的硬件像素格式。
     *
     * 对于编码器,编码器可能能够接受的像素格式。
     * 如果设置为 AV_PIX_FMT_NONE,则适用于编解码器支持的所有像素格式。
     */
    enum AVPixelFormat pix_fmt;
    /**
     * AV_CODEC_HW_CONFIG_METHOD_* 标志的位集,描述可以与此配置一起使用的
     * 可能的设置方法。
     */
    int methods;
    /**
     * 与配置相关的设备类型。
     *
     * 必须为 AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX 和
     * AV_CODEC_HW_CONFIG_METHOD_HW_FRAMES_CTX 设置,否则未使用。
     */
    enum AVHWDeviceType device_type;
} AVCodecHWConfig;

/**
 * 检索编解码器支持的硬件配置。
 *
 * 从零到某个最大值的索引值返回索引配置
 * 描述符;所有其他值返回 NULL。如果编解码器不支持
 * 任何硬件配置,则始终返回 NULL。
 */
const AVCodecHWConfig *avcodec_get_hw_config(const AVCodec *codec, int index);

/**
 * @}
 */

#endif /* AVCODEC_CODEC_H */