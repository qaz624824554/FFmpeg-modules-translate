#ifndef AVFILTER_BUFFERSINK_H
#define AVFILTER_BUFFERSINK_H

/**
 * @file
 * @ingroup lavfi_buffersink
 * 音视频内存缓冲区接收器API
 */

#include "avfilter.h"

/**
 * @defgroup lavfi_buffersink 缓冲区接收器API
 * @ingroup lavfi
 * @{
 *
 * buffersink和abuffersink过滤器用于将过滤器图连接到应用程序。
 * 它们有一个连接到图的单一输入,没有输出。
 * 必须使用av_buffersink_get_frame()或av_buffersink_get_samples()来提取帧。
 *
 * 在配置期间由图协商的格式可以使用以下访问器函数获取:
 * - av_buffersink_get_time_base(),
 * - av_buffersink_get_format(),
 * - av_buffersink_get_frame_rate(),
 * - av_buffersink_get_w(),
 * - av_buffersink_get_h(),
 * - av_buffersink_get_sample_aspect_ratio(),
 * - av_buffersink_get_channels(),
 * - av_buffersink_get_ch_layout(),
 * - av_buffersink_get_sample_rate().
 *
 * av_buffersink_get_ch_layout()返回的布局必须由调用者取消初始化。
 *
 * 可以通过使用带有AV_OPT_SEARCH_CHILDREN标志的av_opt_set()和相关函数来设置选项来约束格式。
 *  - pix_fmts (整数列表),
 *  - sample_fmts (整数列表),
 *  - sample_rates (整数列表),
 *  - ch_layouts (字符串),
 *  - channel_counts (整数列表),
 *  - all_channel_counts (布尔值)。
 * 这些选项大多数是二进制类型,应该使用av_opt_set_int_list()或av_opt_set_bin()来设置。
 * 如果未设置,则接受所有相应的格式。
 *
 * 作为特殊情况,如果未设置ch_layouts,则接受所有有效的通道布局,
 * 除了UNSPEC布局(除非设置了all_channel_counts)。
 */

/**
 * 从接收器获取经过过滤的数据帧并将其放入frame中。
 *
 * @param ctx    指向buffersink或abuffersink过滤器上下文的指针。
 * @param frame  指向将填充数据的已分配帧的指针。
 *               必须使用av_frame_unref() / av_frame_free()释放数据
 * @param flags  AV_BUFFERSINK_FLAG_*标志的组合
 *
 * @return  成功时返回 >= 0,失败时返回负的AVERROR代码。
 */
int av_buffersink_get_frame_flags(AVFilterContext *ctx, AVFrame *frame, int flags);

/**
 * 告诉av_buffersink_get_buffer_ref()读取视频/样本缓冲区引用,
 * 但不从缓冲区中移除它。如果你只需要读取视频/样本缓冲区而不需要获取它,这很有用。
 */
#define AV_BUFFERSINK_FLAG_PEEK 1

/**
 * 告诉av_buffersink_get_buffer_ref()不要从其输入请求帧。
 * 如果已经缓冲了一帧,则读取它(并从缓冲区中移除),
 * 但如果没有帧存在,则返回AVERROR(EAGAIN)。
 */
#define AV_BUFFERSINK_FLAG_NO_REQUEST 2

/**
 * 为音频缓冲区接收器设置帧大小。
 *
 * 所有对av_buffersink_get_buffer_ref的调用将返回一个具有
 * 指定数量样本的缓冲区,如果没有足够的样本则返回AVERROR(EAGAIN)。
 * EOF时的最后一个缓冲区将用0填充。
 */
void av_buffersink_set_frame_size(AVFilterContext *ctx, unsigned frame_size);

/**
 * @defgroup lavfi_buffersink_accessors 缓冲区接收器访问器
 * 获取流的属性
 * @{
 */

enum AVMediaType av_buffersink_get_type                (const AVFilterContext *ctx);
AVRational       av_buffersink_get_time_base           (const AVFilterContext *ctx);
int              av_buffersink_get_format              (const AVFilterContext *ctx);

AVRational       av_buffersink_get_frame_rate          (const AVFilterContext *ctx);
int              av_buffersink_get_w                   (const AVFilterContext *ctx);
int              av_buffersink_get_h                   (const AVFilterContext *ctx);
AVRational       av_buffersink_get_sample_aspect_ratio (const AVFilterContext *ctx);

int              av_buffersink_get_channels            (const AVFilterContext *ctx);
#if FF_API_OLD_CHANNEL_LAYOUT
attribute_deprecated
uint64_t         av_buffersink_get_channel_layout      (const AVFilterContext *ctx);
#endif
int              av_buffersink_get_ch_layout           (const AVFilterContext *ctx,
                                                        AVChannelLayout *ch_layout);
int              av_buffersink_get_sample_rate         (const AVFilterContext *ctx);

AVBufferRef *    av_buffersink_get_hw_frames_ctx       (const AVFilterContext *ctx);

/** @} */

/**
 * 从接收器获取经过过滤的数据帧并将其放入frame中。
 *
 * @param ctx 指向buffersink或abuffersink AVFilter上下文的指针。
 * @param frame 指向将填充数据的已分配帧的指针。
 *              必须使用av_frame_unref() / av_frame_free()释放数据
 *
 * @return
 *         - >= 0 如果成功返回一帧。
 *         - AVERROR(EAGAIN) 如果此时没有可用帧;必须向过滤器图添加更多输入帧以获得更多输出。
 *         - AVERROR_EOF 如果此接收器将不再有更多输出帧。
 *         - 其他失败情况下的不同负AVERROR代码。
 */
int av_buffersink_get_frame(AVFilterContext *ctx, AVFrame *frame);

/**
 * 与av_buffersink_get_frame()相同,但可以指定读取的样本数。
 * 这个函数比av_buffersink_get_frame()效率低,因为它会复制数据。
 *
 * @param ctx 指向abuffersink AVFilter上下文的指针。
 * @param frame 指向将填充数据的已分配帧的指针。
 *              必须使用av_frame_unref() / av_frame_free()释放数据
 *              frame将包含恰好nb_samples个音频样本,除了在流结束时,
 *              它可能包含少于nb_samples个样本。
 *
 * @return 返回代码与av_buffersink_get_frame()的含义相同。
 *
 * @warning 不要将此函数与av_buffersink_get_frame()混用。
 * 对单个接收器只使用其中之一,不要同时使用两者。
 */
int av_buffersink_get_samples(AVFilterContext *ctx, AVFrame *frame, int nb_samples);

/**
 * @}
 */

#endif /* AVFILTER_BUFFERSINK_H */
