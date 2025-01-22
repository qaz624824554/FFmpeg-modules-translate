#ifndef AVCODEC_AVCODEC_H
#define AVCODEC_AVCODEC_H

/**
 * @file
 * @ingroup libavc
 * Libavcodec外部API头文件
 */

#include "libavutil/samplefmt.h"
#include "libavutil/attributes.h"
#include "libavutil/avutil.h"
#include "libavutil/buffer.h"
#include "libavutil/channel_layout.h"
#include "libavutil/dict.h"
#include "libavutil/frame.h"
#include "libavutil/log.h"
#include "libavutil/pixfmt.h"
#include "libavutil/rational.h"

#include "codec.h"
#include "codec_id.h"
#include "defs.h"
#include "packet.h"
#include "version_major.h"
#ifndef HAVE_AV_CONFIG_H
/* 作为ffmpeg构建的一部分时,仅包含主版本号以避免不必要的重建。
 * 当外部包含时,保持包含完整版本信息。 */
#include "version.h"

#include "codec_desc.h"
#include "codec_par.h"
#endif

struct AVCodecParameters;

/**
 * @defgroup libavc libavcodec
 * 编码/解码库
 *
 * @{
 *
 * @defgroup lavc_decoding 解码
 * @{
 * @}
 *
 * @defgroup lavc_encoding 编码
 * @{
 * @}
 *
 * @defgroup lavc_codec 编解码器
 * @{
 * @defgroup lavc_codec_native 原生编解码器
 * @{
 * @}
 * @defgroup lavc_codec_wrappers 外部库封装
 * @{
 * @}
 * @defgroup lavc_codec_hwaccel 硬件加速桥接
 * @{
 * @}
 * @}
 * @defgroup lavc_internal 内部
 * @{
 * @}
 * @}
 */

/**
 * @ingroup libavc
 * @defgroup lavc_encdec 发送/接收编码和解码API概述
 * @{
 *
 * avcodec_send_packet()/avcodec_receive_frame()/avcodec_send_frame()/
 * avcodec_receive_packet()函数提供了一个解耦输入和输出的编码/解码API。
 *
 * 这个API在编码/解码和音频/视频方面非常相似,工作方式如下:
 * - 像往常一样设置并打开AVCodecContext。
 * - 发送有效输入:
 *   - 对于解码,调用avcodec_send_packet()向解码器提供AVPacket中的原始压缩数据。
 *   - 对于编码,调用avcodec_send_frame()向编码器提供包含未压缩音频或视频的AVFrame。
 *
 *   在这两种情况下,建议AVPackets和AVFrames使用引用计数,否则libavcodec可能必须复制输入数据。
 *   (libavformat总是返回引用计数的AVPackets,av_frame_get_buffer()分配引用计数的AVFrames。)
 * - 在循环中接收输出。定期调用avcodec_receive_*()函数之一并处理其输出:
 *   - 对于解码,调用avcodec_receive_frame()。成功时,它将返回包含未压缩音频或视频数据的AVFrame。
 *   - 对于编码,调用avcodec_receive_packet()。成功时,它将返回包含压缩帧的AVPacket。
 *
 *   重复此调用直到返回AVERROR(EAGAIN)或错误。AVERROR(EAGAIN)返回值意味着需要新的输入数据才能
 *   返回新的输出。在这种情况下,继续发送输入。对于每个输入帧/数据包,编解码器通常会返回1个输出
 *   帧/数据包,但也可能是0个或多于1个。
 *
 * 在解码或编码开始时,编解码器可能会接受多个输入帧/数据包而不返回帧,直到其内部缓冲区被填满。
 * 如果您遵循上述步骤,这种情况将被透明处理。
 *
 * 理论上,发送输入可能会导致EAGAIN - 这应该只在未接收所有输出时发生。您可以使用此方法构建
 * 除上述建议之外的其他解码或编码循环。例如,您可以尝试在每次迭代时发送新输入,如果返回EAGAIN
 * 则尝试接收输出。
 *
 * 流结束情况。这需要"刷新"(又称排空)编解码器,因为编解码器可能会出于性能或必要性(考虑B帧)
 * 在内部缓冲多个帧或数据包。处理方法如下:
 * - 不发送有效输入,而是向avcodec_send_packet()(解码)或avcodec_send_frame()(编码)
 *   函数发送NULL。这将进入排空模式。
 * - 在循环中调用avcodec_receive_frame()(解码)或avcodec_receive_packet()(编码),
 *   直到返回AVERROR_EOF。除非您忘记进入排空模式,否则这些函数不会返回AVERROR(EAGAIN)。
 * - 在恢复解码之前,必须使用avcodec_flush_buffers()重置编解码器。
 *
 * 强烈建议按上述方式使用API。但也可以在这个严格的模式之外调用函数。例如,您可以
 * 重复调用avcodec_send_packet()而不调用avcodec_receive_frame()。在这种情况下,
 * avcodec_send_packet()将成功,直到编解码器的内部缓冲区被填满(通常每个输出帧的大小为1,
 * 在初始输入之后),然后用AVERROR(EAGAIN)拒绝输入。一旦它开始拒绝输入,您别无选择,
 * 只能读取至少一些输出。
 *
 * 并非所有编解码器都会遵循严格和可预测的数据流;唯一的保证是,在一端的发送/接收调用
 * 返回AVERROR(EAGAIN)意味着在另一端的接收/发送调用将成功,或至少不会因AVERROR(EAGAIN)
 * 而失败。一般来说,没有编解码器会允许无限缓冲输入或输出。
 *
 * 编解码器不允许对发送和接收都返回AVERROR(EAGAIN)。这将是一个无效状态,可能会使编解码器
 * 用户陷入无限循环。API也没有时间的概念:不可能发生尝试执行avcodec_send_packet()
 * 导致AVERROR(EAGAIN),但1秒后重复调用接受数据包的情况(不涉及其他接收/刷新API调用)。
 * API是一个严格的状态机,时间的流逝不应该影响它。在某些情况下,一些依赖时间的行为可能
 * 仍被认为是可接受的。但它绝不能导致发送/接收在任何时候同时返回EAGAIN。还必须绝对
 * 避免当前状态"不稳定"并且可以在发送/接收API之间"来回摆动"允许进展。例如,不允许
 * 编解码器在刚刚在avcodec_send_packet()调用上返回AVERROR(EAGAIN)之后,随机决定它
 * 现在实际上想要消耗一个数据包而不是返回一帧。
 * @}
 */

/**
 * @defgroup lavc_core 核心函数/结构
 * @ingroup libavc
 *
 * 基本定义、查询libavcodec功能的函数、分配核心结构等。
 * @{
 */

/**
 * @ingroup lavc_encoding
 * 最小编码缓冲区大小
 * 用于在写入头部时避免一些检查。
 */
#define AV_INPUT_BUFFER_MIN_SIZE 16384

/**
 * @ingroup lavc_encoding
 */
typedef struct RcOverride{
    int start_frame;
    int end_frame;
    int qscale; // 如果这是0,则将使用quality_factor。
    float quality_factor;
} RcOverride;

/* 编码支持
   这些标志可以在初始化前在AVCodecContext.flags中传递。
   注意:目前还不是所有功能都支持。
*/

/**
 * 允许解码器生成数据平面不对齐CPU要求的帧
 * (例如由于裁剪)。
 */
#define AV_CODEC_FLAG_UNALIGNED       (1 <<  0)
/**
 * 使用固定qscale。
 */
#define AV_CODEC_FLAG_QSCALE          (1 <<  1)
/**
 * 允许每个宏块使用4个运动矢量 / H.263的高级预测。
 */
#define AV_CODEC_FLAG_4MV             (1 <<  2)
/**
 * 输出那些可能已损坏的帧。
 */
#define AV_CODEC_FLAG_OUTPUT_CORRUPT  (1 <<  3)
/**
 * 使用qpel运动补偿。
 */
#define AV_CODEC_FLAG_QPEL            (1 <<  4)
#if FF_API_DROPCHANGED
/**
 * 不输出参数与流中第一个解码帧不同的帧。
 *
 * @deprecated 调用者应在自己的代码中实现此功能
 */
#define AV_CODEC_FLAG_DROPCHANGED     (1 <<  5)
#endif
/**
 * 请求编码器输出重建帧，即通过解码编码后的比特流可以产生的帧。
 * 这些帧可以在成功调用avcodec_receive_packet()后立即通过调用
 * avcodec_receive_frame()获取。
 *
 * 仅应用于具有 @ref AV_CODEC_CAP_ENCODER_RECON_FRAME 能力的编码器。
 *
 * @note
 * 编码器返回的每个重建帧对应于最后编码的数据包，即帧按编码顺序而不是显示顺序返回。
 *
 * @note
 * 帧参数(如像素格式或尺寸)不必与AVCodecContext的值匹配。
 * 确保使用返回帧中的值。
 */
#define AV_CODEC_FLAG_RECON_FRAME     (1 <<  6)
/**
 * @par 解码时:
 * 请求解码器将每个数据包的AVPacket.opaque和AVPacket.opaque_ref传播到其对应的输出AVFrame。
 *
 * @par 编码时:
 * 请求编码器将每个帧的AVFrame.opaque和AVFrame.opaque_ref值传播到其对应的输出AVPacket。
 *
 * @par
 * 仅可在具有 @ref AV_CODEC_CAP_ENCODER_REORDERED_OPAQUE 能力标志的编码器上设置。
 *
 * @note
 * 虽然在典型情况下一个输入帧恰好产生一个输出数据包(可能有延迟)，但通常帧到数据包的映射是M对N的，所以:
 * - 任意数量的输入帧可能与任何给定的输出数据包相关联。
 *   这包括零 - 例如某些编码器可能输出仅包含整个流元数据的数据包。
 * - 给定的输入帧可能与任意数量的输出数据包相关联。
 *   这也包括零 - 例如某些编码器可能在某些条件下丢弃帧。
 * .
 * 这意味着使用此标志时，调用者不能假设:
 * - 给定输入帧的opaque值一定会出现在某个输出数据包中;
 * - 每个输出数据包都会有一些非NULL的opaque值。
 * .
 * 当输出数据包包含多个帧时，opaque值将取自这些帧中的第一个。
 *
 * @note
 * 对于解码器来说情况相反，帧和数据包互换。
 */
#define AV_CODEC_FLAG_COPY_OPAQUE     (1 <<  7)
/**
 * 向编码器发出信号，表明AVFrame.duration的值是有效的，应该使用
 * (通常用于将其传输到输出数据包)。
 *
 * 如果未设置此标志，则忽略帧持续时间。
 */
#define AV_CODEC_FLAG_FRAME_DURATION  (1 <<  8)
/**
 * 在第一遍模式下使用内部2遍码率控制。
 */
#define AV_CODEC_FLAG_PASS1           (1 <<  9)
/**
 * 在第二遍模式下使用内部2遍码率控制。
 */
#define AV_CODEC_FLAG_PASS2           (1 << 10)
/**
 * 环路滤波。
 */
#define AV_CODEC_FLAG_LOOP_FILTER     (1 << 11)
/**
 * 仅解码/编码灰度。
 */
#define AV_CODEC_FLAG_GRAY            (1 << 13)
/**
 * 在编码期间将设置error[?]变量。
 */
#define AV_CODEC_FLAG_PSNR            (1 << 15)
/**
 * 使用隔行DCT。
 */
#define AV_CODEC_FLAG_INTERLACED_DCT  (1 << 18)
/**
 * 强制低延迟。
 */
#define AV_CODEC_FLAG_LOW_DELAY       (1 << 19)
/**
 * 将全局头部放在extradata中而不是每个关键帧中。
 */
#define AV_CODEC_FLAG_GLOBAL_HEADER   (1 << 22)
/**
 * 仅使用位精确的内容(除了(I)DCT)。
 */
#define AV_CODEC_FLAG_BITEXACT        (1 << 23)
/* Fx : H.263+额外选项的标志 */
/**
 * H.263高级帧内编码 / MPEG-4 AC预测
 */
#define AV_CODEC_FLAG_AC_PRED         (1 << 24)
/**
 * 隔行运动估计
 */
#define AV_CODEC_FLAG_INTERLACED_ME   (1 << 29)
#define AV_CODEC_FLAG_CLOSED_GOP      (1U << 31)

/**
 * 允许不符合规范的加速技巧。
 */
#define AV_CODEC_FLAG2_FAST           (1 <<  0)
/**
 * 跳过比特流编码。
 */
#define AV_CODEC_FLAG2_NO_OUTPUT      (1 <<  2)
/**
 * 在每个关键帧而不是extradata中放置全局头部。
 */
#define AV_CODEC_FLAG2_LOCAL_HEADER   (1 <<  3)

/**
 * 输入比特流可能在数据包边界而不是仅在帧边界处被截断。
 */
#define AV_CODEC_FLAG2_CHUNKS         (1 << 15)
/**
 * 忽略来自SPS的裁剪信息。
 */
#define AV_CODEC_FLAG2_IGNORE_CROP    (1 << 16)

/**
 * 显示第一个关键帧之前的所有帧
 */
#define AV_CODEC_FLAG2_SHOW_ALL       (1 << 22)
/**
 * 通过帧侧数据导出运动矢量
 */
#define AV_CODEC_FLAG2_EXPORT_MVS     (1 << 28)
/**
 * 不跳过样本并将跳过信息作为帧侧数据导出
 */
#define AV_CODEC_FLAG2_SKIP_MANUAL    (1 << 29)
/**
 * 刷新时不重置ASS ReadOrder字段(字幕解码)
 */
#define AV_CODEC_FLAG2_RO_FLUSH_NOOP  (1 << 30)
/**
 * 在编码/解码时生成/解析ICC配置文件，根据文件类型适当处理。
 * 对不能包含嵌入ICC配置文件的编解码器，或在没有lcms2支持的情况下编译时无效。
 */
#define AV_CODEC_FLAG2_ICC_PROFILES   (1U << 31)

