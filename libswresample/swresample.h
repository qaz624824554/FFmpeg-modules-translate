#ifndef SWRESAMPLE_SWRESAMPLE_H
#define SWRESAMPLE_SWRESAMPLE_H

/**
 * @file
 * @ingroup lswr
 * libswresample 公共头文件
 */

/**
 * @defgroup lswr libswresample
 * @{
 *
 * 音频重采样、采样格式转换和混音库。
 *
 * 与 lswr 的交互是通过 SwrContext 进行的,它通过 swr_alloc() 或 swr_alloc_set_opts2() 分配。
 * 它是不透明的,所以所有参数必须通过 @ref avoptions API 设置。
 *
 * 要使用 lswr,首先需要做的是分配 SwrContext。这可以通过 swr_alloc() 或 swr_alloc_set_opts2() 完成。
 * 如果使用前者,必须通过 @ref avoptions API 设置选项。后者提供相同的功能,但允许在同一语句中设置一些常用选项。
 *
 * 例如,以下代码将设置从平面浮点采样格式到交错有符号16位整数的转换,从48kHz降采样到44.1kHz,
 * 并从5.1声道降混到立体声(使用默认混音矩阵)。这里使用 swr_alloc() 函数。
 * @code
 * SwrContext *swr = swr_alloc();
 * av_opt_set_channel_layout(swr, "in_channel_layout",  AV_CH_LAYOUT_5POINT1, 0);
 * av_opt_set_channel_layout(swr, "out_channel_layout", AV_CH_LAYOUT_STEREO,  0);
 * av_opt_set_int(swr, "in_sample_rate",     48000,                0);
 * av_opt_set_int(swr, "out_sample_rate",    44100,                0);
 * av_opt_set_sample_fmt(swr, "in_sample_fmt",  AV_SAMPLE_FMT_FLTP, 0);
 * av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_S16,  0);
 * @endcode
 *
 * 同样的工作也可以使用 swr_alloc_set_opts2() 完成:
 * @code
 * SwrContext *swr = NULL;
 * int ret = swr_alloc_set_opts2(&swr,         // 我们正在分配一个新的上下文
 *                       &(AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO, // out_ch_layout
 *                       AV_SAMPLE_FMT_S16,    // out_sample_fmt
 *                       44100,                // out_sample_rate
 *                       &(AVChannelLayout)AV_CHANNEL_LAYOUT_5POINT1, // in_ch_layout
 *                       AV_SAMPLE_FMT_FLTP,   // in_sample_fmt
 *                       48000,                // in_sample_rate
 *                       0,                    // log_offset
 *                       NULL);                // log_ctx
 * @endcode
 *
 * 设置完所有值后,必须用 swr_init() 初始化。如果需要更改转换参数,可以使用 @ref avoptions 更改参数,
 * 如上面第一个示例所述;或使用 swr_alloc_set_opts2(),但第一个参数是已分配的上下文。
 * 然后必须再次调用 swr_init()。
 *
 * 转换本身是通过重复调用 swr_convert() 完成的。
 * 注意,如果提供的输出空间不足,或者进行采样率转换(需要"未来"样本)时,样本可能会在 swr 中被缓冲。
 * 不需要未来输入的样本可以随时使用 swr_convert() 获取(in_count 可以设置为 0)。
 * 在转换结束时,可以通过调用 swr_convert() 并将 in 设为 NULL 且 in_count 为 0 来刷新重采样缓冲区。
 *
 * 转换过程中使用的样本可以通过 libavutil @ref lavu_sampmanip "样本操作" API 管理,
 * 包括在以下示例中使用的 av_samples_alloc() 函数。
 *
 * 输入和输出之间的延迟可以随时使用 swr_get_delay() 查找。
 *
 * 以下代码演示了转换循环,假设使用上述参数和调用者定义的函数 get_input() 和 handle_output():
 * @code
 * uint8_t **input;
 * int in_samples;
 *
 * while (get_input(&input, &in_samples)) {
 *     uint8_t *output;
 *     int out_samples = av_rescale_rnd(swr_get_delay(swr, 48000) +
 *                                      in_samples, 44100, 48000, AV_ROUND_UP);
 *     av_samples_alloc(&output, NULL, 2, out_samples,
 *                      AV_SAMPLE_FMT_S16, 0);
 *     out_samples = swr_convert(swr, &output, out_samples,
 *                                      input, in_samples);
 *     handle_output(output, out_samples);
 *     av_freep(&output);
 * }
 * @endcode
 *
 * 当转换完成时,必须使用 swr_free() 释放转换上下文和与之关联的所有内容。
 * 也提供了 swr_close() 函数,但它主要是为了与 libavresample 兼容,不需要调用。
 *
 * 如果在调用 swr_free() 之前数据没有完全刷新,也不会发生内存泄漏。
 */

