/**
 * @file
 * @ingroup lavu_frame
 * 引用计数帧 API
 */

#ifndef AVUTIL_FRAME_H
#define AVUTIL_FRAME_H

#include <stddef.h>
#include <stdint.h>

#include "avutil.h"
#include "buffer.h"
#include "channel_layout.h"
#include "dict.h"
#include "rational.h"
#include "samplefmt.h"
#include "pixfmt.h"
#include "version.h"

/**
 * @defgroup lavu_frame AVFrame
 * @ingroup lavu_data
 *
 * @{
 * AVFrame 是一个引用计数的原始多媒体数据的抽象。
 */

enum AVFrameSideDataType {
    /**
     * 数据是 libavcodec 中定义的 AVPanScan 结构体。
     */
    AV_FRAME_DATA_PANSCAN,
    /**
     * ATSC A53 Part 4 隐藏字幕。
     * A53 CC 比特流以 uint8_t 形式存储在 AVFrameSideData.data 中。
     * CC 数据的字节数是 AVFrameSideData.size。
     */
    AV_FRAME_DATA_A53_CC,
    /**
     * 立体 3D 元数据。
     * 数据是 libavutil/stereo3d.h 中定义的 AVStereo3D 结构体。
     */
    AV_FRAME_DATA_STEREO3D,
    /**
     * 数据是 libavutil/channel_layout.h 中定义的 AVMatrixEncoding 枚举。
     */
    AV_FRAME_DATA_MATRIXENCODING,
    /**
     * 与降混过程相关的元数据。
     * 数据是 libavutil/downmix_info.h 中定义的 AVDownmixInfo 结构体。
     */
    AV_FRAME_DATA_DOWNMIX_INFO,
    /**
     * 以 AVReplayGain 结构体形式的回放增益信息。
     */
    AV_FRAME_DATA_REPLAYGAIN,
    /**
     * 此边数据包含一个 3x3 变换矩阵，描述了需要应用于帧以正确显示的仿射变换。
     *
     * 详细的数据描述请参见 libavutil/display.h。
     */
    AV_FRAME_DATA_DISPLAYMATRIX,
    /**
     * 活动格式描述数据，由单个字节组成，
     * 按照 ETSI TS 101 154 使用 AVActiveFormatDescription 枚举指定。
     */
    AV_FRAME_DATA_AFD,
    /**
     * 由某些编解码器导出的运动向量(通过在 libavcodec AVCodecContext flags2 选项中设置的 export_mvs 标志按需导出)。
     * 数据是 libavutil/motion_vector.h 中定义的 AVMotionVector 结构体。
     */
    AV_FRAME_DATA_MOTION_VECTORS,
    /**
     * 建议跳过指定数量的样本。仅当在 libavcodec 中设置了 "skip_manual" AVOption 时才会导出。
     * 这与 AV_PKT_DATA_SKIP_SAMPLES 具有相同的格式。
     * @code
     * u32le 从此数据包开始跳过的样本数
     * u32le 从此数据包结尾跳过的样本数
     * u8    开始跳过的原因
     * u8    结束跳过的原因(0=填充静音，1=收敛)
     * @endcode
     */
    AV_FRAME_DATA_SKIP_SAMPLES,
    /**
     * 此边数据必须与音频帧关联，对应于 avcodec.h 中定义的 AVAudioServiceType 枚举。
     */
    AV_FRAME_DATA_AUDIO_SERVICE_TYPE,
    /**
     * 与视频帧关联的主显示元数据。有效载荷是 AVMasteringDisplayMetadata 类型，
     * 包含有关主显示色彩体积的信息。
     */
    AV_FRAME_DATA_MASTERING_DISPLAY_METADATA,
    /**
     * 25 位时间码格式的 GOP 时间码。数据格式为 64 位整数。
     * 这在 GOP 中时间参考为 0 的第一帧上设置。
     */
    AV_FRAME_DATA_GOP_TIMECODE,

    /**
     * 数据表示 libavutil/spherical.h 中定义的 AVSphericalMapping 结构体。
     */
    AV_FRAME_DATA_SPHERICAL,

    /**
     * 内容光级别(基于 CTA-861.3)。此有效载荷包含 AVContentLightMetadata 结构体形式的数据。
     */
    AV_FRAME_DATA_CONTENT_LIGHT_LEVEL,

    /**
     * 数据包含一个 ICC 配置文件，作为不透明的八位字节缓冲区，
     * 遵循 ISO 15076-1 描述的格式，在元数据键条目 "name" 中可以定义可选名称。
     */
    AV_FRAME_DATA_ICC_PROFILE,

    /**
     * 符合 SMPTE ST 12-1 的时间码。数据是一个包含 4 个 uint32_t 的数组，
     * 其中第一个 uint32_t 描述了使用了多少个(1-3)其他时间码。
     * 时间码格式在 libavutil/timecode.h 中 av_timecode_get_smpte_from_framenum() 函数的文档中描述。
     */
    AV_FRAME_DATA_S12M_TIMECODE,

    /**
     * 与视频帧关联的 HDR 动态元数据。有效载荷是 AVDynamicHDRPlus 类型，
     * 包含色彩体积变换的信息 - SMPTE 2094-40:2016 标准的应用 4。
     */
    AV_FRAME_DATA_DYNAMIC_HDR_PLUS,

