#ifndef SWSCALE_SWSCALE_H
#define SWSCALE_SWSCALE_H

/**
 * @file
 * @ingroup libsws
 * 外部API头文件
 */

#include <stdint.h>

#include "libavutil/avutil.h"
#include "libavutil/frame.h"
#include "libavutil/log.h"
#include "libavutil/pixfmt.h"
#include "version_major.h"
#ifndef HAVE_AV_CONFIG_H
/* 作为ffmpeg构建的一部分时,仅包含主版本号以避免不必要的重建。
 * 当外部包含时,保持包含完整版本信息。 */
#include "version.h"
#endif

/**
 * @defgroup libsws libswscale
 * 颜色转换和缩放库。
 *
 * @{
 *
 * 返回LIBSWSCALE_VERSION_INT常量。
 */
unsigned swscale_version(void);

/**
 * 返回libswscale的编译时配置。
 */
const char *swscale_configuration(void);

/**
 * 返回libswscale的许可证。
 */
const char *swscale_license(void);

/* 标志的值,命令行上的东西是不同的 */
#define SWS_FAST_BILINEAR     1
#define SWS_BILINEAR          2
#define SWS_BICUBIC           4
#define SWS_X                 8
#define SWS_POINT          0x10
#define SWS_AREA           0x20
#define SWS_BICUBLIN       0x40
#define SWS_GAUSS          0x80
#define SWS_SINC          0x100
#define SWS_LANCZOS       0x200
#define SWS_SPLINE        0x400

#define SWS_SRC_V_CHR_DROP_MASK     0x30000
#define SWS_SRC_V_CHR_DROP_SHIFT    16

#define SWS_PARAM_DEFAULT           123456

#define SWS_PRINT_INFO              0x1000

//以下3个标志尚未完全实现
//内部色度子采样信息
#define SWS_FULL_CHR_H_INT    0x2000
//输入子采样信息
#define SWS_FULL_CHR_H_INP    0x4000
#define SWS_DIRECT_BGR        0x8000
#define SWS_ACCURATE_RND      0x40000
#define SWS_BITEXACT          0x80000
#define SWS_ERROR_DIFFUSION  0x800000

#define SWS_MAX_REDUCE_CUTOFF 0.002

#define SWS_CS_ITU709         1
#define SWS_CS_FCC            4
#define SWS_CS_ITU601         5
#define SWS_CS_ITU624         5
#define SWS_CS_SMPTE170M      5
#define SWS_CS_SMPTE240M      7
#define SWS_CS_DEFAULT        5
#define SWS_CS_BT2020         9

/**
 * 返回指向给定色彩空间的yuv<->rgb系数的指针,
 * 适用于sws_setColorspaceDetails()。
 *
 * @param colorspace SWS_CS_*宏之一。如果无效,
 * 使用SWS_CS_DEFAULT。
 */
const int *sws_getCoefficients(int colorspace);

// 当用于滤波器时,它们必须有奇数个元素
// 系数不能在向量之间共享
typedef struct SwsVector {
    double *coeff;              ///< 指向系数列表的指针
    int length;                 ///< 向量中的系数数量
} SwsVector;

// 向量可以共享
typedef struct SwsFilter {
    SwsVector *lumH;
    SwsVector *lumV;
    SwsVector *chrH;
    SwsVector *chrV;
} SwsFilter;

struct SwsContext;

/**
 * 如果pix_fmt是支持的输入格式则返回正值,
 * 否则返回0。
 */
int sws_isSupportedInput(enum AVPixelFormat pix_fmt);

/**
 * 如果pix_fmt是支持的输出格式则返回正值,
 * 否则返回0。
 */
int sws_isSupportedOutput(enum AVPixelFormat pix_fmt);

/**
 * @param[in]  pix_fmt 像素格式
 * @return 如果支持pix_fmt的字节序转换则返回正值,
 * 否则返回0。
 */
int sws_isSupportedEndiannessConversion(enum AVPixelFormat pix_fmt);

/**
 * 分配一个空的SwsContext。这必须填充并传递给
 * sws_init_context()。关于填充请参见AVOptions、options.c和
 * sws_setColorspaceDetails()。
 */
struct SwsContext *sws_alloc_context(void);

/**
 * 初始化缩放器上下文sws_context。
 *
 * @return 成功时返回零或正值,失败时返回负值
 */
av_warn_unused_result
int sws_init_context(struct SwsContext *sws_context, SwsFilter *srcFilter, SwsFilter *dstFilter);

/**
 * 释放缩放器上下文swsContext。
 * 如果swsContext为NULL,则不执行任何操作。
 */
void sws_freeContext(struct SwsContext *swsContext);