#include <stdint.h>
#include "libavutil/channel_layout.h"
#include "libavutil/frame.h"
#include "libavutil/samplefmt.h"

#include "libswresample/version_major.h"
#ifndef HAVE_AV_CONFIG_H
/* 作为 ffmpeg 构建的一部分时,仅包含主版本号以避免不必要的重建。
 * 当外部包含时,保持包含完整版本信息。 */
#include "libswresample/version.h"
#endif

/**
 * @name 选项常量
 * 这些常量用于 lswr 的 @ref avoptions 接口。
 * @{
 *
 */

#define SWR_FLAG_RESAMPLE 1 ///< 即使采样率相同也强制重采样
//TODO 使用 int resample ?
//长期 TODO 我们能动态启用这个吗?

/** 抖动算法 */
enum SwrDitherType {
    SWR_DITHER_NONE = 0,
    SWR_DITHER_RECTANGULAR,
    SWR_DITHER_TRIANGULAR,
    SWR_DITHER_TRIANGULAR_HIGHPASS,

    SWR_DITHER_NS = 64,         ///< 不属于 API/ABI
    SWR_DITHER_NS_LIPSHITZ,
    SWR_DITHER_NS_F_WEIGHTED,
    SWR_DITHER_NS_MODIFIED_E_WEIGHTED,
    SWR_DITHER_NS_IMPROVED_E_WEIGHTED,
    SWR_DITHER_NS_SHIBATA,
    SWR_DITHER_NS_LOW_SHIBATA,
    SWR_DITHER_NS_HIGH_SHIBATA,
    SWR_DITHER_NB,              ///< 不属于 API/ABI
};

/** 重采样引擎 */
enum SwrEngine {
    SWR_ENGINE_SWR,             /**< 软件重采样器 */
    SWR_ENGINE_SOXR,            /**< SoX 重采样器 */
    SWR_ENGINE_NB,              ///< 不属于 API/ABI
};

/** 重采样滤波器类型 */
enum SwrFilterType {
    SWR_FILTER_TYPE_CUBIC,              /**< 三次方 */
    SWR_FILTER_TYPE_BLACKMAN_NUTTALL,   /**< Blackman Nuttall 窗口化 sinc */
    SWR_FILTER_TYPE_KAISER,             /**< Kaiser 窗口化 sinc */
};

/**
 * @}
 */

/**
 * libswresample 上下文。与 libavcodec 和 libavformat 不同,这个结构是不透明的。
 * 这意味着如果要设置选项,必须使用 @ref avoptions API,不能直接设置结构的成员值。
 */
typedef struct SwrContext SwrContext;

/**
 * 获取 SwrContext 的 AVClass。它可以与 AV_OPT_SEARCH_FAKE_OBJ 结合使用来检查选项。
 *
 * @see av_opt_find()。
 * @return SwrContext 的 AVClass
 */
const AVClass *swr_get_class(void);

/**
 * @name SwrContext 构造函数
 * @{
 */

/**
 * 分配 SwrContext。
 *
 * 如果使用此函数,在调用 swr_init() 之前需要设置参数
 * (手动或使用 swr_alloc_set_opts2())。
 *
 * @see swr_alloc_set_opts2(), swr_init(), swr_free()
 * @return 错误时返回 NULL,否则返回已分配的上下文
 */
struct SwrContext *swr_alloc(void);

/**
 * 在设置用户参数后初始化上下文。
 * @note 上下文必须使用 AVOption API 配置。
 *
 * @see av_opt_set_int()
 * @see av_opt_set_dict()
 *
 * @param[in,out]   s 要初始化的 Swr 上下文
 * @return 失败时返回 AVERROR 错误代码。
 */
int swr_init(struct SwrContext *s);