/* 导出的侧数据。
   这些标志可以在初始化前在AVCodecContext.export_side_data中传递。
*/
/**
 * 通过帧侧数据导出运动矢量
 */
#define AV_CODEC_EXPORT_DATA_MVS         (1 << 0)
/**
 * 通过数据包侧数据导出编码器生产者参考时间
 */
#define AV_CODEC_EXPORT_DATA_PRFT        (1 << 1)
/**
 * 仅解码。
 * 通过帧侧数据导出AVVideoEncParams结构。
 */
#define AV_CODEC_EXPORT_DATA_VIDEO_ENC_PARAMS (1 << 2)
/**
 * 仅解码。
 * 不应用胶片颗粒，而是导出它。
 */
#define AV_CODEC_EXPORT_DATA_FILM_GRAIN (1 << 3)

/**
 * 解码器将保持对帧的引用，并可能稍后重用它。
 */
#define AV_GET_BUFFER_FLAG_REF (1 << 0)

/**
 * 编码器将保持对数据包的引用，并可能稍后重用它。
 */
#define AV_GET_ENCODE_BUFFER_FLAG_REF (1 << 0)

/**
 * 主要外部API结构。
 * 可以通过次要版本升级在末尾添加新字段。
 * 删除、重新排序和更改现有字段需要主要版本升级。
 * 你可以使用AVOptions (av_opt* / av_set/get*())从用户应用程序访问这些字段。
 * AVOptions选项的名称字符串与相关的命令行参数名称匹配，
 * 可以在libavcodec/options_table.h中找到。
 * 由于历史原因或简洁性，AVOption/命令行参数名称在某些情况下
 * 与C结构字段名称不同。
 * sizeof(AVCodecContext)不得在libav*外部使用。
 */
