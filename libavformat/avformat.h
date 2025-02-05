#ifndef AVFORMAT_AVFORMAT_H
#define AVFORMAT_AVFORMAT_H

/**
 * @file
 * @ingroup libavf
 * libavformat主要公共API头文件
 */

/**
 * @defgroup libavf libavformat
 * I/O和复用/解复用库
 *
 * Libavformat (lavf)是一个用于处理各种媒体容器格式的库。它的两个主要目的是解复用
 * - 即将媒体文件分割成组件流,以及相反的复用过程 - 将提供的数据写入指定的容器格式。
 * 它还有一个 @ref lavf_io "I/O模块",支持多种访问数据的协议(如文件、tcp、http等)。
 * 除非你完全确定不会使用libavformat的网络功能,否则你也应该调用avformat_network_init()。
 *
 * 支持的输入格式由AVInputFormat结构体描述,相应地输出格式由AVOutputFormat描述。
 * 你可以使用av_demuxer_iterate / av_muxer_iterate()函数遍历所有输入/输出格式。
 * 协议层不是公共API的一部分,所以你只能使用avio_enum_protocols()函数获取支持的协议名称。
 *
 * 用于复用和解复用的主要lavf结构是AVFormatContext,它导出了正在读取或写入的文件的所有信息。
 * 与大多数Libavformat结构一样,其大小不是公共ABI的一部分,所以不能在栈上分配或直接用
 * av_malloc()分配。要创建AVFormatContext,请使用avformat_alloc_context()(某些函数,
 * 如avformat_open_input()可能会为你做这件事)。
 *
 * 最重要的是AVFormatContext包含:
 * @li @ref AVFormatContext.iformat "输入"或 @ref AVFormatContext.oformat "输出"格式。
 * 对于输入它是自动检测或由用户设置;对于输出总是由用户设置。
 * @li 一个 @ref AVFormatContext.streams "数组"的AVStreams,描述存储在文件中的所有基本流。
 * AVStreams通常使用它们在此数组中的索引来引用。
 * @li 一个 @ref AVFormatContext.pb "I/O上下文"。对于输入它是由lavf打开或由用户设置,
 * 对于输出总是由用户设置(除非你处理的是AVFMT_NOFILE格式)。
 *
 * @section lavf_options 向(解)复用器传递选项
 * 可以使用 @ref avoptions 机制配置lavf复用器和解复用器。通用(与格式无关)的libavformat选项
 * 由AVFormatContext提供,用户程序可以通过在已分配的AVFormatContext上调用av_opt_next() / 
 * av_opt_find()来检查这些选项(或从avformat_get_class()获取其AVClass)。私有(特定格式)选项
 * 由AVFormatContext.priv_data提供,当且仅当相应格式结构体的AVInputFormat.priv_class /
 * AVOutputFormat.priv_class非NULL时。更多选项可能由 @ref AVFormatContext.pb "I/O上下文"
 * 提供,如果其AVClass非NULL,以及协议层。请参阅 @ref avoptions 文档中关于嵌套的讨论以了解
 * 如何访问这些选项。
 *
 * @section urls
 * libavformat中的URL字符串由方案/协议、':'和特定于方案的字符串组成。不带方案和':'的URL
 * 用于本地文件是支持的但已弃用。应该为本地文件使用"file:"。
 *
 * 重要的是不要在未经检查的情况下从不受信任的来源获取方案字符串。
 *
 * 注意某些方案/协议非常强大,允许访问本地和远程文件、它们的部分、它们的连接、本地音频和
 * 视频设备等。
 *
 * @{
 *
 * @defgroup lavf_decoding 解复用
 * @{
 * 解复用器读取媒体文件并将其分割成数据块(@em packets)。一个 @ref AVPacket "包"包含
 * 属于单个基本流的一个或多个编码帧。在lavf API中,这个过程由avformat_open_input()函数
 * 打开文件,av_read_frame()读取单个包,最后avformat_close_input()进行清理来表示。
 *
 * @section lavf_decoding_open 打开媒体文件
 * 打开文件所需的最小信息是其URL,它被传递给avformat_open_input(),如下面的代码:
 * @code
 * const char    *url = "file:in.mp3";
 * AVFormatContext *s = NULL;
 * int ret = avformat_open_input(&s, url, NULL, NULL);
 * if (ret < 0)
 *     abort();
 * @endcode
 * 上面的代码尝试分配一个AVFormatContext,打开指定的文件(自动检测格式)并读取头部,
 * 将存储在那里的信息导出到s。某些格式没有头部或存储的信息不够,因此建议调用
 * avformat_find_stream_info()函数,它尝试读取和解码几帧以找到缺失的信息。
 *
 * 在某些情况下,你可能想要自己用avformat_alloc_context()预分配AVFormatContext,
 * 并在将其传递给avformat_open_input()之前对其进行一些调整。这种情况之一是当你想要
 * 使用自定义函数来读取输入数据而不是lavf内部I/O层时。要做到这一点,用avio_alloc_context()
 * 创建你自己的AVIOContext,将你的读取回调传递给它。然后将你的AVFormatContext的 @em pb 
 * 字段设置为新创建的AVIOContext。
 *
 * 由于在avformat_open_input()返回之前通常不知道打开文件的格式,因此不可能在预分配的上下文
 * 上设置解复用器私有选项。相反,选项应该包装在AVDictionary中传递给avformat_open_input():
 * @code
 * AVDictionary *options = NULL;
 * av_dict_set(&options, "video_size", "640x480", 0);
 * av_dict_set(&options, "pixel_format", "rgb24", 0);
 *
 * if (avformat_open_input(&s, url, NULL, &options) < 0)
 *     abort();
 * av_dict_free(&options);
 * @endcode
 * 这段代码将私有选项'video_size'和'pixel_format'传递给解复用器。例如对于rawvideo解复用器
 * 这些选项是必需的,因为它无法知道如何解释原始视频数据。如果格式最终是原始视频以外的其他格式,
 * 这些选项将不会被解复用器识别,因此不会被应用。这些未识别的选项然后会在选项字典中返回
 * (已识别的选项会被消耗)。调用程序可以根据需要处理这些未识别的选项,例如
 * @code
 * AVDictionaryEntry *e;
 * if (e = av_dict_get(options, "", NULL, AV_DICT_IGNORE_SUFFIX)) {
 *     fprintf(stderr, "选项 %s 未被解复用器识别。\n", e->key);
 *     abort();
 * }
 * @endcode
 *
 * 读取完文件后,必须用avformat_close_input()关闭它。它将释放与文件相关的所有内容。
 *
 * @section lavf_decoding_read 从已打开的文件读取
 * 从已打开的AVFormatContext读取数据是通过反复调用av_read_frame()来完成的。每次调用
 * 如果成功,将返回一个包含一个AVStream编码数据的AVPacket,由AVPacket.stream_index标识。
 * 如果调用者希望解码数据,这个包可以直接传递给libavcodec解码函数avcodec_send_packet()
 * 或avcodec_decode_subtitle2()。
 *
 * 如果已知,将设置AVPacket.pts、AVPacket.dts和AVPacket.duration时间信息。如果流不提供
 * 它们,它们也可能未设置(即pts/dts为AV_NOPTS_VALUE,duration为0)。时间信息将以
 * AVStream.time_base单位表示,即必须乘以timebase才能转换为秒。
 *
 * av_read_frame()返回的包总是引用计数的,即设置了AVPacket.buf,用户可以无限期保留它。
 * 当不再需要时,必须用av_packet_unref()释放包。
 *
 * @section lavf_decoding_seek 定位
 * @}
 *
 * @defgroup lavf_encoding 复用
 * @{
 * 复用器以 @ref AVPacket "AVPackets"形式接收编码数据,并将其写入指定容器格式的文件或
 * 其他输出字节流。
 *
 * 复用的主要API函数是avformat_write_header()用于写入文件头,av_write_frame() / 
 * av_interleaved_write_frame()用于写入包,以及av_write_trailer()用于完成文件。
 *
 * 在复用过程开始时,调用者必须首先调用avformat_alloc_context()创建一个复用上下文。
 * 然后调用者通过填充此上下文中的各个字段来设置复用器:
 *
 * - 必须设置 @ref AVFormatContext.oformat "oformat"字段以选择将使用的复用器。
 * - 除非格式是AVFMT_NOFILE类型,否则必须将 @ref AVFormatContext.pb "pb"字段设置为
 *   已打开的IO上下文,可以是从avio_open2()返回的或自定义的。
 * - 除非格式是AVFMT_NOSTREAMS类型,否则必须使用avformat_new_stream()函数创建至少一个流。
 *   调用者应该填写 @ref AVStream.codecpar "流编解码器参数"信息,如编解码器
 *   @ref AVCodecParameters.codec_type "类型", @ref AVCodecParameters.codec_id "ID"
 *   和其他参数(如宽度/高度、像素或采样格式等)。@ref AVStream.time_base "流时基"
 *   应该设置为调用者希望用于此流的时基(注意复用器实际使用的时基可能不同,稍后将描述)。
 * - 建议在重新复用时只手动初始化AVCodecParameters中的相关字段,而不是使用
 *   @ref avcodec_parameters_copy():没有保证编解码器上下文值对输入和输出格式上下文
 *   都保持有效。
 * - 调用者可以填写额外信息,如 @ref AVFormatContext.metadata "全局"或
 *   @ref AVStream.metadata "每个流"的元数据, @ref AVFormatContext.chapters "章节",
 *   @ref AVFormatContext.programs "节目"等,如AVFormatContext文档中所述。这些信息是否
 *   实际存储在输出中取决于容器格式和复用器支持什么。
 *
 * 当复用上下文完全设置好后,调用者必须调用avformat_write_header()来初始化复用器内部
 * 并写入文件头。在这一步是否实际写入IO上下文取决于复用器,但必须始终调用此函数。
 * 任何复用器私有选项必须在options参数中传递给此函数。
 *
 * 然后通过反复调用av_write_frame()或av_interleaved_write_frame()将数据发送到复用器
 * (请查阅这些函数的文档以了解它们之间的区别;单个复用上下文只能使用其中之一,不能混用)。
 * 请注意,发送到复用器的包上的时间信息必须在相应AVStream的时基中。该时基由复用器设置
 * (在avformat_write_header()步骤中),可能与调用者请求的时基不同。
 *
 * 写入所有数据后,调用者必须调用av_write_trailer()来刷新任何缓冲的包并完成输出文件,
 * 然后关闭IO上下文(如果有),最后用avformat_free_context()释放复用上下文。
 * @}
 *
 * @defgroup lavf_io I/O读/写
 * @{
 * @section lavf_io_dirlist 目录列表
 * 目录列表API使得列出远程服务器上的文件成为可能。
 *
 * 一些可能的用例:
 * - 一个"打开文件"对话框来从远程位置选择文件,
 * - 一个递归媒体查找器,为播放器提供播放给定目录中所有文件的能力。
 *
 * @subsection lavf_io_dirlist_open 打开目录
 * 首先,需要通过调用avio_open_dir()打开一个目录,提供URL和可选的包含协议特定参数的
 * ::AVDictionary。该函数在成功时返回零或正整数并分配AVIODirContext。
 *
 * @code
 * AVIODirContext *ctx = NULL;
 * if (avio_open_dir(&ctx, "smb://example.com/some_dir", NULL) < 0) {
 *     fprintf(stderr, "无法打开目录。\n");
 *     abort();
 * }
 * @endcode
 *
 * 这段代码尝试使用smb协议打开一个示例目录,不带任何额外参数。
 *
 * @subsection lavf_io_dirlist_read 读取条目
 * 每个目录的条目(即文件、另一个目录、::AVIODirEntryType中的任何其他内容)由AVIODirEntry
 * 表示。通过在已打开的AVIODirContext上反复调用avio_read_dir()来读取连续的条目。
 * 每次调用如果成功则返回零或正整数。在读取到NULL条目后可以停止读取 - 这意味着没有剩余
 * 条目可读。以下代码读取与ctx关联的目录中的所有条目,并将它们的名称打印到标准输出。
 * @code
 * AVIODirEntry *entry = NULL;
 * for (;;) {
 *     if (avio_read_dir(ctx, &entry) < 0) {
 *         fprintf(stderr, "无法列出目录。\n");
 *         abort();
 *     }
 *     if (!entry)
 *         break;
 *     printf("%s\n", entry->name);
 *     avio_free_directory_entry(&entry);
 * }
 * @endcode
 * @}
 *
 * @defgroup lavf_codec 解复用器
 * @{
 * @defgroup lavf_codec_native 原生解复用器
 * @{
 * @}
 * @defgroup lavf_codec_wrappers 外部库包装器
 * @{
 * @}
 * @}
 * @defgroup lavf_protos I/O协议
 * @{
 * @}
 * @defgroup lavf_internal 内部
 * @{
 * @}
 * @}
 */

#include <stdio.h>  /* FILE */

#include "libavcodec/codec_par.h"
#include "libavcodec/defs.h"
#include "libavcodec/packet.h"

#include "libavutil/dict.h"
#include "libavutil/log.h"

#include "avio.h"
#include "libavformat/version_major.h"
#ifndef HAVE_AV_CONFIG_H
/* 作为ffmpeg构建的一部分包含时,仅包含主版本号以避免不必要的重建。
 * 外部包含时,继续包含完整版本信息。 */
#include "libavformat/version.h"