/**
 * 分配并返回一个SwsContext。你需要它来使用sws_scale()
 * 执行缩放/转换操作。
 *
 * @param srcW 源图像的宽度
 * @param srcH 源图像的高度
 * @param srcFormat 源图像格式
 * @param dstW 目标图像的宽度
 * @param dstH 目标图像的高度
 * @param dstFormat 目标图像格式
 * @param flags 指定用于重缩放的算法和选项
 * @param param 用于调整所用缩放器的额外参数
 *              对于SWS_BICUBIC,param[0]和[1]调整基函数的形状,
 *              param[0]调整f(1),param[1]调整f´(1)
 *              对于SWS_GAUSS,param[0]调整指数从而调整截止频率
 *              对于SWS_LANCZOS,param[0]调整窗口函数的宽度
 * @return 成功时返回已分配上下文的指针,失败时返回NULL
 * @note 此函数将在编写更合理的替代方案后删除
 */
struct SwsContext *sws_getContext(int srcW, int srcH, enum AVPixelFormat srcFormat,
                                  int dstW, int dstH, enum AVPixelFormat dstFormat,
                                  int flags, SwsFilter *srcFilter,
                                  SwsFilter *dstFilter, const double *param);

/**
 * 缩放srcSlice中的图像切片,并将结果缩放的切片
 * 放入dst中的图像。切片是图像中连续的行序列。
 *
 * 切片必须按顺序提供,可以是从上到下或从下到上的顺序。
 * 如果以非顺序方式提供切片,函数的行为是未定义的。
 *
 * @param c         之前用sws_getContext()创建的缩放上下文
 * @param srcSlice  包含源切片平面指针的数组
 * @param srcStride 包含源图像每个平面步长的数组
 * @param srcSliceY 要处理的切片在源图像中的位置,
 *                  即切片第一行在图像中的编号(从零开始计数)
 * @param srcSliceH 源切片的高度,即切片中的行数
 * @param dst       包含目标图像平面指针的数组
 * @param dstStride 包含目标图像每个平面步长的数组
 * @return          输出切片的高度
 */
int sws_scale(struct SwsContext *c, const uint8_t *const srcSlice[],
              const int srcStride[], int srcSliceY, int srcSliceH,
              uint8_t *const dst[], const int dstStride[]);

/**
 * 从src缩放源数据并将输出写入dst。
 *
 * 这只是以下操作的便捷包装:
 * - sws_frame_start()
 * - sws_send_slice(0, src->height)
 * - sws_receive_slice(0, dst->height)
 * - sws_frame_end()
 *
 * @param c   缩放上下文
 * @param dst 目标帧。更多详细信息请参见sws_frame_start()的文档。
 * @param src 源帧。
 *
 * @return 成功时返回0,失败时返回负的AVERROR代码
 */
int sws_scale_frame(struct SwsContext *c, AVFrame *dst, const AVFrame *src);

/**
 * 为给定的源/目标帧对初始化缩放过程。
 * 必须在调用sws_send_slice()和sws_receive_slice()之前调用。
 *
 * 此函数将保留对src和dst的引用,因此它们必须都使用
 * 引用计数缓冲区(如果由调用者分配,对于dst的情况)。
 *
 * @param c   缩放上下文
 * @param dst 目标帧。
 *
 *            数据缓冲区可以由调用者预先分配,也可以
 *            留空,在这种情况下将由缩放器分配。
 *            后者可能具有性能优势 - 例如在某些情况下,
 *            某些输出平面可能是输入平面的引用,而不是副本。
 *
 *            输出数据将在成功的sws_receive_slice()调用中
 *            写入此帧。
 * @param src 源帧。数据缓冲区必须已分配,但此时帧数据
 *            不必就绪。数据可用性随后由sws_send_slice()
 *            发出信号。
 * @return 成功时返回0,失败时返回负的AVERROR代码
 *
 * @see sws_frame_end()
 */
int sws_frame_start(struct SwsContext *c, AVFrame *dst, const AVFrame *src);

/**
 * 完成之前用sws_frame_start()提交的源/目标帧对的缩放过程。
 * 必须在所有sws_send_slice()和sws_receive_slice()调用完成后,
 * 在任何新的sws_frame_start()调用之前调用。
 *
 * @param c   缩放上下文
 */
void sws_frame_end(struct SwsContext *c);

/**
 * 指示之前提供给sws_frame_start()的源帧中有水平切片的输入数据可用。
 * 切片可以按任意顺序提供,但不能重叠。对于垂直子采样的像素格式,
 * 切片必须根据子采样对齐。
 *
 * @param c   缩放上下文
 * @param slice_start 切片的第一行
 * @param slice_height 切片中的行数
 *
 * @return 成功时返回非负数,失败时返回负的AVERROR代码。
 */
int sws_send_slice(struct SwsContext *c, unsigned int slice_start,
                   unsigned int slice_height);

/**
 * 请求将输出数据的水平切片写入之前提供给sws_frame_start()的帧中。
 *
 * @param c   缩放上下文
 * @param slice_start 切片的第一行;必须是sws_receive_slice_alignment()的倍数
 * @param slice_height 切片中的行数;必须是sws_receive_slice_alignment()的倍数,
 *                     除了最后一个切片(即当slice_start+slice_height等于输出
 *                     帧高度时)
 *
 * @return 如果数据成功写入输出则返回非负数
 *         如果在产生输出之前需要更多输入数据则返回AVERROR(EAGAIN)
 *         其他类型的缩放失败则返回另一个负的AVERROR代码
 */