typedef struct AVCodecContext {
    /**
     * 用于av_log的结构信息
     * - 由avcodec_alloc_context3设置
     */
    const AVClass *av_class;
    int log_level_offset;

    enum AVMediaType codec_type; /* 参见 AVMEDIA_TYPE_xxx */
    const struct AVCodec  *codec;
    enum AVCodecID     codec_id; /* 参见 AV_CODEC_ID_xxx */

    /**
     * fourcc (LSB优先,所以"ABCD" -> ('D'<<24) + ('C'<<16) + ('B'<<8) + 'A')。
     * 这用于解决一些编码器的bug。
     * 解复用器应该将其设置为用于标识编解码器的字段中存储的值。
     * 如果容器中有多个这样的字段,解复用器应该选择能最大程度表示所用编解码器信息的字段。
     * 如果容器中的编解码器标签字段大于32位,解复用器应该通过表格或其他结构将较长的ID重新映射为32位。
     * 或者可以添加新的extra_codec_tag + size,但必须首先证明这样做有明显优势。
     * - 编码: 由用户设置,如果未设置则使用基于codec_id的默认值。
     * - 解码: 由用户设置,在初始化期间libavcodec会将其转换为大写。
     */
    unsigned int codec_tag;

    void *priv_data;

    /**
     * 用于内部数据的私有上下文。
     *
     * 与priv_data不同,这不是特定于编解码器的。它用于通用的
     * libavcodec函数。
     */
    struct AVCodecInternal *internal;

    /**
     * 用户的私有数据,可用于携带应用程序特定的内容。
     * - 编码: 由用户设置。
     * - 解码: 由用户设置。
     */
    void *opaque;

    /**
     * 平均比特率
     * - 编码: 由用户设置;对于固定量化器编码未使用。
     * - 解码: 由用户设置,如果流中有此信息,可能会被libavcodec覆盖
     */
    int64_t bit_rate;

    /**
     * 允许比特流偏离参考值的位数。
     *           参考可以是CBR(对于CBR pass1)或VBR(对于pass2)
     * - 编码: 由用户设置;对于固定量化器编码未使用。
     * - 解码: 未使用
     */
    int bit_rate_tolerance;

    /**
     * 无法按帧更改的编解码器的全局质量。
     * 这应该与MPEG-1/2/4 qscale成比例。
     * - 编码: 由用户设置。
     * - 解码: 未使用
     */
    int global_quality;

    /**
     * - 编码: 由用户设置。
     * - 解码: 未使用
     */
    int compression_level;
#define FF_COMPRESSION_DEFAULT -1

    /**
     * AV_CODEC_FLAG_*。
     * - 编码: 由用户设置。
     * - 解码: 由用户设置。
     */
    int flags;

    /**
     * AV_CODEC_FLAG2_*
     * - 编码: 由用户设置。
     * - 解码: 由用户设置。
     */
    int flags2;

    /**
     * 一些编解码器需要/可以使用额外数据,如霍夫曼表。
     * MJPEG: 霍夫曼表
     * rv10: 附加标志
     * MPEG-4: 全局头(它们可以在比特流中或这里)
     * 分配的内存应该比extradata_size大AV_INPUT_BUFFER_PADDING_SIZE字节,
     * 以避免使用比特流阅读器读取时出现问题。
     * extradata的字节内容不得依赖于架构或CPU字节序。
     * 必须使用av_malloc()系列函数分配。
     * - 编码: 由libavcodec设置/分配/释放。
     * - 解码: 由用户设置/分配/释放。
     */
    uint8_t *extradata;
    int extradata_size;

    /**
     * 这是表示帧时间戳的基本时间单位(以秒为单位)。对于固定帧率内容,
     * timebase应该是1/帧率,时间戳增量应该恰好为1。
     * 这通常(但不总是)是视频帧率或场率的倒数。如果帧率不恒定,
     * 1/time_base不是平均帧率。
     *
     * 像容器一样,基本流也可以存储时间戳,1/time_base
     * 是指定这些时间戳的单位。
     * 这种编解码器时基的例子见ISO/IEC 14496-2:2001(E)
     * vop_time_increment_resolution和fixed_vop_rate
     * (fixed_vop_rate == 0意味着它与帧率不同)
     *
     * - 编码: 必须由用户设置。
     * - 解码: 未使用。
     */
    AVRational time_base;

#if FF_API_TICKS_PER_FRAME
    /**
     * 对于某些编解码器,时基更接近场率而不是帧率。
     * 最明显的是,如果不使用电视电影,H.264和MPEG-2将time_base指定为帧持续时间的一半...
     *
     * 设置为每帧的time_base刻度。默认1,例如,H.264/MPEG-2将其设为2。
     *
     * @已弃用
     * - 解码: 使用AVCodecDescriptor.props & AV_CODEC_PROP_FIELDS
     * - 编码: 改为设置AVCodecContext.framerate
     *
     */
    attribute_deprecated
    int ticks_per_frame;
#endif

    /**
     * 编解码器延迟。
     *
     * 编码: 从编码器输入到解码器输出将有多少帧延迟。(我们假设解码器符合规范)
     * 解码: 除了规范中指定的标准解码器会产生的延迟外,还有多少帧延迟。
     *
     * 视频:
     *   解码输出相对于编码输入将延迟的帧数。
     *
     * 音频:
     *   对于编码,此字段未使用(参见initial_padding)。
     *
     *   对于解码,这是解码器在输出有效之前需要输出的样本数。
     *   在查找时,你应该在所需的查找点之前开始解码这么多样本。
     *
     * - 编码: 由libavcodec设置。
     * - 解码: 由libavcodec设置。
     */
    int delay;

    /* 仅视频 */
    /**
     * 图像宽度/高度。
     *
     * @注意 由于帧重排序,这些字段可能与avcodec_receive_frame()
     * 输出的最后一个AVFrame的值不匹配。
     *
     * - 编码: 必须由用户设置。
     * - 解码: 如果已知(例如从容器获知),用户可以在打开解码器之前设置。
     *         某些解码器将要求调用者设置尺寸。在解码过程中,
     *         解码器可能会根据需要在解析数据时覆盖这些值。
     */
    int width, height;

    /**
     * 比特流宽度/高度,可能与width/height不同,例如当解码帧在输出前被裁剪
     * 或启用了lowres。
     *
     * @注意 由于帧重排序,这些字段可能与avcodec_receive_frame()
     * 输出的最后一个AVFrame的值不匹配。
     *
     * - 编码: 未使用
     * - 解码: 如果已知(例如从容器获知),用户可以在打开解码器之前设置。
     *         在解码过程中,解码器可能会根据需要在解析数据时覆盖这些值。
     */
    int coded_width, coded_height;

    /**
     * 图像组中的图像数量,或者对于仅帧内编码为0
     * - 编码: 由用户设置。
     * - 解码: 未使用
     */
    int gop_size;

    /**
     * 像素格式,参见AV_PIX_FMT_xxx。
     * 如果从头部已知,可能由解复用器设置。
     * 如果解码器更清楚,可能会被解码器覆盖。
     *
     * @注意 由于帧重排序,此字段可能与avcodec_receive_frame()
     * 输出的最后一个AVFrame的值不匹配。
     *
     * - 编码: 由用户设置。
     * - 解码: 如果已知则由用户设置,在解析数据时被libavcodec覆盖。
     */
    enum AVPixelFormat pix_fmt;

    /**
     * 如果非NULL,'draw_horiz_band'会被libavcodec解码器调用来绘制水平条带。
     * 这可以改善缓存使用。不是所有编解码器都能做到这一点。你必须事先检查
     * 编解码器的功能。
     * 当使用多线程时,可能会同时从多个线程调用;线程可能会绘制同一AVFrame的
     * 不同部分,或多个AVFrame,并且不保证条带会按顺序绘制。
     * 该函数也被硬件加速API使用。
     * 在帧解码期间至少调用一次,以传递硬件渲染所需的数据。
     * 在该模式下,AVFrame指向特定于加速API的结构,而不是像素数据。
     * 应用程序读取该结构并可以更改一些字段以指示进度或标记状态。
     * - 编码: 未使用
     * - 解码: 由用户设置。
     * @param height 条带的高度
     * @param y 条带的y位置
     * @param type 1->顶场, 2->底场, 3->帧
     * @param offset 应从中读取条带的AVFrame.data的偏移量
     */
    void (*draw_horiz_band)(struct AVCodecContext *s,
                            const AVFrame *src, int offset[AV_NUM_DATA_POINTERS],
                            int y, int type, int height);

    /**
     * 协商像素格式的回调函数。仅用于解码,可以在调用avcodec_open2()之前由调用者设置。
     *
     * 由一些解码器调用以选择将用于输出帧的像素格式。这主要用于设置硬件加速,
     * 提供的格式列表包含相应的硬件加速像素格式和"软件"格式。软件像素格式也可以
     * 从 \ref sw_pix_fmt 获取。
     *
     * 当编码帧属性(如分辨率、像素格式等)发生变化且新属性支持多种输出格式时,
     * 将调用此回调。如果选择了硬件像素格式但其初始化失败,可能会立即再次调用回调。
     *
     * 如果解码器是多线程的,此回调可能会从不同线程调用,但不会同时从多个线程调用。
     *
     * @param fmt 在当前配置中可能使用的格式列表,以 AV_PIX_FMT_NONE 结束。
     * @warning 如果回调返回的值不是 fmt 中的格式之一或 AV_PIX_FMT_NONE,行为是未定义的。
     * @return 选择的格式或 AV_PIX_FMT_NONE
     */
    enum AVPixelFormat (*get_format)(struct AVCodecContext *s, const enum AVPixelFormat * fmt);

    /**
     * 非B帧之间的最大B帧数量
     * 注意:相对于输入,输出将延迟 max_b_frames+1
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    int max_b_frames;

    /**
     * IP帧和B帧之间的qscale因子
     * 如果 > 0,则将使用最后一个P帧量化器(q = lastp_q*factor+offset)。
     * 如果 < 0,则将进行正常的码率控制(q = -normal_q*factor+offset)。
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    float b_quant_factor;

    /**
     * IP帧和B帧之间的qscale偏移
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    float b_quant_offset;

    /**
     * 解码器中帧重排序缓冲区的大小。
     * 对于MPEG-2,它是1 IPB或0低延迟IP。
     * - 编码: 由libavcodec设置
     * - 解码: 由libavcodec设置
     */
    int has_b_frames;

    /**
     * P帧和I帧之间的qscale因子
     * 如果 > 0,则将使用最后一个P帧量化器(q = lastp_q * factor + offset)。
     * 如果 < 0,则将进行正常的码率控制(q= -normal_q*factor+offset)。
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    float i_quant_factor;

    /**
     * P帧和I帧之间的qscale偏移
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    float i_quant_offset;

    /**
     * 亮度遮蔽(0->禁用)
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    float lumi_masking;

    /**
     * 临时复杂度遮蔽(0->禁用)
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    float temporal_cplx_masking;

    /**
     * 空间复杂度遮蔽(0->禁用)
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    float spatial_cplx_masking;

    /**
     * p块遮蔽(0->禁用)
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    float p_masking;

    /**
     * 暗度遮蔽(0->禁用)
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    float dark_masking;

#if FF_API_SLICE_OFFSET
    /**
     * 切片数量
     * - 编码: 由libavcodec设置
     * - 解码: 由用户设置(或0)
     */
    attribute_deprecated
    int slice_count;

    /**
     * 帧中切片的字节偏移量
     * - 编码: 由libavcodec设置/分配
     * - 解码: 由用户设置/分配(或NULL)
     */
    attribute_deprecated
    int *slice_offset;
#endif

    /**
     * 采样宽高比(0表示未知)
     * 即像素的宽度除以像素的高度。
     * 分子和分母必须互质且对某些视频标准来说必须小于256。
     * - 编码: 由用户设置
     * - 解码: 由libavcodec设置
     */
    AVRational sample_aspect_ratio;

    /**
     * 运动估计比较函数
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    int me_cmp;
    /**
     * 亚像素运动估计比较函数
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    int me_sub_cmp;
    /**
     * 宏块比较函数(尚不支持)
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    int mb_cmp;
    /**
     * 隔行DCT比较函数
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    int ildct_cmp;
#define FF_CMP_SAD          0
#define FF_CMP_SSE          1
#define FF_CMP_SATD         2
#define FF_CMP_DCT          3
#define FF_CMP_PSNR         4
#define FF_CMP_BIT          5
#define FF_CMP_RD           6
#define FF_CMP_ZERO         7
#define FF_CMP_VSAD         8
#define FF_CMP_VSSE         9
#define FF_CMP_NSSE         10
#define FF_CMP_W53          11
#define FF_CMP_W97          12
#define FF_CMP_DCTMAX       13
#define FF_CMP_DCT264       14
#define FF_CMP_MEDIAN_SAD   15
#define FF_CMP_CHROMA       256

    /**
     * ME菱形大小和形状
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    int dia_size;

    /**
     * 先前MV预测器的数量(2a+1 x 2a+1方形)
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    int last_predictor_count;

    /**
     * 运动估计预处理比较函数
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    int me_pre_cmp;

    /**
     * ME预处理菱形大小和形状
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    int pre_dia_size;

    /**
     * 亚像素ME质量
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    int me_subpel_quality;

    /**
     * 亚像素单位中的最大运动估计搜索范围
     * 如果为0则无限制。
     *
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    int me_range;

    /**
     * 切片标志
     * - 编码: 未使用
     * - 解码: 由用户设置
     */
    int slice_flags;
#define SLICE_FLAG_CODED_ORDER    0x0001 ///< draw_horiz_band()按编码顺序而不是显示顺序调用
#define SLICE_FLAG_ALLOW_FIELD    0x0002 ///< 允许使用场切片调用draw_horiz_band()(MPEG-2场图像)
#define SLICE_FLAG_ALLOW_PLANE    0x0004 ///< 允许一次使用1个分量调用draw_horiz_band()(SVQ1)

    /**
     * 宏块决策模式
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    int mb_decision;
#define FF_MB_DECISION_SIMPLE 0        ///< 使用mb_cmp
#define FF_MB_DECISION_BITS   1        ///< 选择需要最少比特的
#define FF_MB_DECISION_RD     2        ///< 码率失真

    /**
     * 自定义帧内量化矩阵
     * 必须使用av_malloc()系列函数分配,并将在avcodec_free_context()中释放。
     * - 编码: 由用户设置/分配,由libavcodec释放。可以为NULL。
     * - 解码: 由libavcodec设置/分配/释放。
     */
    uint16_t *intra_matrix;

    /**
     * 自定义帧间量化矩阵
     * 必须使用av_malloc()系列函数分配,并将在avcodec_free_context()中释放。
     * - 编码: 由用户设置/分配,由libavcodec释放。可以为NULL。
     * - 解码: 由libavcodec设置/分配/释放。
     */
    uint16_t *inter_matrix;

    /**
     * 帧内DC系数的精度 - 8
     * - 编码: 由用户设置
     * - 解码: 由libavcodec设置
     */
    int intra_dc_precision;

    /**
     * 顶部跳过的宏块行数
     * - 编码: 未使用
     * - 解码: 由用户设置
     */
    int skip_top;

    /**
     * 底部跳过的宏块行数
     * - 编码: 未使用
     * - 解码: 由用户设置
     */
    int skip_bottom;

    /**
     * 最小MB拉格朗日乘子
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    int mb_lmin;

    /**
     * 最大MB拉格朗日乘子
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    int mb_lmax;

    /**
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    int bidir_refine;

    /**
     * 最小GOP大小
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    int keyint_min;

    /**
     * 参考帧数量
     * - 编码: 由用户设置
     * - 解码: 由lavc设置
     */
    int refs;

    /**
     * 注意:值取决于用于全像素ME的比较函数。
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    int mv0_threshold;

    /**
     * 源原色的色度坐标。
     * - 编码: 由用户设置
     * - 解码: 由libavcodec设置
     */
    enum AVColorPrimaries color_primaries;

    /**
     * 色彩传输特性。
     * - 编码: 由用户设置
     * - 解码: 由libavcodec设置
     */
    enum AVColorTransferCharacteristic color_trc;

    /**
     * YUV色彩空间类型。
     * - 编码: 由用户设置
     * - 解码: 由libavcodec设置
     */
    enum AVColorSpace colorspace;

    /**
     * MPEG vs JPEG YUV范围。
     * - 编码: 由用户设置以覆盖默认输出色彩范围值,
     *   如果未指定,libavcodec会根据输出格式设置色彩范围。
     * - 解码: 由libavcodec设置,用户可以设置以将色彩范围传播到
     *   从解码器上下文读取的组件。
     */
    enum AVColorRange color_range;

    /**
     * 这定义了色度样本的位置。
     * - 编码: 由用户设置
     * - 解码: 由libavcodec设置
     */
    enum AVChromaLocation chroma_sample_location;

    /**
     * 切片数量。
     * 表示图像细分的数量。用于并行解码。
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    int slices;

    /** 场序
     * - 编码: 由libavcodec设置
     * - 解码: 由用户设置
     */
    enum AVFieldOrder field_order;

    /* 仅音频 */
    int sample_rate; ///< 每秒采样数

#if FF_API_OLD_CHANNEL_LAYOUT
    /**
     * 音频通道数
     * @已弃用 使用ch_layout.nb_channels
     */
    attribute_deprecated
    int channels;
#endif

    /**
     * 音频采样格式
     * - 编码: 由用户设置
     * - 解码: 由libavcodec设置
     */
    enum AVSampleFormat sample_fmt;  ///< 采样格式

    /* 以下数据不应初始化。 */
    /**
     * 音频帧中每个通道的采样数。
     *
     * - 编码: 在avcodec_open2()中由libavcodec设置。除最后一帧外,
     *   每个提交的帧必须包含每个通道恰好frame_size个采样。
     *   当编解码器设置了AV_CODEC_CAP_VARIABLE_FRAME_SIZE时可能为0,
     *   此时帧大小不受限制。
     * - 解码: 可能由某些解码器设置以指示恒定帧大小
     */
    int frame_size;

#if FF_API_AVCTX_FRAME_NUMBER
    /**
     * 帧计数器,由libavcodec设置。
     *
     * - 解码: 到目前为止从解码器返回的帧总数。
     * - 编码: 到目前为止传递给编码器的帧总数。
     *
     *   @注意 如果编码/解码导致错误,计数器不会增加。
     *   @已弃用 使用frame_num代替
     */
    attribute_deprecated
    int frame_number;
#endif

    /**
     * 如果恒定且已知则为每个数据包的字节数,否则为0
     * 由一些基于WAV的音频编解码器使用。
     */
    int block_align;

    /**
     * 音频截止带宽(0表示"自动")
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    int cutoff;

#if FF_API_OLD_CHANNEL_LAYOUT
    /**
     * 音频通道布局。
     * - 编码: 由用户设置
     * - 解码: 由用户设置,可能被libavcodec覆盖
     * @已弃用 使用ch_layout
     */
    attribute_deprecated
    uint64_t channel_layout;

    /**
     * 请求解码器在可能的情况下使用此通道布局(0表示默认)
     * - 编码: 未使用
     * - 解码: 由用户设置
     * @已弃用 使用"downmix"编解码器私有选项
     */
    attribute_deprecated
    uint64_t request_channel_layout;
#endif

    /**
     * 音频流传输的服务类型。
     * - 编码: 由用户设置
     * - 解码: 由libavcodec设置
     */
    enum AVAudioServiceType audio_service_type;

    /**
     * 期望的采样格式
     * - 编码: 未使用
     * - 解码: 由用户设置
     * 如果可能,解码器将解码为此格式。
     */
    enum AVSampleFormat request_sample_fmt;

    /**
     * 此回调在每帧开始时调用以获取其数据缓冲区。可能有一个用于所有数据的连续缓冲区,
     * 也可能每个数据平面有一个缓冲区,或者介于两者之间的任何情况。这意味着,
     * 你可以根据需要在buf[]中设置任意多个条目。每个缓冲区必须使用AVBuffer API
     * 进行引用计数(参见下面buf[]的描述)。
     *
     * 在调用此回调之前,将在帧中设置以下字段:
     * - format
     * - width, height (仅视频)
     * - sample_rate, channel_layout, nb_samples (仅音频)
     * 它们的值可能与AVCodecContext中的相应值不同。此回调必须使用帧值,
     * 而不是编解码器上下文值来计算所需的缓冲区大小。
     *
     * 此回调必须填充帧中的以下字段:
     * - data[]
     * - linesize[]
     * - extended_data:
     *   * 如果数据是具有超过8个通道的平面音频,则此回调必须分配并填充extended_data
     *     以包含所有数据平面的所有指针。data[]必须保存尽可能多的指针。
     *     extended_data必须使用av_malloc()分配,并将在av_frame_unref()中释放。
     *   * 否则extended_data必须指向data
     * - buf[]必须包含一个或多个指向AVBufferRef结构的指针。帧的所有data和extended_data
     *   指针都必须包含在这些结构中。也就是说,每个分配的内存块一个AVBufferRef,
     *   不一定每个data[]条目一个AVBufferRef。参见:av_buffer_create(),av_buffer_alloc(),
     *   和av_buffer_ref()。
     * - extended_buf和nb_extended_buf必须由此回调使用av_malloc()分配,
     *   如果有比buf[]能容纳的更多缓冲区,则填充额外的缓冲区。extended_buf将在
     *   av_frame_unref()中释放。
     *
     * 如果未设置AV_CODEC_CAP_DR1,则get_buffer2()必须调用avcodec_default_get_buffer2(),
     * 而不是通过其他方式提供分配的缓冲区。
     *
     * 每个数据平面必须对齐到目标CPU所需的最大值。
     *
     * @参见 avcodec_default_get_buffer2()
     *
     * 视频:
     *
     * 如果在flags中设置了AV_GET_BUFFER_FLAG_REF,则帧可能稍后被libavcodec重用
     * (如果它是可写的,则可以读取和/或写入)。
     *
     * 应使用avcodec_align_dimensions2()来找到所需的宽度和高度,
     * 因为它们通常需要向上舍入到16的下一个倍数。
     *
     * 某些解码器不支持在帧之间更改linesize。
     *
     * 如果使用帧多线程,此回调可能从不同的线程调用,但不会同时从多个线程调用。
     * 不需要可重入。
     *
     * @参见 avcodec_align_dimensions2()
     *
     * 音频:
     *
     * 解码器通过在调用get_buffer2()之前设置AVFrame.nb_samples来请求特定大小的缓冲区。
     * 但是,解码器可能仅使用部分缓冲区,方法是在输出帧中将AVFrame.nb_samples设置为较小的值。
     *
     * 作为便利,自定义get_buffer2()函数可以使用libavutil中的av_samples_get_buffer_size()
     * 和av_samples_fill_arrays()来找到所需的数据大小并填充数据指针和linesize。
     * 在AVFrame.linesize中,对于音频只能设置linesize[0],因为所有平面必须相同大小。
     *
     * @参见 av_samples_get_buffer_size(), av_samples_fill_arrays()
     *
     * - 编码: 未使用
     * - 解码: 由libavcodec设置,用户可以覆盖
     */
    int (*get_buffer2)(struct AVCodecContext *s, AVFrame *frame, int flags);

    /* - 编码参数 */
    float qcompress;  ///< 简单和困难场景之间qscale变化量(0.0-1.0)
    float qblur;      ///< 随时间的qscale平滑量(0.0-1.0)

    /**
     * 最小量化器
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    int qmin;

    /**
     * 最大量化器
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    int qmax;

    /**
     * 帧之间的最大量化器差异
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    int max_qdiff;

    /**
     * 解码器比特流缓冲区大小
     * - 编码: 由用户设置
     * - 解码: 可能由libavcodec设置
     */
    int rc_buffer_size;

    /**
     * 速率控制覆盖,参见RcOverride
     * - 编码: 由用户分配/设置/释放
     * - 解码: 未使用
     */
    int rc_override_count;
    RcOverride *rc_override;

    /**
     * 最大比特率
     * - 编码: 由用户设置
     * - 解码: 由用户设置,可能被libavcodec覆盖
     */
    int64_t rc_max_rate;

    /**
     * 最小比特率
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    int64_t rc_min_rate;

    /**
     * 速率控制尝试使用的最大可用VBV缓冲区使用量,不会导致下溢。
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    float rc_max_available_vbv_use;

    /**
     * 速率控制尝试使用的最小VBV溢出使用量倍数,以防止VBV溢出。
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    float rc_min_vbv_overflow_use;

    /**
     * 在开始解码之前应该加载到rc缓冲区中的位数。
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    int rc_initial_buffer_occupancy;

    /**
     * trellis RD量化
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    int trellis;

    /**
     * pass1编码统计输出缓冲区
     * - 编码: 由libavcodec设置
     * - 解码: 未使用
     */
    char *stats_out;

    /**
     * pass2编码统计输入缓冲区
     * 应该将pass1的stats_out中的连接内容放在这里。
     * - 编码: 由用户分配/设置/释放
     * - 解码: 未使用
     */
    char *stats_in;

    /**
     * 解决编码器中有时无法自动检测的bug。
     * - 编码: 由用户设置
     * - 解码: 由用户设置
     */
    int workaround_bugs;
#define FF_BUG_AUTODETECT       1  ///< 自动检测
#define FF_BUG_XVID_ILACE       4
#define FF_BUG_UMP4             8
#define FF_BUG_NO_PADDING       16
#define FF_BUG_AMV              32
#define FF_BUG_QPEL_CHROMA      64
#define FF_BUG_STD_QPEL         128
#define FF_BUG_QPEL_CHROMA2     256
#define FF_BUG_DIRECT_BLOCKSIZE 512
#define FF_BUG_EDGE             1024
#define FF_BUG_HPEL_CHROMA      2048
#define FF_BUG_DC_CLIP          4096
#define FF_BUG_MS               8192 ///< 解决Microsoft破损解码器中的各种bug
#define FF_BUG_TRUNCATED       16384
#define FF_BUG_IEDGE           32768

    /**
     * 严格遵循标准(MPEG-4,...)。
     * - 编码: 由用户设置
     * - 解码: 由用户设置
     * 将其设置为STRICT或更高意味着编码器和解码器通常会做愚蠢的事情,
     * 而将其设置为unofficial或更低意味着编码器可能会产生不被所有符合规范的
     * 解码器支持的输出。解码器不区分normal、unofficial和experimental
     * (也就是说,它们总是在可能的情况下尝试解码),除非它们被明确要求做愚蠢的事情
     * (=严格遵守规范)
     * 这只能设置为defs.h中的FF_COMPLIANCE_*值之一。
     */
    int strict_std_compliance;

    /**
     * 错误隐藏标志
     * - 编码: 未使用
     * - 解码: 由用户设置
     */
    int error_concealment;
#define FF_EC_GUESS_MVS   1
#define FF_EC_DEBLOCK     2
#define FF_EC_FAVOR_INTER 256

    /**
     * 调试
     * - 编码: 由用户设置
     * - 解码: 由用户设置
     */
    int debug;
#define FF_DEBUG_PICT_INFO   1
#define FF_DEBUG_RC          2
#define FF_DEBUG_BITSTREAM   4
#define FF_DEBUG_MB_TYPE     8
#define FF_DEBUG_QP          16
#define FF_DEBUG_DCT_COEFF   0x00000040
#define FF_DEBUG_SKIP        0x00000080
#define FF_DEBUG_STARTCODE   0x00000100
#define FF_DEBUG_ER          0x00000400
#define FF_DEBUG_MMCO        0x00000800
#define FF_DEBUG_BUGS        0x00001000
#define FF_DEBUG_BUFFERS     0x00008000
#define FF_DEBUG_THREADS     0x00010000
#define FF_DEBUG_GREEN_MD    0x00800000
#define FF_DEBUG_NOMC        0x01000000

    /**
     * 错误识别;可能会错误检测一些或多或少有效的部分。
     * 这是在defs.h中定义的AV_EF_*值的位字段。
     *
     * - 编码: 由用户设置
     * - 解码: 由用户设置
     */
    int err_recognition;

#if FF_API_REORDERED_OPAQUE
    /**
     * 64位不透明数字(通常是PTS),将被重新排序并输出到AVFrame.reordered_opaque
     * - 编码: 由libavcodec设置为对应于最后返回数据包的输入帧的reordered_opaque。
     *         仅支持具有AV_CODEC_CAP_ENCODER_REORDERED_OPAQUE能力的编码器。
     * - 解码: 由用户设置
     *
     * @已弃用 请改用AV_CODEC_FLAG_COPY_OPAQUE
     */
    attribute_deprecated
    int64_t reordered_opaque;
#endif

    /**
     * 正在使用的硬件加速器
     * - 编码: 未使用
     * - 解码: 由libavcodec设置
     */
    const struct AVHWAccel *hwaccel;

    /**
     * 传统硬件加速器上下文。
     *
     * 对于某些硬件加速方法,调用者可以使用此字段向编解码器发送特定于hwaccel的数据。
     * 此指针指向的结构依赖于hwaccel,并在相应的头文件中定义。
     * 请参考FFmpeg硬件加速器文档以了解如何填写此内容。
     *
     * 在大多数情况下,此字段是可选的 - 必要的信息也可以通过@ref hw_frames_ctx或
     * @ref hw_device_ctx提供给libavcodec(参见avcodec_get_hw_config())。
     * 但在某些情况下,它可能是发送一些(可选)信息的唯一方法。
     *
     * 该结构及其内容由调用者拥有。
     *
     * - 编码: 可以在avcodec_open2()之前由调用者设置。必须保持有效直到avcodec_free_context()。
     * - 解码: 可以由调用者在get_format()回调中设置。必须保持有效直到下一次get_format()调用,
     *         或avcodec_free_context()(以先到者为准)。
     */
    void *hwaccel_context;

    /**
     * 错误
     * - 编码: 如果flags & AV_CODEC_FLAG_PSNR,则由libavcodec设置
     * - 解码: 未使用
     */
    uint64_t error[AV_NUM_DATA_POINTERS];

    /**
     * DCT算法,参见下面的FF_DCT_*
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    int dct_algo;
#define FF_DCT_AUTO    0
#define FF_DCT_FASTINT 1
#define FF_DCT_INT     2
#define FF_DCT_MMX     3
#define FF_DCT_ALTIVEC 5
#define FF_DCT_FAAN    6

    /**
     * IDCT算法,参见下面的FF_IDCT_*
     * - 编码: 由用户设置
     * - 解码: 由用户设置
     */
    int idct_algo;
#define FF_IDCT_AUTO          0
#define FF_IDCT_INT           1
#define FF_IDCT_SIMPLE        2
#define FF_IDCT_SIMPLEMMX     3
#define FF_IDCT_ARM           7
#define FF_IDCT_ALTIVEC       8
#define FF_IDCT_SIMPLEARM     10
#define FF_IDCT_XVID          14
#define FF_IDCT_SIMPLEARMV5TE 16
#define FF_IDCT_SIMPLEARMV6   17
#define FF_IDCT_FAAN          20
#define FF_IDCT_SIMPLENEON    22
#if FF_API_IDCT_NONE
// 以前由xvmc使用
#define FF_IDCT_NONE          24
#endif
#define FF_IDCT_SIMPLEAUTO    128

    /**
     * 来自解复用器的每个样本/像素的位数(huffyuv需要)
     * - 编码: 由libavcodec设置
     * - 解码: 由用户设置
     */
     int bits_per_coded_sample;

    /**
     * libavcodec内部像素/样本格式的每个样本/像素的位数
     * - 编码: 由用户设置
     * - 解码: 由libavcodec设置
     */
    int bits_per_raw_sample;

    /**
     * 低分辨率解码, 1-> 1/2尺寸, 2->1/4尺寸
     * - 编码: 未使用
     * - 解码: 由用户设置
     */
     int lowres;

    /**
     * 线程数
     * 用于决定应该传递给execute()多少个独立任务
     * - 编码: 由用户设置
     * - 解码: 由用户设置
     */
    int thread_count;

    /**
     * 使用哪些多线程方法。
     * 使用FF_THREAD_FRAME将使每个线程的解码延迟增加一帧,
     * 因此无法提供未来帧的客户端不应使用它。
     *
     * - 编码: 由用户设置,否则使用默认值
     * - 解码: 由用户设置,否则使用默认值
     */
    int thread_type;
#define FF_THREAD_FRAME   1 ///< 一次解码多个帧
#define FF_THREAD_SLICE   2 ///< 一次解码单个帧的多个部分

    /**
     * 编解码器正在使用的多线程方法
     * - 编码: 由libavcodec设置
     * - 解码: 由libavcodec设置
     */
    int active_thread_type;

    /**
     * 编解码器可能会调用此函数来执行几个独立的任务。
     * 它只会在完成所有任务后返回。
     * 用户可以用一些多线程实现替换它,
     * 默认实现将串行执行这些部分。
     * @param count 要执行的任务数量
     * - 编码: 由libavcodec设置,用户可以覆盖
     * - 解码: 由libavcodec设置,用户可以覆盖
     */
    int (*execute)(struct AVCodecContext *c, int (*func)(struct AVCodecContext *c2, void *arg), void *arg2, int *ret, int count, int size);

    /**
     * 编解码器可能会调用此函数来执行几个独立的任务。
     * 它只会在完成所有任务后返回。
     * 用户可以用一些多线程实现替换它,
     * 默认实现将串行执行这些部分。
     * @param c 也传递给func的上下文
     * @param count 要执行的任务数量
     * @param arg2 不变地传递给func的参数
     * @param ret 已执行函数的返回值,必须有"count"个值的空间。可以为NULL。
     * @param func 将被调用count次的函数,jobnr从0到count-1。
     *             threadnr将在0到c->thread_count-1 < MAX_THREADS的范围内,并且使得
     *             同时执行的func的两个实例不会有相同的threadnr。
     * @return 目前总是返回0,但代码应该处理未来的改进,即当任何对func的调用返回< 0时,
     *         不应再对func进行调用,并返回< 0。
     * - 编码: 由libavcodec设置,用户可以覆盖
     * - 解码: 由libavcodec设置,用户可以覆盖
     */
    int (*execute2)(struct AVCodecContext *c, int (*func)(struct AVCodecContext *c2, void *arg, int jobnr, int threadnr), void *arg2, int *ret, int count);

    /**
     * nsse比较函数的噪声vs. sse权重
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
     int nsse_weight;

    /**
     * 配置文件
     * - 编码: 由用户设置
     * - 解码: 由libavcodec设置
     * 参见defs.h中的AV_PROFILE_*定义。
     */
     int profile;
#if FF_API_FF_PROFILE_LEVEL
    /** @已弃用 以下定义已弃用;请改用defs.h中的AV_PROFILE_* */
#define FF_PROFILE_UNKNOWN -99
#define FF_PROFILE_RESERVED -100

#define FF_PROFILE_AAC_MAIN 0
#define FF_PROFILE_AAC_LOW  1
#define FF_PROFILE_AAC_SSR  2
#define FF_PROFILE_AAC_LTP  3
#define FF_PROFILE_AAC_HE   4
#define FF_PROFILE_AAC_HE_V2 28
#define FF_PROFILE_AAC_LD   22
#define FF_PROFILE_AAC_ELD  38
#define FF_PROFILE_MPEG2_AAC_LOW 128
#define FF_PROFILE_MPEG2_AAC_HE  131

#define FF_PROFILE_DNXHD         0
#define FF_PROFILE_DNXHR_LB      1
#define FF_PROFILE_DNXHR_SQ      2
#define FF_PROFILE_DNXHR_HQ      3
#define FF_PROFILE_DNXHR_HQX     4
#define FF_PROFILE_DNXHR_444     5

#define FF_PROFILE_DTS                20
#define FF_PROFILE_DTS_ES             30
#define FF_PROFILE_DTS_96_24          40
#define FF_PROFILE_DTS_HD_HRA         50
#define FF_PROFILE_DTS_HD_MA          60
#define FF_PROFILE_DTS_EXPRESS        70
#define FF_PROFILE_DTS_HD_MA_X        61
#define FF_PROFILE_DTS_HD_MA_X_IMAX   62

#define FF_PROFILE_EAC3_DDP_ATMOS         30

#define FF_PROFILE_TRUEHD_ATMOS           30

#define FF_PROFILE_MPEG2_422    0
#define FF_PROFILE_MPEG2_HIGH   1
#define FF_PROFILE_MPEG2_SS     2
#define FF_PROFILE_MPEG2_SNR_SCALABLE  3
#define FF_PROFILE_MPEG2_MAIN   4
#define FF_PROFILE_MPEG2_SIMPLE 5

#define FF_PROFILE_H264_CONSTRAINED  (1<<9)  // 8+1; constraint_set1_flag
#define FF_PROFILE_H264_INTRA        (1<<11) // 8+3; constraint_set3_flag

#define FF_PROFILE_H264_BASELINE             66
#define FF_PROFILE_H264_CONSTRAINED_BASELINE (66|FF_PROFILE_H264_CONSTRAINED)
#define FF_PROFILE_H264_MAIN                 77
#define FF_PROFILE_H264_EXTENDED             88
#define FF_PROFILE_H264_HIGH                 100
#define FF_PROFILE_H264_HIGH_10              110
#define FF_PROFILE_H264_HIGH_10_INTRA        (110|FF_PROFILE_H264_INTRA)
#define FF_PROFILE_H264_MULTIVIEW_HIGH       118
#define FF_PROFILE_H264_HIGH_422             122
#define FF_PROFILE_H264_HIGH_422_INTRA       (122|FF_PROFILE_H264_INTRA)
#define FF_PROFILE_H264_STEREO_HIGH          128
#define FF_PROFILE_H264_HIGH_444             144
#define FF_PROFILE_H264_HIGH_444_PREDICTIVE  244
#define FF_PROFILE_H264_HIGH_444_INTRA       (244|FF_PROFILE_H264_INTRA)
#define FF_PROFILE_H264_CAVLC_444            44

#define FF_PROFILE_VC1_SIMPLE   0
#define FF_PROFILE_VC1_MAIN     1
#define FF_PROFILE_VC1_COMPLEX  2
#define FF_PROFILE_VC1_ADVANCED 3

#define FF_PROFILE_MPEG4_SIMPLE                     0
#define FF_PROFILE_MPEG4_SIMPLE_SCALABLE            1
#define FF_PROFILE_MPEG4_CORE                       2
#define FF_PROFILE_MPEG4_MAIN                       3
#define FF_PROFILE_MPEG4_N_BIT                      4
#define FF_PROFILE_MPEG4_SCALABLE_TEXTURE           5
#define FF_PROFILE_MPEG4_SIMPLE_FACE_ANIMATION      6
#define FF_PROFILE_MPEG4_BASIC_ANIMATED_TEXTURE     7
#define FF_PROFILE_MPEG4_HYBRID                     8
#define FF_PROFILE_MPEG4_ADVANCED_REAL_TIME         9
#define FF_PROFILE_MPEG4_CORE_SCALABLE             10
#define FF_PROFILE_MPEG4_ADVANCED_CODING           11
#define FF_PROFILE_MPEG4_ADVANCED_CORE             12
#define FF_PROFILE_MPEG4_ADVANCED_SCALABLE_TEXTURE 13
#define FF_PROFILE_MPEG4_SIMPLE_STUDIO             14
#define FF_PROFILE_MPEG4_ADVANCED_SIMPLE           15

#define FF_PROFILE_JPEG2000_CSTREAM_RESTRICTION_0   1
#define FF_PROFILE_JPEG2000_CSTREAM_RESTRICTION_1   2
#define FF_PROFILE_JPEG2000_CSTREAM_NO_RESTRICTION  32768
#define FF_PROFILE_JPEG2000_DCINEMA_2K              3
#define FF_PROFILE_JPEG2000_DCINEMA_4K              4

#define FF_PROFILE_VP9_0                            0
#define FF_PROFILE_VP9_1                            1
#define FF_PROFILE_VP9_2                            2
#define FF_PROFILE_VP9_3                            3

#define FF_PROFILE_HEVC_MAIN                        1
#define FF_PROFILE_HEVC_MAIN_10                     2
#define FF_PROFILE_HEVC_MAIN_STILL_PICTURE          3
#define FF_PROFILE_HEVC_REXT                        4
#define FF_PROFILE_HEVC_SCC                         9

#define FF_PROFILE_VVC_MAIN_10                      1
#define FF_PROFILE_VVC_MAIN_10_444                 33

#define FF_PROFILE_AV1_MAIN                         0
#define FF_PROFILE_AV1_HIGH                         1
#define FF_PROFILE_AV1_PROFESSIONAL                 2

#define FF_PROFILE_MJPEG_HUFFMAN_BASELINE_DCT            0xc0
#define FF_PROFILE_MJPEG_HUFFMAN_EXTENDED_SEQUENTIAL_DCT 0xc1
#define FF_PROFILE_MJPEG_HUFFMAN_PROGRESSIVE_DCT         0xc2
#define FF_PROFILE_MJPEG_HUFFMAN_LOSSLESS                0xc3
#define FF_PROFILE_MJPEG_JPEG_LS                         0xf7

#define FF_PROFILE_SBC_MSBC                         1

#define FF_PROFILE_PRORES_PROXY     0
#define FF_PROFILE_PRORES_LT        1
#define FF_PROFILE_PRORES_STANDARD  2
#define FF_PROFILE_PRORES_HQ        3
#define FF_PROFILE_PRORES_4444      4
#define FF_PROFILE_PRORES_XQ        5

#define FF_PROFILE_ARIB_PROFILE_A 0
#define FF_PROFILE_ARIB_PROFILE_C 1

#define FF_PROFILE_KLVA_SYNC 0
#define FF_PROFILE_KLVA_ASYNC 1

#define FF_PROFILE_EVC_BASELINE             0
#define FF_PROFILE_EVC_MAIN                 1
#endif

    /**
     * 编码级别描述符。
     * - 编码: 由用户设置,对应于编解码器定义的特定级别,
     *   通常对应于配置文件级别,如果未指定则设置为FF_LEVEL_UNKNOWN。
     * - 解码: 由libavcodec设置。
     * 参见defs.h中的AV_LEVEL_*。
     */
     int level;
#if FF_API_FF_PROFILE_LEVEL
    /** @已弃用 以下定义已弃用;请使用defs.h中的AV_LEVEL_UNKOWN代替。 */
#define FF_LEVEL_UNKNOWN -99
#endif

    /**
     * 跳过选定帧的环路滤波。
     * - 编码: 未使用
     * - 解码: 由用户设置
     */
    enum AVDiscard skip_loop_filter;

    /**
     * 跳过选定帧的IDCT/反量化。
     * - 编码: 未使用
     * - 解码: 由用户设置
     */
    enum AVDiscard skip_idct;

    /**
     * 跳过选定帧的解码。
     * - 编码: 未使用
     * - 解码: 由用户设置
     */
    enum AVDiscard skip_frame;

    /**
     * 包含文本字幕样式信息的头部。
     * 对于SUBTITLE_ASS字幕类型,它应该包含整个ASS
     * [Script Info]和[V4+ Styles]部分,以及[Events]行和
     * 后面的Format行。它不应包含任何Dialogue行。
     * - 编码: 由用户设置/分配/释放(在avcodec_open2()之前)
     * - 解码: 由libavcodec设置/分配/释放(通过avcodec_open2())
     */
    uint8_t *subtitle_header;
    int subtitle_header_size;

    /**
     * 仅音频。编码器在音频开始处插入的"预填充"样本(填充)数量。
     * 即调用者必须丢弃这些前导解码样本,以获得没有前导填充的原始音频。
     *
     * - 解码: 未使用
     * - 编码: 由libavcodec设置。输出数据包上的时间戳由编码器调整,
     *         使其始终引用数据包实际包含的数据的第一个样本,
     *         包括任何添加的填充。例如,如果时基是1/采样率,
     *         第一个输入样本的时间戳是0,则第一个输出数据包的时间戳将是
     *         -initial_padding。
     */
    int initial_padding;

    /**
     * - 解码: 对于在压缩比特流中存储帧率值的编解码器,
     *         解码器可以在此处导出它。未知时为{0, 1}。
     * - 编码: 可用于向编码器发送CFR内容的帧率。
     */
    AVRational framerate;

    /**
     * 标称未加速像素格式,参见AV_PIX_FMT_xxx。
     * - 编码: 未使用。
     * - 解码: 在调用get_format()之前由libavcodec设置
     */
    enum AVPixelFormat sw_pix_fmt;

    /**
     * pkt_dts/pts和AVPacket.dts/pts表示的时基。
     * - 编码: 未使用。
     * - 解码: 由用户设置。
     */
    AVRational pkt_timebase;

    /**
     * AVCodecDescriptor
     * - 编码: 未使用。
     * - 解码: 由libavcodec设置。
     */
    const struct AVCodecDescriptor *codec_descriptor;

    /**
     * PTS校正的当前统计信息。
     * - 解码: 由libavcodec维护和使用,不打算供用户应用程序使用
     * - 编码: 未使用
     */
    int64_t pts_correction_num_faulty_pts; /// 到目前为止不正确的PTS值数量
    int64_t pts_correction_num_faulty_dts; /// 到目前为止不正确的DTS值数量
    int64_t pts_correction_last_pts;       /// 最后一帧的PTS
    int64_t pts_correction_last_dts;       /// 最后一帧的DTS

    /**
     * 输入字幕文件的字符编码。
     * - 解码: 由用户设置
     * - 编码: 未使用
     */
    char *sub_charenc;

    /**
     * 字幕字符编码模式。格式或编解码器可能会调整此设置
     * (例如,如果它们自己进行转换)。
     * - 解码: 由libavcodec设置
     * - 编码: 未使用
     */
    int sub_charenc_mode;
#define FF_SUB_CHARENC_MODE_DO_NOTHING  -1  ///< 不做任何事(解复用器输出的流应该已经是UTF-8,或者编解码器是位图)
#define FF_SUB_CHARENC_MODE_AUTOMATIC    0  ///< libavcodec将自行选择模式
#define FF_SUB_CHARENC_MODE_PRE_DECODER  1  ///< AVPacket数据需要在送入解码器之前重新编码为UTF-8,需要iconv
#define FF_SUB_CHARENC_MODE_IGNORE       2  ///< 既不转换字幕,也不检查其是否为有效的UTF-8

    /**
     * 如果编解码器支持,则跳过alpha处理。
     * 注意,如果格式使用预乘alpha(VP6常见,
     * 并且由于更好的视频质量/压缩而推荐),
     * 图像看起来就像是alpha混合到黑色背景上。
     * 但是对于不使用预乘alpha的格式,
     * 可能会出现严重的伪影(尽管例如libswscale目前
     * 无论如何都假定预乘alpha)。
     *
     * - 解码: 由用户设置
     * - 编码: 未使用
     */
    int skip_alpha;

    /**
     * 在不连续之后要跳过的样本数
     * - 解码: 未使用
     * - 编码: 由libavcodec设置
     */
    int seek_preroll;

    /**
     * 自定义帧内量化矩阵
     * - 编码: 由用户设置,可以为NULL。
     * - 解码: 未使用。
     */
    uint16_t *chroma_intra_matrix;

    /**
     * 转储格式分隔符。
     * 可以是", "或"\n      "或其他任何内容
     * - 编码: 由用户设置。
     * - 解码: 由用户设置。
     */
    uint8_t *dump_separator;

    /**
     * 允许的解码器列表,用','分隔。
     * 如果为NULL则允许所有
     * - 编码: 未使用
     * - 解码: 由用户设置
     */
    char *codec_whitelist;

    /**
     * 被解码流的属性
     * - 编码: 未使用
     * - 解码: 由libavcodec设置
     */
    unsigned properties;
#define FF_CODEC_PROPERTY_LOSSLESS        0x00000001
#define FF_CODEC_PROPERTY_CLOSED_CAPTIONS 0x00000002
#define FF_CODEC_PROPERTY_FILM_GRAIN      0x00000004

    /**
     * 与整个编码流相关的附加数据。
     *
     * - 解码: 可以在调用avcodec_open2()之前由用户设置。
     * - 编码: 可以在avcodec_open2()之后由libavcodec设置。
     */
    AVPacketSideData *coded_side_data;
    int            nb_coded_side_data;

    /**
     * 对描述输入(用于编码)或输出(解码)帧的AVHWFramesContext的引用。
     * 该引用由调用者设置,之后由libavcodec拥有(并释放) - 
     * 在设置后调用者不应该再读取它。
     *
     * - 解码: 此字段应由调用者从get_format()回调中设置。
     *         libavcodec将在get_format()调用之前始终取消引用之前的引用(如果有)。
     *
     *         如果使用hwaccel像素格式的默认get_buffer2(),
     *         则此AVHWFramesContext将用于分配帧缓冲区。
     *
     * - 编码: 对于配置为使用hwaccel像素格式的硬件编码器,
     *         调用者应将此字段设置为对描述输入帧的AVHWFramesContext的引用。
     *         AVHWFramesContext.format必须等于AVCodecContext.pix_fmt。
     *
     *         应在调用avcodec_open2()之前设置此字段。
     */
    AVBufferRef *hw_frames_ctx;

    /**
     * 仅音频。编码器在音频末尾附加的填充量(以样本为单位)。
     * 即调用者必须从流末尾丢弃这些解码样本,
     * 以获得没有任何尾部填充的原始音频。
     *
     * - 解码: 未使用
     * - 编码: 未使用
     */
    int trailing_padding;

    /**
     * 每个图像最大接受的像素数。
     *
     * - 解码: 由用户设置
     * - 编码: 由用户设置
     */
    int64_t max_pixels;

    /**
     * 对描述将由硬件编码器/解码器使用的设备的AVHWDeviceContext的引用。
     * 该引用由调用者设置,之后由libavcodec拥有(并释放)。
     *
     * 如果编解码器设备不需要硬件帧,或者使用的任何帧都将由libavcodec内部分配,
     * 则应使用此选项。如果用户希望提供用作编码器输入或解码器输出的任何帧,
     * 则应改用hw_frames_ctx。当在解码器的get_format()中设置hw_frames_ctx时,
     * 在解码相关的流段时将忽略此字段,但在另一个get_format()调用之后可能会再次使用。
     *
     * 对于编码器和解码器,此字段都应在调用avcodec_open2()之前设置,
     * 之后不得写入。
     *
     * 注意,某些解码器可能需要最初设置此字段才能支持hw_frames_ctx - 
     * 在这种情况下,所有使用的帧上下文都必须在同一设备上创建。
     */
    AVBufferRef *hw_device_ctx;

    /**
     * AV_HWACCEL_FLAG_*标志的位集,影响硬件加速解码(如果激活)。
     * - 编码: 未使用
     * - 解码: 由用户设置(在avcodec_open2()之前,或在
     *         AVCodecContext.get_format回调中)
     */
    int hwaccel_flags;

    /**
     * 仅视频解码。某些视频编解码器支持裁剪,这意味着
     * 只有解码帧的子矩形用于显示。此选项控制libavcodec
     * 如何处理裁剪。
     *
     * 当设置为1(默认值)时,libavcodec将在内部应用裁剪。
     * 即它将修改输出帧宽度/高度字段并偏移数据指针
     * (仅在保持对齐的情况下尽可能多,或者如果设置了AV_CODEC_FLAG_UNALIGNED标志,
     * 则完全偏移),以便解码器输出的帧仅引用裁剪区域。
     * 输出帧的crop_*字段将为零。
     *
     * 当设置为0时,输出帧的宽度/高度字段将设置为编码尺寸,
     * crop_*字段将描述裁剪矩形。应用裁剪留给调用者。
     *
     * @warning 当使用具有不透明输出帧的硬件加速时,
     * libavcodec无法从上/左边界应用裁剪。
     *
     * @note 当此选项设置为零时,AVCodecContext和输出AVFrames的
     * 宽度/高度字段具有不同的含义。编解码器上下文字段存储显示尺寸
     * (编码尺寸在coded_width/height中),而帧字段存储编码尺寸
     * (显示尺寸由crop_*字段确定)。
     */
    int apply_cropping;

    /*
     * 仅视频解码。设置解码器将为调用者使用而分配的
     * 额外硬件帧数。这必须在调用avcodec_open2()之前设置。
     *
     * 某些硬件解码器要求在开始解码之前预先定义
     * 它们将用于输出的所有帧。对于这样的解码器,
     * 硬件帧池必须是固定大小的。这里设置的额外帧
     * 是在解码器为正常运行而需要的任何数量之上的
     * (例如,用作参考图片的帧)。
     */
    int extra_hw_frames;

    /**
     * 丢弃帧的损坏样本百分比。
     *
     * - 解码: 由用户设置
     * - 编码: 未使用
     */
    int discard_damaged_percentage;

    /**
     * 每帧最大接受的样本数。
     *
     * - 解码: 由用户设置
     * - 编码: 由用户设置
     */
    int64_t max_samples;

    /**
     * AV_CODEC_EXPORT_DATA_*标志的位集,影响解码器和编码器
     * 在帧、数据包或编码流侧数据中导出的元数据类型。
     *
     * - 解码: 由用户设置
     * - 编码: 由用户设置
     */
    int export_side_data;

    /**
     * 此回调在每个数据包开始时调用以获取其数据缓冲区。
     *
     * 在调用此回调之前,将在数据包中设置以下字段:
     * - size
     * 此回调必须使用上述值计算所需的缓冲区大小,
     * 该大小必须至少填充AV_INPUT_BUFFER_PADDING_SIZE字节。
     *
     * 在某些特定情况下,编码器可能不会使用此回调分配的整个缓冲区。
     * 这将反映在通过avcodec_receive_packet()返回的数据包中的size值。
     *
     * 此回调必须填写数据包中的以下字段:
     * - data: 如果有的话,AVPacket的对齐要求适用。某些架构和
     *   编码器可能会从对齐的数据中受益。
     * - buf: 必须包含指向AVBufferRef结构的指针。数据包的
     *   数据指针必须包含在其中。参见: av_buffer_create(), av_buffer_alloc(),
     *   和av_buffer_ref()。
     *
     * 如果未设置AV_CODEC_CAP_DR1,则get_encode_buffer()必须调用
     * avcodec_default_get_encode_buffer(),而不是通过其他方式提供分配的缓冲区。
     *
     * flags字段可能包含AV_GET_ENCODE_BUFFER_FLAG_标志的组合。
     * 例如,它们可用于提示在创建后缓冲区可能获得的用途。
     * 此回调的实现可能会忽略它们不理解的标志。
     * 如果在flags中设置了AV_GET_ENCODE_BUFFER_FLAG_REF,
     * 则libavcodec可能会稍后重用该数据包(如果可写则读取和/或写入)。
     *
     * 此回调必须是线程安全的,因为当使用帧线程时,
     * 可能会从多个线程同时调用它。
     *
     * @see avcodec_default_get_encode_buffer()
     *
     * - 编码: 由libavcodec设置,用户可以覆盖。
     * - 解码: 未使用
     */
    int (*get_encode_buffer)(struct AVCodecContext *s, AVPacket *pkt, int flags);

    /**
     * 音频通道布局。
     * - 编码: 必须由调用者设置为AVCodec.ch_layouts之一。
     * - 解码: 如果已知,可以由调用者设置,例如从容器中。
     *         解码器然后可以在需要时在解码过程中覆盖它。
     */
    AVChannelLayout ch_layout;

    /**
     * 帧计数器,由libavcodec设置。
     *
     * - 解码: 到目前为止从解码器返回的帧总数。
     * - 编码: 到目前为止传递给编码器的帧总数。
     *
     *   @note 如果编码/解码导致错误,计数器不会增加。
     */
    int64_t frame_num;
} AVCodecContext;

