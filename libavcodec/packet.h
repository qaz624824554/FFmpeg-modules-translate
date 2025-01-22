#ifndef AVCODEC_PACKET_H
#define AVCODEC_PACKET_H

#include <stddef.h>
#include <stdint.h>

#include "libavutil/attributes.h"
#include "libavutil/buffer.h"
#include "libavutil/dict.h"
#include "libavutil/rational.h"
#include "libavutil/version.h"

#include "libavcodec/version_major.h"

/**
 * @defgroup lavc_packet_side_data AVPacketSideData
 *
 * 用于处理AVPacketSideData的类型和函数。
 * @{
 */
enum AVPacketSideDataType {
    /**
     * AV_PKT_DATA_PALETTE 边数据包包含正好 AVPALETTE_SIZE 字节的调色板。
     * 此边数据表示存在新的调色板。
     */
    AV_PKT_DATA_PALETTE,

    /**
     * AV_PKT_DATA_NEW_EXTRADATA 用于通知编解码器或格式,extradata缓冲区已更改,
     * 接收方应相应地处理。新的extradata嵌入在边数据缓冲区中,
     * 应立即用于处理当前帧或数据包。
     */
    AV_PKT_DATA_NEW_EXTRADATA,

    /**
     * AV_PKT_DATA_PARAM_CHANGE 边数据包的布局如下:
     * @code
     * u32le param_flags
     * if (param_flags & AV_SIDE_DATA_PARAM_CHANGE_CHANNEL_COUNT)
     *     s32le channel_count
     * if (param_flags & AV_SIDE_DATA_PARAM_CHANGE_CHANNEL_LAYOUT)
     *     u64le channel_layout
     * if (param_flags & AV_SIDE_DATA_PARAM_CHANGE_SAMPLE_RATE)
     *     s32le sample_rate
     * if (param_flags & AV_SIDE_DATA_PARAM_CHANGE_DIMENSIONS)
     *     s32le width
     *     s32le height
     * @endcode
     */
    AV_PKT_DATA_PARAM_CHANGE,

    /**
     * AV_PKT_DATA_H263_MB_INFO 边数据包包含多个结构体,其中包含有关宏块的信息,
     * 这些信息与在宏块边界上将数据包拆分为较小的数据包有关(例如用于RFC 2190)。
     * 也就是说,它不一定包含所有宏块的信息,只要信息中宏块之间的距离小于目标有效负载大小即可。
     * 每个MB信息结构为12字节,布局如下:
     * @code
     * u32le 从数据包开始的位偏移量
     * u8    宏块开始时的当前量化器
     * u8    GOB编号
     * u16le GOB内的宏块地址
     * u8    水平MV预测器
     * u8    垂直MV预测器
     * u8    块3的水平MV预测器
     * u8    块3的垂直MV预测器
     * @endcode
     */
    AV_PKT_DATA_H263_MB_INFO,

    /**
     * 此边数据应与音频流关联,并以AVReplayGain结构的形式包含回放增益信息。
     */
    AV_PKT_DATA_REPLAYGAIN,

    /**
     * 此边数据包含一个3x3变换矩阵,描述了需要应用于解码视频帧以正确显示的仿射变换。
     *
     * 有关数据的详细说明,请参见libavutil/display.h。
     */
    AV_PKT_DATA_DISPLAYMATRIX,

    /**
     * 此边数据应与视频流关联,并以AVStereo3D结构的形式包含立体3D信息。
     */
    AV_PKT_DATA_STEREO3D,

    /**
     * 此边数据应与音频流关联,对应于enum AVAudioServiceType。
     */
    AV_PKT_DATA_AUDIO_SERVICE_TYPE,

    /**
     * 此边数据包含来自编码器的质量相关信息。
     * @code
     * u32le 压缩帧的质量因子。允许范围在1(好)和FF_LAMBDA_MAX(差)之间。
     * u8    图片类型
     * u8    错误计数
     * u16   保留
     * u64le[error count] 编码器输入和输出之间的平方差之和
     * @endcode
     */
    AV_PKT_DATA_QUALITY_STATS,