#include "libavutil/frame.h"
#include "libavcodec/codec.h"
#endif

struct AVFormatContext;
struct AVFrame;
struct AVDeviceInfoList;

/**
 * @defgroup metadata_api 公共元数据API
 * @{
 * @ingroup libavf
 * 元数据API允许libavformat在解复用时向客户端应用程序导出元数据标签。
 * 相反,它允许客户端应用程序在复用时设置元数据。
 *
 * 元数据作为键/值字符串对导出或设置在AVFormatContext、AVStream、AVChapter和
 * AVProgram结构的'metadata'字段中,使用 @ref lavu_dict "AVDictionary" API。
 * 像FFmpeg中的所有字符串一样,元数据假定为UTF-8编码的Unicode。注意在大多数情况下,
 * 解复用器导出的元数据不会检查是否为有效的UTF-8。
 *
 * 需要记住的重要概念:
 * -  键是唯一的;永远不能有2个具有相同键的标签。这也是语义上的意思,即解复用器
 *    不应故意产生字面上不同但语义上相同的几个键。例如,key=Author5, key=Author6。
 *    在这个例子中,所有作者必须放在同一个标签中。
 * -  元数据是扁平的,不是层次的;没有子标签。如果你想存储,例如,制作人Alice和
 *    演员Bob的孩子的电子邮件地址,可以使用key=alice_and_bobs_childs_email_address。
 * -  可以对标签名称应用几个修饰符。这是通过按照它们在下面列表中出现的顺序附加
 *    破折号字符('-')和修饰符名称来完成的 -- 例如foo-eng-sort,而不是foo-sort-eng。
 *    -  language -- 为特定语言本地化的标签值附加ISO 639-2/B 3字母语言代码。
 *       例如: Author-ger=Michael, Author-eng=Mike
 *       原始/默认语言在未限定的"Author"标签中。
 *       如果解复用器设置任何翻译标签,应该设置一个默认值。
 *    -  sorting  -- 应该用于排序的标签的修改版本将附加'-sort'。
 *       例如 artist="The Beatles", artist-sort="Beatles, The"。
 * - 一些协议和解复用器支持元数据更新。在成功调用av_read_frame()后,
 *   AVFormatContext.event_flags或AVStream.event_flags将更新以指示元数据是否
 *   发生变化。为了检测流上的元数据变化,你需要循环遍历AVFormatContext中的所有流
 *   并检查它们各自的event_flags。
 *
 * -  解复用器尝试以通用格式导出元数据,但是没有通用等价物的标签保持它们在容器中
 *    存储的方式。以下是通用标签名称列表:
 *
 @verbatim
 album        -- 这个作品所属的集合的名称
 album_artist -- 集合/专辑的主要创作者,如果与artist不同。
                 例如编译专辑的"Various Artists"。
 artist       -- 作品的主要创作者
 comment      -- 文件的任何附加描述。
 composer     -- 谁创作了这个作品,如果与artist不同。
 copyright    -- 版权持有者的名称。
 creation_time-- 文件创建的日期,最好是ISO 8601格式。
 date         -- 作品创建的日期,最好是ISO 8601格式。
 disc         -- 子集的编号,例如多碟集合中的碟片。
 encoder      -- 生成文件的软件/硬件的名称/设置。
 encoded_by   -- 创建文件的人/组。
 filename     -- 文件的原始名称。
 genre        -- <不言自明>。
 language     -- 表演作品的主要语言,最好是ISO 639-2格式。
                 可以用逗号分隔指定多种语言。
 performer    -- 表演作品的艺术家,如果与artist不同。
                 例如对于"查拉图斯特拉如是说",artist将是"理查德·施特劳斯",
                 performer将是"伦敦爱乐乐团"。
 publisher    -- 唱片公司/出版商的名称。
 service_name     -- 广播中的服务名称(频道名称)。
 service_provider -- 广播中的服务提供商名称。
 title        -- 作品的名称。
 track        -- 这个作品在集合中的编号,可以是current/total形式。
 variant_bitrate -- 当前流所属的比特率变体的总比特率
 @endverbatim
 *
 * 查看示例部分了解如何使用元数据API的应用程序示例。
 *
 * @}
 */

/* 包函数 */


/**
 * 分配并读取包的有效载荷并用默认值初始化其字段。
 *
 * @param s    关联的IO上下文
 * @param pkt 包
 * @param size 期望的有效载荷大小
 * @return >0 (读取大小)如果成功,否则AVERROR_xxx
 */
int av_get_packet(AVIOContext *s, AVPacket *pkt, int size);

/**
 * 读取数据并将其附加到AVPacket的当前内容中。
 * 如果pkt->size为0,这与av_get_packet相同。
 * 注意这使用av_grow_packet,因此涉及重新分配,
 * 这是低效的。因此此函数应该只在无法合理知道
 * (或预估)最终大小的情况下使用。
 *
 * @param s    关联的IO上下文
 * @param pkt  数据包
 * @param size 要读取的数据量
 * @return >0 (读取大小)如果成功,否则AVERROR_xxx,即使发生错误
 *         之前的数据也不会丢失。
 */
int av_append_packet(AVIOContext *s, AVPacket *pkt, int size);

/*************************************************/
/* 输入/输出格式 */

struct AVCodecTag;

/**
 * 此结构包含格式探测文件所需的数据。
 */
typedef struct AVProbeData {
    const char *filename;
    unsigned char *buf; /**< 缓冲区必须有AVPROBE_PADDING_SIZE个额外分配的字节并填充零。 */
    int buf_size;       /**< buf的大小,不包括额外分配的字节 */
    const char *mime_type; /**< mime类型,如果已知。 */
} AVProbeData;

#define AVPROBE_SCORE_RETRY (AVPROBE_SCORE_MAX/4)
#define AVPROBE_SCORE_STREAM_RETRY (AVPROBE_SCORE_MAX/4-1)

#define AVPROBE_SCORE_EXTENSION  50 ///< 文件扩展名得分
#define AVPROBE_SCORE_MIME       75 ///< 文件mime类型得分
#define AVPROBE_SCORE_MAX       100 ///< 最高得分

#define AVPROBE_PADDING_SIZE 32             ///< 探测缓冲区末尾的额外分配字节数

/// 解复用器将使用avio_open,调用者不应提供已打开的文件。
#define AVFMT_NOFILE        0x0001
#define AVFMT_NEEDNUMBER    0x0002 /**< 文件名中需要'%d'。 */
/**
 * 复用器/解复用器是实验性的,应谨慎使用。
 *
 * - 解复用器: 不会通过探测自动选择,必须明确指定。
 */
#define AVFMT_EXPERIMENTAL  0x0004
#define AVFMT_SHOW_IDS      0x0008 /**< 显示格式流ID号。 */
#define AVFMT_GLOBALHEADER  0x0040 /**< 格式需要全局头。 */
#define AVFMT_NOTIMESTAMPS  0x0080 /**< 格式不需要/没有任何时间戳。 */
#define AVFMT_GENERIC_INDEX 0x0100 /**< 使用通用索引构建代码。 */
#define AVFMT_TS_DISCONT    0x0200 /**< 格式允许时间戳不连续。注意,复用器始终需要有效(单调)的时间戳 */
#define AVFMT_VARIABLE_FPS  0x0400 /**< 格式允许可变帧率。 */
#define AVFMT_NODIMENSIONS  0x0800 /**< 格式不需要宽度/高度 */
#define AVFMT_NOSTREAMS     0x1000 /**< 格式不需要任何流 */
#define AVFMT_NOBINSEARCH   0x2000 /**< 格式不允许通过read_timestamp回退到二进制搜索 */
#define AVFMT_NOGENSEARCH   0x4000 /**< 格式不允许回退到通用搜索 */
#define AVFMT_NO_BYTE_SEEK  0x8000 /**< 格式不允许按字节搜索 */
#if FF_API_ALLOW_FLUSH
#define AVFMT_ALLOW_FLUSH  0x10000 /**< @deprecated: 如果要刷新复用器,只需发送NULL数据包。 */
#endif
#define AVFMT_TS_NONSTRICT 0x20000 /**< 格式不要求严格递增的时间戳,
                                        但它们仍必须是单调的 */
#define AVFMT_TS_NEGATIVE  0x40000 /**< 格式允许复用负时间戳。如果未设置,
                                        时间戳将在av_write_frame和
                                        av_interleaved_write_frame中移位,
                                        使其从0开始。
                                        用户或复用器可以通过
                                        AVFormatContext.avoid_negative_ts
                                        覆盖此设置
                                        */

#define AVFMT_SEEK_TO_PTS   0x4000000 /**< 搜索基于PTS */

/**
 * @addtogroup lavf_encoding
 * @{
 */
typedef struct AVOutputFormat {
    const char *name;
    /**
     * 格式的描述性名称,旨在比name更易读。
     * 你应该使用NULL_IF_CONFIG_SMALL()宏来定义它。
     */
    const char *long_name;
    const char *mime_type;
    const char *extensions; /**< 逗号分隔的文件扩展名 */
    /* 输出支持 */
    enum AVCodecID audio_codec;    /**< 默认音频编解码器 */
    enum AVCodecID video_codec;    /**< 默认视频编解码器 */
    enum AVCodecID subtitle_codec; /**< 默认字幕编解码器 */
    /**
     * 可以使用标志: AVFMT_NOFILE, AVFMT_NEEDNUMBER,
     * AVFMT_GLOBALHEADER, AVFMT_NOTIMESTAMPS, AVFMT_VARIABLE_FPS,
     * AVFMT_NODIMENSIONS, AVFMT_NOSTREAMS,
     * AVFMT_TS_NONSTRICT, AVFMT_TS_NEGATIVE
     */
    int flags;

    /**
     * 支持的codec_id-codec_tag对列表,按"更好的选择在前"排序。
     * 所有数组都以AV_CODEC_ID_NONE终止。
     */
    const struct AVCodecTag * const *codec_tag;


    const AVClass *priv_class; ///< 私有上下文的AVClass
} AVOutputFormat;
/**
 * @}
 */

/**
 * @addtogroup lavf_decoding
 * @{
 */
typedef struct AVInputFormat {
    /**
     * 格式的简短名称列表,用逗号分隔。可以在次要版本更新时
     * 添加新名称。
     */
    const char *name;

    /**
     * 格式的描述性名称,旨在比name更易读。
     * 你应该使用NULL_IF_CONFIG_SMALL()宏来定义它。
     */
    const char *long_name;

    /**
     * 可以使用标志: AVFMT_NOFILE, AVFMT_NEEDNUMBER, AVFMT_SHOW_IDS,
     * AVFMT_NOTIMESTAMPS, AVFMT_GENERIC_INDEX, AVFMT_TS_DISCONT, AVFMT_NOBINSEARCH,
     * AVFMT_NOGENSEARCH, AVFMT_NO_BYTE_SEEK, AVFMT_SEEK_TO_PTS。
     */
    int flags;

    /**
     * 如果定义了扩展名,则不会进行探测。你通常不应该
     * 使用扩展名格式猜测,因为它不够可靠
     */
    const char *extensions;

    const struct AVCodecTag * const *codec_tag;

    const AVClass *priv_class; ///< 私有上下文的AVClass

    /**
     * 逗号分隔的MIME类型列表。
     * 在探测时用于检查匹配的MIME类型。
     * @see av_probe_input_format2
     */
    const char *mime_type;

    /*****************************************************************
     * 此行以下的字段不属于公共API。它们不能在libavformat之外
     * 使用,并且可以随意更改和删除。
     * 新的公共字段应该添加在上面。
     *****************************************************************
     */
    /**
     * 原始解复用器在此存储其编解码器ID。
     */
    int raw_codec_id;

    /**
     * 私有数据的大小,以便可以在包装器中分配。
     */
    int priv_data_size;

    /**
     * 内部标志。参见internal.h中的FF_FMT_FLAG_*。
     */
    int flags_internal;

    /**
     * 判断给定文件是否可能被解析为此格式。
     * 提供的缓冲区保证至少有AVPROBE_PADDING_SIZE字节大小,
     * 所以除非你需要更多,否则不必检查。
     */
    int (*read_probe)(const AVProbeData *);

    /**
     * 读取格式头并初始化AVFormatContext结构。
     * 成功返回0。应该调用'avformat_new_stream'来创建新流。
     */
    int (*read_header)(struct AVFormatContext *);

    /**
     * 读取一个数据包并放入'pkt'中。pts和flags也会被设置。
     * 只有在使用AVFMTCTX_NOHEADER标志时,且只能在调用线程中
     * (不能在后台线程中)才能调用'avformat_new_stream'。
     * @return 成功返回0,错误时返回< 0。
     *         返回错误时,调用者必须取消引用pkt。
     */
    int (*read_packet)(struct AVFormatContext *, AVPacket *pkt);

    /**
     * 关闭流。此函数不会释放AVFormatContext和AVStreams
     */
    int (*read_close)(struct AVFormatContext *);

    /**
     * 相对于stream_index流组件中的帧,
     * 寻找到给定时间戳。
     * @param stream_index 不能为-1。
     * @param flags 在没有精确匹配时选择应该优先的方向。
     * @return 成功时返回>= 0(但不一定是新的偏移量)
     */
    int (*read_seek)(struct AVFormatContext *,
                     int stream_index, int64_t timestamp, int flags);

    /**
     * 获取stream[stream_index].time_base单位中的下一个时间戳。
     * @return 时间戳,如果发生错误则返回AV_NOPTS_VALUE
     */
    int64_t (*read_timestamp)(struct AVFormatContext *s, int stream_index,
                              int64_t *pos, int64_t pos_limit);

    /**
     * 开始/恢复播放 - 仅对使用基于网络的格式(RTSP)有意义。
     */
    int (*read_play)(struct AVFormatContext *);

    /**
     * 暂停播放 - 仅对使用基于网络的格式(RTSP)有意义。
     */
    int (*read_pause)(struct AVFormatContext *);

    /**
     * 寻找到时间戳ts。
     * 寻找将使所有活动流都可以成功呈现的点最接近ts,
     * 并在min_ts/max_ts范围内。
     * 活动流是所有AVStream.discard < AVDISCARD_ALL的流。
     */
    int (*read_seek2)(struct AVFormatContext *s, int stream_index, int64_t min_ts, int64_t ts, int64_t max_ts, int flags);

    /**
     * 返回带有属性的设备列表。
     * @see avdevice_list_devices()了解更多详情。
     */
    int (*get_device_list)(struct AVFormatContext *s, struct AVDeviceInfoList *device_list);

} AVInputFormat;
/**
 * @}
 */