/**
 * @defgroup lavc_hwaccel AVHWAccel
 *
 * @note  用户不应访问此结构中的任何内容。在将来的某个时候,
 *        它将完全不再对外可见。
 *
 * @{
 */
typedef struct AVHWAccel {
    /**
     * 硬件加速编解码器的名称。
     * 该名称在编码器和解码器中是全局唯一的(但编码器和解码器可以共享相同的名称)。
     */
    const char *name;

    /**
     * 硬件加速器实现的编解码器类型。
     *
     * 参见 AVMEDIA_TYPE_xxx
     */
    enum AVMediaType type;

    /**
     * 硬件加速器实现的编解码器。
     *
     * 参见 AV_CODEC_ID_xxx
     */
    enum AVCodecID id;

    /**
     * 支持的像素格式。
     *
     * 这里只支持硬件加速格式。
     */
    enum AVPixelFormat pix_fmt;

    /**
     * 硬件加速编解码器功能。
     * 参见 AV_HWACCEL_CODEC_CAP_*
     */
    int capabilities;
} AVHWAccel;

/**
 * HWAccel 是实验性的,因此优先使用非实验性编解码器
 */
#define AV_HWACCEL_CODEC_CAP_EXPERIMENTAL 0x0200

/**
 * 即使使用的编解码器级别未知或高于硬件驱动程序报告的最大支持级别,
 * 也应该使用硬件加速进行解码。
 *
 * 除非有特定原因不这样做,否则通常建议传递此标志,
 * 因为硬件往往会低报支持的级别。
 */