    /**
     * 感兴趣区域，数据是 AVRegionOfInterest 类型的数组，
     * 数组元素的数量由 AVFrameSideData.size / AVRegionOfInterest.self_size 推断。
     */
    AV_FRAME_DATA_REGIONS_OF_INTEREST,

    /**
     * 视频帧的编码参数，由 AVVideoEncParams 描述。
     */
    AV_FRAME_DATA_VIDEO_ENC_PARAMS,

    /**
     * 与视频帧关联的未注册用户数据元数据。
     * 这是 H.26[45] UDU SEI 消息，不应用于任何其他目的。
     * 数据以 uint8_t 形式存储在 AVFrameSideData.data 中，
     * 其中包含 16 字节的 uuid_iso_iec_11578，后跟 AVFrameSideData.size - 16 字节的 user_data_payload_byte。
     */
    AV_FRAME_DATA_SEI_UNREGISTERED,

    /**
     * 帧的胶片颗粒参数，由 AVFilmGrainParams 描述。
     * 对于每个应该应用胶片颗粒的帧，必须存在。
     *
     * 可能多次出现，例如当有多个针对不同视频信号特征的替代参数集时。
     * 用户应为应用程序选择最合适的集合。
     */
    AV_FRAME_DATA_FILM_GRAIN_PARAMS,

    /**
     * 对象检测和分类的边界框，由 AVDetectionBBoxHeader 描述。
     */
    AV_FRAME_DATA_DETECTION_BBOXES,

    /**
     * 杜比视界 RPU 原始数据，适用于传递给 x265 或其他库。
     * uint8_t 数组，保持 NAL 仿真字节完整。
     */
    AV_FRAME_DATA_DOVI_RPU_BUFFER,

    /**
     * 解析的杜比视界元数据，适用于传递给软件实现。
     * 有效载荷是 libavutil/dovi_meta.h 中定义的 AVDOVIMetadata 结构体。
     */
    AV_FRAME_DATA_DOVI_METADATA,

    /**
     * 与视频帧关联的 HDR Vivid 动态元数据。有效载荷是 AVDynamicHDRVivid 类型，
     * 包含色彩体积变换的信息 - CUVA 005.1-2021。
     */
    AV_FRAME_DATA_DYNAMIC_HDR_VIVID,

    /**
     * 环境观看环境元数据，由 H.274 定义。
     */
    AV_FRAME_DATA_AMBIENT_VIEWING_ENVIRONMENT,

    /**
     * 提供有关帧的已更改/未更改部分的编码器特定提示信息。
     * 它可用于传递有关哪些宏块可以跳过的信息，因为它们与前一帧中的相应宏块相比没有变化。
     * 这对于预先知道此信息以加快编码速度的应用程序可能很有用。
     */
    AV_FRAME_DATA_VIDEO_HINT,
};

enum AVActiveFormatDescription {
    AV_AFD_SAME         = 8,
    AV_AFD_4_3          = 9,
    AV_AFD_16_9         = 10,
    AV_AFD_14_9         = 11,
    AV_AFD_4_3_SP_14_9  = 13,
    AV_AFD_16_9_SP_14_9 = 14,
    AV_AFD_SP_4_3       = 15,
};

/**
 * 用于保存 AVFrame 边数据的结构体。
 *
 * sizeof(AVFrameSideData) 不是公共 ABI 的一部分，
 * 因此可以在次要版本更新时向末尾添加新字段。
 */
typedef struct AVFrameSideData {
    enum AVFrameSideDataType type;
    uint8_t *data;
    size_t   size;
    AVDictionary *metadata;
    AVBufferRef *buf;
} AVFrameSideData;

enum AVSideDataProps {
    /**
     * 边数据类型可以在流全局结构中使用。
     * 没有此属性的边数据类型仅在每帧基础上有意义。
     */
    AV_SIDE_DATA_PROP_GLOBAL = (1 << 0),

    /**
     * 此边数据类型的多个实例可以在单个边数据数组中有意义地存在。
     */
    AV_SIDE_DATA_PROP_MULTI  = (1 << 1),
};

/**
 * 此结构体描述边数据类型的属性。
 * 可以通过 av_frame_side_data_desc() 获取对应给定类型的实例。
 */
typedef struct AVSideDataDescriptor {
    /**
     * 人类可读的边数据描述。
     */
    const char      *name;

    /**
     * 边数据属性标志，AVSideDataProps 值的组合。
     */
    unsigned         props;
} AVSideDataDescriptor;

/**
 * 描述单个感兴趣区域的结构体。
 *
 * 当在单个边数据块中定义多个区域时，
 * 应该按照从最重要到最不重要的顺序排列 - 某些编码器只能支持有限数量的不同区域，
 * 因此将不得不截断列表。
 *
 * 当定义重叠区域时，包含给定帧区域的第一个区域适用。
 */