enum AVStreamParseType {
    AVSTREAM_PARSE_NONE,
    AVSTREAM_PARSE_FULL,       /**< 完整解析和重新打包 */
    AVSTREAM_PARSE_HEADERS,    /**< 只解析头部,不重新打包 */
    AVSTREAM_PARSE_TIMESTAMPS, /**< 完整解析和对不在数据包边界开始的帧进行时间戳插值 */
    AVSTREAM_PARSE_FULL_ONCE,  /**< 仅对第一帧进行完整解析和重新打包,目前仅实现用于H.264 */
    AVSTREAM_PARSE_FULL_RAW,   /**< 完整解析和重新打包,解析器为原始数据生成时间戳和位置
                                    这假设文件中的每个数据包都不包含解复用器级别的头部,
                                    只包含编解码器级别的数据,否则位置生成将失败 */
};

typedef struct AVIndexEntry {
    int64_t pos;
    int64_t timestamp;        /**<
                               * AVStream.time_base单位的时间戳,最好是在寻找到此条目时
                               * 可以正确解码帧的时间。这意味着在基于关键帧的格式上
                               * 最好是关键帧的PTS。
                               * 但如果对实现更方便或没有更好的已知信息,
                               * 解复用器可以选择存储不同的时间戳
                               */
#define AVINDEX_KEYFRAME 0x0001
#define AVINDEX_DISCARD_FRAME  0x0002    /**
                                          * 该标志用于指示解码后应丢弃哪些帧。
                                          */
    int flags:2;
    int size:30; //是的,试图保持这个结构小一点以减少内存需求(由于可能的8字节对齐,它是24字节而不是32字节)。
    int min_distance;         /**< 此关键帧与前一个关键帧之间的最小距离,用于避免不必要的搜索。 */
} AVIndexEntry;

/**
 * 在同类型的其他流中,该流应该默认被选择,
 * 除非用户明确指定了其他选择。
 */
#define AV_DISPOSITION_DEFAULT              (1 << 0)
/**
 * 该流不是原始语言。
 *
 * @note AV_DISPOSITION_ORIGINAL是此disposition的反面。在正确标记的流中
 *       它们最多应该设置其中之一。
 * @note 此disposition可以应用于任何流类型,不仅仅是音频。
 */
#define AV_DISPOSITION_DUB                  (1 << 1)
/**
 * 该流是原始语言。
 *
 * @see AV_DISPOSITION_DUB的注释
 */
#define AV_DISPOSITION_ORIGINAL             (1 << 2)
/**
 * 该流是评论轨道。
 */
#define AV_DISPOSITION_COMMENT              (1 << 3)
/**
 * 该流包含歌词。
 */
#define AV_DISPOSITION_LYRICS               (1 << 4)
/**
 * 该流包含卡拉OK音频。
 */
#define AV_DISPOSITION_KARAOKE              (1 << 5)

/**
 * 播放时默认应使用该轨道。
 * 对于即使用户没有明确要求字幕也应该显示的
 * 字幕轨道很有用。
 */
#define AV_DISPOSITION_FORCED               (1 << 6)
/**
 * 该流适用于听力障碍观众。
 */
#define AV_DISPOSITION_HEARING_IMPAIRED     (1 << 7)
/**
 * 该流适用于视力障碍观众。
 */
#define AV_DISPOSITION_VISUAL_IMPAIRED      (1 << 8)
/**
 * 音频流包含无人声的音乐和音效。
 */
#define AV_DISPOSITION_CLEAN_EFFECTS        (1 << 9)
/**
 * 该流作为附加图片/"封面艺术"存储在文件中(例如ID3v2中的APIC帧)。
 * 与其关联的第一个(通常是唯一的)数据包将在从文件读取的前几个数据包中返回,
 * 除非发生了寻找。也可以随时在AVStream.attached_pic中访问它。
 */
#define AV_DISPOSITION_ATTACHED_PIC         (1 << 10)
/**
 * 该流是稀疏的,包含缩略图,通常对应于章节标记。
 * 仅与AV_DISPOSITION_ATTACHED_PIC一起使用。
 */
#define AV_DISPOSITION_TIMED_THUMBNAILS     (1 << 11)

/**
 * 该流旨在与空间音频轨道混合。例如,
 * 它可以用于旁白或立体声音乐,并且可能不受
 * 听众头部旋转的影响。
 */
#define AV_DISPOSITION_NON_DIEGETIC         (1 << 12)

/**
 * 字幕流包含字幕,提供音频的转录和可能的翻译。
 * 通常适用于听力障碍观众。
 */
#define AV_DISPOSITION_CAPTIONS             (1 << 16)
/**
 * 字幕流包含视频内容的文字描述。
 * 通常适用于视力障碍观众或无法观看视频的情况。
 */
#define AV_DISPOSITION_DESCRIPTIONS         (1 << 17)
/**
 * 字幕流包含不打算直接呈现给用户的
 * 时间对齐的元数据。
 */
#define AV_DISPOSITION_METADATA             (1 << 18)
/**
 * 音频流旨在在呈现前与另一个流混合。
 * 对应于mpegts中的mix_type=0。
 */
#define AV_DISPOSITION_DEPENDENT            (1 << 19)
/**
 * 视频流包含静态图像。
 */
#define AV_DISPOSITION_STILL_IMAGE          (1 << 20)

/**
 * @return 对应于disp的AV_DISPOSITION_*标志,如果disp不对应
 *         已知的流disposition则返回负错误代码。
 */
int av_disposition_from_string(const char *disp);

/**
 * @param disposition AV_DISPOSITION_*值的组合
 * @return 对应于disposition中最低设置位的字符串描述。
 *         当最低设置位不对应已知disposition或
 *         disposition为0时返回NULL。
 */
const char *av_disposition_to_string(int disposition);

/**
 * 时间戳环绕检测的行为选项。
 */
#define AV_PTS_WRAP_IGNORE      0   ///< 忽略环绕
#define AV_PTS_WRAP_ADD_OFFSET  1   ///< 在检测到环绕时添加格式特定的偏移量
#define AV_PTS_WRAP_SUB_OFFSET  -1  ///< 在检测到环绕时减去格式特定的偏移量

/**
 * 流结构体。
 * 可以通过小版本升级在末尾添加新字段。
 * 删除、重新排序和更改现有字段需要大版本升级。
 * 不得在 libav* 外部使用 sizeof(AVStream)。
 */
typedef struct AVStream {
    /**
     * @ref avoptions 的类。在流创建时设置。
     */
    const AVClass *av_class;

    int index;    /**< AVFormatContext 中的流索引 */
    /**
     * 格式特定的流 ID。
     * 解码: 由 libavformat 设置
     * 编码: 由用户设置,如果未设置则由 libavformat 替换
     */
    int id;

    /**
     * 与此流关联的编解码器参数。由 libavformat 在 avformat_new_stream() 中分配,
     * 在 avformat_free_context() 中释放。
     *
     * - 解复用: 由 libavformat 在流创建时或在 avformat_find_stream_info() 中填充
     * - 复用: 在 avformat_write_header() 之前由调用者填充
     */
    AVCodecParameters *codecpar;

    void *priv_data;

    /**
     * 这是帧时间戳表示的基本时间单位(以秒为单位)。
     *
     * 解码: 由 libavformat 设置
     * 编码: 可以在 avformat_write_header() 之前由调用者设置,
     *       以向复用器提供关于所需时基的提示。在 avformat_write_header() 中,
     *       复用器将用实际用于写入文件的时间戳的时基覆盖此字段
     *       (这可能与用户提供的时基有关,也可能无关,取决于格式)。
     */
    AVRational time_base;

    /**
     * 解码: 流中第一帧的 pts(以流时基表示的演示顺序)。
     * 仅在您 100% 确定设置的值确实是第一帧的 pts 时才设置此值。
     * 这可能未定义(AV_NOPTS_VALUE)。
     * @note ASF 头不包含正确的 start_time,ASF 解复用器不得设置此值。
     */
    int64_t start_time;

    /**
     * 解码: 流的持续时间,以流时基为单位。
     * 如果源文件未指定持续时间,但指定了比特率,
     * 则此值将根据比特率和文件大小估算。
     *
     * 编码: 可以在 avformat_write_header() 之前由调用者设置,
     * 以向复用器提供关于估计持续时间的提示。
     */
    int64_t duration;

    int64_t nb_frames;                 ///< 此流中的帧数(如果已知)或 0

    /**
     * 流处置 - AV_DISPOSITION_* 标志的组合。
     * - 解复用: 由 libavformat 在创建流时或在 avformat_find_stream_info() 中设置。
     * - 复用: 可以在 avformat_write_header() 之前由调用者设置。
     */
    int disposition;

    enum AVDiscard discard; ///< 选择可以随意丢弃且不需要解复用的数据包。

    /**
     * 采样宽高比(0 表示未知)
     * - 编码: 由用户设置。
     * - 解码: 由 libavformat 设置。
     */
    AVRational sample_aspect_ratio;

    AVDictionary *metadata;

    /**
     * 平均帧率
     *
     * - 解复用: 可能由 libavformat 在创建流时或在 avformat_find_stream_info() 中设置。
     * - 复用: 可以在 avformat_write_header() 之前由调用者设置。
     */
    AVRational avg_frame_rate;

    /**
     * 对于具有 AV_DISPOSITION_ATTACHED_PIC 处置的流,
     * 此数据包将包含附加的图片。
     *
     * 解码: 由 libavformat 设置,调用者不得修改。
     * 编码: 未使用
     */
    AVPacket attached_pic;

#if FF_API_AVSTREAM_SIDE_DATA
    /**
     * 适用于整个流的边数据数组(即容器不允许在数据包之间更改)。
     *
     * 此数组中的边数据与数据包中的边数据之间可能没有重叠。
     * 即给定的边数据要么由复用器在此数组中导出(解复用)/由调用者设置(复用),
     * 然后它永远不会出现在数据包中,要么通过数据包导出/发送边数据
     * (始终在值变为已知或更改的第一个数据包中),然后它不会出现在此数组中。
     *
     * - 解复用: 在创建流时由 libavformat 设置。
     * - 复用: 可以在 avformat_write_header() 之前由调用者设置。
     *
     * 由 libavformat 在 avformat_free_context() 中释放。
     *
     * @deprecated 使用 AVStream 的 @ref AVCodecParameters.coded_side_data
     *             "codecpar side data"。
     */
    attribute_deprecated
    AVPacketSideData *side_data;
    /**
     * AVStream.side_data 数组中的元素数。
     *
     * @deprecated 使用 AVStream 的 @ref AVCodecParameters.nb_coded_side_data
     *             "codecpar side data"。
     */
    attribute_deprecated
    int            nb_side_data;
#endif

    /**
     * 指示流上发生的事件的标志,AVSTREAM_EVENT_FLAG_* 的组合。
     *
     * - 解复用: 可能由解复用器在 avformat_open_input()、
     *   avformat_find_stream_info() 和 av_read_frame() 中设置。
     *   一旦处理完事件,用户必须清除标志。
     * - 复用: 可以在 avformat_write_header() 之后由用户设置,
     *   以指示用户触发的事件。复用器将在 av_[interleaved]_write_frame() 中
     *   清除它已处理的事件的标志。
     */
    int event_flags;
/**
 * - 解复用: 解复用器从文件中读取新的元数据并相应更新了 AVStream.metadata
 * - 复用: 用户更新了 AVStream.metadata 并希望复用器将其写入文件
 */
#define AVSTREAM_EVENT_FLAG_METADATA_UPDATED 0x0001
/**
 * - 解复用: 从文件中读取了此流的新数据包。此事件仅供参考,
 *   并不保证从 av_read_frame() 返回此流的新数据包。
 */
#define AVSTREAM_EVENT_FLAG_NEW_PACKETS (1 << 1)

    /**
     * 流的实际基本帧率。
     * 这是可以准确表示所有时间戳的最低帧率
     * (它是流中所有帧率的最小公倍数)。
     * 注意,这个值只是一个猜测!
     * 例如,如果时基是 1/90000,所有帧的计时器刻度约为 3600 或 1800,
     * 则 r_frame_rate 将为 50/1。
     */
    AVRational r_frame_rate;

    /**
     * 时间戳中的位数。用于环绕控制。
     *
     * - 解复用: 由 libavformat 设置
     * - 复用: 由 libavformat 设置
     *
     */
    int pts_wrap_bits;
} AVStream;