    /**
     * 此边数据包含一个整数值,表示"后备"轨道的流索引。
     * 后备轨道表示当当前轨道由于某种原因无法解码时使用的备用轨道。
     * 例如,没有可用的编解码器。
     */
    AV_PKT_DATA_FALLBACK_TRACK,

    /**
     * 此边数据对应于AVCPBProperties结构。
     */
    AV_PKT_DATA_CPB_PROPERTIES,

    /**
     * 建议跳过指定数量的样本
     * @code
     * u32le 从此数据包开始跳过的样本数
     * u32le 从此数据包结尾跳过的样本数
     * u8    开始跳过的原因
     * u8    结束跳过的原因(0=填充静音,1=收敛)
     * @endcode
     */
    AV_PKT_DATA_SKIP_SAMPLES,

    /**
     * AV_PKT_DATA_JP_DUALMONO 边数据包表示数据包可能包含特定于日本DTV的"双声道"音频,
     * 如果为真,建议仅使用所选通道。
     * @code
     * u8    所选通道(0=主/左,1=副/右,2=两者)
     * @endcode
     */
    AV_PKT_DATA_JP_DUALMONO,

    /**
     * 以零结尾的键/值字符串列表。列表没有结束标记,
     * 因此需要依赖边数据大小来停止。
     */
    AV_PKT_DATA_STRINGS_METADATA,

    /**
     * 字幕事件位置
     * @code
     * u32le x1
     * u32le y1
     * u32le x2
     * u32le y2
     * @endcode
     */
    AV_PKT_DATA_SUBTITLE_POSITION,

    /**
     * matroska容器的BlockAdditional元素中找到的数据。
     * 数据没有结束标记,因此需要依赖边数据大小来识别结束。
     * 8字节id(在BlockAddId中找到)后跟数据。
     */
    AV_PKT_DATA_MATROSKA_BLOCKADDITIONAL,

    /**
     * WebVTT提示的可选第一个标识符行。
     */
    AV_PKT_DATA_WEBVTT_IDENTIFIER,

    /**
     * 紧跟在WebVTT提示的时间戳说明符之后的可选设置(渲染说明)。
     */
    AV_PKT_DATA_WEBVTT_SETTINGS,

    /**
     * 以零结尾的键/值字符串列表。列表没有结束标记,
     * 因此需要依赖边数据大小来停止。此边数据包括流中出现的更新元数据。
     */
    AV_PKT_DATA_METADATA_UPDATE,

    /**
     * MPEGTS流ID作为uint8_t,这是从解复用器向相应的复用器传递流ID信息所必需的。
     */
    AV_PKT_DATA_MPEGTS_STREAM_ID,

    /**
     * 主显示元数据(基于SMPTE-2086:2014)。此元数据应与视频流关联,
     * 并以AVMasteringDisplayMetadata结构的形式包含数据。
     */
    AV_PKT_DATA_MASTERING_DISPLAY_METADATA,

    /**
     * 此边数据应与视频流关联,对应于AVSphericalMapping结构。
     */
    AV_PKT_DATA_SPHERICAL,

    /**
     * 内容光级别(基于CTA-861.3)。此元数据应与视频流关联,
     * 并以AVContentLightMetadata结构的形式包含数据。
     */
    AV_PKT_DATA_CONTENT_LIGHT_LEVEL,

    /**
     * ATSC A53 Part 4闭路字幕。此元数据应与视频流关联。
     * A53 CC比特流作为uint8_t存储在AVPacketSideData.data中。
     * CC数据的字节数是AVPacketSideData.size。
     */
    AV_PKT_DATA_A53_CC,

    /**
     * 此边数据是加密初始化数据。
     * 格式不是ABI的一部分,使用av_encryption_init_info_*方法访问。
     */
    AV_PKT_DATA_ENCRYPTION_INIT_INFO,

    /**
     * 此边数据包含有关如何解密数据包的加密信息。
     * 格式不是ABI的一部分,使用av_encryption_info_*方法访问。
     */
    AV_PKT_DATA_ENCRYPTION_INFO,