typedef struct AVRegionOfInterest {
    /**
     * 必须设置为此数据结构的大小
     * (即 sizeof(AVRegionOfInterest))。
     */
    uint32_t self_size;
    /**
     * 从帧的顶边到矩形定义的此感兴趣区域的顶边和底边的像素距离，
     * 以及从帧的左边到左边和右边的像素距离。
     *
     * 区域的约束取决于编码器，因此实际受影响的区域可能会因对齐或其他原因而略大。
     */
    int top;
    int bottom;
    int left;
    int right;
    /**
     * 量化偏移。
     *
     * 必须在 -1 到 +1 范围内。值为零表示没有质量变化。
     * 负值要求更好的质量(更少量化)，而正值要求更差的质量(更多量化)。
     *
     * 范围经过校准，使极值表示最大可能的偏移 - 如果帧的其余部分以最差可能的质量编码，
     * 偏移 -1 表示此区域仍应以最佳可能的质量编码。中间值然后以某种编解码器相关的方式插值。
     *
     * 例如，在 10 位 H.264 中，量化参数在 -12 和 51 之间变化。
     * 因此，典型的 qoffset 值 -1/10 表示此区域应该以比帧其余部分好约十分之一的全范围的 QP 编码。
     * 所以，如果帧的大部分要以大约 30 的 QP 编码，这个区域会得到大约 24 的 QP
     * (偏移约为 -1/10 * (51 - -12) = -6.3)。
     * 极值 -1 将表示无论帧的其余部分如何处理，此区域都应以最佳可能的质量编码 - 即应以 -12 的 QP 编码。
     */
    AVRational qoffset;
} AVRegionOfInterest;

/**
 * 此结构体描述解码后的（原始）音频或视频数据。
 *
 * AVFrame 必须使用 av_frame_alloc() 分配。注意这只会分配 AVFrame 本身，
 * 数据缓冲区必须通过其他方式管理（见下文）。
 * AVFrame 必须使用 av_frame_free() 释放。
 *
 * AVFrame 通常只分配一次，然后重复使用来保存不同的数据
 * （例如，一个 AVFrame 用于保存从解码器接收的帧）。在这种情况下，
 * av_frame_unref() 将释放帧持有的任何引用，并将其重置为原始干净状态，
 * 然后才能再次重用。
 *
 * AVFrame 描述的数据通常通过 AVBuffer API 进行引用计数。
 * 底层缓冲区引用存储在 AVFrame.buf / AVFrame.extended_buf 中。
 * 当至少设置了一个引用时，即 AVFrame.buf[0] != NULL 时，AVFrame 被认为是引用计数的。
 * 在这种情况下，每个数据平面都必须包含在 AVFrame.buf 或 AVFrame.extended_buf 中的某个缓冲区中。
 * 可能所有数据都在一个缓冲区中，或者每个平面一个单独的缓冲区，或者介于两者之间的任何情况。
 *
 * sizeof(AVFrame) 不是公共 ABI 的一部分，因此可以通过次要版本升级在末尾添加新字段。
 *
 * 字段可以通过 AVOptions 访问，使用的名称字符串与可通过 AVOptions 访问的 C 结构体字段名称匹配。
 */