struct AVCodecParserContext *av_stream_get_parser(const AVStream *s);

#if FF_API_GET_END_PTS
/**
 * 返回最后一个复用数据包的 pts + 其持续时间
 *
 * 与解复用器一起使用时返回值未定义。
 */
attribute_deprecated
int64_t    av_stream_get_end_pts(const AVStream *st);
#endif

#define AV_PROGRAM_RUNNING 1

/**
 * 可以通过小版本升级在末尾添加新字段。
 * 删除、重新排序和更改现有字段需要大版本升级。
 * 不得在 libav* 外部使用 sizeof(AVProgram)。
 */
typedef struct AVProgram {
    int            id;
    int            flags;
    enum AVDiscard discard;        ///< 选择要丢弃的程序和要提供给调用者的程序
    unsigned int   *stream_index;
    unsigned int   nb_stream_indexes;
    AVDictionary *metadata;

    int program_num;
    int pmt_pid;
    int pcr_pid;
    int pmt_version;

    /*****************************************************************
     * 此行以下的所有字段都不是公共 API 的一部分。它们
     * 不得在 libavformat 外部使用,可以随意更改和
     * 删除。
     * 新的公共字段应添加在正上方。
     *****************************************************************
     */
    int64_t start_time;
    int64_t end_time;

    int64_t pts_wrap_reference;    ///< 用于环绕检测的参考 dts
    int pts_wrap_behavior;         ///< 环绕检测时的行为
} AVProgram;

#define AVFMTCTX_NOHEADER      0x0001 /**< 表示不存在头部
                                         (动态添加流) */
#define AVFMTCTX_UNSEEKABLE    0x0002 /**< 表示流绝对不可查找,
                                         尝试调用查找函数将失败。
                                         对于某些网络协议(如 HLS),
                                         这可能在运行时动态更改。 */

typedef struct AVChapter {
    int64_t id;             ///< 用于标识章节的唯一 ID
    AVRational time_base;   ///< 指定开始/结束时间戳的时基
    int64_t start, end;     ///< 以时基单位表示的章节开始/结束时间
    AVDictionary *metadata;
} AVChapter;


/**
 * 设备用于与应用程序通信的回调。
 */
typedef int (*av_format_control_message)(struct AVFormatContext *s, int type,
                                         void *data, size_t data_size);

typedef int (*AVOpenCallback)(struct AVFormatContext *s, AVIOContext **pb, const char *url, int flags,
                              const AVIOInterruptCB *int_cb, AVDictionary **options);

/**
 * 视频的持续时间可以通过各种方式估算,此枚举可用于
 * 了解持续时间是如何估算的。
 */
enum AVDurationEstimationMethod {
    AVFMT_DURATION_FROM_PTS,    ///< 从 PTS 准确估算的持续时间
    AVFMT_DURATION_FROM_STREAM, ///< 从具有已知持续时间的流估算的持续时间
    AVFMT_DURATION_FROM_BITRATE ///< 从比特率估算的持续时间(较不准确)
};

/**
 * 格式 I/O 上下文。
 * 可以通过小版本升级在末尾添加新字段。
 * 删除、重新排序和更改现有字段需要大版本升级。
 * 不得在 libav* 外部使用 sizeof(AVFormatContext),
 * 使用 avformat_alloc_context() 创建 AVFormatContext。
 *
 * 可以通过 AVOptions (av_opt*) 访问字段,
 * 使用的名称字符串与相关的命令行参数名称匹配,
 * 可以在 libavformat/options_table.h 中找到。
 * 由于历史原因或简洁性,AVOption/命令行参数名称在某些情况下
 * 与 C 结构字段名称不同。
 */