    /**
     * 活动格式描述数据,由ETSI TS 101 154使用AVActiveFormatDescription枚举指定的单个字节组成。
     */
    AV_PKT_DATA_AFD,

    /**
     * 生产者参考时间数据对应于AVProducerReferenceTime结构,
     * 通常由某些编码器导出(通过在AVCodecContext export_side_data字段中设置的prft标志按需导出)。
     */
    AV_PKT_DATA_PRFT,

    /**
     * ICC配置文件数据,由遵循ISO 15076-1描述的格式的不透明八位字节缓冲区组成。
     */
    AV_PKT_DATA_ICC_PROFILE,

    /**
     * DOVI配置
     * 参考:
     * dolby-vision-bitstreams-within-the-iso-base-media-file-format-v2.1.2,第2.2节
     * dolby-vision-bitstreams-in-mpeg-2-transport-stream-multiplex-v1.2,第3.3节
     * 标签存储在struct AVDOVIDecoderConfigurationRecord中。
     */
    AV_PKT_DATA_DOVI_CONF,

    /**
     * 符合SMPTE ST 12-1:2014的时间码。数据是一个4个uint32_t的数组,
     * 其中第一个uint32_t描述使用了多少个(1-3)其他时间码。
     * 时间码格式在libavutil/timecode.h中的av_timecode_get_smpte_from_framenum()
     * 函数的文档中描述。
     */
    AV_PKT_DATA_S12M_TIMECODE,

    /**
     * 与视频帧关联的HDR10+动态元数据。元数据采用AVDynamicHDRPlus结构的形式,
     * 包含色彩体积变换的信息 - SMPTE 2094-40:2016标准的应用4。
     */
    AV_PKT_DATA_DYNAMIC_HDR10_PLUS,

    /**
     * 边数据类型的数量。
     * 这不是公共API/ABI的一部分,因为在添加新的边数据类型时可能会更改。
     * 这必须保持为最后一个枚举值。
     * 如果其值变得很大,则需要更新使用它的某些代码,
     * 因为它假定它小于其他限制。
     */
    AV_PKT_DATA_NB
};

#define AV_PKT_DATA_QUALITY_FACTOR AV_PKT_DATA_QUALITY_STATS //已弃用

/**
 * 此结构存储用于解码、显示或以其他方式处理编码流的辅助信息。
 * 它通常由解复用器和编码器导出,可以按每个数据包的方式提供给解码器和复用器,
 * 或作为全局边数据(应用于整个编码流)。
 *
 * 全局边数据的处理方式如下:
 * - 在解复用期间,它可以通过@ref AVStream.codecpar.side_data "AVStream的编解码器参数"导出,
 *   然后可以通过@ref AVCodecContext.coded_side_data "解码器上下文的边数据"作为输入传递给解码器,
 *   用于初始化。
 * - 对于复用,它可以通过@ref AVStream.codecpar.side_data "AVStream的编解码器参数"提供,
 *   通常是通过@ref AVCodecContext.coded_side_data "编码器上下文的边数据"从编码器输出,
 *   用于初始化。
 *
 * 数据包特定的边数据处理方式如下:
 * - 在解复用期间,它可以通过@ref AVPacket.side_data "AVPacket的边数据"导出,
 *   然后可以作为输入传递给解码器。
 * - 对于复用,它可以通过@ref AVPacket.side_data "AVPacket的边数据"提供,
 *   通常是编码器的输出。
 *
 * 不同的模块可能会根据媒体类型和编解码器接受或导出不同类型的边数据。
 * 有关已定义类型的列表以及可以在何处找到或使用它们,请参阅@ref AVPacketSideDataType。
 */
typedef struct AVPacketSideData {
    uint8_t *data;
    size_t   size;
    enum AVPacketSideDataType type;
} AVPacketSideData;

/**
 * 分配一个新的数据包边数据。
 *
 * @param sd    指向应添加边数据的边数据数组的指针。*sd可能为NULL,
 *              在这种情况下将初始化数组。
 * @param nb_sd 指向包含数组中条目数的整数的指针。成功时整数值将增加1。
 * @param type  边数据类型
 * @param size  所需的边数据大小
 * @param flags 当前未使用。必须为零
 *
 * @return 成功时返回新分配的边数据的指针,否则返回NULL。
 */
