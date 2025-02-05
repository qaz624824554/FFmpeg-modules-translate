#ifndef AVFILTER_BUFFERSRC_H
#define AVFILTER_BUFFERSRC_H

/**
 * @file
 * @ingroup lavfi_buffersrc
 * 内存缓冲区源API。
 */

#include "avfilter.h"

/**
 * @defgroup lavfi_buffersrc 缓冲区源API
 * @ingroup lavfi
 * @{
 */

enum {

    /**
     * 不检查格式变化。
     */
    AV_BUFFERSRC_FLAG_NO_CHECK_FORMAT = 1,

    /**
     * 立即将帧推送到输出。
     */
    AV_BUFFERSRC_FLAG_PUSH = 4,

    /**
     * 保持对帧的引用。
     * 如果帧是引用计数的,则创建一个新的引用;
     * 否则复制帧数据。
     */
    AV_BUFFERSRC_FLAG_KEEP_REF = 8,

};

/**
 * 获取失败请求的数量。
 *
 * 失败请求是指在缓冲区中没有帧时调用request_frame方法。
 * 当添加一帧时,该数量会重置。
 */
unsigned av_buffersrc_get_nb_failed_requests(AVFilterContext *buffer_src);

/**
 * 此结构包含描述将传递给此过滤器的帧的参数。
 *
 * 应使用av_buffersrc_parameters_alloc()分配,并使用av_free()释放。
 * 其中所有已分配的字段仍归调用者所有。
 */
typedef struct AVBufferSrcParameters {
    /**
     * 视频:像素格式,值对应于枚举AVPixelFormat
     * 音频:采样格式,值对应于枚举AVSampleFormat
     */
    int format;
    /**
     * 用于输入帧时间戳的时基。
     */
    AVRational time_base;

    /**
     * 仅视频,输入帧的显示尺寸。
     */
    int width, height;

    /**
     * 仅视频,采样(像素)宽高比。
     */
    AVRational sample_aspect_ratio;

    /**
     * 仅视频,输入视频的帧率。此字段仅在输入流具有已知的恒定帧率时
     * 才应设置为非零值,如果帧率是可变的或未知的,则应保持其初始值。
     */
    AVRational frame_rate;

    /**
     * 仅适用于具有hwaccel像素格式的视频。这应该是对描述输入帧的
     * AVHWFramesContext实例的引用。
     */
    AVBufferRef *hw_frames_ctx;

    /**
     * 仅音频,音频采样率(每秒采样数)。
     */
    int sample_rate;

#if FF_API_OLD_CHANNEL_LAYOUT
    /**
     * 仅音频,音频通道布局
     * @deprecated 使用ch_layout
     */
    attribute_deprecated
    uint64_t channel_layout;
#endif

    /**
     * 仅音频,音频通道布局
     */
    AVChannelLayout ch_layout;
} AVBufferSrcParameters;

/**
 * 分配一个新的AVBufferSrcParameters实例。应由调用者使用av_free()释放。
 */
AVBufferSrcParameters *av_buffersrc_parameters_alloc(void);

/**
 * 使用提供的参数初始化buffersrc或abuffersrc过滤器。
 * 此函数可以多次调用,后面的调用会覆盖前面的调用。
 * 某些参数也可以通过AVOptions设置,然后无论使用哪种方法,
 * 最后使用的方法优先。
 *
 * @param ctx buffersrc或abuffersrc过滤器的实例
 * @param param 流参数。稍后传递给此过滤器的帧必须符合这些参数。
 *             param中所有已分配的字段仍归调用者所有,libavfilter将在
 *             必要时进行内部复制或引用。
 * @return 成功返回0,失败返回负的AVERROR代码。
 */
int av_buffersrc_parameters_set(AVFilterContext *ctx, AVBufferSrcParameters *param);

/**
 * 向缓冲区源添加一帧。
 *
 * @param ctx   buffersrc过滤器的实例
 * @param frame 要添加的帧。如果帧是引用计数的,此函数将创建一个新的引用。
 *             否则将复制帧数据。
 *
 * @return 成功返回0,错误返回负的AVERROR
 *
 * 此函数等同于带有AV_BUFFERSRC_FLAG_KEEP_REF标志的av_buffersrc_add_frame_flags()。
 */
av_warn_unused_result
int av_buffersrc_write_frame(AVFilterContext *ctx, const AVFrame *frame);

/**
 * 向缓冲区源添加一帧。
 *
 * @param ctx   buffersrc过滤器的实例
 * @param frame 要添加的帧。如果帧是引用计数的,此函数将获取引用的所有权
 *             并重置帧。否则将复制帧数据。如果此函数返回错误,
 *             输入帧不会被修改。
 *
 * @return 成功返回0,错误返回负的AVERROR。
 *
 * @note 此函数与av_buffersrc_write_frame()的区别在于
 * av_buffersrc_write_frame()创建对输入帧的新引用,
 * 而此函数获取传递给它的引用的所有权。
 *
 * 此函数等同于不带AV_BUFFERSRC_FLAG_KEEP_REF标志的av_buffersrc_add_frame_flags()。
 */
av_warn_unused_result
int av_buffersrc_add_frame(AVFilterContext *ctx, AVFrame *frame);

/**
 * 向缓冲区源添加一帧。
 *
 * 默认情况下,如果帧是引用计数的,此函数将获取引用的所有权并重置帧。
 * 这可以使用标志来控制。
 *
 * 如果此函数返回错误,输入帧不会被修改。
 *
 * @param buffer_src  指向缓冲区源上下文的指针
 * @param frame       一帧,或NULL表示EOF
 * @param flags       AV_BUFFERSRC_FLAG_*的组合
 * @return           成功时返回 >= 0,失败时返回负的AVERROR代码
 */
av_warn_unused_result
int av_buffersrc_add_frame_flags(AVFilterContext *buffer_src,
                                 AVFrame *frame, int flags);

/**
 * EOF后关闭缓冲区源。
 *
 * 这类似于向av_buffersrc_add_frame_flags()传递NULL,
 * 不同之处在于它接受EOF的时间戳,即最后一帧结束的时间戳。
 */
int av_buffersrc_close(AVFilterContext *ctx, int64_t pts, unsigned flags);

/**
 * @}
 */

#endif /* AVFILTER_BUFFERSRC_H */