typedef struct AVFormatContext {
    /**
     * 用于日志记录和 @ref avoptions 的类。由 avformat_alloc_context() 设置。
     * 如果存在,则导出(解)复用器私有选项。
     */
    const AVClass *av_class;

    /**
     * 输入容器格式。
     *
     * 仅用于解复用,由 avformat_open_input() 设置。
     */
    const struct AVInputFormat *iformat;

    /**
     * 输出容器格式。
     *
     * 仅用于复用,必须在调用 avformat_write_header() 之前由调用者设置。
     */
    const struct AVOutputFormat *oformat;

    /**
     * 格式私有数据。仅当 iformat/oformat.priv_class 不为 NULL 时,
     * 这是一个启用了 AVOptions 的结构。
     *
     * - 复用: 由 avformat_write_header() 设置
     * - 解复用: 由 avformat_open_input() 设置
     */
    void *priv_data;

    /**
     * I/O 上下文。
     *
     * - 解复用: 要么在 avformat_open_input() 之前由用户设置(然后用户必须手动关闭),
     *           要么由 avformat_open_input() 设置。
     * - 复用: 在 avformat_write_header() 之前由用户设置。调用者必须负责关闭/释放 IO 上下文。
     *
     * 如果在 iformat/oformat.flags 中设置了 AVFMT_NOFILE 标志,则不要设置此字段。
     * 在这种情况下,(解)复用器将以其他方式处理 I/O,此字段将为 NULL。
     */
    AVIOContext *pb;

    /* 流信息 */
    /**
     * 表示流属性的标志。AVFMTCTX_* 的组合。
     * 由 libavformat 设置。
     */
    int ctx_flags;

    /**
     * AVFormatContext.streams 中的元素数量。
     *
     * 由 avformat_new_stream() 设置,不得由任何其他代码修改。
     */
    unsigned int nb_streams;
    /**
     * 文件中所有流的列表。使用 avformat_new_stream() 创建新流。
     *
     * - 解复用: 在 avformat_open_input() 中由 libavformat 创建流。
     *           如果在 ctx_flags 中设置了 AVFMTCTX_NOHEADER,
     *           则新流也可能在 av_read_frame() 中出现。
     * - 复用: 在 avformat_write_header() 之前由用户创建流。
     *
     * 由 libavformat 在 avformat_free_context() 中释放。
     */
    AVStream **streams;

    /**
     * 输入或输出 URL。与旧的 filename 字段不同,此字段没有长度限制。
     *
     * - 解复用: 由 avformat_open_input() 设置,如果在 avformat_open_input() 中
     *           url 参数为 NULL,则初始化为空字符串。
     * - 复用: 可以在调用 avformat_write_header() 之前(或如果先调用 avformat_init_output())
     *         由调用者设置为可由 av_free() 释放的字符串。如果在 avformat_init_output() 中为 NULL,
     *         则设置为空字符串。
     *
     * 由 libavformat 在 avformat_free_context() 中释放。
     */
    char *url;

    /**
     * 组件第一帧的位置,以 AV_TIME_BASE 分数秒为单位。
     * 永远不要直接设置此值:它是从 AVStream 值推导出来的。
     *
     * 仅用于解复用,由 libavformat 设置。
     */
    int64_t start_time;

    /**
     * 流的持续时间,以 AV_TIME_BASE 分数秒为单位。
     * 仅当您不知道任何单个流的持续时间并且也不设置其中任何一个时,
     * 才设置此值。如果未设置,则从 AVStream 值推导。
     *
     * 仅用于解复用,由 libavformat 设置。
     */
    int64_t duration;

    /**
     * 总流比特率(bit/s),如果不可用则为 0。
     * 如果已知文件大小和持续时间,则永远不要直接设置它,
     * 因为 FFmpeg 可以自动计算它。
     */
    int64_t bit_rate;

    unsigned int packet_size;
    int max_delay;

    /**
     * 修改(解)复用器行为的标志。AVFMT_FLAG_* 的组合。
     * 在 avformat_open_input() / avformat_write_header() 之前由用户设置。
     */
    int flags;
#define AVFMT_FLAG_GENPTS       0x0001 ///< 即使需要解析未来帧也要生成缺失的 pts。
#define AVFMT_FLAG_IGNIDX       0x0002 ///< 忽略索引。
#define AVFMT_FLAG_NONBLOCK     0x0004 ///< 从输入读取数据包时不阻塞。
#define AVFMT_FLAG_IGNDTS       0x0008 ///< 忽略同时包含 DTS 和 PTS 的帧上的 DTS
#define AVFMT_FLAG_NOFILLIN     0x0010 ///< 不从其他值推断任何值,只返回容器中存储的内容
#define AVFMT_FLAG_NOPARSE      0x0020 ///< 不使用 AVParsers,您还必须设置 AVFMT_FLAG_NOFILLIN,因为填充代码在帧上工作,没有解析 -> 没有帧。此外,如果禁用了解析以查找帧边界,则无法进行帧定位
#define AVFMT_FLAG_NOBUFFER     0x0040 ///< 尽可能不缓冲帧
#define AVFMT_FLAG_CUSTOM_IO    0x0080 ///< 调用者提供了自定义的 AVIOContext,不要调用 avio_close()。
#define AVFMT_FLAG_DISCARD_CORRUPT  0x0100 ///< 丢弃标记为损坏的帧
#define AVFMT_FLAG_FLUSH_PACKETS    0x0200 ///< 每个数据包都刷新 AVIOContext。
/**
 * 复用时,尽量避免向输出写入任何随机/易变数据。
 * 这包括任何随机 ID、实时时间戳/日期、复用器版本等。
 *
 * 此标志主要用于测试。
 */
#define AVFMT_FLAG_BITEXACT         0x0400
#define AVFMT_FLAG_SORT_DTS    0x10000 ///< 尝试按 dts 交错输出数据包(使用此标志可能会降低解复用速度)
#define AVFMT_FLAG_FAST_SEEK   0x80000 ///< 为某些格式启用快速但不准确的定位
#if FF_API_LAVF_SHORTEST
#define AVFMT_FLAG_SHORTEST   0x100000 ///< 最短流停止时停止复用。
#endif
#define AVFMT_FLAG_AUTO_BSF   0x200000 ///< 根据复用器的要求添加比特流过滤器

    /**
     * 为确定流属性而从输入读取的最大字节数。
     * 用于读取全局头部和在 avformat_find_stream_info() 中。
     *
     * 仅用于解复用,在 avformat_open_input() 之前由调用者设置。
     *
     * @note 这 \e 不用于确定 \ref AVInputFormat "输入格式"
     * @sa format_probesize
     */
    int64_t probesize;

    /**
     * 在 avformat_find_stream_info() 中从输入读取数据的最大持续时间
     * (以 AV_TIME_BASE 单位)。
     * 仅用于解复用,在 avformat_find_stream_info() 之前由调用者设置。
     * 可以设置为 0 让 avformat 使用启发式方法选择。
     */
    int64_t max_analyze_duration;

    const uint8_t *key;
    int keylen;

    unsigned int nb_programs;
    AVProgram **programs;

    /**
     * 强制视频编解码器 ID。
     * 解复用: 由用户设置。
     */
    enum AVCodecID video_codec_id;

    /**
     * 强制音频编解码器 ID。
     * 解复用: 由用户设置。
     */
    enum AVCodecID audio_codec_id;

    /**
     * 强制字幕编解码器 ID。
     * 解复用: 由用户设置。
     */
    enum AVCodecID subtitle_codec_id;

    /**
     * 每个流索引使用的最大内存字节数。
     * 如果索引超过此大小,将根据需要丢弃条目以维持较小的大小。
     * 这可能导致定位速度较慢或准确性较低(取决于解复用器)。
     * 对于必须使用完整内存索引的解复用器将忽略此设置。
     * - 复用: 未使用
     * - 解复用: 由用户设置
     */
    unsigned int max_index_size;

    /**
     * 用于缓冲从实时捕获设备获得的帧的最大内存字节数。
     */
    unsigned int max_picture_buffer;

    /**
     * AVChapter 数组中的章节数。
     * 复用时,章节通常写入文件头,因此在调用 write_header 之前
     * 应该初始化 nb_chapters。一些复用器(如 mov 和 mkv)也可以
     * 在尾部写入章节。要在尾部写入章节,调用 write_header 时
     * nb_chapters 必须为零,调用 write_trailer 时必须为非零。
     * - 复用: 由用户设置
     * - 解复用: 由 libavformat 设置
     */
    unsigned int nb_chapters;
    AVChapter **chapters;

    /**
     * 适用于整个文件的元数据。
     *
     * - 解复用: 在 avformat_open_input() 中由 libavformat 设置
     * - 复用: 可以在 avformat_write_header() 之前由调用者设置
     *
     * 由 libavformat 在 avformat_free_context() 中释放。
     */
    AVDictionary *metadata;

    /**
     * 流在真实世界时间中的开始时间,以微秒为单位,
     * 从 Unix 纪元(1970 年 1 月 1 日 00:00)开始。也就是说,
     * 流中的 pts=0 是在这个真实世界时间捕获的。
     * - 复用: 在 avformat_write_header() 之前由调用者设置。如果设置为
     *         0 或 AV_NOPTS_VALUE,则使用当前墙上时间。
     * - 解复用: 由 libavformat 设置。如果未知则为 AV_NOPTS_VALUE。注意
     *           该值可能在接收到一定数量的帧后才能知道。
     */
    int64_t start_time_realtime;

    /**
     * 在 avformat_find_stream_info() 中用于确定帧率的帧数。
     * 仅用于解复用,在 avformat_find_stream_info() 之前由调用者设置。
     */
    int fps_probe_size;

    /**
     * 错误识别;较高的值将检测到更多错误,但可能会将一些或多或少
     * 有效的部分误判为错误。
     * 仅用于解复用,在 avformat_open_input() 之前由调用者设置。
     */
    int error_recognition;

    /**
     * I/O 层的自定义中断回调。
     *
     * 解复用: 在 avformat_open_input() 之前由用户设置。
     * 复用: 在 avformat_write_header() 之前由用户设置
     * (主要用于 AVFMT_NOFILE 格式)。如果用于打开文件,
     * 回调也应传递给 avio_open2()。
     */
    AVIOInterruptCB interrupt_callback;

    /**
     * 用于启用调试的标志。
     */
    int debug;
#define FF_FDEBUG_TS        0x0001

    /**
     * 交错的最大缓冲持续时间。
     *
     * 为确保所有流正确交错,av_interleaved_write_frame() 将等待,
     * 直到每个流至少有一个数据包,然后才实际将任何数据包写入输出文件。
     * 当某些流是"稀疏的"(即连续数据包之间有很大间隔)时,
     * 这可能导致过度缓冲。
     *
     * 此字段指定复用队列中第一个和最后一个数据包的时间戳之间的最大差异,
     * 超过此差异时,libavformat 将输出数据包,而不管是否已为所有流排队数据包。
     *
     * 仅用于复用,在 avformat_write_header() 之前由调用者设置。
     */
    int64_t max_interleave_delta;

    /**
     * 允许非标准和实验性扩展
     * @see AVCodecContext.strict_std_compliance
     */
    int strict_std_compliance;

    /**
     * 指示文件上发生事件的标志,AVFMT_EVENT_FLAG_* 的组合。
     *
     * - 解复用: 可能由解复用器在 avformat_open_input()、
     *   avformat_find_stream_info() 和 av_read_frame() 中设置。
     *   一旦事件被处理,用户必须清除标志。
     * - 复用: 可能在 avformat_write_header() 之后由用户设置,
     *   以指示用户触发的事件。复用器将在 av_[interleaved]_write_frame()
     *   中清除它已处理的事件的标志。
     */
    int event_flags;
/**
 * - 解复用: 解复用器从文件中读取新的元数据并相应更新了
 *   AVFormatContext.metadata
 * - 复用: 用户更新了 AVFormatContext.metadata 并希望复用器
 *   将其写入文件
 */
#define AVFMT_EVENT_FLAG_METADATA_UPDATED 0x0001

    /**
     * 等待第一个时间戳时要读取的最大数据包数。
     * 仅用于解码。
     */
    int max_ts_probe;

    /**
     * 在复用期间避免负时间戳。
     * AVFMT_AVOID_NEG_TS_* 常量的任何值。
     * 注意,使用 av_interleaved_write_frame() 时效果更好。
     * - 复用: 由用户设置
     * - 解复用: 未使用
     */
    int avoid_negative_ts;
#define AVFMT_AVOID_NEG_TS_AUTO             -1 ///< 当目标格式需要时启用
#define AVFMT_AVOID_NEG_TS_DISABLED          0 ///< 即使时间戳为负也不移动时间戳
#define AVFMT_AVOID_NEG_TS_MAKE_NON_NEGATIVE 1 ///< 移动时间戳使其为非负
#define AVFMT_AVOID_NEG_TS_MAKE_ZERO         2 ///< 移动时间戳使其从 0 开始

    /**
     * 传输流 ID。
     * 这将移至解复用器私有选项。因此没有 API/ABI 兼容性
     */
    int ts_id;

    /**
     * 音频预加载时间(微秒)。
     * 注意,并非所有格式都支持此功能,在不支持时使用可能会发生不可预测的事情。
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    int audio_preload;

    /**
     * 最大块持续时间(微秒)。
     * 注意,并非所有格式都支持此功能,在不支持时使用可能会发生不可预测的事情。
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    int max_chunk_duration;

    /**
     * 最大块大小(字节)
     * 注意,并非所有格式都支持此功能,在不支持时使用可能会发生不可预测的事情。
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    int max_chunk_size;

    /**
     * 强制使用墙上时钟时间戳作为数据包的 pts/dts
     * 在存在 B 帧时这会产生未定义的结果。
     * - 编码: 未使用
     * - 解码: 由用户设置
     */
    int use_wallclock_as_timestamps;

    /**
     * avio 标志,用于强制 AVIO_FLAG_DIRECT。
     * - 编码: 未使用
     * - 解码: 由用户设置
     */
    int avio_flags;

    /**
     * 持续时间字段可以通过各种方式估算,此字段可用于
     * 了解持续时间是如何估算的。
     * - 编码: 未使用
     * - 解码: 由用户读取
     */
    enum AVDurationEstimationMethod duration_estimation_method;

    /**
     * 打开流时跳过初始字节
     * - 编码: 未使用
     * - 解码: 由用户设置
     */
    int64_t skip_initial_bytes;

    /**
     * 纠正单个时间戳溢出
     * - 编码: 未使用
     * - 解码: 由用户设置
     */
    unsigned int correct_ts_overflow;

    /**
     * 强制定位到任何帧(也包括非关键帧)。
     * - 编码: 未使用
     * - 解码: 由用户设置
     */
    int seek2any;

    /**
     * 每个数据包后刷新 I/O 上下文。
     * - 编码: 由用户设置
     * - 解码: 未使用
     */
    int flush_packets;

    /**
     * 格式探测分数。
     * 最大分数是 AVPROBE_SCORE_MAX,当解复用器探测格式时设置。
     * - 编码: 未使用
     * - 解码: 由 avformat 设置,由用户读取
     */
    int probe_score;

    /**
     * 为识别 \ref AVInputFormat "输入格式" 而从输入读取的最大字节数。
     * 仅当格式未由调用者显式设置时使用。
     *
     * 仅用于解复用,在 avformat_open_input() 之前由调用者设置。
     *
     * @sa probesize
     */
    int format_probesize;

    /**
     * ',' 分隔的允许解码器列表。
     * 如果为 NULL 则允许所有解码器
     * - 编码: 未使用
     * - 解码: 由用户设置
     */
    char *codec_whitelist;

    /**
     * ',' 分隔的允许解复用器列表。
     * 如果为 NULL 则允许所有解复用器
     * - 编码: 未使用
     * - 解码: 由用户设置
     */
    char *format_whitelist;

    /**
     * IO 重定位标志。
     * 当底层 IO 上下文读取指针重定位时,avformat 会设置此标志,
     * 例如在进行基于字节的定位时。
     * 解复用器可以使用该标志检测此类更改。
     */
    int io_repositioned;

    /**
     * 强制视频编解码器。
     * 这允许强制使用特定解码器,即使有多个具有相同 codec_id 的解码器。
     * 解复用: 由用户设置
     */
    const struct AVCodec *video_codec;

    /**
     * 强制音频编解码器。
     * 这允许强制使用特定解码器,即使有多个具有相同 codec_id 的解码器。
     * 解复用: 由用户设置
     */
    const struct AVCodec *audio_codec;

    /**
     * 强制字幕编解码器。
     * 这允许强制使用特定解码器,即使有多个具有相同 codec_id 的解码器。
     * 解复用: 由用户设置
     */
    const struct AVCodec *subtitle_codec;

    /**
     * 强制数据编解码器。
     * 这允许强制使用特定解码器,即使有多个具有相同 codec_id 的解码器。
     * 解复用: 由用户设置
     */
    const struct AVCodec *data_codec;

    /**
     * 要在元数据头部写入的填充字节数。
     * 解复用: 未使用。
     * 复用: 通过 av_format_set_metadata_header_padding 由用户设置。
     */
    int metadata_header_padding;

    /**
     * 用户数据。
     * 这是用户私有数据的存放位置。
     */
    void *opaque;

    /**
     * 设备用于与应用程序通信的回调。
     */
    av_format_control_message control_message_cb;

    /**
     * 输出时间戳偏移量,以微秒为单位。
     * 复用: 由用户设置
     */
    int64_t output_ts_offset;

    /**
     * 转储格式分隔符。
     * 可以是 ", " 或 "\n      " 或其他任何内容
     * - 复用: 由用户设置。
     * - 解复用: 由用户设置。
     */
    uint8_t *dump_separator;

    /**
     * 强制数据编解码器 ID。
     * 解复用: 由用户设置。
     */
    enum AVCodecID data_codec_id;

    /**
     * ',' 分隔的允许协议列表。
     * - 编码: 未使用
     * - 解码: 由用户设置
     */
    char *protocol_whitelist;

    /**
     * 用于打开新 IO 流的回调。
     *
     * 每当复用器或解复用器需要打开 IO 流时(通常是解复用器从
     * avformat_open_input() 调用,但对某些格式也可能在其他时间发生),
     * 它将调用此回调来获取 IO 上下文。
     *
     * @param s 格式上下文
     * @param pb 成功时,新打开的 IO 上下文应返回到此处
     * @param url 要打开的 url
     * @param flags AVIO_FLAG_* 的组合
     * @param options 附加选项的字典,语义与 avio_open2() 中相同
     * @return 成功返回 0,失败返回负的 AVERROR 代码
     *
     * @note 某些复用器和解复用器会进行嵌套,即它们会打开一个或多个
     * 额外的内部格式上下文。因此传递给此回调的 AVFormatContext 指针
     * 可能与面向调用者的指针不同。但是,它将具有相同的 'opaque' 字段。
     */
    int (*io_open)(struct AVFormatContext *s, AVIOContext **pb, const char *url,
                   int flags, AVDictionary **options);

#if FF_API_AVFORMAT_IO_CLOSE
    /**
     * 用于关闭通过 AVFormatContext.io_open() 打开的流的回调。
     *
     * @deprecated 使用 io_close2
     */
    attribute_deprecated
    void (*io_close)(struct AVFormatContext *s, AVIOContext *pb);
#endif

    /**
     * ',' 分隔的禁用协议列表。
     * - 编码: 未使用
     * - 解码: 由用户设置
     */
    char *protocol_blacklist;

    /**
     * 最大流数量。
     * - 编码: 未使用
     * - 解码: 由用户设置
     */
    int max_streams;

    /**
     * 在 estimate_timings_from_pts 中跳过持续时间计算。
     * - 编码: 未使用
     * - 解码: 由用户设置
     */
    int skip_estimate_duration_from_pts;

    /**
     * 可以探测的最大数据包数量
     * - 编码: 未使用
     * - 解码: 由用户设置
     */
    int max_probe_packets;

    /**
     * 用于关闭通过 AVFormatContext.io_open() 打开的流的回调。
     *
     * 相比 io_close,使用这个更好,因为它可以返回错误。
     * 因此,如果 io_close 为 NULL 或默认值,通用的 libavformat 代码
     * 会使用此回调代替 io_close。
     *
     * @param s 格式上下文
     * @param pb 要关闭和释放的 IO 上下文
     * @return 成功返回 0,失败返回负的 AVERROR 代码
     */
    int (*io_close2)(struct AVFormatContext *s, AVIOContext *pb);
} AVFormatContext;

/**
 * 此函数将导致全局边数据被注入到每个流的下一个数据包中,
 * 以及任何后续查找之后。
 *
 * @note 全局边数据始终在每个 AVStream 的
 *       @ref AVCodecParameters.coded_side_data "codecpar 边数据" 数组中可用,
 *       如果使用该流的 codecpar 初始化,则在
 *       @ref AVCodecContext.coded_side_data "解码器的边数据" 数组中可用。
 * @see av_packet_side_data_get()
 */
void av_format_inject_global_side_data(AVFormatContext *s);

/**
 * 返回用于设置 ctx->duration 的方法。
 *
 * @return AVFMT_DURATION_FROM_PTS、AVFMT_DURATION_FROM_STREAM 或 AVFMT_DURATION_FROM_BITRATE。
 */
enum AVDurationEstimationMethod av_fmt_ctx_get_duration_estimation_method(const AVFormatContext* ctx);

/**
 * @defgroup lavf_core 核心函数
 * @ingroup libavf
 *
 * 用于查询 libavformat 功能、分配核心结构等的函数。
 * @{
 */

/**
 * 返回 LIBAVFORMAT_VERSION_INT 常量。
 */
unsigned avformat_version(void);

/**
 * 返回 libavformat 的编译时配置。
 */
const char *avformat_configuration(void);

/**
 * 返回 libavformat 的许可证。
 */
const char *avformat_license(void);

/**
 * 执行网络库的全局初始化。这是可选的,
 * 并且不再推荐使用。
 *
 * 此函数的存在仅是为了解决与较旧的 GnuTLS 或 OpenSSL 库
 * 相关的线程安全问题。如果 libavformat 链接到这些库的较新版本,
 * 或者您不使用它们,则无需调用此函数。否则,您需要在启动
 * 使用它们的其他线程之前调用此函数。
 *
 * 一旦移除对较旧的 GnuTLS 和 OpenSSL 库的支持,
 * 此函数将被弃用,因为此函数将不再有任何用途。
 */
int avformat_network_init(void);

/**
 * 撤消 avformat_network_init 所做的初始化。
 * 对于每次调用 avformat_network_init,只调用一次。
 */
int avformat_network_deinit(void);

/**
 * 遍历所有已注册的复用器。
 *
 * @param opaque 一个指针,libavformat 将在其中存储迭代状态。
 *              必须指向 NULL 以开始迭代。
 *
 * @return 下一个已注册的复用器,或在迭代完成时返回 NULL
 */