#define AV_HWACCEL_FLAG_IGNORE_LEVEL (1 << 0)

/**
 * 硬件加速可以输出具有不同于4:2:0的色度采样和/或每个分量超过8位的YUV像素格式。
 */
#define AV_HWACCEL_FLAG_ALLOW_HIGH_DEPTH (1 << 1)

/**
 * 当编解码器配置文件与硬件报告的功能不匹配时,仍应尝试使用硬件加速进行解码。
 *
 * 例如,这可以用于尝试在硬件中解码基线配置文件H.264流 - 它通常会成功,
 * 因为许多标记为基线配置文件的流实际上符合受限基线配置文件。
 *
 * @warning 如果流实际上不受支持,则行为是未定义的,
 *          可能包括在指示成功的同时返回完全不正确的输出。
 */
#define AV_HWACCEL_FLAG_ALLOW_PROFILE_MISMATCH (1 << 2)

/**
 * 某些硬件解码器(即nvdec)可以直接输出解码器表面,或者进行设备内拷贝并返回该拷贝。
 * 解码器表面的数量有硬性限制,而且无法提前准确猜测。
 * 对于某些处理链来说这可能没问题,但其他处理链会达到限制,
 * 进而产生非常令人困惑的错误,需要用户对或多或少晦涩的选项进行微调,
 * 在极端情况下,如果不插入强制复制的avfilter,则完全无法解决。
 *
 * 因此,为了安全和稳定性,hwaccel默认会进行复制。
 * 如果用户真的想要最小化复制次数,他们可以设置此标志,
 * 并确保他们的处理链不会耗尽表面池。
 */