typedef struct AVFrame {
#define AV_NUM_DATA_POINTERS 8
    /**
     * 指向图像/声道平面的指针。
     * 这可能与第一个分配的字节不同。对于视频，
     * 它甚至可能指向图像数据的末尾。
     *
     * data 和 extended_data 中的所有指针必须指向 buf 或 extended_buf 中的某个 AVBufferRef。
     *
     * 一些解码器会访问 0,0 - width,height 之外的区域，请参见 avcodec_align_dimensions2()。
     * 一些过滤器和 swscale 可以读取平面之外最多 16 个字节，如果要使用这些过滤器，
     * 则必须额外分配 16 个字节。
     *
     * 注意：格式不需要的指针必须设置为 NULL。
     *
     * @attention 对于视频，data[] 指针可以指向图像数据的末尾以反转行顺序，
     * 当与 linesize[] 数组中的负值一起使用时。
     */
    uint8_t *data[AV_NUM_DATA_POINTERS];

    /**
     * 对于视频，是一个正值或负值，通常表示每个图像行的字节大小，但也可以是：
     * - 用于垂直翻转的行的负字节大小
     *   (data[n] 指向数据末尾)
     * - 字节大小的正或负倍数，用于访问帧的偶数和奇数场
     *   (可能被翻转)
     *
     * 对于音频，只能设置 linesize[0]。对于平面音频，每个声道平面必须大小相同。
     *
     * 对于视频，linesize 应该是 CPU 对齐偏好的倍数，
     * 对现代桌面 CPU 来说是 16 或 32。
     * 某些代码需要这样的对齐，其他代码在没有正确对齐时可能会更慢，
     * 对于其他代码来说则没有区别。
     *
     * @note linesize 可能大于可用数据的大小 -- 出于性能原因可能存在额外的填充。
     *
     * @attention 对于视频，行大小值可以为负，以实现对图像行的垂直反向迭代。
     */
    int linesize[AV_NUM_DATA_POINTERS];

    /**
     * 指向数据平面/声道的指针。
     *
     * 对于视频，这应该简单地指向 data[]。
     *
     * 对于平面音频，每个声道有一个单独的数据指针，
     * linesize[0] 包含每个声道缓冲区的大小。
     * 对于打包音频，只有一个数据指针，
     * linesize[0] 包含所有声道的缓冲区的总大小。
     *
     * 注意：data 和 extended_data 在有效帧中应该始终被设置，
     * 但对于声道数超过 data 可容纳的平面音频，
     * 必须使用 extended_data 才能访问所有声道。
     */
    uint8_t **extended_data;

    /**
     * @name 视频尺寸
     * 仅用于视频帧。视频帧的编码尺寸（以像素为单位），
     * 即包含一些明确定义值的矩形的大小。
     *
     * @note 用于显示/呈现的帧部分进一步受到 @ref cropping "裁剪矩形" 的限制。
     * @{
     */
    int width, height;
    /**
     * @}
     */

    /**
     * 此帧描述的音频样本数（每个声道）
     */
    int nb_samples;

    /**
     * 帧的格式，如果未知或未设置则为 -1
     * 值对应于视频帧的 AVPixelFormat 枚举，
     * 音频的 AVSampleFormat 枚举
     */
    int format;

#if FF_API_FRAME_KEY
    /**
     * 1 -> 关键帧，0 -> 非关键帧
     *
     * @deprecated 改用 AV_FRAME_FLAG_KEY
     */
    attribute_deprecated
    int key_frame;
#endif

    /**
     * 帧的图像类型。
     */
    enum AVPictureType pict_type;

    /**
     * 视频帧的样本宽高比，如果未知/未指定则为 0/1。
     */
    AVRational sample_aspect_ratio;

    /**
     * 以 time_base 单位表示的显示时间戳（应该向用户显示帧的时间）。
     */
    int64_t pts;

    /**
     * 从触发返回此帧的 AVPacket 复制的 DTS（如果不使用帧线程）。
     * 这也是仅使用 AVPacket.dts 值计算的此 AVFrame 的显示时间，
     * 不使用 pts 值。
     */
    int64_t pkt_dts;

    /**
     * 此帧中时间戳的时间基准。
     * 将来，此字段可能会在解码器或过滤器输出的帧上设置，
     * 但默认情况下，其值在编码器或过滤器输入时将被忽略。
     */
    AVRational time_base;

    /**
     * 质量（在 1（好）和 FF_LAMBDA_MAX（差）之间）
     */
    int quality;

    /**
     * 帧所有者的私有数据。
     *
     * 此字段可以由分配/拥有帧数据的代码设置。
     * 然后它不会被任何库函数触及，除了：
     * - 它会被 av_frame_copy_props() 复制到其他引用（因此也被 av_frame_ref() 复制）；
     * - 当帧被 av_frame_unref() 清除时，它会被设置为 NULL
     * - 在调用者的明确请求下。例如，如果调用者设置 @ref AV_CODEC_FLAG_COPY_OPAQUE，
     *   libavcodec 编码器/解码器将在 @ref AVPacket "AVPackets" 之间复制此字段。
     *
     * @see opaque_ref 引用计数的类似物
     */
    void *opaque;

    /**
     * 此帧中应重复的场数，即此帧的总持续时间应为 repeat_pict + 2 个正常场持续时间。
     *
     * 对于隔行扫描帧，此字段可能设置为 1，这表示此帧应该呈现为 3 个场：
     * 从第一个场开始（由 AV_FRAME_FLAG_TOP_FIELD_FIRST 的设置与否决定），
     * 然后是第二个场，最后再次是第一个场。
     *
     * 对于逐行扫描帧，此字段可能设置为 2 的倍数，这表示此帧的持续时间应为
     * (repeat_pict + 2) / 2 个正常帧持续时间。
     *
     * @note 此字段是从 MPEG2 repeat_first_field 标志及其相关标志、
     * H.264 图像时序 SEI 中的 pic_struct 以及其他编解码器中的类似物计算得出的。
     * 通常只有在没有更高层时序信息时才应使用它。
     */
    int repeat_pict;

#if FF_API_INTERLACED_FRAME
    /**
     * 图像内容是隔行扫描的。
     *
     * @deprecated 改用 AV_FRAME_FLAG_INTERLACED
     */
    attribute_deprecated
    int interlaced_frame;

    /**
     * 如果内容是隔行扫描的，是否先显示顶场。
     *
     * @deprecated 改用 AV_FRAME_FLAG_TOP_FIELD_FIRST
     */
    attribute_deprecated
    int top_field_first;
#endif

#if FF_API_PALETTE_HAS_CHANGED
    /**
     * 告诉用户应用程序调色板已从上一帧更改。
     */
    attribute_deprecated
    int palette_has_changed;
#endif

    /**
     * 音频数据的采样率。
     */
    int sample_rate;

    /**
     * 支持此帧数据的 AVBuffer 引用。data 和 extended_data 中的所有指针
     * 必须指向 buf 或 extended_buf 中的某个缓冲区内。此数组必须连续填充 -- 
     * 如果 buf[i] 非 NULL，则对于所有 j < i，buf[j] 也必须非 NULL。
     *
     * 每个数据平面最多可以有一个 AVBuffer，因此对于视频，此数组始终包含所有引用。
     * 对于具有超过 AV_NUM_DATA_POINTERS 个声道的平面音频，可能有更多的缓冲区
     * 无法放入此数组。然后额外的 AVBufferRef 指针存储在 extended_buf 数组中。
     */
    AVBufferRef *buf[AV_NUM_DATA_POINTERS];

    /**
     * 对于需要超过 AV_NUM_DATA_POINTERS 个 AVBufferRef 指针的平面音频，
     * 此数组将保存所有无法放入 AVFrame.buf 的引用。
     *
     * 注意这与 AVFrame.extended_data 不同，后者始终包含所有指针。
     * 此数组只包含无法放入 AVFrame.buf 的额外指针。
     *
     * 此数组始终由构造帧的人使用 av_malloc() 分配。
     * 它在 av_frame_unref() 中被释放。
     */
    AVBufferRef **extended_buf;
    /**
     * extended_buf 中的元素数量。
     */
    int        nb_extended_buf;

    AVFrameSideData **side_data;
    int            nb_side_data;

/**
 * @defgroup lavu_frame_flags AV_FRAME_FLAGS
 * @ingroup lavu_frame
 * 描述额外帧属性的标志。
 *
 * @{
 */

/**
 * 帧数据可能已损坏，例如由于解码错误。
 */
#define AV_FRAME_FLAG_CORRUPT       (1 << 0)
/**
 * 用于标记关键帧的标志。
 */
#define AV_FRAME_FLAG_KEY (1 << 1)
/**
 * 用于标记需要解码但不应输出的帧的标志。
 */
#define AV_FRAME_FLAG_DISCARD   (1 << 2)
/**
 * 用于标记内容为隔行扫描的帧的标志。
 */
#define AV_FRAME_FLAG_INTERLACED (1 << 3)
/**
 * 用于标记如果内容是隔行扫描的，是否先显示顶场的标志。
 */
#define AV_FRAME_FLAG_TOP_FIELD_FIRST (1 << 4)
/**
 * @}
 */

    /**
     * 帧标志，@ref lavu_frame_flags 的组合
     */
    int flags;

    /**
     * MPEG 与 JPEG YUV 范围。
     * - 编码：由用户设置
     * - 解码：由 libavcodec 设置
     */
    enum AVColorRange color_range;

    enum AVColorPrimaries color_primaries;

    enum AVColorTransferCharacteristic color_trc;

    /**
     * YUV 色彩空间类型。
     * - 编码：由用户设置
     * - 解码：由 libavcodec 设置
     */
    enum AVColorSpace colorspace;

    enum AVChromaLocation chroma_location;

    /**
     * 使用各种启发式方法估计的帧时间戳，以流时间基为单位
     * - 编码：未使用
     * - 解码：由 libavcodec 设置，由用户读取
     */
    int64_t best_effort_timestamp;

#if FF_API_FRAME_PKT
    /**
     * 从输入到解码器的最后一个 AVPacket 重新排序的位置
     * - 编码：未使用
     * - 解码：由用户读取
     * @deprecated 使用 AV_CODEC_FLAG_COPY_OPAQUE 从数据包传递任意用户数据到帧
     */
    attribute_deprecated
    int64_t pkt_pos;
#endif

    /**
     * 元数据。
     * - 编码：由用户设置
     * - 解码：由 libavcodec 设置
     */
    AVDictionary *metadata;

    /**
     * 帧的解码错误标志，如果解码器产生了帧但在解码过程中出现错误，
     * 则设置为 FF_DECODE_ERROR_xxx 标志的组合。
     * - 编码：未使用
     * - 解码：由 libavcodec 设置，由用户读取
     */
    int decode_error_flags;
#define FF_DECODE_ERROR_INVALID_BITSTREAM   1
#define FF_DECODE_ERROR_MISSING_REFERENCE   2
#define FF_DECODE_ERROR_CONCEALMENT_ACTIVE  4
#define FF_DECODE_ERROR_DECODE_SLICES       8

#if FF_API_FRAME_PKT
    /**
     * 包含压缩帧的相应数据包的大小。
     * 如果未知则设置为负值。
     * - 编码：未使用
     * - 解码：由 libavcodec 设置，由用户读取
     * @deprecated 使用 AV_CODEC_FLAG_COPY_OPAQUE 从数据包传递任意用户数据到帧
     */
    attribute_deprecated
    int pkt_size;
#endif

    /**
     * 对于 hwaccel 格式的帧，这应该是对描述该帧的
     * AVHWFramesContext 的引用。
     */
    AVBufferRef *hw_frames_ctx;

    /**
     * 帧所有者的私有数据。
     *
     * 此字段可以由分配/拥有帧数据的代码设置。
     * 然后它不会被任何库函数触及，除了：
     * - av_frame_copy_props()（因此也包括 av_frame_ref()）会传播对底层缓冲区的新引用；
     * - 它在 av_frame_unref() 中被取消引用；
     * - 在调用者的明确请求下。例如，如果调用者设置 @ref AV_CODEC_FLAG_COPY_OPAQUE，
     *   libavcodec 编码器/解码器将在 @ref AVPacket "AVPackets" 之间传播新的引用。
     *
     * @see opaque 普通指针的类似物
     */
    AVBufferRef *opaque_ref;

    /**
     * @anchor cropping
     * @name 裁剪
     * 仅用于视频帧。从帧的上/下/左/右边界丢弃的像素数，
     * 以获得用于呈现的帧子矩形。
     * @{
     */
    size_t crop_top;
    size_t crop_bottom;
    size_t crop_left;
    size_t crop_right;
    /**
     * @}
     */

    /**
     * 供单个 libav* 库内部使用的 AVBufferRef。
     * 不得用于在库之间传输数据。
     * 当帧的所有权离开相应的库时必须为 NULL。
     *
     * FFmpeg 库外的代码不应检查或更改缓冲区引用的内容。
     *
     * 当帧未被引用时，FFmpeg 会对其调用 av_buffer_unref()。
     * av_frame_copy_props() 使用 av_buffer_ref() 为目标帧的 private_ref 字段
     * 创建新的引用。
     */
    AVBufferRef *private_ref;

    /**
     * 音频数据的声道布局。
     */
    AVChannelLayout ch_layout;

    /**
     * 帧的持续时间，单位与 pts 相同。如果未知则为 0。
     */
    int64_t duration;
} AVFrame;