const AVOutputFormat *av_muxer_iterate(void **opaque);

/**
 * 遍历所有已注册的解复用器。
 *
 * @param opaque 一个指针,libavformat 将在其中存储迭代状态。
 *              必须指向 NULL 以开始迭代。
 *
 * @return 下一个已注册的解复用器,或在迭代完成时返回 NULL
 */
const AVInputFormat *av_demuxer_iterate(void **opaque);

/**
 * 分配一个 AVFormatContext。
 * avformat_free_context() 可用于释放该上下文以及框架在其中分配的所有内容。
 */
AVFormatContext *avformat_alloc_context(void);

/**
 * 释放一个 AVFormatContext 及其所有流。
 * @param s 要释放的上下文
 */
void avformat_free_context(AVFormatContext *s);

/**
 * 获取 AVFormatContext 的 AVClass。它可以与 AV_OPT_SEARCH_FAKE_OBJ 结合使用
 * 来检查选项。
 *
 * @see av_opt_find()。
 */
const AVClass *avformat_get_class(void);

/**
 * 获取 AVStream 的 AVClass。它可以与 AV_OPT_SEARCH_FAKE_OBJ 结合使用
 * 来检查选项。
 *
 * @see av_opt_find()。
 */
const AVClass *av_stream_get_class(void);

/**
 * 向媒体文件添加新的流。
 *
 * 在解复用时,由解复用器在 read_header() 中调用。如果在 s.ctx_flags 中
 * 设置了 AVFMTCTX_NOHEADER 标志,则也可能在 read_packet() 中调用。
 *
 * 在复用时,应该由用户在 avformat_write_header() 之前调用。
 *
 * 用户需要调用 avformat_free_context() 来清理由 avformat_new_stream() 
 * 进行的分配。
 *
 * @param s 媒体文件句柄
 * @param c 未使用,不做任何事
 *
 * @return 新创建的流,如果出错则返回 NULL。
 */
AVStream *avformat_new_stream(AVFormatContext *s, const struct AVCodec *c);

#if FF_API_AVSTREAM_SIDE_DATA
/**
 * 将现有数组包装为流的边数据。
 *
 * @param st   流
 * @param type 边信息类型
 * @param data 边数据数组。它必须使用 av_malloc() 系列函数分配。
 *             数据的所有权转移给 st。
 * @param size 边信息大小
 *
 * @return 成功时返回零,失败时返回负的 AVERROR 代码。失败时,
 *         流保持不变,数据仍归调用者所有。
 * @deprecated 使用 av_packet_side_data_add() 和流的
 *             @ref AVCodecParameters.coded_side_data "codecpar side data"
 */
attribute_deprecated
int av_stream_add_side_data(AVStream *st, enum AVPacketSideDataType type,
                            uint8_t *data, size_t size);

/**
 * 从流中分配新的信息。
 *
 * @param stream 流
 * @param type   所需的边信息类型
 * @param size   边信息大小
 *
 * @return 指向新分配数据的指针,否则返回 NULL
 * @deprecated 使用 av_packet_side_data_new() 和流的
 *             @ref AVCodecParameters.coded_side_data "codecpar side data"
 */
attribute_deprecated
uint8_t *av_stream_new_side_data(AVStream *stream,
                                 enum AVPacketSideDataType type, size_t size);
/**
 * 从流中获取边信息。
 *
 * @param stream 流
 * @param type   所需的边信息类型
 * @param size   如果提供,*size 将被设置为边数据的大小,
 *               如果所需的边数据不存在则设为零。
 *
 * @return 如果存在则返回指向数据的指针,否则返回 NULL
 * @deprecated 使用 av_packet_side_data_get() 和流的
 *             @ref AVCodecParameters.coded_side_data "codecpar side data"
 */
attribute_deprecated
uint8_t *av_stream_get_side_data(const AVStream *stream,
                                 enum AVPacketSideDataType type, size_t *size);
#endif

AVProgram *av_new_program(AVFormatContext *s, int id);

/**
 * @}
 */


/**
 * 为输出格式分配一个 AVFormatContext。
 * avformat_free_context() 可用于释放该上下文以及框架在其中分配的所有内容。
 *
 * @param ctx           指针被设置为创建的格式上下文,
 *                      或在失败的情况下设置为 NULL
 * @param oformat       用于分配上下文的格式,如果为 NULL,
 *                      则使用 format_name 和 filename
 * @param format_name   用于分配上下文的输出格式名称,
 *                      如果为 NULL 则使用 filename
 * @param filename      用于分配上下文的文件名,可以为 NULL
 *
 * @return  成功时返回 >= 0,失败时返回负的 AVERROR 代码
 */
int avformat_alloc_output_context2(AVFormatContext **ctx, const AVOutputFormat *oformat,
                                   const char *format_name, const char *filename);

/**
 * @addtogroup lavf_decoding
 * @{
 */

/**
 * 根据输入格式的短名称查找 AVInputFormat。
 */
const AVInputFormat *av_find_input_format(const char *short_name);

/**
 * 猜测文件格式。
 *
 * @param pd        要探测的数据
 * @param is_opened 文件是否已经打开;决定是探测带有还是不带有
 *                  AVFMT_NOFILE 的解复用器。
 */
const AVInputFormat *av_probe_input_format(const AVProbeData *pd, int is_opened);

/**
 * 猜测文件格式。
 *
 * @param pd        要探测的数据
 * @param is_opened 文件是否已经打开;决定是探测带有还是不带有
 *                  AVFMT_NOFILE 的解复用器。
 * @param score_max 需要大于此值的探测分数才能接受检测,
 *                  变量随后被设置为实际的检测分数。
 *                  如果分数 <= AVPROBE_SCORE_MAX / 4,建议
 *                  使用更大的探测缓冲区重试。
 */
const AVInputFormat *av_probe_input_format2(const AVProbeData *pd,
                                            int is_opened, int *score_max);

/**
 * 猜测文件格式。
 *
 * @param is_opened 文件是否已经打开;决定是探测带有还是不带有
 *                  AVFMT_NOFILE 的解复用器。
 * @param score_ret 最佳检测的分数。
 */
const AVInputFormat *av_probe_input_format3(const AVProbeData *pd,
                                            int is_opened, int *score_ret);

/**
 * 探测字节流以确定输入格式。每当探测返回的分数太低时,
 * 探测缓冲区大小会增加并重新尝试。当达到最大探测大小时,
 * 返回得分最高的输入格式。
 *
 * @param pb             要探测的字节流
 * @param fmt           输入格式放在这里
 * @param url           流的 url
 * @param logctx        日志上下文
 * @param offset        从字节流中探测的偏移量
 * @param max_probe_size 最大探测缓冲区大小(零表示默认值)
 *
 * @return 成功时返回分数,失败时返回对应于最大分数的负 AVERROR 代码
 *         最大分数是 AVPROBE_SCORE_MAX
 */
int av_probe_input_buffer2(AVIOContext *pb, const AVInputFormat **fmt,
                           const char *url, void *logctx,
                           unsigned int offset, unsigned int max_probe_size);

/**
 * 类似于 av_probe_input_buffer2() 但成功时返回 0
 */
int av_probe_input_buffer(AVIOContext *pb, const AVInputFormat **fmt,
                          const char *url, void *logctx,
                          unsigned int offset, unsigned int max_probe_size);

/**
 * 打开输入流并读取头部。不打开编解码器。
 * 必须使用 avformat_close_input() 关闭流。
 *
 * @param ps       指向用户提供的 AVFormatContext 的指针(由 avformat_alloc_context 分配)。
 *                 可以是指向 NULL 的指针,在这种情况下,
 *                 由该函数分配 AVFormatContext 并写入 ps。
 *                 注意,用户提供的 AVFormatContext 在失败时将被释放。
 * @param url      要打开的流的 URL。
 * @param fmt      如果非 NULL,此参数强制使用特定的输入格式。
 *                 否则格式将被自动检测。
 * @param options  填充有 AVFormatContext 和解复用器私有选项的字典。
 *                 返回时此参数将被销毁并替换为包含未找到选项的字典。
 *                 可以为 NULL。
 *
 * @return 成功时返回 0,失败时返回负的 AVERROR。
 *
 * @note 如果要使用自定义 IO,请预先分配格式上下文并设置其 pb 字段。
 */
int avformat_open_input(AVFormatContext **ps, const char *url,
                        const AVInputFormat *fmt, AVDictionary **options);

/**
 * 读取媒体文件的数据包以获取流信息。这对于没有头部的文件格式
 * (如 MPEG)很有用。此函数还会在 MPEG-2 重复帧模式下计算实际帧率。
 * 此函数不会改变逻辑文件位置;
 * 检查的数据包可能会被缓存以供后续处理。
 *
 * @param ic 媒体文件句柄
 * @param options  如果非 NULL,一个 ic.nb_streams 长的指向字典的指针数组,
 *                 其中第 i 个成员包含对应第 i 个流的编解码器的选项。
 *                 返回时每个字典将填充未找到的选项。
 * @return >=0 表示成功,AVERROR_xxx 表示错误
 *
 * @note 此函数不保证打开所有编解码器,所以
 *       返回时选项非空是完全正常的行为。
 *
 * @todo 让用户以某种方式决定需要什么信息,这样
 *       我们就不会浪费时间获取用户不需要的内容。
 */
int avformat_find_stream_info(AVFormatContext *ic, AVDictionary **options);

/**
 * 查找属于给定流的节目。
 *
 * @param ic    媒体文件句柄
 * @param last  最后找到的节目,搜索将从此节目之后开始,
 *              如果为 NULL 则从头开始
 * @param s     流索引
 *
 * @return 属于 s 的下一个节目,如果未找到节目或最后一个节目
 *         不在 ic 的节目中则返回 NULL。
 */
AVProgram *av_find_program_from_stream(AVFormatContext *ic, AVProgram *last, int s);

void av_program_add_stream_index(AVFormatContext *ac, int progid, unsigned int idx);

/**
 * 在文件中查找"最佳"流。
 * 根据各种启发式方法确定最佳流,作为最可能是用户期望的流。
 * 如果 decoder 参数非 NULL,av_find_best_stream 将为流的编解码器
 * 找到默认解码器;找不到解码器的流将被忽略。
 *
 * @param ic                媒体文件句柄
 * @param type              流类型:视频、音频、字幕等
 * @param wanted_stream_nb  用户请求的流编号,
 *                          或 -1 表示自动选择
 * @param related_stream    尝试查找与此流相关的流(例如在同一个
 *                          节目中),或 -1 表示无
 * @param decoder_ret       如果非 NULL,返回所选流的解码器
 * @param flags            标志;当前未定义任何标志
 *
 * @return  成功时返回非负流编号,
 *          如果找不到请求类型的流则返回 AVERROR_STREAM_NOT_FOUND,
 *          如果找到流但没有解码器则返回 AVERROR_DECODER_NOT_FOUND
 *
 * @note  如果 av_find_best_stream 成功返回且 decoder_ret 不为
 *        NULL,则保证 *decoder_ret 被设置为有效的 AVCodec。
 */
int av_find_best_stream(AVFormatContext *ic,
                        enum AVMediaType type,
                        int wanted_stream_nb,
                        int related_stream,
                        const struct AVCodec **decoder_ret,
                        int flags);

/**
 * 返回流的下一帧。
 * 此函数返回文件中存储的内容,不验证其中的内容是否为解码器的有效帧。
 * 它会将文件中存储的内容分割成帧,每次调用返回一帧。它不会在有效帧之间
 * 忽略无效数据,以便为解码器提供最大的解码信息。
 *
 * 成功时,返回的数据包是引用计数的(设置了 pkt->buf)且永久有效。
 * 当不再需要时,必须使用 av_packet_unref() 释放数据包。
 * 对于视频,数据包包含恰好一帧。
 * 对于音频,如果每帧具有已知的固定大小(例如 PCM 或 ADPCM 数据),
 * 则包含整数个帧。如果音频帧具有可变大小(例如 MPEG 音频),
 * 则包含一帧。
 *
 * pkt->pts、pkt->dts 和 pkt->duration 始终在 AVStream.time_base 单位中
 * 设置为正确的值(如果格式无法提供则进行猜测)。如果视频格式有 B 帧,
 * pkt->pts 可能是 AV_NOPTS_VALUE,所以如果不解压载荷,最好依赖 pkt->dts。
 *
 * @return 如果成功则返回 0,如果出错或到达文件末尾则返回 < 0。出错时,pkt 将为空
 *         (就像它来自 av_packet_alloc())。
 *
 * @note pkt 将被初始化,所以它可能未初始化,但不能包含需要释放的数据。
 */
int av_read_frame(AVFormatContext *s, AVPacket *pkt);

/**
 * 查找时间戳处的关键帧。
 * 'stream_index' 中的 'timestamp'。
 *
 * @param s            媒体文件句柄
 * @param stream_index 如果 stream_index 为 (-1),则选择默认流,
 *                     并自动将时间戳从 AV_TIME_BASE 单位转换为
 *                     流特定的 time_base。
 * @param timestamp    以 AVStream.time_base 为单位的时间戳,如果未指定流,
 *                     则以 AV_TIME_BASE 为单位。
 * @param flags        选择方向和查找模式的标志
 *
 * @return 成功时返回 >= 0
 */
int av_seek_frame(AVFormatContext *s, int stream_index, int64_t timestamp,
                  int flags);

