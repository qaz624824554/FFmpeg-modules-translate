#ifndef AVFILTER_AVFILTER_H
#define AVFILTER_AVFILTER_H

/**
 * @file
 * @ingroup lavfi
 * libavfilter主要公共API头文件
 */

/**
 * @defgroup lavfi libavfilter
 * 基于图的帧编辑库。
 *
 * @{
 */

#include <stddef.h>

#include "libavutil/attributes.h"
#include "libavutil/avutil.h"
#include "libavutil/buffer.h"
#include "libavutil/dict.h"
#include "libavutil/frame.h"
#include "libavutil/log.h"
#include "libavutil/samplefmt.h"
#include "libavutil/pixfmt.h"
#include "libavutil/rational.h"

#include "libavfilter/version_major.h"
#ifndef HAVE_AV_CONFIG_H
/* 当作为ffmpeg构建的一部分时,仅包含主版本号
 * 以避免不必要的重新构建。当外部包含时,继续包含
 * 完整的版本信息。 */
#include "libavfilter/version.h"
#endif

/**
 * 返回LIBAVFILTER_VERSION_INT常量。
 */
unsigned avfilter_version(void);

/**
 * 返回libavfilter的编译时配置。
 */
const char *avfilter_configuration(void);

/**
 * 返回libavfilter的许可证。
 */
const char *avfilter_license(void);

typedef struct AVFilterContext AVFilterContext;
typedef struct AVFilterLink    AVFilterLink;
typedef struct AVFilterPad     AVFilterPad;
typedef struct AVFilterFormats AVFilterFormats;
typedef struct AVFilterChannelLayouts AVFilterChannelLayouts;

/**
 * 获取AVFilterPad的名称。
 *
 * @param pads AVFilterPad数组
 * @param pad_idx 数组中pad的索引;调用者负责
 *                确保索引有效
 *
 * @return pads中第pad_idx个pad的名称
 */
const char *avfilter_pad_get_name(const AVFilterPad *pads, int pad_idx);

/**
 * 获取AVFilterPad的类型。
 *
 * @param pads AVFilterPad数组
 * @param pad_idx 数组中pad的索引;调用者负责
 *                确保索引有效
 *
 * @return pads中第pad_idx个pad的类型
 */
enum AVMediaType avfilter_pad_get_type(const AVFilterPad *pads, int pad_idx);

/**
 * 过滤器输入的数量不仅由AVFilter.inputs决定。
 * 过滤器可能会根据提供给它的选项在初始化期间添加额外的输入。
 */
#define AVFILTER_FLAG_DYNAMIC_INPUTS        (1 << 0)
/**
 * 过滤器输出的数量不仅由AVFilter.outputs决定。
 * 过滤器可能会根据提供给它的选项在初始化期间添加额外的输出。
 */
#define AVFILTER_FLAG_DYNAMIC_OUTPUTS       (1 << 1)
/**
 * 过滤器通过将帧分割成多个部分并并发处理它们来支持多线程。
 */
#define AVFILTER_FLAG_SLICE_THREADS         (1 << 2)
/**
 * 这是一个"元数据"过滤器 - 它不会以任何方式修改帧数据。
 * 它可能只会影响元数据(即那些由av_frame_copy_props()复制的字段)。
 *
 * 更准确地说,这意味着:
 * - 视频:过滤器输出的任何帧的数据必须完全等于
 *   在其输入之一上接收的某个帧。此外,在给定输出上
 *   产生的所有帧必须对应于在同一输入上接收的帧,
 *   并且它们的顺序必须保持不变。注意过滤器仍然可能
 *   丢弃或复制帧。
 * - 音频:过滤器在其任何输出上产生的数据(例如,
 *   视为交错样本的数组)必须完全等于过滤器在其
 *   输入之一上接收的数据。
 */
#define AVFILTER_FLAG_METADATA_ONLY         (1 << 3)

/**
 * 过滤器可以使用AVFilterContext.hw_device_ctx创建硬件帧。
 */
#define AVFILTER_FLAG_HWDEVICE              (1 << 4)
/**
 * 一些过滤器支持通用的"enable"表达式选项,可用于
 * 在时间线中启用或禁用过滤器。支持此选项的过滤器
 * 设置此标志。当enable表达式为false时,将调用默认的
 * 无操作filter_frame()函数,而不是在每个输入pad上定义的
 * filter_frame()回调,因此帧将不变地传递给下一个过滤器。
 */
#define AVFILTER_FLAG_SUPPORT_TIMELINE_GENERIC  (1 << 16)
/**
 * 与AVFILTER_FLAG_SUPPORT_TIMELINE_GENERIC相同,除了即使
 * enable表达式为false时,过滤器仍将像往常一样调用其
 * filter_frame()回调。过滤器将在filter_frame()回调内
 * 禁用过滤,例如执行依赖于AVFilterContext->is_disabled
 * 值的代码。
 */
#define AVFILTER_FLAG_SUPPORT_TIMELINE_INTERNAL (1 << 17)
/**
 * 用于测试过滤器是否支持时间线功能的便捷掩码
 * (内部或通用)。
 */
#define AVFILTER_FLAG_SUPPORT_TIMELINE (AVFILTER_FLAG_SUPPORT_TIMELINE_GENERIC | AVFILTER_FLAG_SUPPORT_TIMELINE_INTERNAL)

/**
 * 过滤器定义。这定义了过滤器包含的pad,以及所有用于
 * 与过滤器交互的回调函数。
 */