/**
 * 分配一个 AVFrame 并将其字段设置为默认值。生成的结构体
 * 必须使用 av_frame_free() 释放。
 *
 * @return 一个填充了默认值的 AVFrame，失败时返回 NULL。
 *
 * @note 这只分配 AVFrame 本身，不分配数据缓冲区。数据缓冲区
 * 必须通过其他方式分配，例如使用 av_frame_get_buffer() 或手动分配。
 */
AVFrame *av_frame_alloc(void);

/**
 * 释放帧及其中任何动态分配的对象，
 * 例如 extended_data。如果帧是引用计数的，它将首先
 * 被取消引用。
 *
 * @param frame 要释放的帧。指针将被设置为 NULL。
 */
void av_frame_free(AVFrame **frame);

/**
 * 为源帧描述的数据设置一个新的引用。
 *
 * 从 src 复制帧属性到 dst，并为 src 中的每个
 * AVBufferRef 创建一个新的引用。
 *
 * 如果 src 不是引用计数的，将分配新的缓冲区并复制数据。
 *
 * @warning: 在调用此函数之前，dst 必须已经通过 av_frame_unref(dst) 取消引用，
 *           或者通过 av_frame_alloc() 新分配，否则将发生未定义行为。
 *
 * @return 成功返回 0，失败返回负的 AVERROR
 */