#define AV_HWACCEL_FLAG_UNSAFE_OUTPUT (1 << 3)

/**
 * @}
 */

enum AVSubtitleType {
    SUBTITLE_NONE,

    SUBTITLE_BITMAP,                ///< 位图,将设置pict

    /**
     * 纯文本,文本字段必须由解码器设置且具有权威性。
     * ass和pict字段可能包含近似值。
     */
    SUBTITLE_TEXT,

    /**
     * 格式化文本,ass字段必须由解码器设置且具有权威性。
     * pict和text字段可能包含近似值。
     */
    SUBTITLE_ASS,
};

#define AV_SUBTITLE_FLAG_FORCED 0x00000001

typedef struct AVSubtitleRect {
    int x;         ///< pict的左上角,当pict未设置时未定义
    int y;         ///< pict的左上角,当pict未设置时未定义
    int w;         ///< pict的宽度,当pict未设置时未定义
    int h;         ///< pict的高度,当pict未设置时未定义
    int nb_colors; ///< pict中的颜色数,当pict未设置时未定义

    /**
     * 此字幕的位图的data+linesize。
     * 一旦渲染完成,也可以为text/ass设置。
     */
    uint8_t *data[4];
    int linesize[4];

    enum AVSubtitleType type;

    char *text;                     ///< 以0结尾的纯UTF-8文本

    /**
     * 以0结尾的ASS/SSA兼容事件行。
     * 此结构中的其他值不影响其呈现。
     */
    char *ass;

    int flags;
} AVSubtitleRect;

typedef struct AVSubtitle {
    uint16_t format; /* 0 = 图形 */
    uint32_t start_display_time; /* 相对于数据包pts,以ms为单位 */
    uint32_t end_display_time; /* 相对于数据包pts,以ms为单位 */
    unsigned num_rects;
    AVSubtitleRect **rects;
    int64_t pts;    ///< 与数据包pts相同,以AV_TIME_BASE为单位
} AVSubtitle;

/**
 * 返回LIBAVCODEC_VERSION_INT常量。
 */
unsigned avcodec_version(void);

/**
 * 返回libavcodec的构建时配置。
 */
const char *avcodec_configuration(void);

/**
 * 返回libavcodec的许可证。
 */
const char *avcodec_license(void);

/**
 * 分配一个AVCodecContext并将其字段设置为默认值。
 * 生成的结构体应使用avcodec_free_context()释放。
 *
 * @param codec 如果非NULL,为给定的编解码器分配私有数据并初始化默认值。
 *              然后使用不同的编解码器调用avcodec_open2()是非法的。
 *              如果为NULL,则不会初始化特定于编解码器的默认值,
 *              这可能导致次优的默认设置(这对编码器来说很重要,例如libx264)。
 *
 * @return 填充了默认值的AVCodecContext,失败时返回NULL。
 */
AVCodecContext *avcodec_alloc_context3(const AVCodec *codec);

/**
 * 释放编解码器上下文和与其关联的所有内容,并将NULL写入提供的指针。
 */
void avcodec_free_context(AVCodecContext **avctx);

/**
 * 获取AVCodecContext的AVClass。它可以与AV_OPT_SEARCH_FAKE_OBJ一起使用来检查选项。
 *
 * @see av_opt_find()。
 */
const AVClass *avcodec_get_class(void);

/**
 * 获取AVSubtitleRect的AVClass。它可以与AV_OPT_SEARCH_FAKE_OBJ一起使用来检查选项。
 *
 * @see av_opt_find()。
 */
const AVClass *avcodec_get_subtitle_rect_class(void);

/**
 * 根据提供的编解码器上下文中的值填充参数结构体。
 * par中的任何已分配字段都将被释放并替换为codec中相应字段的副本。
 *
 * @return >= 0表示成功,负AVERROR代码表示失败
 */
int avcodec_parameters_from_context(struct AVCodecParameters *par,
                                    const AVCodecContext *codec);

/**
 * 根据提供的编解码器参数中的值填充编解码器上下文。
 * codec中任何已分配的字段,如果在par中有对应字段,
 * 都将被释放并替换为par中相应字段的副本。
 * codec中没有在par中对应的字段不会被触及。
 *
 * @return >= 0表示成功,负AVERROR代码表示失败。
 */
int avcodec_parameters_to_context(AVCodecContext *codec,
                                  const struct AVCodecParameters *par);

/**
 * 初始化AVCodecContext以使用给定的AVCodec。
 * 在使用此函数之前,上下文必须使用avcodec_alloc_context3()分配。
 *
 * 函数avcodec_find_decoder_by_name()、avcodec_find_encoder_by_name()、
 * avcodec_find_decoder()和avcodec_find_encoder()提供了一种检索编解码器的简单方法。
 *
 * 根据编解码器,您可能还需要为解码设置编解码器上下文中的选项
 * (例如宽度、高度或像素或音频采样格式,如果比特流中没有这些信息,
 * 例如在解码原始音频或视频时)。
 *
 * 编解码器上下文中的选项可以通过在选项AVDictionary中设置,
 * 或者在调用此函数之前直接在上下文本身中设置值,
 * 或者使用av_opt_set() API来设置。
 *
 * 示例:
 * @code
 * av_dict_set(&opts, "b", "2.5M", 0);
 * codec = avcodec_find_decoder(AV_CODEC_ID_H264);
 * if (!codec)
 *     exit(1);
 *
 * context = avcodec_alloc_context3(codec);
 *
 * if (avcodec_open2(context, codec, opts) < 0)
 *     exit(1);
 * @endcode
 *
 * 如果有AVCodecParameters可用(例如使用libavformat解复用流时,
 * 并访问解复用器中包含的AVStream),可以使用avcodec_parameters_to_context()
 * 将编解码器参数复制到编解码器上下文,如以下示例所示:
 *
 * @code
 * AVStream *stream = ...;
 * context = avcodec_alloc_context3(codec);
 * if (avcodec_parameters_to_context(context, stream->codecpar) < 0)
 *     exit(1);
 * if (avcodec_open2(context, codec, NULL) < 0)
 *     exit(1);
 * @endcode
 *
 * @note 在使用解码例程(如@ref avcodec_receive_frame())之前,
 * 必须始终调用此函数。
 *
 * @param avctx 要初始化的上下文。
 * @param codec 为此上下文打开的编解码器。如果之前已为此上下文传递了非NULL编解码器
 *              给avcodec_alloc_context3(),则此参数必须为NULL或等于之前传递的编解码器。
 * @param options 填充了AVCodecContext和编解码器私有选项的字典,
 *                这些选项设置在avctx已设置选项之上,可以为NULL。
 *                返回时,此对象将填充未在avctx编解码器上下文中找到的选项。
 *
 * @return 成功返回零,错误返回负值
 * @see avcodec_alloc_context3(), avcodec_find_decoder(), avcodec_find_encoder(),
 *      av_dict_set(), av_opt_set(), av_opt_find(), avcodec_parameters_to_context()
 */
int avcodec_open2(AVCodecContext *avctx, const AVCodec *codec, AVDictionary **options);

/**
 * 关闭给定的AVCodecContext并释放与其关联的所有数据
 * (但不包括AVCodecContext本身)。
 *
 * 在尚未打开的AVCodecContext上调用此函数将释放在avcodec_alloc_context3()中
 * 使用非NULL编解码器分配的特定于编解码器的数据。后续调用将不执行任何操作。
 *
 * @note 不要使用此函数。使用avcodec_free_context()销毁编解码器上下文
 * (无论是打开还是关闭)。不再支持多次打开和关闭编解码器上下文 - 
 * 请改用多个编解码器上下文。
 */
int avcodec_close(AVCodecContext *avctx);

/**
 * 释放给定字幕结构中的所有已分配数据。
 *
 * @param sub 要释放的AVSubtitle。
 */
void avsubtitle_free(AVSubtitle *sub);

/**
 * @}
 */

/**
 * @addtogroup lavc_decoding
 * @{
 */

/**
 * AVCodecContext.get_buffer2()的默认回调。它被公开以便可以被没有设置
 * AV_CODEC_CAP_DR1的解码器的自定义get_buffer2()实现调用。
 */
int avcodec_default_get_buffer2(AVCodecContext *s, AVFrame *frame, int flags);

/**
 * AVCodecContext.get_encode_buffer()的默认回调。它被公开以便可以被没有设置
 * AV_CODEC_CAP_DR1的编码器的自定义get_encode_buffer()实现调用。
 */
int avcodec_default_get_encode_buffer(AVCodecContext *s, AVPacket *pkt, int flags);

/**
 * 修改宽度和高度值,使其在不使用任何水平填充的情况下,
 * 将产生编解码器可接受的内存缓冲区。
 *
 * 仅当打开了具有AV_CODEC_CAP_DR1的编解码器时才可使用。
 */