typedef struct AVFilter {
    /**
     * 过滤器名称。必须非NULL且在过滤器中唯一。
     */
    const char *name;

    /**
     * 过滤器的描述。可以为NULL。
     *
     * 你应该使用NULL_IF_CONFIG_SMALL()宏来定义它。
     */
    const char *description;

    /**
     * 静态输入列表。
     *
     * 如果没有(静态)输入则为NULL。设置了AVFILTER_FLAG_DYNAMIC_INPUTS
     * 的过滤器实例可能有比此列表更多的输入。
     */
    const AVFilterPad *inputs;

    /**
     * 静态输出列表。
     *
     * 如果没有(静态)输出则为NULL。设置了AVFILTER_FLAG_DYNAMIC_OUTPUTS
     * 的过滤器实例可能有比此列表更多的输出。
     */
    const AVFilterPad *outputs;

    /**
     * 用于私有数据的类,用于声明过滤器私有AVOptions。
     * 对于不声明任何选项的过滤器,此字段为NULL。
     *
     * 如果此字段非NULL,过滤器私有数据的第一个成员
     * 必须是指向AVClass的指针,libavfilter通用代码
     * 将把它设置为这个类。
     */
    const AVClass *priv_class;

    /**
     * AVFILTER_FLAG_*的组合
     */
    int flags;

    /*****************************************************************
     * 此行以下的所有字段都不是公共API的一部分。它们
     * 不能在libavfilter外使用,并且可以随意更改和
     * 删除。
     * 新的公共字段应该添加在上面。
     *****************************************************************
     */

    /**
     * 输入列表中的条目数。
     */
    uint8_t nb_inputs;

    /**
     * 输出列表中的条目数。
     */
    uint8_t nb_outputs;

    /**
     * 此字段确定formats联合的状态。
     * 它是一个FilterFormatsState枚举值。
     */
    uint8_t formats_state;

    /**
     * 过滤器预初始化函数
     *
     * 此回调将在过滤器上下文分配后立即调用,
     * 以允许分配和初始化子对象。
     *
     * 如果此回调非NULL,则在分配失败时将调用uninit回调。
     *
     * @return 成功时返回0,
     *         失败时返回AVERROR代码(但调用代码会
     *           丢弃该代码并将其视为ENOMEM)
     */
    int (*preinit)(AVFilterContext *ctx);

    /**
     * 过滤器初始化函数。
     *
     * 此回调在过滤器生命周期内只会调用一次,在所有
     * 选项设置之后,但在建立过滤器之间的链接和
     * 进行格式协商之前。
     *
     * 应该在这里完成基本的过滤器初始化。具有动态
     * 输入和/或输出的过滤器应该根据提供的选项在
     * 这里创建这些输入/输出。在此回调之后,不能
     * 对此过滤器的输入/输出进行更多更改。
     *
     * 此回调不能假设过滤器链接存在或帧参数已知。
     *
     * 即使初始化失败,也保证会调用@ref AVFilter.uninit "uninit",
     * 因此此回调不需要在失败时进行清理。
     *
     * @return 成功时返回0,失败时返回负的AVERROR
     */
    int (*init)(AVFilterContext *ctx);

    /**
     * 过滤器取消初始化函数。
     *
     * 仅在过滤器释放之前调用一次。应该释放过滤器
     * 持有的任何内存,释放任何缓冲区引用等。它不
     * 需要释放AVFilterContext.priv内存本身。
     *
     * 即使@ref AVFilter.init "init"未调用或失败,
     * 也可能调用此回调,因此它必须准备好处理这种
     * 情况。
     */
    void (*uninit)(AVFilterContext *ctx);

    /**
     * 以下联合的状态由formats_state确定。
     * 请参阅internal.h中FilterFormatsState枚举的文档。
     */
    union {
        /**
         * 查询过滤器在其输入和输出上支持的格式。
         *
         * 此回调在过滤器初始化后调用(因此输入和输出是固定的),
         * 在格式协商之前不久。此回调可能被调用多次。
         *
         * 此回调必须在每个输入链接上设置::AVFilterLink的
         * @ref AVFilterFormatsConfig.formats "outcfg.formats"
         * 和在每个输出链接上设置
         * @ref AVFilterFormatsConfig.formats "incfg.formats"
         * 为过滤器在该链接上支持的像素/样本格式列表。
         * 对于音频链接,此过滤器还必须设置
         * @ref AVFilterFormatsConfig.samplerates "incfg.samplerates"
         *  /
         * @ref AVFilterFormatsConfig.samplerates "outcfg.samplerates"
         * 和@ref AVFilterFormatsConfig.channel_layouts "incfg.channel_layouts"
         *  /
         * @ref AVFilterFormatsConfig.channel_layouts "outcfg.channel_layouts"
         * 类似地。
         *
         * 当联合处于此状态时,此回调决不能为NULL。
         *
         * @return 成功时返回零,失败时返回对应于
         * AVERROR代码的负值
         */
        int (*query_func)(AVFilterContext *);
        /**
         * 指向由AV_PIX_FMT_NONE分隔的可接受像素格式数组的指针。
         * 通用代码将使用此列表来表明此过滤器支持这些像素格式中的
         * 每一个,前提是所有输入和输出使用相同的像素格式。
         *
         * 当联合处于此状态时,此列表决不能为NULL。
         * 使用此的所有过滤器的输入和输出的类型必须是
         * AVMEDIA_TYPE_VIDEO。
         */
        const enum AVPixelFormat *pixels_list;
        /**
         * 类似于pixels,但由AV_SAMPLE_FMT_NONE分隔
         * 并限制为仅具有AVMEDIA_TYPE_AUDIO输入和输出的过滤器。
         *
         * 除此之外,通用代码将标记所有输入和所有输出
         * 支持所有采样率和每个声道数和声道布局,只要所有
         * 输入和输出使用相同的采样率和声道数/布局。
         */
        const enum AVSampleFormat *samples_list;
        /**
         * 等同于{ pix_fmt, AV_PIX_FMT_NONE }作为pixels_list。
         */
        enum AVPixelFormat  pix_fmt;
        /**
         * 等同于{ sample_fmt, AV_SAMPLE_FMT_NONE }作为samples_list。
         */
        enum AVSampleFormat sample_fmt;
    } formats;

    int priv_size;      ///< 为过滤器分配的私有数据的大小

    int flags_internal; ///< 仅供avfilter内部使用的附加标志。

    /**
     * 使过滤器实例处理命令。
     *
     * @param cmd    要处理的命令,为了处理简单,所有命令必须只包含字母数字
     * @param arg    命令的参数
     * @param res    大小为res_size的缓冲区,过滤器可以在其中返回响应。当命令不支持时,这不能改变。
     * @param flags  如果设置了AVFILTER_CMD_FLAG_FAST且命令会
     *               耗时,则过滤器应将其视为不支持的命令
     *
     * @returns 成功时返回>=0,否则返回错误代码。
     *          不支持的命令返回AVERROR(ENOSYS)
     */
    int (*process_command)(AVFilterContext *, const char *cmd, const char *arg, char *res, int res_len, int flags);

    /**
     * 过滤器激活函数。
     *
     * 当需要从过滤器进行任何处理时调用,而不是在pad上
     * 调用任何filter_frame和request_frame。
     *
     * 该函数必须检查输入链接和输出链接并执行单个
     * 处理步骤。如果没有要做的事情,该函数必须
     * 什么都不做且不返回错误。如果更多步骤是可能的
     * 或可能的,它必须使用ff_filter_set_ready()来
     * 安排另一次激活。
     */
    int (*activate)(AVFilterContext *ctx);
} AVFilter;

/**
 * 获取AVFilter的输入或输出数组中的元素数量。
 */