AVPacketSideData *av_packet_side_data_new(AVPacketSideData **psd, int *pnb_sd,
                                          enum AVPacketSideDataType type,
                                          size_t size, int flags);

/**
 * 将现有数据包装为数据包边数据。
 *
 * @param sd    指向应添加边数据的边数据数组的指针。*sd可能为NULL,
 *              在这种情况下将初始化数组
 * @param nb_sd 指向包含数组中条目数的整数的指针。成功时整数值将增加1。
 * @param type  边数据类型
 * @param data  数据数组。它必须使用av_malloc()系列函数分配。
 *             成功时数据的所有权将转移到边数据数组。
 * @param size  边数据大小
 * @param flags 当前未使用。必须为零
 *
 * @return 成功时返回非负数,失败时返回负AVERROR代码。
 *         失败时,数据包保持不变,数据仍由调用者拥有。
 */
AVPacketSideData *av_packet_side_data_add(AVPacketSideData **sd, int *nb_sd,
                                          enum AVPacketSideDataType type,
                                          void *data, size_t size, int flags);

/**
 * 从边数据数组中获取边信息。
 *
 * @param sd    要从中获取边数据的数组
 * @param nb_sd 包含数组中条目数的值。
 * @param type  所需的边信息类型
 *
 * @return 如果存在则返回指向数据的指针,否则返回NULL
 */
const AVPacketSideData *av_packet_side_data_get(const AVPacketSideData *sd,
                                                int nb_sd,
                                                enum AVPacketSideDataType type);

/**
 * 从边数据数组中删除给定类型的边数据。
 *
 * @param sd    要从中删除边数据的数组
 * @param nb_sd 指向包含数组中条目数的整数的指针。返回时将减少删除的条目数
 * @param type  边信息类型
 */
void av_packet_side_data_remove(AVPacketSideData *sd, int *nb_sd,
                                enum AVPacketSideDataType type);

/**
 * 释放存储在数组中的所有边数据以及数组本身的便利函数。
 *
 * @param sd    指向要释放的边数据数组的指针。返回时将设置为NULL。
 * @param nb_sd 指向包含数组中条目数的整数的指针。返回时将设置为0。
 */
void av_packet_side_data_free(AVPacketSideData **sd, int *nb_sd);

const char *av_packet_side_data_name(enum AVPacketSideDataType type);

/**
 * @}
 */

/**
 * @defgroup lavc_packet AVPacket
 *
 * 用于处理AVPacket的类型和函数。
 * @{
 */

/**
 * 此结构存储压缩数据。它通常由解复用器导出,然后作为输入传递给解码器,
 * 或从编码器接收输出,然后传递给复用器。
 *
 * 对于视频,它通常应包含一个压缩帧。对于音频,它可能包含几个压缩帧。
 * 编码器允许输出空数据包,不包含压缩数据,只包含边数据
 * (例如,在编码结束时更新某些流参数)。
 *
 * 数据所有权的语义取决于buf字段。
 * 如果设置了该字段,则数据包数据是动态分配的,在调用av_packet_unref()
 * 将引用计数减少到0之前一直有效。
 *
 * 如果未设置buf字段,av_packet_ref()将进行复制而不是增加引用计数。
 *
 * 边数据始终使用av_malloc()分配,由av_packet_ref()复制,
 * 由av_packet_unref()释放。
 *
 * sizeof(AVPacket)作为公共ABI的一部分已弃用。一旦删除av_init_packet(),
 * 新的数据包将只能使用av_packet_alloc()分配,并且可以在结构末尾添加新字段,
 * 并进行次要版本升级。
 *
 * @see av_packet_alloc
 * @see av_packet_ref
 * @see av_packet_unref
 */