/**
 * 检查 swr 上下文是否已初始化。
 *
 * @param[in]       s 要检查的 Swr 上下文
 * @see swr_init()
 * @return 如果已初始化则返回正数,如果未初始化则返回 0
 */
int swr_is_initialized(struct SwrContext *s);

#if FF_API_OLD_CHANNEL_LAYOUT
/**
 * 如果需要则分配 SwrContext 并设置/重置通用参数。
 *
 * 此函数不要求 s 使用 swr_alloc() 分配。另一方面,
 * swr_alloc() 可以使用 swr_alloc_set_opts() 在已分配的上下文上设置参数。
 *
 * @param s               现有的 Swr 上下文(如果可用),否则为 NULL
 * @param out_ch_layout   输出声道布局(AV_CH_LAYOUT_*)
 * @param out_sample_fmt  输出采样格式(AV_SAMPLE_FMT_*)
 * @param out_sample_rate 输出采样率(Hz)
 * @param in_ch_layout    输入声道布局(AV_CH_LAYOUT_*)
 * @param in_sample_fmt   输入采样格式(AV_SAMPLE_FMT_*)
 * @param in_sample_rate  输入采样率(Hz)
 * @param log_offset      日志级别偏移
 * @param log_ctx         父日志上下文,可以为 NULL
 *
 * @see swr_init(), swr_free()
 * @return 错误时返回 NULL,否则返回已分配的上下文
 * @deprecated 使用 @ref swr_alloc_set_opts2()
 */
attribute_deprecated
struct SwrContext *swr_alloc_set_opts(struct SwrContext *s,
                                      int64_t out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate,
                                      int64_t  in_ch_layout, enum AVSampleFormat  in_sample_fmt, int  in_sample_rate,
                                      int log_offset, void *log_ctx);
#endif

/**
 * 如果需要则分配 SwrContext 并设置/重置通用参数。
 *
 * 此函数不要求 *ps 使用 swr_alloc() 分配。另一方面,
 * swr_alloc() 可以使用 swr_alloc_set_opts2() 在已分配的上下文上设置参数。
 *
 * @param ps              指向现有 Swr 上下文的指针(如果可用),否则指向 NULL。
 *                        成功时,*ps 将被设置为已分配的上下文。
 * @param out_ch_layout   输出声道布局(例如 AV_CHANNEL_LAYOUT_*)
 * @param out_sample_fmt  输出采样格式(AV_SAMPLE_FMT_*)
 * @param out_sample_rate 输出采样率(Hz)
 * @param in_ch_layout    输入声道布局(例如 AV_CHANNEL_LAYOUT_*)
 * @param in_sample_fmt   输入采样格式(AV_SAMPLE_FMT_*)
 * @param in_sample_rate  输入采样率(Hz)
 * @param log_offset      日志级别偏移
 * @param log_ctx         父日志上下文,可以为 NULL
 *
 * @see swr_init(), swr_free()
 * @return 成功时返回 0,错误时返回负的 AVERROR 代码。
 *         出错时,Swr 上下文被释放,*ps 被设置为 NULL。
 */
int swr_alloc_set_opts2(struct SwrContext **ps,
                        const AVChannelLayout *out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate,
                        const AVChannelLayout *in_ch_layout, enum AVSampleFormat  in_sample_fmt, int  in_sample_rate,
                        int log_offset, void *log_ctx);
/**
 * @}
 *
 * @name SwrContext 析构函数
 * @{
 */

/**
 * 释放给定的 SwrContext 并将指针设置为 NULL。
 *
 * @param[in] s 指向 Swr 上下文指针的指针
 */
void swr_free(struct SwrContext **s);

/**
 * 关闭上下文,使 swr_is_initialized() 返回 0。
 *
 * 可以通过运行 swr_init() 使上下文恢复,
 * swr_init() 也可以在不使用 swr_close() 的情况下使用。
 * 提供此函数主要是为了简化支持 libavresample 和 libswresample 的用例。
 *
 * @param[in,out] s 要关闭的 Swr 上下文
 */
void swr_close(struct SwrContext *s);

/**
 * @}
 *
 * @name 核心转换函数
 * @{
 */