unsigned avfilter_filter_pad_count(const AVFilter *filter, int is_output);

/**
 * 并发处理帧的多个部分。
 */
#define AVFILTER_THREAD_SLICE (1 << 0)

typedef struct AVFilterInternal AVFilterInternal;

/** 过滤器的一个实例 */
struct AVFilterContext {
    const AVClass *av_class;        ///< 用于av_log()和过滤器通用选项所需

    const AVFilter *filter;         ///< 此实例所属的AVFilter

    char *name;                     ///< 此过滤器实例的名称

    AVFilterPad   *input_pads;      ///< 输入pad数组
    AVFilterLink **inputs;          ///< 指向输入链接的指针数组
    unsigned    nb_inputs;          ///< 输入pad的数量

    AVFilterPad   *output_pads;     ///< 输出pad数组
    AVFilterLink **outputs;         ///< 指向输出链接的指针数组
    unsigned    nb_outputs;         ///< 输出pad的数量

    void *priv;                     ///< 供过滤器使用的私有数据

    struct AVFilterGraph *graph;    ///< 此过滤器所属的过滤器图

    /**
     * 允许/使用的多线程类型。AVFILTER_THREAD_* 标志的组合。
     *
     * 在初始化过滤器之前,调用者可以设置此字段以禁止此过滤器的某些
     * 或所有类型的多线程。默认允许所有类型。
     *
     * 当过滤器初始化时,此字段与AVFilterGraph.thread_type进行按位与运算
     * 以获得用于确定允许的线程类型的最终掩码。即一个线程类型需要在两处
     * 都设置才被允许。
     *
     * 过滤器初始化后,libavfilter将此字段设置为实际使用的线程类型
     * (0表示无多线程)。
     */
    int thread_type;

    /**
     * 供libavfilter内部使用的不透明结构。
     */
    AVFilterInternal *internal;

    struct AVFilterCommand *command_queue;

    char *enable_str;               ///< 启用表达式字符串
    void *enable;                   ///< 解析后的表达式(AVExpr*)
    double *var_values;             ///< 启用表达式的变量值
    int is_disabled;                ///< 上次表达式求值的启用状态

    /**
     * 对于将创建硬件帧的过滤器,设置过滤器应在其中创建帧的设备。
     * 所有其他过滤器将忽略此字段:特别是,消费或处理硬件帧的过滤器
     * 将使用AVFilterLink中的hw_frames_ctx字段来携带硬件上下文信息。
     *
     * 在使用avfilter_init_str()或avfilter_init_dict()初始化过滤器之前,
     * 可由调用者在标记为AVFILTER_FLAG_HWDEVICE的过滤器上设置。
     */
    AVBufferRef *hw_device_ctx;

    /**
     * 此过滤器实例允许的最大线程数。
     * 如果 <= 0,则忽略其值。
     * 覆盖每个过滤器图设置的全局线程数。
     */
    int nb_threads;

    /**
     * 过滤器的就绪状态。
     * 非0值表示过滤器需要激活;
     * 更高的值表示更紧急的激活需求。
     */
    unsigned ready;

    /**
     * 设置过滤器将在其输出链接上分配的额外硬件帧数量,
     * 供后续过滤器或调用者使用。
     *
     * 某些硬件过滤器要求在开始过滤之前预先定义所有将用于
     * 输出的帧。对于这类过滤器,用于输出的任何硬件帧池
     * 必须是固定大小的。这里设置的额外帧数是在过滤器正常
     * 运行所需的帧数之外的。
     *
     * 必须在配置包含此过滤器的图之前设置此字段。
     */
    int extra_hw_frames;
};

/**
 * 链接一端支持的格式/等列表。
 *
 * 此结构直接作为AVFilterLink的一部分,有两个副本:
 * 一个用于源过滤器,一个用于目标过滤器。
 *
 * 这些列表用于协商实际要使用的格式,
 * 选定后将加载到AVFilterLink的format和channel_layout成员中。
 */
typedef struct AVFilterFormatsConfig {

    /**
     * 支持的格式列表(像素或采样)。
     */
    AVFilterFormats *formats;

    /**
     * 支持的采样率列表,仅用于音频。
     */
    AVFilterFormats  *samplerates;

    /**
     * 支持的声道布局列表,仅用于音频。
     */
    AVFilterChannelLayouts  *channel_layouts;

} AVFilterFormatsConfig;

/**
 * 两个过滤器之间的链接。包含此链接存在的源过滤器和
 * 目标过滤器之间的指针,以及涉及的pad索引。此外,
 * 此链接还包含在过滤器之间协商和约定的参数,如
 * 图像尺寸、格式等。
 *
 * 应用程序通常不应直接访问链接结构。
 * 应使用buffersrc和buffersink API。
 * 将来,对头文件的访问可能仅限于过滤器实现。
 */
struct AVFilterLink {
    AVFilterContext *src;       ///< 源过滤器
    AVFilterPad *srcpad;        ///< 源过滤器上的输出pad

    AVFilterContext *dst;       ///< 目标过滤器
    AVFilterPad *dstpad;        ///< 目标过滤器上的输入pad

    enum AVMediaType type;      ///< 过滤器媒体类型

    /* 这些参数仅适用于视频 */
    int w;                      ///< 约定的图像宽度
    int h;                      ///< 约定的图像高度
    AVRational sample_aspect_ratio; ///< 约定的采样宽高比
    /* 这些参数仅适用于音频 */
#if FF_API_OLD_CHANNEL_LAYOUT
    /**
     * 当前缓冲区的声道布局(参见libavutil/channel_layout.h)
     * @deprecated 使用ch_layout
     */
    attribute_deprecated
    uint64_t channel_layout;
#endif
    int sample_rate;            ///< 每秒采样数

    int format;                 ///< 约定的媒体格式

    /**
     * 定义将通过此链接传递的帧/样本的PTS使用的时基。
     * 在配置阶段,每个过滤器应该只改变输出时基,
     * 而输入链接的时基被认为是一个不可改变的属性。
     */
    AVRational time_base;

    AVChannelLayout ch_layout;  ///< 当前缓冲区的声道布局(参见libavutil/channel_layout.h)

    /*****************************************************************
     * 此行以下的所有字段都不是公共API的一部分。它们
     * 不能在libavfilter外部使用,并且可以随意更改和
     * 删除。
     * 新的公共字段应该添加在上面。
     *****************************************************************
     */

    /**
     * 输入过滤器支持的格式/等列表。
     */
    AVFilterFormatsConfig incfg;

    /**
     * 输出过滤器支持的格式/等列表。
     */
    AVFilterFormatsConfig outcfg;