/**
 * 查找时间戳 ts。
 * 查找将使所有活动流都可以成功呈现的点最接近 ts 且在 min/max_ts 范围内。
 * 活动流是所有 AVStream.discard < AVDISCARD_ALL 的流。
 *
 * 如果 flags 包含 AVSEEK_FLAG_BYTE,则所有时间戳都以字节为单位,
 * 是文件位置(并非所有解复用器都支持)。
 * 如果 flags 包含 AVSEEK_FLAG_FRAME,则所有时间戳都是 stream_index 
 * 流中的帧数(并非所有解复用器都支持)。
 * 否则所有时间戳都以 stream_index 选择的流的单位为单位,
 * 如果 stream_index 为 -1,则以 AV_TIME_BASE 为单位。
 * 如果 flags 包含 AVSEEK_FLAG_ANY,则非关键帧被视为
 * 关键帧(并非所有解复用器都支持)。
 * 如果 flags 包含 AVSEEK_FLAG_BACKWARD,则忽略它。
 *
 * @param s            媒体文件句柄
 * @param stream_index 用作时间基准参考的流的索引
 * @param min_ts       最小可接受时间戳
 * @param ts           目标时间戳
 * @param max_ts       最大可接受时间戳
 * @param flags        标志
 * @return 成功时返回 >=0,否则返回错误代码
 *
 * @note 这是新查找 API 的一部分,该 API 仍在构建中。
 */
int avformat_seek_file(AVFormatContext *s, int stream_index, int64_t min_ts, int64_t ts, int64_t max_ts, int flags);

/**
 * 丢弃所有内部缓冲的数据。在处理字节流中的不连续性时这很有用。
 * 通常只适用于可以重新同步的格式。这包括无头格式如 MPEG-TS/TS,
 * 但也应该适用于 NUT、Ogg,以及在有限程度上适用于 AVI 等。
 *
 * 调用此函数时,流集、检测到的持续时间、流参数和编解码器不会改变。
 * 如果要完全重置,最好打开新的 AVFormatContext。
 *
 * 这不会刷新 AVIOContext (s->pb)。如有必要,在调用此函数之前
 * 调用 avio_flush(s->pb)。
 *
 * @param s 媒体文件句柄
 * @return 成功时返回 >=0,否则返回错误代码
 */
int avformat_flush(AVFormatContext *s);

/**
 * 从当前位置开始播放基于网络的流(例如 RTSP 流)。
 */
int av_read_play(AVFormatContext *s);

/**
 * 暂停基于网络的流(例如 RTSP 流)。
 *
 * 使用 av_read_play() 恢复。
 */
int av_read_pause(AVFormatContext *s);

/**
 * 关闭已打开的输入 AVFormatContext。释放它及其所有内容
 * 并将 *s 设置为 NULL。
 */
void avformat_close_input(AVFormatContext **s);
/**
 * @}
 */

#define AVSEEK_FLAG_BACKWARD 1 ///< 向后查找
#define AVSEEK_FLAG_BYTE     2 ///< 基于字节位置的查找
#define AVSEEK_FLAG_ANY      4 ///< 查找任何帧,即使是非关键帧
#define AVSEEK_FLAG_FRAME    8 ///< 基于帧号的查找

/**
 * @addtogroup lavf_encoding
 * @{
 */

#define AVSTREAM_INIT_IN_WRITE_HEADER 0 ///< 流参数在 avformat_write_header 中初始化
#define AVSTREAM_INIT_IN_INIT_OUTPUT  1 ///< 流参数在 avformat_init_output 中初始化

/**
 * 分配流私有数据并将流头写入输出媒体文件。
 *
 * @param s        媒体文件句柄,必须通过 avformat_alloc_context() 分配。
 *                 其 \ref AVFormatContext.oformat "oformat" 字段必须设置为
 *                 所需的输出格式;
 *                 其 \ref AVFormatContext.pb "pb" 字段必须设置为
 *                 已打开的 ::AVIOContext。
 * @param options  一个包含 AVFormatContext 和复用器私有选项的 ::AVDictionary。
 *                 返回时此参数将被销毁并替换为包含未找到选项的字典。可以为 NULL。
 *
 * @retval AVSTREAM_INIT_IN_WRITE_HEADER 成功,如果编解码器尚未在 avformat_init_output() 中完全初始化。
 * @retval AVSTREAM_INIT_IN_INIT_OUTPUT  成功,如果编解码器已在 avformat_init_output() 中完全初始化。
 * @retval AVERROR                       失败时返回负的 AVERROR。
 *
 * @see av_opt_find, av_dict_set, avio_open, av_oformat_next, avformat_init_output.
 */
av_warn_unused_result
int avformat_write_header(AVFormatContext *s, AVDictionary **options);

/**
 * 分配流私有数据并初始化编解码器,但不写入头部。
 * 可以选择在实际写入头部之前使用此函数来初始化流参数。
 * 如果使用此函数,请不要将相同的选项传递给 avformat_write_header()。
 *
 * @param s        媒体文件句柄,必须通过 avformat_alloc_context() 分配。
 *                 其 \ref AVFormatContext.oformat "oformat" 字段必须设置为
 *                 所需的输出格式;
 *                 其 \ref AVFormatContext.pb "pb" 字段必须设置为
 *                 已打开的 ::AVIOContext。
 * @param options  一个包含 AVFormatContext 和复用器私有选项的 ::AVDictionary。
 *                 返回时此参数将被销毁并替换为包含未找到选项的字典。可以为 NULL。
 *
 * @retval AVSTREAM_INIT_IN_WRITE_HEADER 成功,如果编解码器需要 avformat_write_header 来完全初始化。
 * @retval AVSTREAM_INIT_IN_INIT_OUTPUT  成功,如果编解码器已完全初始化。
 * @retval AVERROR                       失败时返回负的 AVERROR。
 *
 * @see av_opt_find, av_dict_set, avio_open, av_oformat_next, avformat_write_header.
 */
av_warn_unused_result
int avformat_init_output(AVFormatContext *s, AVDictionary **options);

/**
 * 将数据包写入输出媒体文件。
 *
 * 此函数直接将数据包传递给复用器,不进行任何缓冲或重排序。调用者负责在格式需要时
 * 正确交错数据包。希望由 libavformat 处理交错的调用者应该调用 av_interleaved_write_frame() 
 * 而不是此函数。
 *
 * @param s 媒体文件句柄
 * @param pkt 包含要写入数据的数据包。注意,与 av_interleaved_write_frame() 不同,
 *            此函数不会获取传递给它的数据包的所有权(尽管某些复用器可能会对输入数据包
 *            进行内部引用)。
 *            <br>
 *            此参数可以为 NULL(在任何时候,不仅仅是在结尾),以立即刷新复用器内部缓冲的数据,
 *            对于在内部缓冲数据直到写入输出的复用器。
 *            <br>
 *            数据包的 @ref AVPacket.stream_index "stream_index" 字段必须设置为
 *            @ref AVFormatContext.streams "s->streams" 中相应流的索引。
 *            <br>
 *            时间戳(@ref AVPacket.pts "pts", @ref AVPacket.dts "dts")必须在流的
 *            时基中设置为正确的值(除非输出格式标记为 AVFMT_NOTIMESTAMPS,则可以设置为
 *            AV_NOPTS_VALUE)。传递给此函数的后续数据包的 dts 在其各自的时基中比较时
 *            必须严格递增(除非输出格式标记为 AVFMT_TS_NONSTRICT,则它们只需要是非递减的)。
 *            @ref AVPacket.duration "duration")如果已知也应该设置。
 * @return < 0 表示错误, = 0 表示成功, 1 表示已刷新且没有更多数据要刷新
 *
 * @see av_interleaved_write_frame()
 */
int av_write_frame(AVFormatContext *s, AVPacket *pkt);

/**
 * 将数据包写入输出媒体文件,确保正确的交错。
 *
 * 此函数将根据需要在内部缓冲数据包,以确保输出文件中的数据包正确交错,通常按 dts 
 * 递增排序。自行处理交错的调用者应该调用 av_write_frame() 而不是此函数。
 *
 * 使用此函数而不是 av_write_frame() 可以让复用器提前了解未来的数据包,改善例如
 * mp4 复用器在分片模式下处理 VFR 内容的行为。
 *
 * @param s 媒体文件句柄
 * @param pkt 包含要写入数据的数据包。
 *            <br>
 *            如果数据包是引用计数的,此函数将获取此引用的所有权,并在认为合适时取消引用。
 *            如果数据包不是引用计数的,libavformat 将创建一个副本。
 *            返回的数据包将为空白(如同从 av_packet_alloc() 返回),即使出错。
 *            <br>
 *            此参数可以为 NULL(在任何时候,不仅仅是在结尾),以刷新交错队列。
 *            <br>
 *            数据包的 @ref AVPacket.stream_index "stream_index" 字段必须设置为
 *            @ref AVFormatContext.streams "s->streams" 中相应流的索引。
 *            <br>
 *            时间戳(@ref AVPacket.pts "pts", @ref AVPacket.dts "dts")必须在流的
 *            时基中设置为正确的值(除非输出格式标记为 AVFMT_NOTIMESTAMPS,则可以设置为
 *            AV_NOPTS_VALUE)。一个流中后续数据包的 dts 必须严格递增(除非输出格式标记为
 *            AVFMT_TS_NONSTRICT,则它们只需要是非递减的)。
 *            @ref AVPacket.duration "duration" 如果已知也应该设置。
 *
 * @return 成功返回 0,失败返回负的 AVERROR。
 *
 * @see av_write_frame(), AVFormatContext.max_interleave_delta
 */
int av_interleaved_write_frame(AVFormatContext *s, AVPacket *pkt);

/**
 * 将未编码的帧写入输出媒体文件。
 *
 * 帧必须根据容器规范正确交错;如果不是,必须使用 av_interleaved_write_uncoded_frame()。
 *
 * 详见 av_interleaved_write_uncoded_frame()。
 */
int av_write_uncoded_frame(AVFormatContext *s, int stream_index,
                           struct AVFrame *frame);

/**
 * 将未编码的帧写入输出媒体文件。
 *
 * 如果复用器支持,此函数可以直接写入 AVFrame 结构,而无需将其编码为数据包。
 * 它主要用于使用原始视频或 PCM 数据且不会将其序列化为字节流的设备和类似的特殊复用器。
 *
 * 要测试是否可以与给定的复用器和流一起使用,请使用 av_write_uncoded_frame_query()。
 *
 * 调用者放弃帧的所有权,之后不得访问它。
 *
 * @return  >=0 表示成功,负值表示错误代码
 */
int av_interleaved_write_uncoded_frame(AVFormatContext *s, int stream_index,
                                       struct AVFrame *frame);

/**
 * 测试复用器是否支持未编码帧。
 *
 * @return  >=0 如果可以将未编码帧写入该复用器和流,
 *          <0 如果不能
 */
int av_write_uncoded_frame_query(AVFormatContext *s, int stream_index);

/**
 * 将流尾部写入输出媒体文件并释放文件私有数据。
 *
 * 只能在成功调用 avformat_write_header 后调用。
 *
 * @param s 媒体文件句柄
 * @return 成功返回 0,失败返回 AVERROR_xxx
 */
int av_write_trailer(AVFormatContext *s);

/**
 * 在已注册的输出格式列表中返回最匹配提供参数的输出格式,
 * 如果没有匹配则返回 NULL。
 *
 * @param short_name 如果非 NULL,检查 short_name 是否与已注册格式的名称匹配
 * @param filename   如果非 NULL,检查 filename 是否以已注册格式的扩展名结尾
 * @param mime_type  如果非 NULL,检查 mime_type 是否与已注册格式的 MIME 类型匹配
 */
const AVOutputFormat *av_guess_format(const char *short_name,
                                      const char *filename,
                                      const char *mime_type);

/**
 * 根据复用器和文件名猜测编解码器 ID。
 */
enum AVCodecID av_guess_codec(const AVOutputFormat *fmt, const char *short_name,
                              const char *filename, const char *mime_type,
                              enum AVMediaType type);

/**
 * 获取当前输出数据的时间信息。
 * "当前输出"的确切含义取决于格式。
 * 它主要与具有内部缓冲区和/或实时工作的设备相关。
 * @param s          媒体文件句柄
 * @param stream     媒体文件中的流
 * @param[out] dts   流中最后输出数据包的 DTS,以流时基为单位
 * @param[out] wall  输出该数据包时的绝对时间,以微秒为单位
 * @retval  0               成功
 * @retval  AVERROR(ENOSYS) 格式不支持
 *
 * @note 某些格式或设备可能不允许原子地测量 dts 和 wall。
 */
int av_get_output_timestamp(struct AVFormatContext *s, int stream,
                            int64_t *dts, int64_t *wall);


/**
 * @}
 */


/**
 * @defgroup lavf_misc 实用函数
 * @ingroup libavf
 * @{
 *
 * 与复用和解复用相关的杂项实用函数(或两者都不相关)。
 */

/**
 * 将缓冲区的十六进制转储发送到指定的文件流。
 *
 * @param f 应发送转储的文件流指针。
 * @param buf 缓冲区
 * @param size 缓冲区大小
 *
 * @see av_hex_dump_log, av_pkt_dump2, av_pkt_dump_log2
 */
void av_hex_dump(FILE *f, const uint8_t *buf, int size);