/** 转换音频。
 *
 * in 和 in_count 可以设置为 0 以在结尾刷出最后几个样本。
 *
 * 如果提供的输入比输出空间多,那么输入将被缓冲。
 * 你可以通过使用 swr_get_out_samples() 来避免这种缓冲,它可以获取给定输入样本数所需的输出样本数的上限。
 * 在可能的情况下,转换将直接运行而不进行复制。
 *
 * @param s         已分配的 Swr 上下文,已设置参数
 * @param out       输出缓冲区,在打包音频的情况下只需设置第一个
 * @param out_count 每个声道可用的输出空间(以样本为单位)
 * @param in        输入缓冲区,在打包音频的情况下只需设置第一个
 * @param in_count  一个声道中可用的输入样本数
 *
 * @return 每个声道输出的样本数,错误时返回负值
 */
int swr_convert(struct SwrContext *s, uint8_t **out, int out_count,
                                const uint8_t **in , int in_count);

/**
 * 将下一个时间戳从输入转换为输出
 * 时间戳单位为 1/(in_sample_rate * out_sample_rate)。
 *
 * @note 有 2 种略有不同的行为模式。
 *       @li 当不使用自动时间戳补偿时,(min_compensation >= FLT_MAX)
 *              在这种情况下,时间戳将在补偿延迟后传递
 *       @li 当使用自动时间戳补偿时,(min_compensation < FLT_MAX)
 *              在这种情况下,输出时间戳将匹配输出样本数。
 *              参见 ffmpeg-resampler(1) 了解两种补偿模式。
 *
 * @param[in] s     已初始化的 Swr 上下文
 * @param[in] pts   下一个输入样本的时间戳,如果未知则为 INT64_MIN
 * @see swr_set_compensation(), swr_drop_output(), 和 swr_inject_silence() 是
 *      内部用于时间戳补偿的函数。
 * @return 下一个输出样本的输出时间戳
 */
int64_t swr_next_pts(struct SwrContext *s, int64_t pts);

/**
 * @}
 *
 * @name 低级选项设置函数
 * 这些函数提供了一种设置低级选项的方法,这在 AVOption API 中是不可能的。
 * @{
 */

/**
 * 激活重采样补偿("软"补偿)。当在 swr_next_pts() 中需要时,
 * 此函数会在内部被调用。
 *
 * @param[in,out] s             已分配的 Swr 上下文。如果它未初始化,
 *                              或未设置 SWR_FLAG_RESAMPLE,将使用该标志调用 swr_init()。
 * @param[in]     sample_delta  每个样本的 PTS 增量
 * @param[in]     compensation_distance 需要补偿的样本数
 * @return    >= 0 表示成功,在以下情况下返回 AVERROR 错误代码:
 *            @li @c s 为 NULL,
 *            @li @c compensation_distance 小于 0,
 *            @li @c compensation_distance 为 0 但 sample_delta 不为 0,
 *            @li 重采样器不支持补偿,或
 *            @li 调用 swr_init() 失败。
 */
int swr_set_compensation(struct SwrContext *s, int sample_delta, int compensation_distance);

/**
 * 设置自定义输入声道映射。
 *
 * @param[in,out] s           已分配但尚未初始化的 Swr 上下文
 * @param[in]     channel_map 自定义输入声道映射(声道索引数组,
 *                            -1 表示静音声道)
 * @return >= 0 表示成功,失败时返回 AVERROR 错误代码。
 */
int swr_set_channel_mapping(struct SwrContext *s, const int *channel_map);

#if FF_API_OLD_CHANNEL_LAYOUT
/**
 * 生成声道混音矩阵。
 *
 * 这个函数是 libswresample 内部用于构建默认混音矩阵的函数。
 * 它作为一个实用函数公开,用于构建自定义矩阵。
 *
 * @param in_layout           输入声道布局
 * @param out_layout         输出声道布局
 * @param center_mix_level    中置声道的混音级别
 * @param surround_mix_level  环绕声道的混音级别
 * @param lfe_mix_level       低频效果声道的混音级别
 * @param rematrix_maxval     如果为 1.0,系数将被归一化以防止溢出。
 *                            如果为 INT_MAX,系数将不被归一化。
 * @param[out] matrix         混音系数;matrix[i + stride * o] 是
 *                            输入声道 i 在输出声道 o 中的权重。
 * @param stride              矩阵数组中相邻输入声道之间的距离
 * @param matrix_encoding     矩阵立体声下混模式(例如 dplii)
 * @param log_ctx             父日志上下文,可以为 NULL
 * @return                    成功返回 0,失败返回负的 AVERROR 代码
 * @deprecated                使用 @ref swr_build_matrix2()
 */