    /** 链接属性(尺寸等)的初始化阶段 */
    enum {
        AVLINK_UNINIT = 0,      ///< 未开始
        AVLINK_STARTINIT,       ///< 已开始,但不完整
        AVLINK_INIT             ///< 完成
    } init_state;

    /**
     * 过滤器所属的图。
     */
    struct AVFilterGraph *graph;

    /**
     * 链接的当前时间戳,由最近的帧定义,
     * 以链接time_base为单位。
     */
    int64_t current_pts;

    /**
     * 链接的当前时间戳,由最近的帧定义,
     * 以AV_TIME_BASE为单位。
     */
    int64_t current_pts_us;

    /**
     * 在age数组中的索引。
     */
    int age_index;

    /**
     * 链接上流的帧率,如果未知或可变则为1/0;
     * 如果保持为0/0,将自动从源过滤器的第一个输入复制
     * (如果存在)。
     *
     * 源应该设置为实际帧率的最佳估计。
     * 如果源帧率未知或可变,设置为1/0。
     * 过滤器应根据其功能在必要时更新它。
     * 接收器可以使用它来设置默认输出帧率。
     * 它类似于AVStream中的r_frame_rate字段。
     */
    AVRational frame_rate;

    /**
     * 一次过滤的最小样本数。如果使用较少的样本调用
     * filter_frame(),它将在fifo中累积它们。
     * 过滤开始后不能更改此字段和相关字段。
     * 如果为0,则忽略所有相关字段。
     */
    int min_samples;

    /**
     * 一次过滤的最大样本数。如果使用更多样本调用
     * filter_frame(),它将分割它们。
     */
    int max_samples;

    /**
     * 通过链接发送的过去帧数。
     */
    int64_t frame_count_in, frame_count_out;

    /**
     * 通过链接发送的过去样本数。
     */
    int64_t sample_count_in, sample_count_out;

    /**
     * 指向FFFramePool结构的指针。
     */
    void *frame_pool;

    /**
     * 如果当前在此过滤器的输出上需要一帧则为True。
     * 当输出调用ff_request_frame()时设置,
     * 当过滤一帧时清除。
     */
    int frame_wanted_out;

    /**
     * 对于hwaccel像素格式,这应该是对描述帧的
     * AVHWFramesContext的引用。
     */
    AVBufferRef *hw_frames_ctx;

#ifndef FF_INTERNAL_FIELDS

    /**
     * 内部结构成员。
     * 此限制以下的字段是供libavfilter内部使用的,
     * 在任何情况下都不能被应用程序访问。
     */
    char reserved[0xF000];

#else /* FF_INTERNAL_FIELDS */

    /**
     * 等待过滤的帧队列。
     */
    FFFrameQueue fifo;

    /**
     * 如果设置,源过滤器目前无法生成帧。
     * 目的是避免在同一链接上重复调用request_frame()方法。
     */
    int frame_blocked_in;

    /**
     * 链接输入状态。
     * 如果非零,所有filter_frame尝试都将以相应的代码失败。
     */
    int status_in;

    /**
     * 输入状态更改的时间戳。
     */
    int64_t status_in_pts;

    /**
     * 链接输出状态。
     * 如果非零,所有request_frame尝试都将以相应的代码失败。
     */
    int status_out;

#endif /* FF_INTERNAL_FIELDS */

};

/**
 * 将两个过滤器链接在一起。
 *
 * @param src    源过滤器
 * @param srcpad 源过滤器上输出pad的索引
 * @param dst    目标过滤器
 * @param dstpad 目标过滤器上输入pad的索引
 * @return       成功时返回零
 */
int avfilter_link(AVFilterContext *src, unsigned srcpad,
                  AVFilterContext *dst, unsigned dstpad);

/**
 * 释放*link中的链接,并将其指针设置为NULL。
 */
void avfilter_link_free(AVFilterLink **link);

/**
 * 协商过滤器所有输入的媒体格式、尺寸等。
 *
 * @param filter 要为其输入协商属性的过滤器
 * @return       成功协商时返回零
 */
int avfilter_config_links(AVFilterContext *filter);

#define AVFILTER_CMD_FLAG_ONE   1 ///< 一旦过滤器理解了命令就停止(例如对于target=all),自动优先考虑快速过滤器
#define AVFILTER_CMD_FLAG_FAST  2 ///< 仅在命令快速时执行(如支持硬件对比度调整的视频输出)

/**
 * 使过滤器实例处理命令。
 * 建议使用avfilter_graph_send_command()。
 */
int avfilter_process_command(AVFilterContext *filter, const char *cmd, const char *arg, char *res, int res_len, int flags);

/**
 * 遍历所有已注册的过滤器。
 *
 * @param opaque 一个指针,libavfilter将在其中存储迭代状态。必须
 *               指向NULL以开始迭代。
 *
 * @return 下一个已注册的过滤器,如果迭代结束则返回NULL
 */
const AVFilter *av_filter_iterate(void **opaque);

/**
 * 获取与给定名称匹配的过滤器定义。
 *
 * @param name 要查找的过滤器名称
 * @return     如果有匹配的过滤器定义则返回该定义。
 *             如果未找到则返回NULL。
 */
const AVFilter *avfilter_get_by_name(const char *name);

/**
 * 使用提供的参数初始化过滤器。
 *
 * @param ctx  要初始化的未初始化的过滤器上下文
 * @param args 用于初始化过滤器的选项。这必须是一个
 *             以':'分隔的'key=value'形式的选项列表。
 *             如果选项已通过AVOptions API直接设置或
 *             没有需要设置的选项,则可以为NULL。
 * @return 成功时返回0,失败时返回负的AVERROR
 */
int avfilter_init_str(AVFilterContext *ctx, const char *args);

/**
 * 使用提供的选项字典初始化过滤器。
 *
 * @param ctx     要初始化的未初始化的过滤器上下文
 * @param options 一个填充了此过滤器选项的AVDictionary。在
 *                返回时,此参数将被销毁并替换为包含未找到
 *                选项的字典。调用者必须释放此字典。
 *                可以为NULL,则此函数等同于将第二个参数
 *                设置为NULL的avfilter_init_str()。
 * @return 成功时返回0,失败时返回负的AVERROR
 *
 * @note 此函数和avfilter_init_str()本质上做相同的事情,
 * 区别在于传递选项的方式。由调用代码选择更合适的方式。
 * 当过滤器不支持某些提供的选项时,这两个函数的行为也不同。
 * 在这种情况下,avfilter_init_str()将失败,但此函数会将这些
 * 额外选项保留在options AVDictionary中并继续正常执行。
 */