int av_frame_ref(AVFrame *dst, const AVFrame *src);

/**
 * 确保目标帧引用与源帧描述的相同数据，
 * 如果 src 中的 AVBufferRef 与 dst 中的不同，则为每个 AVBufferRef 创建新的引用，
 * 如果 src 不是引用计数的，则分配新的缓冲区并复制数据，
 * 如果 src 为空则取消引用。
 *
 * dst 上的帧属性将被 src 中的属性替换。
 *
 * @return 成功返回 0，失败返回负的 AVERROR。失败时，dst 被取消引用。
 */
int av_frame_replace(AVFrame *dst, const AVFrame *src);

/**
 * 创建一个引用与 src 相同数据的新帧。
 *
 * 这是 av_frame_alloc()+av_frame_ref() 的快捷方式。
 *
 * @return 成功时返回新创建的 AVFrame，失败时返回 NULL。
 */
AVFrame *av_frame_clone(const AVFrame *src);

/**
 * 取消引用帧引用的所有缓冲区并重置帧字段。
 */
void av_frame_unref(AVFrame *frame);

/**
 * 将 src 中包含的所有内容移动到 dst 并重置 src。
 *
 * @warning: dst 不会被取消引用，而是直接被覆盖而不读取
 *           或释放其内容。在调用此函数之前手动调用 av_frame_unref(dst)
 *           以确保不会泄漏内存。
 */
void av_frame_move_ref(AVFrame *dst, AVFrame *src);

/**
 * 为音频或视频数据分配新的缓冲区。
 *
 * 在调用此函数之前必须在帧上设置以下字段：
 * - format (视频的像素格式，音频的采样格式)
 * - width 和 height 用于视频
 * - nb_samples 和 ch_layout 用于音频
 *
 * 此函数将填充 AVFrame.data 和 AVFrame.buf 数组，如果
 * 必要的话，还会分配并填充 AVFrame.extended_data 和 AVFrame.extended_buf。
 * 对于平面格式，每个平面将分配一个缓冲区。
 *
 * @warning: 如果帧已经被分配，调用此函数将
 *           泄漏内存。此外，在某些情况下可能会发生未定义行为。
 *
 * @param frame 用于存储新缓冲区的帧。
 * @param align 所需的缓冲区大小对齐。如果等于 0，将为当前 CPU
 *              自动选择对齐。强烈建议在此处传递 0，除非你知道自己在做什么。
 *
 * @return 成功返回 0，失败返回负的 AVERROR。
 */
int av_frame_get_buffer(AVFrame *frame, int align);

/**
 * 检查帧数据是否可写。
 *
 * @return 如果帧数据可写则返回正值(当且仅当每个底层缓冲区
 * 只有一个引用，即存储在此帧中的引用时为真)。否则返回 0。
 *
 * 如果返回 1，则答案在对任何底层 AVBufferRef 调用 av_buffer_ref() 之前
 * 都是有效的(例如通过 av_frame_ref() 或直接调用)。
 *
 * @see av_frame_make_writable(), av_buffer_is_writable()
 */
int av_frame_is_writable(AVFrame *frame);

