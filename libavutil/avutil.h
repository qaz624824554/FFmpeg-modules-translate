#ifndef AVUTIL_AVUTIL_H
#define AVUTIL_AVUTIL_H

/**
 * @file
 * @ingroup lavu
 * 包含 @ref lavu "libavutil" 核心的便利头文件。
 */

/**
 * @mainpage
 *
 * @section ffmpeg_intro 简介
 *
 * 本文档描述了FFmpeg提供的不同库的用法。
 *
 * @li @ref libavc "libavcodec" 编码/解码库
 * @li @ref lavfi "libavfilter" 基于图的帧编辑库
 * @li @ref libavf "libavformat" I/O和复用/解复用库
 * @li @ref lavd "libavdevice" 特殊设备复用/解复用库
 * @li @ref lavu "libavutil" 通用工具库
 * @li @ref lswr "libswresample" 音频重采样、格式转换和混音库
 * @li @ref lpp  "libpostproc" 后处理库
 * @li @ref libsws "libswscale" 颜色转换和缩放库
 *
 * @section ffmpeg_versioning 版本控制和兼容性
 *
 * 每个FFmpeg库都包含一个version.h头文件，它使用<em>LIBRARYNAME_VERSION_{MAJOR,MINOR,MICRO}</em>宏定义了主版本号、次版本号和微版本号。
 * 主版本号在有向后不兼容的更改时递增 - 例如删除公共API的部分、重新排序公共结构成员等。
 * 次版本号在有向后兼容的API更改或重要新功能时递增 - 例如添加新的公共函数或新的解码器。
 * 微版本号在有较小的更改时递增，这些更改可能是调用程序仍想检查的 - 例如在以前未指定的情况下更改行为。
 *
 * 只要主版本号保持不变，FFmpeg就保证每个库的向后API和ABI兼容性。这意味着不会删除或重命名公共符号。
 * 公共结构成员的类型和名称以及公共宏和枚举的值将保持不变(除非它们被明确声明为不属于公共API的一部分)。
 * 已记录的行为不会改变。
 *
 * 换句话说，任何与给定FFmpeg快照一起工作的正确程序，在使用相同主版本的任何更新快照时都应该同样可以工作，无需任何更改。
 * 这适用于针对新FFmpeg版本重新构建程序，或替换程序链接的动态FFmpeg库。
 *
 * 但是，可能会添加新的公共符号，并且可能会向大小不属于公共ABI的公共结构(FFmpeg中的大多数公共结构)添加新成员。
 * 可能会添加新的宏和枚举值。未记录情况下的行为可能会略有改变(并被记录)。
 * 所有这些都会在doc/APIchanges中有相应条目，并增加次版本号或微版本号。
 */

/**
 * @defgroup lavu libavutil
 * 所有FFmpeg库共享的通用代码。
 *
 * @note
 * libavutil被设计为模块化的。在大多数情况下，为了使用libavutil一个组件提供的函数，
 * 你必须显式包含包含该功能的特定头文件。如果你只使用媒体相关的组件，
 * 你可以简单地包含libavutil/avutil.h，它会引入大多数"核心"组件。
 *
 * @{
 *
 * @defgroup lavu_crypto 加密和哈希
 *
 * @{
 * @}
 *
 * @defgroup lavu_math 数学
 * @{
 *
 * @}
 *
 * @defgroup lavu_string 字符串操作
 *
 * @{
 *
 * @}
 *
 * @defgroup lavu_mem 内存管理
 *
 * @{
 *
 * @}
 *
 * @defgroup lavu_data 数据结构
 * @{
 *
 * @}
 *
 * @defgroup lavu_video 视频相关
 *
 * @{
 *
 * @}
 *
 * @defgroup lavu_audio 音频相关
 *
 * @{
 *
 * @}
 *
 * @defgroup lavu_error 错误代码
 *
 * @{
 *
 * @}
 *
 * @defgroup lavu_log 日志功能
 *
 * @{
 *
 * @}
 *
 * @defgroup lavu_misc 其他
 *
 * @{
 *
 * @defgroup preproc_misc 预处理器字符串宏
 *
 * @{
 *
 * @}
 *
 * @defgroup version_utils 库版本宏
 *
 * @{
 *
 * @}
 */

/**
 * @addtogroup lavu_ver
 * @{
 */

/**
 * 返回LIBAVUTIL_VERSION_INT常量。
 */
unsigned avutil_version(void);

/**
 * 返回一个信息版本字符串。这通常是实际的发布版本号或git提交描述。
 * 此字符串没有固定格式，可以随时更改。代码不应该解析它。
 */
const char *av_version_info(void);

/**
 * 返回libavutil的构建时配置。
 */
const char *avutil_configuration(void);

/**
 * 返回libavutil的许可证。
 */
const char *avutil_license(void);

/**
 * @}
 */

/**
 * @addtogroup lavu_media 媒体类型
 * @brief 媒体类型
 */

enum AVMediaType {
    AVMEDIA_TYPE_UNKNOWN = -1,  ///< 通常被视为AVMEDIA_TYPE_DATA
    AVMEDIA_TYPE_VIDEO,
    AVMEDIA_TYPE_AUDIO,
    AVMEDIA_TYPE_DATA,          ///< 通常连续的不透明数据信息
    AVMEDIA_TYPE_SUBTITLE,
    AVMEDIA_TYPE_ATTACHMENT,    ///< 通常稀疏的不透明数据信息
    AVMEDIA_TYPE_NB
};