int avfilter_init_dict(AVFilterContext *ctx, AVDictionary **options);

/**
 * 释放过滤器上下文。这也会从其过滤器图的过滤器列表中
 * 移除该过滤器。
 *
 * @param filter 要释放的过滤器
 */
void avfilter_free(AVFilterContext *filter);

/**
 * 在现有链接中间插入过滤器。
 *
 * @param link 要插入过滤器的链接
 * @param filt 要插入的过滤器
 * @param filt_srcpad_idx 要连接的过滤器输入pad
 * @param filt_dstpad_idx 要连接的过滤器输出pad
 * @return     成功时返回零
 */
int avfilter_insert_filter(AVFilterLink *link, AVFilterContext *filt,
                           unsigned filt_srcpad_idx, unsigned filt_dstpad_idx);

/**
 * @return AVFilterContext的AVClass。
 *
 * @see av_opt_find()。
 */
const AVClass *avfilter_get_class(void);

typedef struct AVFilterGraphInternal AVFilterGraphInternal;

/**
 * 传递给@ref AVFilterGraph.execute回调的函数指针,
 * 用于多次执行,可能并行执行。
 *
 * @param ctx 作业所属的过滤器上下文
 * @param arg 从@ref AVFilterGraph.execute传递的不透明参数
 * @param jobnr 正在执行的作业索引
 * @param nb_jobs 作业总数
 *
 * @return 成功时返回0,错误时返回负的AVERROR
 */
typedef int (avfilter_action_func)(AVFilterContext *ctx, void *arg, int jobnr, int nb_jobs);

/**
 * 执行多个作业的函数,可能并行执行。
 *
 * @param ctx 作业所属的过滤器上下文
 * @param func 要多次调用的函数
 * @param arg 要传递给func的参数
 * @param ret 大小为nb_jobs的数组,用于填充每次调用func的返回值
 * @param nb_jobs 要执行的作业数
 *
 * @return 成功时返回0,错误时返回负的AVERROR
 */
typedef int (avfilter_execute_func)(AVFilterContext *ctx, avfilter_action_func *func,
                                    void *arg, int *ret, int nb_jobs);

typedef struct AVFilterGraph {
    const AVClass *av_class;
    AVFilterContext **filters;
    unsigned nb_filters;

    char *scale_sws_opts; ///< 用于自动插入的scale过滤器的sws选项

    /**
     * 此图中过滤器允许的多线程类型。AVFILTER_THREAD_*标志的组合。
     *
     * 调用者可以在任何时候设置,该设置将应用于之后初始化的所有过滤器。
     * 默认允许所有类型。
     *
     * 当此图中的过滤器被初始化时,此字段与AVFilterContext.thread_type
     * 进行按位与运算以获得用于确定允许线程类型的最终掩码。即线程类型
     * 需要在两处都设置才被允许。
     */
    int thread_type;

    /**
     * 此图中过滤器使用的最大线程数。调用者可以在向过滤器图添加任何
     * 过滤器之前设置。零(默认值)表示线程数自动确定。
     */
    int nb_threads;

    /**
     * libavfilter内部使用的不透明对象。
     */
    AVFilterGraphInternal *internal;

    /**
     * 不透明的用户数据。调用者可以设置为任意值,例如从@ref AVFilterGraph.execute
     * 等回调中使用。libavfilter不会以任何方式触及此字段。
     */
    void *opaque;

    /**
     * 调用者可以在分配图后立即设置此回调,并在添加任何过滤器之前,
     * 以提供自定义的多线程实现。
     *
     * 如果设置,具有切片线程能力的过滤器将调用此回调以并行执行多个作业。
     *
     * 如果此字段未设置,libavfilter将使用其内部实现,根据平台和构建选项,
     * 该实现可能是多线程的也可能不是。
     */
    avfilter_execute_func *execute;

    char *aresample_swr_opts; ///< 用于自动插入的aresample过滤器的swr选项,仅通过AVOptions访问

    /**
     * 私有字段
     *
     * 以下字段仅供内部使用。
     * 它们的类型、偏移量、数量和语义可能在没有通知的情况下更改。
     */

    AVFilterLink **sink_links;
    int sink_links_count;

    unsigned disable_auto_convert;
} AVFilterGraph;

/**
 * 分配一个过滤器图。
 *
 * @return 成功时返回分配的过滤器图,否则返回NULL。
 */
AVFilterGraph *avfilter_graph_alloc(void);

/**
 * 在过滤器图中创建新的过滤器实例。
 *
 * @param graph 新过滤器将使用的图
 * @param filter 要创建实例的过滤器
 * @param name 给新实例的名称(将被复制到AVFilterContext.name)。
 *             调用者可以使用此参数来识别不同的过滤器,libavfilter本身
 *             不为此参数分配语义。可以为NULL。
 *
 * @return 新创建的过滤器实例的上下文(注意它也可以直接通过
 *         AVFilterGraph.filters或avfilter_graph_get_filter()获取)
 *         成功时返回,失败时返回NULL。
 */
AVFilterContext *avfilter_graph_alloc_filter(AVFilterGraph *graph,
                                             const AVFilter *filter,
                                             const char *name);

/**
 * 从图中获取由实例名称标识的过滤器实例。
 *
 * @param graph 要搜索的过滤器图。
 * @param name 过滤器实例名称(在图中应该是唯一的)。
 * @return 找到的过滤器实例的指针,如果找不到则返回NULL。
 */
AVFilterContext *avfilter_graph_get_filter(AVFilterGraph *graph, const char *name);

/**
 * 在现有图中创建并添加过滤器实例。
 * 过滤器实例从过滤器filt创建并使用参数args初始化。
 * opaque当前被忽略。
 *
 * 如果成功,将在*filt_ctx中放入创建的过滤器实例的指针,
 * 否则将*filt_ctx设置为NULL。
 *
 * @param name 给创建的过滤器实例的实例名称
 * @param graph_ctx 过滤器图
 * @return 失败时返回负的AVERROR错误代码,否则返回非负值
 */
int avfilter_graph_create_filter(AVFilterContext **filt_ctx, const AVFilter *filt,
                                 const char *name, const char *args, void *opaque,
                                 AVFilterGraph *graph_ctx);

/**
 * 启用或禁用图内的自动格式转换。
 *
 * 注意格式转换仍然可能发生在显式插入的scale和aresample过滤器中。
 *
 * @param flags AVFILTER_AUTO_CONVERT_*常量中的任何一个
 */
void avfilter_graph_set_auto_convert(AVFilterGraph *graph, unsigned flags);