attribute_deprecated
int swr_build_matrix(uint64_t in_layout, uint64_t out_layout,
                     double center_mix_level, double surround_mix_level,
                     double lfe_mix_level, double rematrix_maxval,
                     double rematrix_volume, double *matrix,
                     int stride, enum AVMatrixEncoding matrix_encoding,
                     void *log_ctx);
#endif

/**
 * 生成声道混音矩阵。
 *
 * 这个函数是 libswresample 内部用于构建默认混音矩阵的函数。
 * 它作为一个实用函数公开,用于构建自定义矩阵。
 *
 * @param in_layout           输入声道布局
 * @param out_layout         输出声道布局
 * @param center_mix_level    中置声道的混音级别
 * @param surround_mix_level  环绕声道的混音级别
 * @param lfe_mix_level       低频效果声道的混音级别
 * @param rematrix_maxval     如果为 1.0,系数将被归一化以防止溢出。
 *                            如果为 INT_MAX,系数将不被归一化。
 * @param[out] matrix         混音系数;matrix[i + stride * o] 是
 *                            输入声道 i 在输出声道 o 中的权重。
 * @param stride              矩阵数组中相邻输入声道之间的距离
 * @param matrix_encoding     矩阵立体声下混模式(例如 dplii)
 * @param log_ctx             父日志上下文,可以为 NULL
 * @return                    成功返回 0,失败返回负的 AVERROR 代码
 */
int swr_build_matrix2(const AVChannelLayout *in_layout, const AVChannelLayout *out_layout,
                      double center_mix_level, double surround_mix_level,
                      double lfe_mix_level, double maxval,
                      double rematrix_volume, double *matrix,
                      ptrdiff_t stride, enum AVMatrixEncoding matrix_encoding,
                      void *log_context);

/**
 * 设置自定义重混矩阵。
 *
 * @param s       已分配但尚未初始化的 Swr 上下文
 * @param matrix  重混系数;matrix[i + stride * o] 是
 *                输入声道 i 在输出声道 o 中的权重
 * @param stride  矩阵行之间的偏移量
 * @return  >= 0 表示成功,失败时返回 AVERROR 错误代码。
 */
int swr_set_matrix(struct SwrContext *s, const double *matrix, int stride);

/**
 * @}
 *
 * @name 样本处理函数
 * @{
 */

/**
 * 丢弃指定数量的输出样本。
 *
 * 如果需要"硬"补偿,此函数与 swr_inject_silence() 一起由 swr_next_pts() 调用。
 *
 * @param s     已分配的 Swr 上下文
 * @param count 要丢弃的样本数
 *
 * @return >= 0 表示成功,失败时返回负的 AVERROR 代码
 */
int swr_drop_output(struct SwrContext *s, int count);

/**
 * 注入指定数量的静音样本。
 *
 * 如果需要"硬"补偿,此函数与 swr_drop_output() 一起由 swr_next_pts() 调用。
 *
 * @param s     已分配的 Swr 上下文
 * @param count 要注入的样本数
 *
 * @return >= 0 表示成功,失败时返回负的 AVERROR 代码
 */
int swr_inject_silence(struct SwrContext *s, int count);

/**
 * 获取下一个输入样本相对于下一个输出样本将经历的延迟。
 *
 * 如果提供的输入比可用的输出空间多,Swresample 可以缓冲数据,
 * 同时在采样率之间转换也需要延迟。
 * 此函数返回所有此类延迟的总和。
 * 精确的延迟不一定是输入或输出采样率中的整数值。
 * 特别是在大幅度降采样时,输出采样率可能不是表示延迟的好选择,
 * 同样,对于上采样和输入采样率也是如此。
 *
 * @param s     swr 上下文
 * @param base  返回的延迟将使用的时间基:
 *              @li 如果设置为 1,返回的延迟以秒为单位
 *              @li 如果设置为 1000,返回的延迟以毫秒为单位
 *              @li 如果设置为输入采样率,则返回的延迟以输入样本为单位
 *              @li 如果设置为输出采样率,则返回的延迟以输出样本为单位
 *              @li 如果设置为 in_sample_rate 和 out_sample_rate 的最小公倍数,
 *                  则将返回精确的无舍入延迟
 * @returns     以 1 / @c base 为单位的延迟。
 */