/**
 * 将缓冲区的十六进制转储发送到日志。
 *
 * @param avcl 指向任意结构的指针,其第一个字段是指向 AVClass 结构的指针。
 * @param level 消息的重要性级别,较低的值表示较高的重要性。
 * @param buf 缓冲区
 * @param size 缓冲区大小
 *
 * @see av_hex_dump, av_pkt_dump2, av_pkt_dump_log2
 */
void av_hex_dump_log(void *avcl, int level, const uint8_t *buf, int size);

/**
 * 将数据包的详细信息发送到指定的文件流。
 *
 * @param f 应发送转储的文件流指针。
 * @param pkt 要转储的数据包
 * @param dump_payload 如果为 True,则也显示有效载荷。
 * @param st 数据包所属的 AVStream
 */
void av_pkt_dump2(FILE *f, const AVPacket *pkt, int dump_payload, const AVStream *st);


/**
 * 将数据包的详细信息发送到日志。
 *
 * @param avcl 指向任意结构的指针,其第一个字段是指向 AVClass 结构的指针。
 * @param level 消息的重要性级别,较低的值表示较高的重要性。
 * @param pkt 要转储的数据包
 * @param dump_payload 如果为 True,则也显示有效载荷。
 * @param st 数据包所属的 AVStream
 */
void av_pkt_dump_log2(void *avcl, int level, const AVPacket *pkt, int dump_payload,
                      const AVStream *st);

/**
 * 获取给定编解码器标签 tag 的 AVCodecID。
 * 如果未找到编解码器 ID,则返回 AV_CODEC_ID_NONE。
 *
 * @param tags 支持的 codec_id-codec_tag 对列表,存储在
 * AVInputFormat.codec_tag 和 AVOutputFormat.codec_tag 中
 * @param tag  要匹配到编解码器 ID 的编解码器标签
 */
enum AVCodecID av_codec_get_id(const struct AVCodecTag * const *tags, unsigned int tag);

/**
 * 获取给定编解码器 ID id 的编解码器标签。
 * 如果未找到编解码器标签,则返回 0。
 *
 * @param tags 支持的 codec_id-codec_tag 对列表,存储在
 * AVInputFormat.codec_tag 和 AVOutputFormat.codec_tag 中
 * @param id   要匹配到编解码器标签的编解码器 ID
 */
unsigned int av_codec_get_tag(const struct AVCodecTag * const *tags, enum AVCodecID id);

/**
 * 获取给定编解码器 ID 的编解码器标签。
 *
 * @param tags 支持的 codec_id - codec_tag 对列表,存储在
 * AVInputFormat.codec_tag 和 AVOutputFormat.codec_tag 中
 * @param id 应在列表中搜索的编解码器 id
 * @param tag 指向找到的标签的指针
 * @return 如果在 tags 中未找到 id 则返回 0,如果找到则返回 > 0
 */
int av_codec_get_tag2(const struct AVCodecTag * const *tags, enum AVCodecID id,
                      unsigned int *tag);

int av_find_default_stream_index(AVFormatContext *s);

/**
 * 获取特定时间戳的索引。
 *
 * @param st        时间戳所属的流
 * @param timestamp 要检索索引的时间戳
 * @param flags 如果设置 AVSEEK_FLAG_BACKWARD,则返回的索引将对应于
 *                 <= 请求时间戳的时间戳,如果 backward 为 0,则为 >=
 *              如果设置 AVSEEK_FLAG_ANY 则搜索任何帧,否则只搜索关键帧
 * @return 如果找不到这样的时间戳则返回 < 0
 */
int av_index_search_timestamp(AVStream *st, int64_t timestamp, int flags);

/**
 * 获取给定 AVStream 的索引条目计数。
 *
 * @param st 流
 * @return 流中的索引条目数
 */
int avformat_index_get_entries_count(const AVStream *st);

/**
 * 获取对应于给定索引的 AVIndexEntry。
 *
 * @param st          包含请求的 AVIndexEntry 的流。
 * @param idx         所需的索引。
 * @return 如果存在,则返回指向请求的 AVIndexEntry 的指针,否则返回 NULL。
 *
 * @note 此函数返回的指针仅在调用以流或父 AVFormatContext 作为输入参数的任何函数之前
 *       保证有效。
 */
const AVIndexEntry *avformat_index_get_entry(AVStream *st, int idx);

/**
 * 获取对应于给定时间戳的 AVIndexEntry。
 *
 * @param st          包含请求的 AVIndexEntry 的流。
 * @param wanted_timestamp   要检索索引条目的时间戳。
 * @param flags       如果设置 AVSEEK_FLAG_BACKWARD,则返回的条目将对应于
 *                    <= 请求时间戳的时间戳,如果 backward 为 0,则为 >=
 *                    如果设置 AVSEEK_FLAG_ANY 则搜索任何帧,否则只搜索关键帧。
 * @return 如果存在,则返回指向请求的 AVIndexEntry 的指针,否则返回 NULL。
 *
 * @note 此函数返回的指针仅在调用以流或父 AVFormatContext 作为输入参数的任何函数之前
 *       保证有效。
 */
const AVIndexEntry *avformat_index_get_entry_from_timestamp(AVStream *st,
                                                            int64_t wanted_timestamp,
                                                            int flags);
/**
 * 向已排序列表中添加索引条目。如果列表已包含该条目,则更新它。
 *
 * @param timestamp 给定流的时基中的时间戳
 */
int av_add_index_entry(AVStream *st, int64_t pos, int64_t timestamp,
                       int size, int distance, int flags);


/**
 * 将 URL 字符串拆分为组件。
 *
 * 用于存储各个组件的缓冲区指针可以为 null,以忽略该组件。未找到组件的缓冲区
 * 设置为空字符串。如果未找到端口,则将其设置为负值。
 *
 * @param proto 协议缓冲区
 * @param proto_size 协议缓冲区大小
 * @param authorization 授权缓冲区
 * @param authorization_size 授权缓冲区大小
 * @param hostname 主机名缓冲区
 * @param hostname_size 主机名缓冲区大小
 * @param port_ptr 存储端口号的指针
 * @param path 路径缓冲区
 * @param path_size 路径缓冲区大小
 * @param url 要拆分的 URL
 */
void av_url_split(char *proto,         int proto_size,
                  char *authorization, int authorization_size,
                  char *hostname,      int hostname_size,
                  int *port_ptr,
                  char *path,          int path_size,
                  const char *url);


/**
 * 打印有关输入或输出格式的详细信息,如持续时间、比特率、流、容器、程序、
 * 元数据、边数据、编解码器和时基。
 *
 * @param ic        要分析的上下文
 * @param index     要转储信息的流的索引
 * @param url       要打印的 URL,如源文件或目标文件
 * @param is_output 选择指定的上下文是输入(0)还是输出(1)
 */
void av_dump_format(AVFormatContext *ic,
                    int index,
                    const char *url,
                    int is_output);


#define AV_FRAME_FILENAME_FLAGS_MULTIPLE 1 ///< 允许多个 %d

/**
 * 在 'buf' 中返回将 '%d' 替换为数字的路径。
 *
 * 还处理 '%0nd' 格式,其中 'n' 是总位数,以及 '%%'。
 *
 * @param buf 目标缓冲区
 * @param buf_size 目标缓冲区大小
 * @param path 编号序列字符串
 * @param number 帧号
 * @param flags AV_FRAME_FILENAME_FLAGS_*
 * @return 成功返回 0,格式错误返回 -1
 */
int av_get_frame_filename2(char *buf, int buf_size,
                          const char *path, int number, int flags);

int av_get_frame_filename(char *buf, int buf_size,
                          const char *path, int number);

/**
 * 检查文件名是否实际上是一个编号序列生成器。
 *
 * @param filename 可能的编号序列字符串
 * @return 如果是有效的编号序列字符串返回1,否则返回0
 */
int av_filename_number_test(const char *filename);

/**
 * 为RTP会话生成SDP。
 *
 * 注意,这会覆盖复用器上下文中AVStreams的id值,
 * 以获取唯一的动态负载类型。
 *
 * @param ac 描述RTP流的AVFormatContext数组。如果数组
 *          仅由一个上下文组成,则该上下文可以包含
 *          多个AVStreams(每个RTP流一个AVStream)。否则,
 *          数组中的所有上下文(每个RTP流一个AVCodecContext)
 *          必须只包含一个AVStream。
 * @param n_files ac中包含的AVCodecContext数量
 * @param buf 存储SDP的缓冲区(必须由调用者分配)
 * @param size 缓冲区大小
 * @return 成功返回0,错误返回AVERROR_xxx
 */
int av_sdp_create(AVFormatContext *ac[], int n_files, char *buf, int size);

/**
 * 如果给定文件名具有给定扩展名之一,则返回正值,
 * 否则返回0。
 *
 * @param filename   要检查的文件名
 * @param extensions 以逗号分隔的文件扩展名列表
 */
int av_match_ext(const char *filename, const char *extensions);

/**
 * 测试给定容器是否可以存储编解码器。
 *
 * @param ofmt           要检查兼容性的容器
 * @param codec_id       可能存储在容器中的编解码器
 * @param std_compliance 标准合规级别,FF_COMPLIANCE_*之一
 *
 * @return 如果codec_id可以存储在ofmt中返回1,如果不能返回0。
 *         如果此信息不可用则返回负数。
 */
int avformat_query_codec(const AVOutputFormat *ofmt, enum AVCodecID codec_id,
                         int std_compliance);

/**
 * @defgroup riff_fourcc RIFF FourCCs
 * @{
 * 获取将RIFF FourCCs映射到libavcodec AVCodecIDs的表。
 * 这些表用于传递给av_codec_get_id()/av_codec_get_tag(),
 * 如以下代码所示:
 * @code
 * uint32_t tag = MKTAG('H', '2', '6', '4');
 * const struct AVCodecTag *table[] = { avformat_get_riff_video_tags(), 0 };
 * enum AVCodecID id = av_codec_get_id(table, tag);
 * @endcode
 */
/**
 * @return 将视频RIFF FourCCs映射到libavcodec AVCodecID的表。
 */
const struct AVCodecTag *avformat_get_riff_video_tags(void);
/**
 * @return 将音频RIFF FourCCs映射到AVCodecID的表。
 */
const struct AVCodecTag *avformat_get_riff_audio_tags(void);
/**
 * @return 将视频MOV FourCCs映射到libavcodec AVCodecID的表。
 */
const struct AVCodecTag *avformat_get_mov_video_tags(void);
/**
 * @return 将音频MOV FourCCs映射到AVCodecID的表。
 */
const struct AVCodecTag *avformat_get_mov_audio_tags(void);

/**
 * @}
 */

/**
 * 根据流和帧的宽高比猜测采样宽高比。
 *
 * 由于帧宽高比由编解码器设置,但流宽高比由解复用器设置,
 * 这两个值可能不相等。如果您想显示帧,此函数尝试返回
 * 您应该使用的值。
 *
 * 基本逻辑是:如果流宽高比设置为合理值则使用它,
 * 否则使用帧宽高比。这样,容器设置(通常易于修改)
 * 可以覆盖帧中的编码值。
 *
 * @param format 流所属的格式上下文
 * @param stream 帧所属的流
 * @param frame 需要确定宽高比的帧
 * @return 猜测的(有效的)sample_aspect_ratio,如果不确定则返回0/1
 */
AVRational av_guess_sample_aspect_ratio(AVFormatContext *format, AVStream *stream,
                                        struct AVFrame *frame);

/**
 * 根据容器和编解码器信息猜测帧率。
 *
 * @param ctx 流所属的格式上下文
 * @param stream 帧所属的流
 * @param frame 需要确定帧率的帧,可以为NULL
 * @return 猜测的(有效的)帧率,如果不确定则返回0/1
 */
AVRational av_guess_frame_rate(AVFormatContext *ctx, AVStream *stream,
                               struct AVFrame *frame);

/**
 * 检查s中包含的流st是否与流说明符spec匹配。
 *
 * 有关spec语法,请参阅文档中的"流说明符"章节。
 *
 * @return  >0 如果st与spec匹配;
 *          0  如果st与spec不匹配;
 *          AVERROR代码,如果spec无效
 *
 * @note  一个流说明符可以匹配格式中的多个流。
 */
int avformat_match_stream_specifier(AVFormatContext *s, AVStream *st,
                                    const char *spec);

int avformat_queue_attached_pictures(AVFormatContext *s);

enum AVTimebaseSource {
    AVFMT_TBCF_AUTO = -1,
    AVFMT_TBCF_DECODER,
    AVFMT_TBCF_DEMUXER,
#if FF_API_R_FRAME_RATE
    AVFMT_TBCF_R_FRAMERATE,
#endif
};

/**
 * 将内部时序信息从一个流传输到另一个流。
 *
 * 此函数在进行流复制时很有用。
 *
 * @param ofmt     ost的目标输出格式
 * @param ost      需要复制和调整时序的输出流
 * @param ist      作为参考的输入流,从中复制时序
 * @param copy_tb  定义从何处导入流编解码器时基
 */
int avformat_transfer_internal_stream_timing_info(const AVOutputFormat *ofmt,
                                                  AVStream *ost, const AVStream *ist,
                                                  enum AVTimebaseSource copy_tb);

/**
 * 从流中获取内部编解码器时基。
 *
 * @param st  要提取时基的输入流
 */
AVRational av_stream_get_codec_timebase(const AVStream *st);

/**
 * @}
 */

#endif /* AVFORMAT_AVFORMAT_H */