enum {
    AVFILTER_AUTO_CONVERT_ALL  =  0, /**< 启用所有自动转换 */
    AVFILTER_AUTO_CONVERT_NONE = -1, /**< 禁用所有自动转换 */
};

/**
 * 检查有效性并配置图中的所有链接和格式。
 *
 * @param graphctx 过滤器图
 * @param log_ctx 用于日志记录的上下文
 * @return 成功时返回 >= 0,否则返回负的AVERROR代码
 */
int avfilter_graph_config(AVFilterGraph *graphctx, void *log_ctx);

/**
 * 释放图,销毁其链接,并将*graph设置为NULL。
 * 如果*graph为NULL,则不执行任何操作。
 */
void avfilter_graph_free(AVFilterGraph **graph);

/**
 * 过滤器链的输入/输出的链表。
 *
 * 这主要用于avfilter_graph_parse() / avfilter_graph_parse2(),
 * 在那里它用于在调用者之间通信图中包含的开放(未链接)输入和输出。
 * 此结构为图中每个未连接的pad指定建立链接所需的过滤器上下文和pad索引。
 */
typedef struct AVFilterInOut {
    /** 列表中此输入/输出的唯一名称 */
    char *name;

    /** 与此输入/输出关联的过滤器上下文 */
    AVFilterContext *filter_ctx;

    /** 用于链接的filt_ctx pad的索引 */
    int pad_idx;

    /** 列表中的下一个输入/输入,如果这是最后一个则为NULL */
    struct AVFilterInOut *next;
} AVFilterInOut;

/**
 * 分配单个AVFilterInOut条目。
 * 必须使用avfilter_inout_free()释放。
 * @return 成功时返回分配的AVFilterInOut,失败时返回NULL。
 */
AVFilterInOut *avfilter_inout_alloc(void);

/**
 * 释放提供的AVFilterInOut列表并将*inout设置为NULL。
 * 如果*inout为NULL,则不执行任何操作。
 */
void avfilter_inout_free(AVFilterInOut **inout);

/**
 * 将由字符串描述的图添加到图中。
 *
 * @note 调用者必须提供输入和输出列表,
 * 因此必须在调用函数之前知道这些列表。
 *
 * @note inputs参数描述已存在图部分的输入;
 * 即从新创建部分的角度来看,它们是输出。
 * 类似地,outputs参数描述现有过滤器的输出,
 * 这些输出作为输入提供给解析的过滤器。
 *
 * @param graph   要链接解析的图上下文的过滤器图
 * @param filters 要解析的字符串
 * @param inputs  图的输入的链表
 * @param outputs 图的输出的链表
 * @return 成功时返回零,错误时返回负的AVERROR代码
 */
int avfilter_graph_parse(AVFilterGraph *graph, const char *filters,
                         AVFilterInOut *inputs, AVFilterInOut *outputs,
                         void *log_ctx);

/**
 * 将由字符串描述的图添加到图中。
 *
 * 在图过滤器描述中,如果未指定第一个过滤器的输入标签,
 * 则假定为"in";如果未指定最后一个过滤器的输出标签,
 * 则假定为"out"。
 *
 * @param graph   要链接解析的图上下文的过滤器图
 * @param filters 要解析的字符串
 * @param inputs  指向图输入链表的指针,可以为NULL。
 *                如果非NULL,*inputs将更新为包含解析后的开放输入列表,
 *                应使用avfilter_inout_free()释放。
 * @param outputs 指向图输出链表的指针,可以为NULL。
 *                如果非NULL,*outputs将更新为包含解析后的开放输出列表,
 *                应使用avfilter_inout_free()释放。
 * @return 成功时返回非负值,错误时返回负的AVERROR代码
 */
int avfilter_graph_parse_ptr(AVFilterGraph *graph, const char *filters,
                             AVFilterInOut **inputs, AVFilterInOut **outputs,
                             void *log_ctx);

/**
 * 将由字符串描述的图添加到图中。
 *
 * @param[in]  graph   要链接解析的图上下文的过滤器图
 * @param[in]  filters 要解析的字符串
 * @param[out] inputs  解析的图的所有空闲(未链接)输入的链表将在此返回。
 *                     调用者必须使用avfilter_inout_free()释放它。
 * @param[out] outputs 解析的图的所有空闲(未链接)输出的链表将在此返回。
 *                     调用者必须使用avfilter_inout_free()释放它。
 * @return 成功时返回零,错误时返回负的AVERROR代码
 *
 * @note 此函数返回解析图后剩余未链接的输入和输出,
 * 然后由调用者处理它们。
 * @note 此函数完全不引用图的已存在部分,返回的inputs
 * 参数将包含新解析的图部分的输入。类似地,outputs
 * 参数将包含新创建过滤器的输出。
 */
int avfilter_graph_parse2(AVFilterGraph *graph, const char *filters,
                          AVFilterInOut **inputs,
                          AVFilterInOut **outputs);

/**
 * 过滤器输入或输出pad的参数。
 *
 * 由avfilter_graph_segment_parse()创建为AVFilterParams的子项。
 * 在avfilter_graph_segment_free()中释放。
 */
typedef struct AVFilterPadParams {
    /**
     * 一个av_malloc()分配的包含pad标签的字符串。
     *
     * 调用者可以av_free()并设置为NULL,在这种情况下,此pad
     * 在链接时将被视为未标记。
     * 也可以替换为另一个av_malloc()分配的字符串。
     */
    char *label;
} AVFilterPadParams;

/**
 * 描述过滤器图中要创建的过滤器的参数。
 *
 * 由avfilter_graph_segment_parse()创建为AVFilterGraphSegment的子项。
 * 在avfilter_graph_segment_free()中释放。
 */