typedef struct AVPacket {
    /**
     * 对存储数据包数据的引用计数缓冲区的引用。
     * 可能为NULL,则数据包数据不是引用计数的。
     */
    AVBufferRef *buf;
    /**
     * 以AVStream->time_base为单位的显示时间戳;
     * 解压缩的数据包将呈现给用户的时间。
     * 如果文件中未存储,可以是AV_NOPTS_VALUE。
     * pts必须大于或等于dts,因为显示不能在解压缩之前发生,
     * 除非想查看十六进制转储。某些格式滥用dts和pts/cts术语来表示不同的含义。
     * 在将这些时间戳存储在AVPacket之前,必须将其转换为真正的pts/dts。
     */
    int64_t pts;
    /**
     * 以AVStream->time_base为单位的解压缩时间戳;
     * 数据包解压缩的时间。
     * 如果文件中未存储,可以是AV_NOPTS_VALUE。
     */
    int64_t dts;
    uint8_t *data;
    int   size;
    int   stream_index;
    /**
     * AV_PKT_FLAG值的组合
     */
    int   flags;
    /**
     * 容器可以提供的附加数据包数据。
     * 数据包可以包含几种类型的边信息。
     */
    AVPacketSideData *side_data;
    int side_data_elems;

    /**
     * 以AVStream->time_base为单位的数据包持续时间,如果未知则为0。
     * 在显示顺序中等于next_pts - this_pts。
     */
    int64_t duration;

    int64_t pos;                            ///< 流中的字节位置,-1表示未知

    /**
     * API用户的一些私有数据
     */
    void *opaque;

    /**
     * 供API用户自由使用的AVBufferRef。FFmpeg永远不会检查缓冲区引用的内容。
     * 当数据包未引用时,FFmpeg会在其上调用av_buffer_unref()。
     * av_packet_copy_props()调用为目标数据包的opaque_ref字段使用av_buffer_ref()
     * 创建新引用。
     *
     * 这与opaque字段无关,尽管它服务于类似的目的。
     */
    AVBufferRef *opaque_ref;

    /**
     * 数据包时间戳的时间基准。
     * 将来,此字段可能会在编码器或解复用器输出的数据包上设置,
     * 但默认情况下,其值在输入到解码器或复用器时将被忽略。
     */
    AVRational time_base;
} AVPacket;

#if FF_API_INIT_PACKET
attribute_deprecated
typedef struct AVPacketList {
    AVPacket pkt;
    struct AVPacketList *next;
} AVPacketList;
#endif

#define AV_PKT_FLAG_KEY     0x0001 ///< 数据包包含关键帧
#define AV_PKT_FLAG_CORRUPT 0x0002 ///< 数据包内容已损坏
/**
 * 标志用于丢弃维护有效解码器状态所需但不需要输出的数据包,
 * 这些数据包在解码后应该丢弃。
 **/
#define AV_PKT_FLAG_DISCARD   0x0004
/**
 * 数据包来自可信来源。
 *
 * 否则不安全的构造(如指向数据包外部数据的任意指针)可能会被跟踪。
 */
#define AV_PKT_FLAG_TRUSTED   0x0008
/**
 * 标志用于指示包含可由解码器丢弃的帧的数据包。
 * 即非参考帧。
 */
#define AV_PKT_FLAG_DISPOSABLE 0x0010

enum AVSideDataParamChangeFlags {
#if FF_API_OLD_CHANNEL_LAYOUT
    /**
     * @已弃用 这些不被任何解码器使用
     */
    AV_SIDE_DATA_PARAM_CHANGE_CHANNEL_COUNT  = 0x0001,
    AV_SIDE_DATA_PARAM_CHANGE_CHANNEL_LAYOUT = 0x0002,
#endif
    AV_SIDE_DATA_PARAM_CHANGE_SAMPLE_RATE    = 0x0004,
    AV_SIDE_DATA_PARAM_CHANGE_DIMENSIONS     = 0x0008,
};

/**
 * 分配一个AVPacket并将其字段设置为默认值。
 * 生成的结构必须使用av_packet_free()释放。
 *
 * @return 填充了默认值的AVPacket,失败时返回NULL。
 *
 * @note 这只分配AVPacket本身,而不是数据缓冲区。
 * 这些必须通过其他方式分配,如av_new_packet。
 *
 * @see av_new_packet
 */