/**
 * 返回描述media_type枚举的字符串，如果media_type未知则返回NULL。
 */
const char *av_get_media_type_string(enum AVMediaType media_type);

/**
 * @defgroup lavu_const 常量
 * @{
 *
 * @defgroup lavu_enc 编码相关
 *
 * @note 这些定义应该移到avcodec
 * @{
 */

#define FF_LAMBDA_SHIFT 7
#define FF_LAMBDA_SCALE (1<<FF_LAMBDA_SHIFT)
#define FF_QP2LAMBDA 118 ///< 用于将H.263 QP转换为lambda的因子
#define FF_LAMBDA_MAX (256*128-1)

#define FF_QUALITY_SCALE FF_LAMBDA_SCALE //FIXME 可能移除

/**
 * @}
 * @defgroup lavu_time 时间戳相关
 *
 * FFmpeg内部时基和时间戳定义
 *
 * @{
 */

/**
 * @brief 未定义的时间戳值
 *
 * 通常由处理不提供pts或dts的容器的解复用器报告。
 */

#define AV_NOPTS_VALUE          ((int64_t)UINT64_C(0x8000000000000000))

/**
 * 以整数表示的内部时基
 */

#define AV_TIME_BASE            1000000

/**
 * 以分数值表示的内部时基
 */

#ifdef __cplusplus
/* ISO C++禁止复合字面量。 */
#define AV_TIME_BASE_Q          av_make_q(1, AV_TIME_BASE)
#else
#define AV_TIME_BASE_Q          (AVRational){1, AV_TIME_BASE}
#endif

/**
 * @}
 * @}
 * @defgroup lavu_picture 图像相关
 *
 * AVPicture类型、像素格式和基本图像平面操作。
 *
 * @{
 */

enum AVPictureType {
    AV_PICTURE_TYPE_NONE = 0, ///< 未定义
    AV_PICTURE_TYPE_I,     ///< 帧内编码
    AV_PICTURE_TYPE_P,     ///< 预测编码
    AV_PICTURE_TYPE_B,     ///< 双向预测编码
    AV_PICTURE_TYPE_S,     ///< S(GMC)-VOP MPEG-4
    AV_PICTURE_TYPE_SI,    ///< 切换帧内编码
    AV_PICTURE_TYPE_SP,    ///< 切换预测编码
    AV_PICTURE_TYPE_BI,    ///< BI类型
};

/**
 * 返回一个单字母来描述给定的图片类型pict_type。
 *
 * @param[in] pict_type 图片类型 @return 表示图片类型的单个字符，
 * 如果pict_type未知则返回'?'
 */
char av_get_picture_type_char(enum AVPictureType pict_type);

/**
 * @}
 */

#include "common.h"
#include "rational.h"
#include "version.h"
#include "macros.h"
#include "mathematics.h"
#include "log.h"
#include "pixfmt.h"

/**
 * 如果p为NULL，则返回默认指针x。
 */
static inline void *av_x_if_null(const void *p, const void *x)
{
    return (void *)(intptr_t)(p ? p : x);
}

/**
 * 计算整数列表的长度。
 *
 * @param elsize  每个列表元素的字节大小(仅1、2、4或8)
 * @param term    列表终止符(通常为0或-1)
 * @param list    指向列表的指针
 * @return  列表的长度，以元素为单位，不计算终止符
 */
unsigned av_int_list_length_for_size(unsigned elsize,
                                     const void *list, uint64_t term) av_pure;

/**
 * 计算整数列表的长度。
 *
 * @param term  列表终止符(通常为0或-1)
 * @param list  指向列表的指针
 * @return  列表的长度，以元素为单位，不计算终止符
 */
#define av_int_list_length(list, term) \
    av_int_list_length_for_size(sizeof(*(list)), list, term)

#if FF_API_AV_FOPEN_UTF8
/**
 * 使用UTF-8文件名打开文件。
 * 此函数的API与POSIX fopen()匹配，错误通过errno返回。
 * @deprecated 避免使用它，因为在Windows上，此函数分配的FILE*可能使用与
 *             使用FILE*的调用者不同的CRT分配。公共API中没有提供替代方案。
 */
attribute_deprecated
FILE *av_fopen_utf8(const char *path, const char *mode);
#endif

/**
 * 返回内部时基的分数表示。
 */
AVRational av_get_time_base_q(void);

#define AV_FOURCC_MAX_STRING_SIZE 32

#define av_fourcc2str(fourcc) av_fourcc_make_string((char[AV_FOURCC_MAX_STRING_SIZE]){0}, fourcc)

/**
 * 用FourCC(四字符代码)表示的字符串填充提供的缓冲区。
 *
 * @param buf    大小至少为AV_FOURCC_MAX_STRING_SIZE字节的缓冲区
 * @param fourcc 要表示的fourcc
 * @return 输入的缓冲区
 */
char *av_fourcc_make_string(char *buf, uint32_t fourcc);

/**
 * @}
 * @}
 */

#endif /* AVUTIL_AVUTIL_H */