typedef struct AVFilterParams {
    /**
     * 过滤器上下文。
     *
     * 由avfilter_graph_segment_create_filters()基于
     * AVFilterParams.filter_name和instance_name创建。
     *
     * 调用者也可以手动创建过滤器上下文,然后他们应该
     * av_free() filter_name并设置为NULL。这样的AVFilterParams实例
     * 将被avfilter_graph_segment_create_filters()跳过。
     */
    AVFilterContext     *filter;

    /**
     * 要使用的AVFilter的名称。
     *
     * 一个由av_malloc()分配的字符串,由avfilter_graph_segment_parse()设置。将被
     * avfilter_graph_segment_create_filters()传递给avfilter_get_by_name()。
     *
     * 调用者可以av_free()此字符串并替换为另一个或NULL。如果调用者手动创建
     * 过滤器实例,此字符串必须设置为NULL。
     *
     * 当AVFilterParams.filter和AVFilterParams.filter_name都为NULL时,
     * 此AVFilterParams实例将被avfilter_graph_segment_*()函数跳过。
     */
    char                *filter_name;
    /**
     * 用于此过滤器实例的名称。
     *
     * 一个由av_malloc()分配的字符串,可能由avfilter_graph_segment_parse()设置
     * 或保留为NULL。调用者可以av_free()此字符串并替换为另一个或NULL。
     *
     * 将被avfilter_graph_segment_create_filters()使用 - 作为第三个参数
     * 传递给avfilter_graph_alloc_filter(),然后释放并设置为NULL。
     */
    char                *instance_name;

    /**
     * 要应用于过滤器的选项。
     *
     * 由avfilter_graph_segment_parse()填充。之后可以由调用者自由修改。
     *
     * 将由avfilter_graph_segment_apply_opts()应用到过滤器,
     * 等效于av_opt_set_dict2(filter, &opts, AV_OPT_SEARCH_CHILDREN),
     * 即任何未应用的选项将保留在此字典中。
     */
    AVDictionary        *opts;

    AVFilterPadParams  **inputs;
    unsigned          nb_inputs;

    AVFilterPadParams  **outputs;
    unsigned          nb_outputs;
} AVFilterParams;

/**
 * 过滤器链是过滤器规格的列表。
 *
 * 由avfilter_graph_segment_parse()创建为AVFilterGraphSegment的子项。
 * 在avfilter_graph_segment_free()中释放。
 */
typedef struct AVFilterChain {
    AVFilterParams  **filters;
    size_t         nb_filters;
} AVFilterChain;

/**
 * 过滤器图段的解析表示。
 *
 * 过滤器图段在概念上是过滤器链的列表,带有一些
 * 补充信息(例如格式转换标志)。
 *
 * 由avfilter_graph_segment_parse()创建。必须使用
 * avfilter_graph_segment_free()释放。
 */
typedef struct AVFilterGraphSegment {
    /**
     * 此段关联的过滤器图。
     * 由avfilter_graph_segment_parse()设置。
     */
    AVFilterGraph *graph;

    /**
     * 此段包含的过滤器链列表。
     * 在avfilter_graph_segment_parse()中设置。
     */
    AVFilterChain **chains;
    size_t       nb_chains;

    /**
     * 包含冒号分隔的键=值选项列表的字符串,
     * 应用于此段中的所有scale过滤器。
     *
     * 可能由avfilter_graph_segment_parse()设置。
     * 调用者可以使用av_free()释放此字符串,并用不同的
     * av_malloc()分配的字符串替换它。
     */
    char *scale_sws_opts;
} AVFilterGraphSegment;

/**
 * 将文本过滤器图描述解析为中间形式。
 *
 * 此中间表示旨在由调用者按照AVFilterGraphSegment及其子项的
 * 文档中所述进行修改,然后手动或使用其他avfilter_graph_segment_*()
 * 函数应用到图中。有关应用AVFilterGraphSegment的规范方式,
 * 请参见avfilter_graph_segment_apply()的文档。
 *
 * @param graph 解析段关联的过滤器图。仅用于日志和类似的辅助目的。
 *              此函数不会实际修改图 - 解析结果会存储在seg中供进一步处理。
 * @param graph_str 描述过滤器图段的字符串
 * @param flags 保留供将来使用,调用者现在必须设置为0
 * @param seg 成功时在此处写入新创建的AVFilterGraphSegment的指针。
 *            图段由调用者拥有,必须在释放graph本身之前使用
 *            avfilter_graph_segment_free()释放。
 *
 * @retval "非负数" 成功
 * @retval "负错误代码" 失败
 */
int avfilter_graph_segment_parse(AVFilterGraph *graph, const char *graph_str,
                                 int flags, AVFilterGraphSegment **seg);

/**
 * 创建图段中指定的过滤器。
 *
 * 遍历段中待创建的AVFilterParams并为它们创建新的过滤器实例。
 * 待创建的参数是那些AVFilterParams.filter_name非NULL的参数
 * (因此AVFilterParams.filter为NULL)。忽略所有其他AVFilterParams实例。
 *
 * 对于此函数创建的任何过滤器,相应的AVFilterParams.filter设置为
 * 新创建的过滤器上下文,AVFilterParams.filter_name和
 * AVFilterParams.instance_name被释放并设置为NULL。
 *
 * @param seg 要处理的过滤器图段
 * @param flags 保留供将来使用,调用者现在必须设置为0
 *
 * @retval "非负数" 成功,所有待创建的过滤器都已成功创建
 * @retval AVERROR_FILTER_NOT_FOUND 某些过滤器的名称不对应已知过滤器
 * @retval "其他负错误代码" 其他失败
 *
 * @note 多次调用此函数是安全的,因为它是幂等的。
 */
int avfilter_graph_segment_create_filters(AVFilterGraphSegment *seg, int flags);

/**
 * 将解析的选项应用于图段中的过滤器实例。
 *
 * 遍历图段中所有具有关联选项字典的过滤器实例,并使用
 * av_opt_set_dict2(..., AV_OPT_SEARCH_CHILDREN)应用这些选项。
 * AVFilterParams.opts被av_opt_set_dict2()输出的字典替换,
 * 如果所有选项都成功应用,该字典应为空(NULL)。
 *
 * 如果任何选项未找到,此函数将继续处理所有其他过滤器,
 * 最后返回AVERROR_OPTION_NOT_FOUND(除非发生其他错误)。
 * 调用程序可以根据需要处理未应用的选项。
 *
 * 段中存在的任何待创建过滤器(参见avfilter_graph_segment_create_filters())
 * 都将导致此函数失败。简单跳过没有关联过滤器上下文的AVFilterParams。
 *
 * @param seg 要处理的过滤器图段
 * @param flags 保留供将来使用,调用者现在必须设置为0
 *
 * @retval "非负数" 成功,所有选项都已成功应用。
 * @retval AVERROR_OPTION_NOT_FOUND 在过滤器中未找到某些选项
 * @retval "其他负错误代码" 其他失败
 *
 * @note 多次调用此函数是安全的,因为它是幂等的。
 */
int avfilter_graph_segment_apply_opts(AVFilterGraphSegment *seg, int flags);