AVPacket *av_packet_alloc(void);

/**
 * 创建一个引用与src相同数据的新数据包。
 *
 * 这是av_packet_alloc()+av_packet_ref()的快捷方式。
 *
 * @return 成功时返回新创建的AVPacket,失败时返回NULL。
 *
 * @see av_packet_alloc
 * @see av_packet_ref
 */
AVPacket *av_packet_clone(const AVPacket *src);

/**
 * 释放数据包,如果数据包是引用计数的,它将首先取消引用。
 *
 * @param pkt 要释放的数据包。指针将设置为NULL。
 * @note 传递NULL是无操作。
 */
void av_packet_free(AVPacket **pkt);

#if FF_API_INIT_PACKET
/**
 * 使用默认值初始化数据包的可选字段。
 *
 * 注意,这不会触及data和size成员,它们必须单独初始化。
 *
 * @param pkt 数据包
 *
 * @see av_packet_alloc
 * @see av_packet_unref
 *
 * @已弃用 此函数已弃用。一旦删除它,
 *         sizeof(AVPacket)将不再是ABI的一部分。
 */
attribute_deprecated
void av_init_packet(AVPacket *pkt);
#endif

/**
 * 分配数据包的有效负载并初始化其字段为默认值。
 *
 * @param pkt 数据包
 * @param size 所需的有效负载大小
 * @return 成功时返回0,否则返回AVERROR_xxx
 */
int av_new_packet(AVPacket *pkt, int size);

/**
 * 减小数据包大小,正确清零填充
 *
 * @param pkt 数据包
 * @param size 新大小
 */
void av_shrink_packet(AVPacket *pkt, int size);

/**
 * 增加数据包大小,正确清零填充
 *
 * @param pkt 数据包
 * @param grow_by 要增加数据包大小的字节数
 */
int av_grow_packet(AVPacket *pkt, int grow_by);

/**
 * 从av_malloc()分配的数据初始化引用计数的数据包。
 *
 * @param pkt 要初始化的数据包。此函数将设置data、size和
 *        buf字段,所有其他字段保持不变。
 * @param data 由av_malloc()分配用作数据包数据的数据。如果此函数
 *        成功返回,则数据由底层AVBuffer拥有。调用者可能不会
 *        通过其他方式访问数据。
 * @param size 数据的字节大小,不包括填充。即完整缓冲区大小
 *        假定为size + AV_INPUT_BUFFER_PADDING_SIZE。
 *
 * @return 成功时返回0,失败时返回负AVERROR
 */
int av_packet_from_data(AVPacket *pkt, uint8_t *data, int size);

/**
 * 分配数据包的新信息。
 *
 * @param pkt 数据包
 * @param type 边信息类型
 * @param size 边信息大小
 * @return 指向新分配数据的指针,否则返回NULL
 */
uint8_t* av_packet_new_side_data(AVPacket *pkt, enum AVPacketSideDataType type,
                                 size_t size);

/**
 * 将现有数组包装为数据包的边信息。
 *
 * @param pkt 数据包
 * @param type 边信息类型
 * @param data 边信息数组。必须使用av_malloc()系列函数分配。
 *            数据的所有权将转移给pkt。
 * @param size 边信息大小
 * @return 成功时返回非负数,失败时返回负的AVERROR代码。
 *         失败时,数据包保持不变,数据仍由调用者拥有。
 */
int av_packet_add_side_data(AVPacket *pkt, enum AVPacketSideDataType type,
                            uint8_t *data, size_t size);

/**
 * 缩小已分配的边信息缓冲区
 *
 * @param pkt 数据包
 * @param type 边信息类型
 * @param size 新的边信息大小
 * @return 成功时返回0,失败时返回<0
 */
int av_packet_shrink_side_data(AVPacket *pkt, enum AVPacketSideDataType type,
                               size_t size);