int64_t swr_get_delay(struct SwrContext *s, int64_t base);

/**
 * 找到下一次 swr_convert 调用将输出的样本数的上限,
 * 如果使用 in_samples 个输入样本调用。这取决于内部状态,
 * 任何改变内部状态的操作(如进一步的 swr_convert() 调用)
 * 都可能改变 swr_get_out_samples() 为相同数量的输入样本返回的数量。
 *
 * @param in_samples    输入样本数。
 * @note 任何对 swr_inject_silence(), swr_convert(), swr_next_pts()
 *       或 swr_set_compensation() 的调用都会使此限制失效
 * @note 建议向所有函数(如 swr_convert())传递正确的可用缓冲区大小,
 *       即使 swr_get_out_samples() 表明会使用更少的样本。
 * @returns 下一次 swr_convert 将输出的样本数的上限,
 *          或表示错误的负值
 */
int swr_get_out_samples(struct SwrContext *s, int in_samples);

/**
 * @}
 *
 * @name 配置访问器
 * @{
 */

/**
 * 返回 @ref LIBSWRESAMPLE_VERSION_INT 常量。
 *
 * 这用于检查构建时的 libswresample 是否与运行时的版本相同。
 *
 * @returns     无符号整型的版本号
 */
unsigned swresample_version(void);

/**
 * 返回 swr 构建时配置。
 *
 * @returns     构建时的 @c ./configure 标志
 */
const char *swresample_configuration(void);

/**
 * 返回 swr 许可证。
 *
 * @returns     libswresample 的许可证,在构建时确定
 */
const char *swresample_license(void);

/**
 * @}
 *
 * @name 基于 AVFrame 的 API
 * @{
 */

/**
 * 转换输入 AVFrame 中的样本并将其写入输出 AVFrame。
 *
 * 输入和输出 AVFrame 必须设置 channel_layout、sample_rate 和 format。
 *
 * 如果输出 AVFrame 没有分配数据指针,将使用 av_frame_get_buffer() 设置 nb_samples 字段
 * 来分配帧。
 *
 * 输出 AVFrame 可以为 NULL 或分配的样本少于所需数量。
 * 在这种情况下,任何未写入输出的剩余样本都将添加到内部 FIFO 缓冲区,
 * 在下次调用此函数或 swr_convert() 时返回。
 *
 * 如果转换采样率,内部重采样延迟缓冲区中可能有剩余数据。
 * swr_get_delay() 告诉剩余样本的数量。要获取这些数据作为输出,
 * 请使用 NULL 输入调用此函数或 swr_convert()。
 *
 * 如果 SwrContext 配置与输出和输入 AVFrame 设置不匹配,
 * 则不会进行转换,并根据哪个 AVFrame 不匹配返回 AVERROR_OUTPUT_CHANGED、
 * AVERROR_INPUT_CHANGED 或它们的按位或结果。
 *
 * @see swr_delay()
 * @see swr_convert()
 * @see swr_get_delay()
 *
 * @param swr             音频重采样上下文
 * @param output          输出 AVFrame
 * @param input           输入 AVFrame
 * @return                成功时返回 0,失败或配置不匹配时返回 AVERROR。
 */
int swr_convert_frame(SwrContext *swr,
                      AVFrame *output, const AVFrame *input);

/**
 * 使用 AVFrames 提供的信息配置或重新配置 SwrContext。
 *
 * 即使失败,原始重采样上下文也会被重置。
 * 如果上下文已打开,该函数会在内部调用 swr_close()。
 *
 * @see swr_close();
 *
 * @param swr             音频重采样上下文
 * @param out             输出 AVFrame
 * @param in              输入 AVFrame
 * @return                成功时返回 0,失败时返回 AVERROR。
 */
int swr_config_frame(SwrContext *swr, const AVFrame *out, const AVFrame *in);

/**
 * @}
 * @}
 */

#endif /* SWRESAMPLE_SWRESAMPLE_H */