/**
 * 初始化图段中的所有过滤器实例。
 *
 * 遍历图段中的所有过滤器实例,对尚未初始化的过滤器调用
 * avfilter_init_dict(..., NULL)。
 *
 * 段中存在的任何待创建过滤器(参见avfilter_graph_segment_create_filters())
 * 都将导致此函数失败。简单跳过没有关联过滤器上下文或其过滤器上下文
 * 已初始化的AVFilterParams。
 *
 * @param seg 要处理的过滤器图段
 * @param flags 保留供将来使用,调用者现在必须设置为0
 *
 * @retval "非负数" 成功,所有过滤器实例都已成功初始化
 * @retval "负错误代码" 失败
 *
 * @note 多次调用此函数是安全的,因为它是幂等的。
 */
int avfilter_graph_segment_init(AVFilterGraphSegment *seg, int flags);

/**
 * 链接图段中的过滤器。
 *
 * 遍历图段中的所有过滤器实例,并尝试链接所有未链接的输入和输出pad。
 * 段中存在的任何待创建过滤器(参见avfilter_graph_segment_create_filters())
 * 都将导致此函数失败。跳过禁用的过滤器和已链接的pad。
 *
 * 每个具有非NULL标签的相应AVFilterPadParams的过滤器输出pad都会:
 * - 如果存在匹配标签的输入,则链接到该输入;
 * - 否则导出到outputs链表中,保留标签。
 * 未标记的输出会:
 * - 如果存在,则链接到链中下一个未禁用过滤器中的第一个未链接未标记输入
 * - 否则导出到outputs链表中,标签为NULL
 *
 * 类似地,未链接的输入pad会导出到inputs链表中。
 *
 * @param seg 要处理的过滤器图段
 * @param flags 保留供将来使用,调用者现在必须设置为0
 * @param[out] inputs  此图段中过滤器的所有空闲(未链接)输入的链表将在此返回。
 *                     调用者必须使用avfilter_inout_free()释放它。
 * @param[out] outputs 此图段中过滤器的所有空闲(未链接)输出的链表将在此返回。
 *                     调用者必须使用avfilter_inout_free()释放它。
 *
 * @retval "非负数" 成功
 * @retval "负错误代码" 失败
 *
 * @note 多次调用此函数是安全的,因为它是幂等的。
 */
int avfilter_graph_segment_link(AVFilterGraphSegment *seg, int flags,
                                AVFilterInOut **inputs,
                                AVFilterInOut **outputs);

/**
 * 将图段中的所有过滤器/链接描述应用到关联的过滤器图。
 *
 * 此函数当前等效于按顺序调用以下函数:
 * - avfilter_graph_segment_create_filters();
 * - avfilter_graph_segment_apply_opts();
 * - avfilter_graph_segment_init();
 * - avfilter_graph_segment_link();
 * 如果其中任何一个失败则失败。此列表将来可能会扩展。
 *
 * 由于上述函数是幂等的,调用者可以手动调用其中一些函数,
 * 然后对过滤器图进行一些自定义处理,然后调用此函数完成其余工作。
 *
 * @param seg 要处理的过滤器图段
 * @param flags 保留供将来使用,调用者现在必须设置为0
 * @param[out] inputs 传递给avfilter_graph_segment_link()
 * @param[out] outputs 传递给avfilter_graph_segment_link()
 *
 * @retval "非负数" 成功
 * @retval "负错误代码" 失败
 *
 * @note 多次调用此函数是安全的,因为它是幂等的。
 */
int avfilter_graph_segment_apply(AVFilterGraphSegment *seg, int flags,
                                 AVFilterInOut **inputs,
                                 AVFilterInOut **outputs);

/**
 * 释放提供的AVFilterGraphSegment及其关联的所有内容。
 *
 * @param seg 指向要释放的AVFilterGraphSegment的双重指针。退出此函数时
 * 将向此指针写入NULL。
 *
 * @note
 * 过滤器上下文(AVFilterParams.filter)由AVFilterGraph而不是
 * AVFilterGraphSegment拥有,因此不会被释放。
 */
void avfilter_graph_segment_free(AVFilterGraphSegment **seg);

/**
 * 向一个或多个过滤器实例发送命令。
 *
 * @param graph  过滤器图
 * @param target 应该接收命令的过滤器
 *               "all"发送到所有过滤器
 *               否则可以是过滤器或过滤器实例名称
 *               这将把命令发送到所有匹配的过滤器。
 * @param cmd    要发送的命令,为了处理简单所有命令必须只包含字母数字
 * @param arg    命令的参数
 * @param res    大小为res_size的缓冲区,过滤器可以在其中返回响应。
 *
 * @returns >=0表示成功,否则返回错误代码。
 *              对于不支持的命令返回AVERROR(ENOSYS)
 */
int avfilter_graph_send_command(AVFilterGraph *graph, const char *target, const char *cmd, const char *arg, char *res, int res_len, int flags);

/**
 * 为一个或多个过滤器实例排队命令。
 *
 * @param graph  过滤器图
 * @param target 应该接收命令的过滤器
 *               "all"发送到所有过滤器
 *               否则可以是过滤器或过滤器实例名称
 *               这将把命令发送到所有匹配的过滤器。
 * @param cmd    要发送的命令,为了处理简单所有命令必须只包含字母数字
 * @param arg    命令的参数
 * @param ts     应该向过滤器发送命令的时间
 *
 * @note 由于这在函数返回后执行命令,因此不提供来自过滤器的返回代码,
 *       也不支持AVFILTER_CMD_FLAG_ONE。
 */
int avfilter_graph_queue_command(AVFilterGraph *graph, const char *target, const char *cmd, const char *arg, int flags, double ts);

/**
 * 将图转储为人类可读的字符串表示。
 *
 * @param graph    要转储的图
 * @param options  格式化选项;当前被忽略
 * @return  一个字符串,或在内存分配失败的情况下为NULL;
 *          必须使用av_free释放该字符串
 */
char *avfilter_graph_dump(AVFilterGraph *graph, const char *options);

/**
 * 在最旧的接收链接上请求一帧。
 *
 * 如果请求返回AVERROR_EOF,则尝试下一个。
 *
 * 注意此函数不是要作为过滤器图的唯一调度机制,
 * 只是一个帮助在正常情况下以平衡方式耗尽过滤器图的便利函数。
 *
 * 还要注意AVERROR_EOF并不意味着在此过程中某些接收器上没有收到帧。
 * 当有多个接收链接时,如果请求的链接返回EOF,这可能导致过滤器
 * 刷新待处理帧,这些帧被发送到另一个接收链接,尽管未被请求。
 *
 * @return  ff_request_frame()的返回值,
 *          或如果所有链接都返回AVERROR_EOF则返回AVERROR_EOF
 */
int avfilter_graph_request_oldest(AVFilterGraph *graph);

/**
 * @}
 */

#endif /* AVFILTER_AVFILTER_H */