/**
 * 确保帧数据是可写的，尽可能避免数据复制。
 *
 * 如果帧是可写的，则不做任何操作，如果不是，则分配新的缓冲区并复制数据。
 * 非引用计数的帧表现为不可写，即总是会进行复制。
 *
 * @return 成功返回 0，失败返回负的 AVERROR。
 *
 * @see av_frame_is_writable(), av_buffer_is_writable(),
 * av_buffer_make_writable()
 */
int av_frame_make_writable(AVFrame *frame);

/**
 * 将帧数据从 src 复制到 dst。
 *
 * 此函数不分配任何内容，dst 必须已经用与 src 相同的参数
 * 初始化和分配。
 *
 * 此函数只复制帧数据(即 data/extended_data 数组的内容)，
 * 不复制任何其他属性。
 *
 * @return 成功返回 >= 0，失败返回负的 AVERROR。
 */
int av_frame_copy(AVFrame *dst, const AVFrame *src);

/**
 * 只从 src 复制"元数据"字段到 dst。
 *
 * 就此函数而言，元数据是指那些不影响缓冲区中数据布局的字段。
 * 例如 pts、采样率(对于音频)或采样宽高比(对于视频)，
 * 但不包括宽度/高度或声道布局。
 * 边数据也会被复制。
 */
int av_frame_copy_props(AVFrame *dst, const AVFrame *src);

/**
 * 获取存储给定数据平面的缓冲区引用。
 *
 * @param frame 获取平面缓冲区的帧
 * @param plane frame->extended_data 中感兴趣的数据平面索引。
 *
 * @return 包含该平面的缓冲区引用，如果输入帧无效则返回 NULL。
 */
AVBufferRef *av_frame_get_plane_buffer(const AVFrame *frame, int plane);

/**
 * 向帧添加新的边数据。
 *
 * @param frame 应添加边数据的帧
 * @param type 添加的边数据类型
 * @param size 边数据的大小
 *
 * @return 成功时返回新添加的边数据，失败时返回 NULL
 */
AVFrameSideData *av_frame_new_side_data(AVFrame *frame,
                                        enum AVFrameSideDataType type,
                                        size_t size);

/**
 * 从现有的 AVBufferRef 向帧添加新的边数据
 *
 * @param frame 应添加边数据的帧
 * @param type  添加的边数据类型
 * @param buf   要作为边数据添加的 AVBufferRef。引用的所有权
 *              转移给帧。
 *
 * @return 成功时返回新添加的边数据，失败时返回 NULL。失败时
 *         帧保持不变，AVBufferRef 仍由调用者拥有。
 */
AVFrameSideData *av_frame_new_side_data_from_buf(AVFrame *frame,
                                                 enum AVFrameSideDataType type,
                                                 AVBufferRef *buf);

/**
 * @return 成功时返回指向给定类型边数据的指针，如果此帧中
 * 没有该类型的边数据则返回 NULL。
 */
AVFrameSideData *av_frame_get_side_data(const AVFrame *frame,
                                        enum AVFrameSideDataType type);

/**
 * 移除并释放给定类型的所有边数据实例。
 */
void av_frame_remove_side_data(AVFrame *frame, enum AVFrameSideDataType type);

/**
 * 帧裁剪的标志。
 */
enum {
    /**
     * 应用最大可能的裁剪，即使这需要将 AVFrame.data[] 条目
     * 设置为未对齐的指针。向 FFmpeg API 传递未对齐的数据
     * 通常是不允许的，会导致未定义行为(如崩溃)。你只能将未对齐
     * 的数据传递给明确记录接受它的 FFmpeg API。仅当你
     * 完全确定自己在做什么时才使用此标志。
     */
    AV_FRAME_CROP_UNALIGNED     = 1 << 0,
};

/**
 * 根据其 crop_left/crop_top/crop_right/crop_bottom 字段裁剪给定的视频 AVFrame。
 * 如果裁剪成功，函数将调整数据指针和宽度/高度字段，并将裁剪字段设置为 0。
 *
 * 在所有情况下，裁剪边界都将舍入到像素格式的固有对齐。在某些情况下，
 * 例如不透明的 hwaccel 格式，左/上裁剪将被忽略。即使裁剪被舍入或
 * 忽略，裁剪字段也会设置为 0。
 *
 * @param frame 应该被裁剪的帧
 * @param flags 一些 AV_FRAME_CROP_* 标志的组合，或 0。
 *
 * @return 成功返回 >= 0，失败返回负的 AVERROR。如果裁剪字段
 * 无效，返回 AVERROR(ERANGE)，并且不会改变任何内容。
 */
int av_frame_apply_cropping(AVFrame *frame, int flags);

/**
 * @return 标识边数据类型的字符串
 */
const char *av_frame_side_data_name(enum AVFrameSideDataType type);

/**
 * @return 对应于给定边数据类型的边数据描述符，不可用时
 *         返回 NULL。
 */
const AVSideDataDescriptor *av_frame_side_data_desc(enum AVFrameSideDataType type);

/**
 * 释放所有边数据条目及其内容，然后将指针指向的值
 * 置零。
 *
 * @param sd    指向要释放的边数据数组的指针。返回时将设置为 NULL。
 * @param nb_sd 指向包含数组条目数的整数的指针。返回时将设置为 0。
 */
void av_frame_side_data_free(AVFrameSideData ***sd, int *nb_sd);

/**
 * 在添加新条目之前移除现有条目。
 */