/**
 * 从数据包获取边信息。
 *
 * @param pkt 数据包
 * @param type 所需的边信息类型
 * @param size 如果提供,*size将被设置为边信息的大小,
 *             如果所需的边信息不存在则设为零。
 * @return 如果存在则返回数据指针,否则返回NULL
 */
uint8_t* av_packet_get_side_data(const AVPacket *pkt, enum AVPacketSideDataType type,
                                 size_t *size);

/**
 * 打包字典以在side_data中使用。
 *
 * @param dict 要打包的字典
 * @param size 用于存储返回数据大小的指针
 * @return 成功时返回数据指针,否则返回NULL
 */
uint8_t *av_packet_pack_dictionary(AVDictionary *dict, size_t *size);
/**
 * 从side_data中解包字典。
 *
 * @param data 来自side_data的数据
 * @param size 数据的大小
 * @param dict 元数据存储字典
 * @return 成功时返回0,失败时返回<0
 */
int av_packet_unpack_dictionary(const uint8_t *data, size_t size,
                                AVDictionary **dict);

/**
 * 释放所有存储的边信息的便利函数。
 * 所有其他字段保持不变。
 *
 * @param pkt 数据包
 */
void av_packet_free_side_data(AVPacket *pkt);

/**
 * 为给定数据包描述的数据设置新的引用
 *
 * 如果src是引用计数的,则将dst设置为src中缓冲区的新引用。
 * 否则在dst中分配新缓冲区并将src中的数据复制到其中。
 *
 * 所有其他字段都从src复制。
 *
 * @see av_packet_unref
 *
 * @param dst 目标数据包。将被完全覆盖。
 * @param src 源数据包
 *
 * @return 成功时返回0,错误时返回负的AVERROR。出错时,dst
 *         将被清空(如同av_packet_alloc()返回的一样)。
 */
int av_packet_ref(AVPacket *dst, const AVPacket *src);

/**
 * 清除数据包。
 *
 * 取消引用数据包引用的缓冲区,并将剩余的数据包字段重置为默认值。
 *
 * @param pkt 要取消引用的数据包。
 */
void av_packet_unref(AVPacket *pkt);

/**
 * 将src中的每个字段移动到dst并重置src。
 *
 * @see av_packet_unref
 *
 * @param src 源数据包,将被重置
 * @param dst 目标数据包
 */
void av_packet_move_ref(AVPacket *dst, AVPacket *src);

/**
 * 仅将src的"属性"字段复制到dst。
 *
 * 此函数的属性是除与数据包数据(buf、data、size)相关的字段之外的所有字段
 *
 * @param dst 目标数据包
 * @param src 源数据包
 *
 * @return 成功时返回0,失败时返回AVERROR。
 */
int av_packet_copy_props(AVPacket *dst, const AVPacket *src);

/**
 * 确保给定数据包描述的数据是引用计数的。
 *
 * @note 此函数不确保引用将是可写的。
 *       为此目的请使用av_packet_make_writable。
 *
 * @see av_packet_ref
 * @see av_packet_make_writable
 *
 * @param pkt 其数据应该被引用计数的数据包。
 *
 * @return 成功时返回0,错误时返回负的AVERROR。失败时,
 *         数据包保持不变。
 */
int av_packet_make_refcounted(AVPacket *pkt);

/**
 * 为给定数据包描述的数据创建可写引用,
 * 尽可能避免数据复制。
 *
 * @param pkt 其数据应该变为可写的数据包。
 *
 * @return 成功时返回0,错误时返回负的AVERROR。失败时,
 *         数据包保持不变。
 */
int av_packet_make_writable(AVPacket *pkt);

/**
 * 将数据包中的有效时间字段(时间戳/持续时间)从一个时基转换到另一个时基。
 * 具有未知值(AV_NOPTS_VALUE)的时间戳将被忽略。
 *
 * @param pkt 将进行转换的数据包
 * @param tb_src 源时基,pkt中的时间字段以此表示
 * @param tb_dst 目标时基,时间字段将被转换到此时基
 */
void av_packet_rescale_ts(AVPacket *pkt, AVRational tb_src, AVRational tb_dst);

/**
 * @}
 */

#endif // AVCODEC_PACKET_H