int sws_receive_slice(struct SwsContext *c, unsigned int slice_start,
                      unsigned int slice_height);

/**
 * 获取切片所需的对齐方式
 *
 * @param c   缩放上下文
 * @return 使用sws_receive_slice()请求输出切片所需的对齐方式。
 *         传递给sws_receive_slice()的切片偏移量和大小必须是
 *         此函数返回值的倍数。
 */
unsigned int sws_receive_slice_alignment(const struct SwsContext *c);

/**
 * @param c 缩放上下文
 * @param dstRange 指示输出的白-黑范围的标志(1=jpeg / 0=mpeg)
 * @param srcRange 指示输入的白-黑范围的标志(1=jpeg / 0=mpeg)
 * @param table 描述输出yuv空间的yuv2rgb系数,通常是ff_yuv2rgb_coeffs[x]
 * @param inv_table 描述输入yuv空间的yuv2rgb系数,通常是ff_yuv2rgb_coeffs[x]
 * @param brightness 16.16定点亮度校正
 * @param contrast 16.16定点对比度校正
 * @param saturation 16.16定点饱和度校正
 *
 * @return 错误时返回负值,否则返回非负值。
 *         如果`LIBSWSCALE_VERSION_MAJOR < 7`,不支持时返回-1。
 */
int sws_setColorspaceDetails(struct SwsContext *c, const int inv_table[4],
                             int srcRange, const int table[4], int dstRange,
                             int brightness, int contrast, int saturation);

/**
 * @return 错误时返回负值,否则返回非负值。
 *         如果`LIBSWSCALE_VERSION_MAJOR < 7`,不支持时返回-1。
 */
int sws_getColorspaceDetails(struct SwsContext *c, int **inv_table,
                             int *srcRange, int **table, int *dstRange,
                             int *brightness, int *contrast, int *saturation);

/**
 * 分配并返回一个具有length个系数的未初始化向量。
 */
SwsVector *sws_allocVec(int length);

/**
 * 返回用于过滤的标准化高斯曲线
 * quality = 3表示高质量,更低则表示更低质量。
 */
SwsVector *sws_getGaussianVec(double variance, double quality);

/**
 * 将a的所有系数乘以标量值。
 */
void sws_scaleVec(SwsVector *a, double scalar);

/**
 * 缩放a的所有系数使其总和等于height。
 */
void sws_normalizeVec(SwsVector *a, double height);

void sws_freeVec(SwsVector *a);

SwsFilter *sws_getDefaultFilter(float lumaGBlur, float chromaGBlur,
                                float lumaSharpen, float chromaSharpen,
                                float chromaHShift, float chromaVShift,
                                int verbose);
void sws_freeFilter(SwsFilter *filter);

/**
 * 检查上下文是否可以重用,否则重新分配一个新的。
 *
 * 如果context为NULL,只是调用sws_getContext()获取一个新的
 * 上下文。否则,检查参数是否与context中已保存的相同。
 * 如果是这种情况,返回当前上下文。否则,释放context并
 * 使用新参数获取一个新的上下文。
 *
 * 请注意srcFilter和dstFilter不会被检查,假定它们保持不变。
 */
struct SwsContext *sws_getCachedContext(struct SwsContext *context,
                                        int srcW, int srcH, enum AVPixelFormat srcFormat,
                                        int dstW, int dstH, enum AVPixelFormat dstFormat,
                                        int flags, SwsFilter *srcFilter,
                                        SwsFilter *dstFilter, const double *param);

/**
 * 将8位调色板帧转换为32位色深的帧。
 *
 * 输出帧将具有与调色板相同的打包格式。
 *
 * @param src        源帧缓冲区
 * @param dst        目标帧缓冲区
 * @param num_pixels 要转换的像素数
 * @param palette    具有[256]个条目的数组,必须匹配src的颜色排列(RGB或BGR)
 */
void sws_convertPalette8ToPacked32(const uint8_t *src, uint8_t *dst, int num_pixels, const uint8_t *palette);

/**
 * 将8位调色板帧转换为24位色深的帧。
 *
 * 对于调色板格式"ABCD",目标帧最终具有格式"ABC"。
 *
 * @param src        源帧缓冲区
 * @param dst        目标帧缓冲区
 * @param num_pixels 要转换的像素数
 * @param palette    具有[256]个条目的数组,必须匹配src的颜色排列(RGB或BGR)
 */
void sws_convertPalette8ToPacked24(const uint8_t *src, uint8_t *dst, int num_pixels, const uint8_t *palette);

/**
 * 获取swsContext的AVClass。它可以与AV_OPT_SEARCH_FAKE_OBJ
 * 结合使用来检查选项。
 *
 * @see av_opt_find()。
 */
const AVClass *sws_get_class(void);

/**
 * @}
 */

#endif /* SWSCALE_SWSCALE_H */