#define AV_FRAME_SIDE_DATA_FLAG_UNIQUE (1 << 0)
/**
 * 如果存在相同类型的另一个条目，则不添加新条目。
 * 仅适用于没有 AV_SIDE_DATA_PROP_MULTI 属性的边数据类型。
 */
#define AV_FRAME_SIDE_DATA_FLAG_REPLACE (1 << 1)

/**
 * 向数组添加新的边数据条目。
 *
 * @param sd    指向要添加另一个条目的边数据数组的指针，
 *              或指向 NULL 以开始新数组。
 * @param nb_sd 指向包含数组条目数的整数的指针。
 * @param type  添加的边数据类型
 * @param size  边数据的大小
 * @param flags 一些 AV_FRAME_SIDE_DATA_FLAG_* 标志的组合，或 0。
 *
 * @return 成功时返回新添加的边数据，失败时返回 NULL。
 * @note 如果设置了 AV_FRAME_SIDE_DATA_FLAG_UNIQUE，在尝试添加之前
 *       将移除匹配 AVFrameSideDataType 的条目。
 * @note 如果设置了 AV_FRAME_SIDE_DATA_FLAG_REPLACE，如果已存在
 *       相同类型的条目，它将被替换而不是添加。
 */
AVFrameSideData *av_frame_side_data_new(AVFrameSideData ***sd, int *nb_sd,
                                        enum AVFrameSideDataType type,
                                        size_t size, unsigned int flags);

/**
 * 从现有的 AVBufferRef 向数组添加新的边数据条目。
 *
 * @param sd    指向要添加另一个条目的边数据数组的指针，
 *              或指向 NULL 以开始新数组。
 * @param nb_sd 指向包含数组条目数的整数的指针。
 * @param type  添加的边数据类型
 * @param buf   指向要添加到数组的 AVBufferRef 的指针。成功时，
 *              函数获取 AVBufferRef 的所有权，*buf 设置为 NULL，
 *              除非设置了 AV_FRAME_SIDE_DATA_FLAG_NEW_REF，
 *              在这种情况下所有权将保留给调用者。
 * @param flags 一些 AV_FRAME_SIDE_DATA_FLAG_* 标志的组合，或 0。
 *
 * @return 成功时返回新添加的边数据，失败时返回 NULL。
 * @note 如果设置了 AV_FRAME_SIDE_DATA_FLAG_UNIQUE，在尝试添加之前
 *       将移除匹配 AVFrameSideDataType 的条目。
 * @note 如果设置了 AV_FRAME_SIDE_DATA_FLAG_REPLACE，如果已存在
 *       相同类型的条目，它将被替换而不是添加。
 *
 */
AVFrameSideData *av_frame_side_data_add(AVFrameSideData ***sd, int *nb_sd,
                                        enum AVFrameSideDataType type,
                                        AVBufferRef **buf, unsigned int flags);

/**
 * 基于现有边数据向数组添加新的边数据条目，
 * 对包含的 AVBufferRef 创建引用。
 *
 * @param sd    指向要添加另一个条目的边数据数组的指针，
 *              或指向 NULL 以开始新数组。
 * @param nb_sd 指向包含数组条目数的整数的指针。
 * @param src   要克隆的边数据，为缓冲区使用新的引用。
 * @param flags 一些 AV_FRAME_SIDE_DATA_FLAG_* 标志的组合，或 0。
 *
 * @return 失败时返回负的错误代码，成功时返回 >=0。
 * @note 如果设置了 AV_FRAME_SIDE_DATA_FLAG_UNIQUE，在尝试添加之前
 *       将移除匹配 AVFrameSideDataType 的条目。
 * @note 如果设置了 AV_FRAME_SIDE_DATA_FLAG_REPLACE，如果已存在
 *       相同类型的条目，它将被替换而不是添加。
 */
int av_frame_side_data_clone(AVFrameSideData ***sd, int *nb_sd,
                             const AVFrameSideData *src, unsigned int flags);

/**
 * 从数组中获取特定类型的边数据条目。
 *
 * @param sd    边数据数组。
 * @param nb_sd 包含数组条目数的整数。
 * @param type  要查询的边数据类型
 *
 * @return 成功时返回指向给定类型边数据的指针，如果此集合中
 *         没有该类型的边数据则返回 NULL。
 */
const AVFrameSideData *av_frame_side_data_get_c(const AVFrameSideData * const *sd,
                                                const int nb_sd,
                                                enum AVFrameSideDataType type);

/**
 * av_frame_side_data_get_c() 的包装器，用于解决在 C 中
 * 对于任何类型 T，从 T * const * 到 const T * const * 的转换
 * 不会自动执行的限制。
 * @see av_frame_side_data_get_c()
 */
static inline
const AVFrameSideData *av_frame_side_data_get(AVFrameSideData * const *sd,
                                              const int nb_sd,
                                              enum AVFrameSideDataType type)
{
    return av_frame_side_data_get_c((const AVFrameSideData * const *)sd,
                                    nb_sd, type);
}

/**
 * 从数组中移除并释放给定类型的所有边数据实例。
 */
void av_frame_side_data_remove(AVFrameSideData ***sd, int *nb_sd,
                               enum AVFrameSideDataType type);
/**
 * @}
 */

#endif /* AVUTIL_FRAME_H */