void avcodec_align_dimensions(AVCodecContext *s, int *width, int *height);

/**
 * 修改宽度和高度值,使其在确保所有行大小都是各自linesize_align[i]的倍数的情况下,
 * 将产生编解码器可接受的内存缓冲区。
 *
 * 仅当打开了具有AV_CODEC_CAP_DR1的编解码器时才可使用。
 */
void avcodec_align_dimensions2(AVCodecContext *s, int *width, int *height,
                               int linesize_align[AV_NUM_DATA_POINTERS]);

#ifdef FF_API_AVCODEC_CHROMA_POS
/**
 * 将AVChromaLocation转换为swscale x/y色度位置。
 *
 * 这些位置表示在坐标系统中的色度(0,0)位置,
 * 其中亮度(0,0)表示原点,亮度(1,1)表示256,256
 *
 * @param xpos  水平色度采样位置
 * @param ypos  垂直色度采样位置
 * @deprecated 改用av_chroma_location_enum_to_pos()。
 */
 attribute_deprecated
int avcodec_enum_to_chroma_pos(int *xpos, int *ypos, enum AVChromaLocation pos);

/**
 * 将swscale x/y色度位置转换为AVChromaLocation。
 *
 * 这些位置表示在坐标系统中的色度(0,0)位置,
 * 其中亮度(0,0)表示原点,亮度(1,1)表示256,256
 *
 * @param xpos  水平色度采样位置
 * @param ypos  垂直色度采样位置
 * @deprecated 改用av_chroma_location_pos_to_enum()。
 */
 attribute_deprecated
enum AVChromaLocation avcodec_chroma_pos_to_enum(int xpos, int ypos);
#endif

/**
 * 解码字幕消息。
 * 错误时返回负值,否则返回使用的字节数。
 * 如果没有字幕可以解压缩,got_sub_ptr为零。
 * 否则,字幕存储在*sub中。
 * 注意,字幕编解码器不支持AV_CODEC_CAP_DR1。这是为了简单起见,
 * 因为预期性能差异可以忽略不计,而且重用为视频编解码器编写的get_buffer
 * 可能会由于潜在的非常不同的分配模式而表现不佳。
 *
 * 一些解码器(标记为AV_CODEC_CAP_DELAY)在输入和输出之间有延迟。
 * 这意味着对于某些数据包,它们不会立即产生解码输出,
 * 需要在解码结束时进行刷新以获取所有解码的数据。
 * 刷新是通过调用此函数并将avpkt->data设置为NULL且avpkt->size设置为0来完成的,
 * 直到它停止返回字幕。即使对那些未标记AV_CODEC_CAP_DELAY的解码器进行刷新也是安全的,
 * 这种情况下不会返回字幕。
 *
 * @note 在向解码器提供数据包之前,必须使用@ref avcodec_open2()打开AVCodecContext。
 *
 * @param avctx 编解码器上下文
 * @param[out] sub 预分配的AVSubtitle,解码的字幕将存储在其中,
 *                 如果设置了*got_sub_ptr,必须使用avsubtitle_free释放。
 * @param[in,out] got_sub_ptr 如果没有字幕可以解压缩则为零,否则为非零。
 * @param[in] avpkt 包含输入缓冲区的输入AVPacket。
 */
int avcodec_decode_subtitle2(AVCodecContext *avctx, AVSubtitle *sub,
                             int *got_sub_ptr, const AVPacket *avpkt);

/**
 * 向解码器提供原始数据包数据作为输入。
 *
 * 在内部,此调用将复制相关的AVCodecContext字段,这些字段可能会影响每个数据包的解码,
 * 并在实际解码数据包时应用它们。(例如AVCodecContext.skip_frame,
 * 它可能会指示解码器丢弃通过此函数发送的数据包中包含的帧。)
 *
 * @warning 输入缓冲区avpkt->data必须比实际读取的字节大AV_INPUT_BUFFER_PADDING_SIZE,
 *          因为一些优化的比特流读取器一次读取32或64位,可能会读取超过末尾。
 *
 * @note 在向解码器提供数据包之前,必须使用@ref avcodec_open2()打开AVCodecContext。
 *
 * @param avctx 编解码器上下文
 * @param[in] avpkt 输入AVPacket。通常,这将是单个视频帧,或几个完整的音频帧。
 *                  数据包的所有权保留在调用者手中,解码器不会写入数据包。
 *                  解码器可能会创建对数据包数据的引用(或如果数据包不是引用计数的,则复制它)。
 *                  与旧API不同,数据包始终被完全消耗,
 *                  如果它包含多个帧(例如某些音频编解码器),
 *                  将需要您在发送新数据包之前多次调用avcodec_receive_frame()。
 *                  它可以为NULL(或数据设置为NULL且大小设置为0的AVPacket);
 *                  在这种情况下,它被视为刷新数据包,表示流的结束。
 *                  发送第一个刷新数据包将返回成功。后续的是不必要的,
 *                  将返回AVERROR_EOF。如果解码器仍有缓冲的帧,
 *                  它将在发送刷新数据包后返回它们。
 *
 * @retval 0                 成功
 * @retval AVERROR(EAGAIN)   在当前状态下不接受输入 - 用户必须使用
 *                           avcodec_receive_frame()读取输出(一旦读取所有输出,
 *                           应重新发送数据包,调用将不会失败并返回EAGAIN)。
 * @retval AVERROR_EOF       解码器已被刷新,不能再向其发送新数据包
 *                           (如果发送多个刷新数据包也会返回此错误)
 * @retval AVERROR(EINVAL)   编解码器未打开,它是编码器,或需要刷新
 * @retval AVERROR(ENOMEM)   无法将数据包添加到内部队列,或类似错误
 * @retval "其他负错误代码" 合法的解码错误
 */
int avcodec_send_packet(AVCodecContext *avctx, const AVPacket *avpkt);

/**
 * 从解码器或编码器(当使用@ref AV_CODEC_FLAG_RECON_FRAME标志时)返回解码后的输出数据。
 *
 * @param avctx 编解码器上下文
 * @param frame 这将被设置为由编解码器分配的引用计数的视频或音频帧(取决于解码器类型)。
 *             注意该函数在做任何其他事情之前总是会调用av_frame_unref(frame)。
 *
 * @retval 0                成功,返回了一帧
 * @retval AVERROR(EAGAIN)  在当前状态下输出不可用 - 用户必须尝试发送新的输入
 * @retval AVERROR_EOF      编解码器已完全刷新,不会再有输出帧
 * @retval AVERROR(EINVAL)  编解码器未打开,或者它是一个未启用@ref AV_CODEC_FLAG_RECON_FRAME标志的编码器
 * @retval "其他负错误代码" 合法的解码错误
 */
int avcodec_receive_frame(AVCodecContext *avctx, AVFrame *frame);

/**
 * 向编码器提供原始视频或音频帧。使用avcodec_receive_packet()检索缓冲的输出数据包。
 *
 * @param avctx     编解码器上下文
 * @param[in] frame 包含要编码的原始音频或视频帧的AVFrame。
 *                  帧的所有权保留在调用者手中,编码器不会写入该帧。编码器可能会创建对帧数据的引用
 *                  (或如果帧不是引用计数的则复制它)。
 *                  它可以为NULL,在这种情况下被视为刷新数据包。这表示流的结束。如果编码器仍有缓冲的数据包,
 *                  它将在此调用后返回它们。一旦进入刷新模式,额外的刷新数据包将被忽略,
 *                  发送帧将返回AVERROR_EOF。
 *
 *                  对于音频:
 *                  如果设置了AV_CODEC_CAP_VARIABLE_FRAME_SIZE,则每帧可以有任意数量的样本。
 *                  如果未设置,除最后一帧外,frame->nb_samples必须等于avctx->frame_size。
 *                  最后一帧可能小于avctx->frame_size。
 * @retval 0                 成功
 * @retval AVERROR(EAGAIN)   在当前状态下不接受输入 - 用户必须使用avcodec_receive_packet()读取输出
 *                           (一旦读取所有输出,应重新发送数据包,调用将不会失败并返回EAGAIN)。
 * @retval AVERROR_EOF       编码器已被刷新,不能再向其发送新帧
 * @retval AVERROR(EINVAL)   编解码器未打开,它是解码器,或需要刷新
 * @retval AVERROR(ENOMEM)   无法将数据包添加到内部队列,或类似错误
 * @retval "其他负错误代码" 合法的编码错误
 */
int avcodec_send_frame(AVCodecContext *avctx, const AVFrame *frame);

/**
 * 从编码器读取编码数据。
 *
 * @param avctx 编解码器上下文
 * @param avpkt 这将被设置为由编码器分配的引用计数数据包。注意该函数在做任何其他事情之前
 *              总是会调用av_packet_unref(avpkt)。
 * @retval 0               成功
 * @retval AVERROR(EAGAIN) 在当前状态下输出不可用 - 用户必须尝试发送输入
 * @retval AVERROR_EOF     编码器已完全刷新,不会再有输出数据包
 * @retval AVERROR(EINVAL) 编解码器未打开,或它是解码器
 * @retval "其他负错误代码" 合法的编码错误
 */
int avcodec_receive_packet(AVCodecContext *avctx, AVPacket *avpkt);

/**
 * 创建并返回一个适合硬件解码的AVHWFramesContext。这是为了在get_format回调中调用,
 * 是为AVCodecContext.hw_frames_ctx准备AVHWFramesContext的辅助函数。
 * 此API仅用于某些硬件加速模式/API的解码。
 *
 * 返回的AVHWFramesContext未初始化。调用者必须使用av_hwframe_ctx_init()进行初始化。
 *
 * 调用此函数不是必需的,但可以简化手动分配帧时避免编解码器或硬件API特定细节。
 *
 * 作为替代方案,API用户可以设置AVCodecContext.hw_device_ctx,
 * 这将完全自动设置AVCodecContext.hw_frames_ctx,并使得不必调用此函数或关心
 * AVHWFramesContext初始化。
 *
 * 调用此函数有一些要求:
 *
 * - 必须从get_format中调用它,使用传递给get_format的相同avctx参数。
 *   在get_format之外调用它是不允许的,可能会触发未定义行为。
 * - 该函数并非总是受支持(参见返回值说明)。即使此函数成功返回,hwaccel初始化
 *   也可能在后面失败。(实现检查流是否真正支持的程度各不相同。有些只在用户的
 *   get_format回调返回后才进行此检查。)
 * - hw_pix_fmt必须是get_format建议的选项之一。如果用户决定使用用此API函数
 *   准备的AVHWFramesContext,用户必须从get_format返回相同的hw_pix_fmt。
 * - 传递给此函数的device_ref必须支持给定的hw_pix_fmt。
 * - 调用此API函数后,用户有责任初始化AVHWFramesContext(由out_frames_ref参数返回),
 *   并将AVCodecContext.hw_frames_ctx设置为它。如果这样做,必须在从get_format返回之前
 *   完成(这是由正常的AVCodecContext.hw_frames_ctx API规则暗示的)。
 * - AVHWFramesContext参数可能在每次调用get_format时都会改变。此外,
 *   AVCodecContext.hw_frames_ctx在get_format之前会被重置。因此你本质上需要
 *   在每次get_format调用时重新执行此过程。
 * - 完全可以调用此函数而不实际使用结果AVHWFramesContext。一个用例可能是
 *   尝试重用先前初始化的AVHWFramesContext,并仅调用此API函数来测试所需的
 *   帧参数是否已更改。
 * - 使用任何类型的动态分配值的字段不能由用户设置,除非文档明确允许设置它们。
 *   如果用户设置AVHWFramesContext.free和AVHWFramesContext.user_opaque,
 *   新的free回调必须调用可能设置的先前free回调。此API调用可能会设置任何
 *   动态分配的字段,包括free回调。
 *
 * 该函数将在AVHWFramesContext上设置至少以下字段(可能更多,取决于hwaccel API):
 *
 * - av_hwframe_ctx_alloc()设置的所有字段。
 * - 将format字段设置为hw_pix_fmt。
 * - 将sw_format字段设置为最适合且最通用的格式。(一个含义是如果可能的话,
 *   这将优先选择通用格式而不是具有任意限制的不透明格式。)
 * - 将width/height字段设置为编码帧大小,向上舍入到API特定的最小对齐。
 * - 仅当hwaccel需要预分配池时:将initial_pool_size字段设置为使用该编解码器
 *   可能的最大参考表面数量,加上1个供用户使用的表面(意味着用户一次最多可以
 *   安全地引用1个解码的表面),再加上帧线程引入的额外缓冲。如果hwaccel不需要
 *   预分配,该字段保持为0,解码器将在解码过程中根据需要分配新的表面。
 * - 可能设置AVHWFramesContext.hwctx字段,取决于底层硬件API。
 *
 * 本质上,out_frames_ref返回与av_hwframe_ctx_alloc()相同的内容,但设置了基本帧参数。
 *
 * 该函数是无状态的,不会更改AVCodecContext或device_ref AVHWDeviceContext。
 *
 * @param avctx 当前正在调用get_format的上下文,它隐含包含填充返回的
 *              AVHWFramesContext所需的所有状态。
 * @param device_ref 对描述硬件解码器将使用的设备的AVHWDeviceContext的引用。
 * @param hw_pix_fmt 你将从get_format返回的hwaccel格式。
 * @param out_frames_ref 成功时,设置为从给定device_ref创建的未初始化AVHWFramesContext的引用。
 *                      字段将被设置为解码所需的值。如果返回错误则不会更改。
 * @return 成功时为零,错误时为负值。以下错误代码具有特殊语义:
 *      AVERROR(ENOENT): 解码器不支持此功能。设置始终是手动的,或者它是一个根本
 *                       不支持设置AVCodecContext.hw_frames_ctx的解码器,
 *                       或者它是软件格式。
 *      AVERROR(EINVAL): 已知此配置不支持硬件解码,或device_ref不支持
 *                       hw_pix_fmt引用的hwaccel。
 */
int avcodec_get_hw_frames_parameters(AVCodecContext *avctx,
                                     AVBufferRef *device_ref,
                                     enum AVPixelFormat hw_pix_fmt,
                                     AVBufferRef **out_frames_ref);

/**
 * @defgroup lavc_parsing 帧解析
 * @{
 */

enum AVPictureStructure {
    AV_PICTURE_STRUCTURE_UNKNOWN,      ///< 未知
    AV_PICTURE_STRUCTURE_TOP_FIELD,    ///< 编码为顶场
    AV_PICTURE_STRUCTURE_BOTTOM_FIELD, ///< 编码为底场
    AV_PICTURE_STRUCTURE_FRAME,        ///< 编码为帧
};

typedef struct AVCodecParserContext {
    void *priv_data;
    const struct AVCodecParser *parser;
    int64_t frame_offset; /* 当前帧的偏移量 */
    int64_t cur_offset; /* 当前偏移量
                           (每次av_parser_parse()都会增加) */
    int64_t next_frame_offset; /* 下一帧的偏移量 */
    /* 视频信息 */
    int pict_type; /* XXX: 将其放回AVCodecContext。 */
    /**
     * 此字段用于在lavf中正确计算帧持续时间。
     * 它表示当前帧的帧持续时间与正常帧持续时间相比要长多少。
     *
     * frame_duration = (1 + repeat_pict) * time_base
     *
     * 它被H.264等编解码器用来显示电视电影材料。
     */
    int repeat_pict; /* XXX: 将其放回AVCodecContext。 */
    int64_t pts;     /* 当前帧的pts */
    int64_t dts;     /* 当前帧的dts */

    /* 私有数据 */
    int64_t last_pts;
    int64_t last_dts;
    int fetch_timestamp;

#define AV_PARSER_PTS_NB 4
    int cur_frame_start_index;
    int64_t cur_frame_offset[AV_PARSER_PTS_NB];
    int64_t cur_frame_pts[AV_PARSER_PTS_NB];
    int64_t cur_frame_dts[AV_PARSER_PTS_NB];

    int flags;
#define PARSER_FLAG_COMPLETE_FRAMES           0x0001
#define PARSER_FLAG_ONCE                      0x0002
/// 如果解析器有有效的文件偏移量则设置
#define PARSER_FLAG_FETCHED_OFFSET            0x0004
#define PARSER_FLAG_USE_CODEC_TS              0x1000

    int64_t offset;      ///< 从起始数据包开始的字节偏移量
    int64_t cur_frame_end[AV_PARSER_PTS_NB];

    /**
     * 由解析器为关键帧设置为1,为非关键帧设置为0。
     * 它被初始化为-1,所以如果解析器不设置此标志,
     * 将使用旧式的回退方法,将AV_PICTURE_TYPE_I图片类型作为关键帧。
     */
    int key_frame;

    // 时间戳生成支持:
    /**
     * 时间戳生成开始的同步点。
     *
     * 对同步点设置为>0,无同步点设置为0,未定义设置为<0(默认)。
     *
     * 例如,这对应于H.264缓冲期间SEI消息的存在。
     */
    int dts_sync_point;

    /**
     * 当前时间戳相对于最后时间戳同步点的偏移量,
     * 单位为AVCodecContext.time_base。
     *
     * 当dts_sync_point未使用时设置为INT_MIN。否则,它必须
     * 包含有效的时间戳偏移量。
     *
     * 注意同步点的时间戳通常有非零的dts_ref_dts_delta,
     * 它指向前一个同步点。时间戳同步点之后的下一帧的
     * 偏移量通常为1。
     *
     * 例如,这对应于H.264 cpb_removal_delay。
     */
    int dts_ref_dts_delta;

    /**
     * 当前帧的显示延迟,单位为AVCodecContext.time_base。
     *
     * 当dts_sync_point未使用时设置为INT_MIN。否则,它必须
     * 包含有效的非负时间戳增量(帧的显示时间不能在过去)。
     *
     * 此延迟表示帧的解码时间和显示时间之间的差异。
     *
     * 例如,这对应于H.264 dpb_output_delay。
     */
    int pts_dts_delta;

    /**
     * 数据包在文件中的位置。
     *
     * 类似于cur_frame_pts/dts
     */
    int64_t cur_frame_pos[AV_PARSER_PTS_NB];

    /**
     * 当前解析的帧在流中的字节位置。
     */
    int64_t pos;

    /**
     * 前一帧的字节位置。
     */
    int64_t last_pos;

    /**
     * 当前帧的持续时间。
     * 对于音频,这以1 / AVCodecContext.sample_rate为单位。
     * 对于所有其他类型,这以AVCodecContext.time_base为单位。
     */
    int duration;

    enum AVFieldOrder field_order;

    /**
     * 指示图片是编码为帧、顶场还是底场。
     *
     * 例如,H.264 field_pic_flag等于0对应于
     * AV_PICTURE_STRUCTURE_FRAME。H.264图片的field_pic_flag
     * 等于1且bottom_field_flag等于0对应于
     * AV_PICTURE_STRUCTURE_TOP_FIELD。
     */
    enum AVPictureStructure picture_structure;

    /**
     * 按显示或输出顺序递增的图片编号。
     * 此字段可能在新序列的第一张图片时重新初始化。
     *
     * 例如,这对应于H.264 PicOrderCnt。
     */
    int output_picture_number;

    /**
     * 用于显示的解码视频的尺寸。
     */
    int width;
    int height;

    /**
     * 编码视频的尺寸。
     */
    int coded_width;
    int coded_height;

    /**
     * 编码数据的格式,对视频对应于enum AVPixelFormat,
     * 对音频对应于enum AVSampleFormat。
     *
     * 注意解码器在如何精确解码数据方面可能有相当大的自由度,
     * 所以这里报告的格式可能与解码器返回的格式不同。
     */
    int format;
} AVCodecParserContext;

typedef struct AVCodecParser {
    int codec_ids[7]; /* 允许多个编解码器ID */
    int priv_data_size;
    int (*parser_init)(AVCodecParserContext *s);
    /* 此回调永远不会返回错误,负值表示帧开始位于前一个数据包中。 */
    int (*parser_parse)(AVCodecParserContext *s,
                        AVCodecContext *avctx,
                        const uint8_t **poutbuf, int *poutbuf_size,
                        const uint8_t *buf, int buf_size);
    void (*parser_close)(AVCodecParserContext *s);
    int (*split)(AVCodecContext *avctx, const uint8_t *buf, int buf_size);
} AVCodecParser;

/**
 * 遍历所有已注册的编解码器解析器。
 *
 * @param opaque 一个指针,libavcodec将在其中存储迭代状态。必须
 *              指向NULL以开始迭代。
 *
 * @return 下一个已注册的编解码器解析器,或在迭代完成时返回NULL
 */
const AVCodecParser *av_parser_iterate(void **opaque);

AVCodecParserContext *av_parser_init(int codec_id);

/**
 * 解析一个数据包。
 *
 * @param s             解析器上下文。
 * @param avctx         编解码器上下文。
 * @param poutbuf       设置为指向已解析缓冲区的指针,如果尚未完成则为NULL。
 * @param poutbuf_size  设置为已解析缓冲区的大小,如果尚未完成则为零。
 * @param buf           输入缓冲区。
 * @param buf_size      不包含填充的缓冲区大小(以字节为单位)。即完整缓冲区
 *                      大小假定为buf_size + AV_INPUT_BUFFER_PADDING_SIZE。
 *                      要发出EOF信号,这应该为0(以便输出最后一帧)。
 * @param pts           输入显示时间戳。
 * @param dts           输入解码时间戳。
 * @param pos           输入流中的字节位置。
 * @return 使用的输入比特流的字节数。
 *
 * 示例:
 * @code
 *   while(in_len){
 *       len = av_parser_parse2(myparser, AVCodecContext, &data, &size,
 *                                        in_data, in_len,
 *                                        pts, dts, pos);
 *       in_data += len;
 *       in_len  -= len;
 *
 *       if(size)
 *          decode_frame(data, size);
 *   }
 * @endcode
 */
int av_parser_parse2(AVCodecParserContext *s,
                     AVCodecContext *avctx,
                     uint8_t **poutbuf, int *poutbuf_size,
                     const uint8_t *buf, int buf_size,
                     int64_t pts, int64_t dts,
                     int64_t pos);

void av_parser_close(AVCodecParserContext *s);

/**
 * @}
 * @}
 */

/**
 * @addtogroup lavc_encoding
 * @{
 */

int avcodec_encode_subtitle(AVCodecContext *avctx, uint8_t *buf, int buf_size,
                            const AVSubtitle *sub);

/**
 * @}
 */

/**
 * @defgroup lavc_misc 实用函数
 * @ingroup libavc
 *
 * 与编码和解码(或两者都不)相关的杂项实用函数。
 * @{
 */

/**
 * @defgroup lavc_misc_pixfmt 像素格式
 *
 * 用于处理像素格式的函数。
 * @{
 */

/**
 * 返回与像素格式pix_fmt关联的fourCC代码值,
 * 如果找不到关联的fourCC代码则返回0。
 */
unsigned int avcodec_pix_fmt_to_codec_tag(enum AVPixelFormat pix_fmt);

/**
 * 在给定源像素格式的情况下,找到最佳的转换目标像素格式。
 * 从一种像素格式转换为另一种像素格式时,可能会发生信息丢失。
 * 例如,从RGB24转换为GRAY时,颜色信息将丢失。
 * 同样,从某些格式转换为其他格式时也会发生其他损失。
 * avcodec_find_best_pix_fmt_of_2()搜索应该使用哪种给定的像素格式
 * 以遭受最少的损失。它从中选择一种像素格式由pix_fmt_list参数确定。
 *
 * @param[in] pix_fmt_list 以AV_PIX_FMT_NONE结尾的可选像素格式数组
 * @param[in] src_pix_fmt 源像素格式
 * @param[in] has_alpha 源像素格式的alpha通道是否使用。
 * @param[out] loss_ptr 标志组合,告知您将发生何种损失。
 * @return 最佳的转换目标像素格式,如果未找到则返回-1。
 */
enum AVPixelFormat avcodec_find_best_pix_fmt_of_list(const enum AVPixelFormat *pix_fmt_list,
                                            enum AVPixelFormat src_pix_fmt,
                                            int has_alpha, int *loss_ptr);

enum AVPixelFormat avcodec_default_get_format(struct AVCodecContext *s, const enum AVPixelFormat * fmt);

/**
 * @}
 */

void avcodec_string(char *buf, int buf_size, AVCodecContext *enc, int encode);

int avcodec_default_execute(AVCodecContext *c, int (*func)(AVCodecContext *c2, void *arg2),void *arg, int *ret, int count, int size);
int avcodec_default_execute2(AVCodecContext *c, int (*func)(AVCodecContext *c2, void *arg2, int, int),void *arg, int *ret, int count);
//FIXME func typedef

/**
 * 填充AVFrame音频数据和linesize指针。
 *
 * 缓冲区buf必须是预先分配的,其大小足够容纳指定的样本数量。
 * 填充的AVFrame数据指针将指向此缓冲区。
 *
 * 如果需要,将为平面音频分配AVFrame extended_data通道指针。
 *
 * @param frame       AVFrame
 *                    在调用函数之前必须设置frame->nb_samples。
 *                    此函数填充frame->data,
 *                    frame->extended_data, frame->linesize[0]。
 * @param nb_channels 通道数
 * @param sample_fmt  采样格式
 * @param buf         用于帧数据的缓冲区
 * @param buf_size    缓冲区大小
 * @param align       平面大小样本对齐(0 = 默认)
 * @return           成功时>=0,失败时为负错误代码
 * @todo 在下一个libavutil版本中,成功时返回存储样本所需的字节大小
 */
int avcodec_fill_audio_frame(AVFrame *frame, int nb_channels,
                             enum AVSampleFormat sample_fmt, const uint8_t *buf,
                             int buf_size, int align);

/**
 * 重置内部编解码器状态/刷新内部缓冲区。应该在例如寻找或切换到不同流时调用。
 *
 * @note 对于解码器,此函数仅释放解码器可能在内部保持的任何引用,
 * 但调用者的引用仍然有效。
 *
 * @note 对于编码器,此函数仅在编码器声明支持AV_CODEC_CAP_ENCODER_FLUSH时才会执行操作。
 * 调用时,编码器将耗尽任何剩余的数据包,然后可以重新用于不同的流
 * (与发送空帧相反,空帧会在耗尽后使编码器处于永久EOF状态)。
 * 如果拆除和替换编码器实例的成本很高,这可能是可取的。
 */
void avcodec_flush_buffers(AVCodecContext *avctx);

/**
 * 返回音频帧持续时间。
 *
 * @param avctx        编解码器上下文
 * @param frame_bytes  帧大小,如果未知则为0
 * @return            如果已知,则返回帧持续时间(以样本为单位)。如果无法确定则返回0。
 */
int av_get_audio_frame_duration(AVCodecContext *avctx, int frame_bytes);

/* 内存 */

/**
 * 与av_fast_malloc行为相同,但缓冲区末尾有额外的
 * AV_INPUT_BUFFER_PADDING_SIZE,它将始终为0。
 *
 * 此外,整个缓冲区在初始化和调整大小后都将被初始化为0,
 * 因此永远不会出现未初始化的数据。
 */
void av_fast_padded_malloc(void *ptr, unsigned int *size, size_t min_size);

/**
 * 与av_fast_padded_malloc行为相同,但缓冲区在调用后
 * 将始终被初始化为0。
 */
void av_fast_padded_mallocz(void *ptr, unsigned int *size, size_t min_size);

/**
 * @return 如果s是打开的(即在其上调用了avcodec_open2()且没有相应的avcodec_close()),
 * 则返回正值,否则返回0。
 */
int avcodec_is_open(AVCodecContext *s);

/**
 * @}
 */

#endif /* AVCODEC_AVCODEC_H */