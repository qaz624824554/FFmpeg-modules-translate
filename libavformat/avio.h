#ifndef AVFORMAT_AVIO_H
#define AVFORMAT_AVIO_H

/**
 * @file
 * @ingroup lavf_io
 * 缓冲 I/O 操作
 */

#include <stdint.h>
#include <stdio.h>

#include "libavutil/attributes.h"
#include "libavutil/dict.h"
#include "libavutil/log.h"

#include "libavformat/version_major.h"

/**
 * 可以像本地文件一样进行定位操作。
 */
#define AVIO_SEEKABLE_NORMAL (1 << 0)

/**
 * 可以使用 avio_seek_time() 按时间戳定位。
 */
#define AVIO_SEEKABLE_TIME   (1 << 1)

/**
 * 用于检查是否中止阻塞函数的回调。
 * 在这种情况下，被中断的函数将返回 AVERROR_EXIT。
 * 在阻塞操作期间，回调函数会以 opaque 作为参数被调用。
 * 如果回调返回 1，阻塞操作将被中止。
 *
 * 如果在 AVFormatContext 或 AVIOContext 中
 * 在此结构体之后添加了新元素，则不能在没有主版本号变更的情况下
 * 向此结构体添加任何成员。
 */
typedef struct AVIOInterruptCB {
    int (*callback)(void*);
    void *opaque;
} AVIOInterruptCB;

/**
 * 目录条目类型。
 */
enum AVIODirEntryType {
    AVIO_ENTRY_UNKNOWN,
    AVIO_ENTRY_BLOCK_DEVICE,
    AVIO_ENTRY_CHARACTER_DEVICE,
    AVIO_ENTRY_DIRECTORY,
    AVIO_ENTRY_NAMED_PIPE,
    AVIO_ENTRY_SYMBOLIC_LINK,
    AVIO_ENTRY_SOCKET,
    AVIO_ENTRY_FILE,
    AVIO_ENTRY_SERVER,
    AVIO_ENTRY_SHARE,
    AVIO_ENTRY_WORKGROUP,
};

/**
 * 描述目录的单个条目。
 *
 * 只保证设置 name 和 type 字段。
 * 其余字段取决于协议和/或平台，可能是未知的。
 */
typedef struct AVIODirEntry {
    char *name;                           /**< 文件名 */
    int type;                             /**< 条目类型 */
    int utf8;                             /**< 当名称使用 UTF-8 编码时设为 1，否则为 0。
                                               即使设置为 0，名称也可能使用 UTF-8 编码。 */
    int64_t size;                         /**< 文件大小（字节），未知时为 -1。 */
    int64_t modification_timestamp;       /**< 最后修改时间，以 unix 纪元以来的微秒表示，
                                               未知时为 -1。 */
    int64_t access_timestamp;             /**< 最后访问时间，以 unix 纪元以来的微秒表示，
                                               未知时为 -1。 */
    int64_t status_change_timestamp;      /**< 最后状态更改时间，以 unix 纪元以来的微秒表示，
                                               未知时为 -1。 */
    int64_t user_id;                      /**< 所有者的用户 ID，未知时为 -1。 */
    int64_t group_id;                     /**< 所有者的组 ID，未知时为 -1。 */
    int64_t filemode;                     /**< Unix 文件模式，未知时为 -1。 */
} AVIODirEntry;

#if FF_API_AVIODIRCONTEXT
typedef struct AVIODirContext {
    struct URLContext *url_context;
} AVIODirContext;
#else
typedef struct AVIODirContext AVIODirContext;
#endif

/**
 * 可以通过 AVIO write_data_type 回调返回的不同数据类型。
 */
enum AVIODataMarkerType {
    /**
     * 头部数据；这些数据对于流的解码是必需的。
     */
    AVIO_DATA_MARKER_HEADER,
    /**
     * 输出字节流中解码器可以开始解码的位置（即关键帧）。
     * 给定带有 AVIO_DATA_MARKER_HEADER 标记的数据，
     * 然后是任何 AVIO_DATA_MARKER_SYNC_POINT，
     * 解复用器/解码器应该能够产生可解码的结果。
     */
    AVIO_DATA_MARKER_SYNC_POINT,
    /**
     * 输出字节流中解复用器可以开始解析的位置
     * （适用于非自同步字节流格式）。
     * 即任何非关键帧数据包的起始点。
     */
    AVIO_DATA_MARKER_BOUNDARY_POINT,
    /**
     * 这是任何未标记的数据。它可能是复用器完全不标记任何位置，
     * 可能是复用器选择不标记的实际边界/同步点，
     * 或者是由于 IO 缓冲区大小限制而被切分成多个写入回调的
     * 数据包/片段的后续部分。
     */
    AVIO_DATA_MARKER_UNKNOWN,
    /**
     * 尾部数据，不包含实际内容，仅用于最终完成输出文件。
     */
    AVIO_DATA_MARKER_TRAILER,
    /**
     * 输出字节流中的一个点，底层 AVIOContext 可能会根据延迟或
     * 缓冲要求刷新缓冲区。通常表示数据包的结束。
     */
    AVIO_DATA_MARKER_FLUSH_POINT,
};

/**
 * 字节流 IO 上下文。
 * 可以通过次版本号升级添加新的公共字段。
 * 删除、重新排序和更改现有公共字段需要
 * 主版本号升级。
 * 不得在 libav* 之外使用 sizeof(AVIOContext)。
 *
 * @note AVIOContext 中的函数指针都不应该直接调用，
 *       它们应该只在实现自定义 I/O 时由客户端应用程序设置。
 *       通常这些会被设置为 avio_alloc_context() 中指定的函数指针
 */
typedef struct AVIOContext {
    /**
     * 用于私有选项的类。
     *
     * 如果这个 AVIOContext 是由 avio_open2() 创建的，则设置 av_class
     * 并将选项传递给协议。
     *
     * 如果这个 AVIOContext 是手动分配的，则 av_class 可以由调用者设置。
     *
     * 警告 -- 该字段可以为 NULL，在这种情况下确保不要将此 AVIOContext
     * 传递给任何 av_opt_* 函数。
     */
    const AVClass *av_class;

    /*
     * 以下显示了在读取和写入时
     * buffer、buf_ptr、buf_ptr_max、buf_end、buf_size 和 pos 之间的关系
     * （因为 AVIOContext 用于两者）：
     *
     **********************************************************************************
     *                                   读取
     **********************************************************************************
     *
     *                            |              buffer_size              |
     *                            |---------------------------------------|
     *                            |                                       |
     *
     *                         buffer          buf_ptr       buf_end
     *                            +---------------+-----------------------+
     *                            |/ / / / / / / /|/ / / / / / /|         |
     *  读取缓冲区：               |/ / 已消费  /   | 待读取    / |         |
     *                            |/ / / / / / / /|/ / / / / / /|         |
     *                            +---------------+-----------------------+
     *
     *                                                         pos
     *              +-------------------------------------------+-----------------+
     *  输入文件：  |                                           |                 |
     *              +-------------------------------------------+-----------------+
     *
     *
     **********************************************************************************
     *                                   写入
     **********************************************************************************
     *
     *                             |          buffer_size                 |
     *                             |--------------------------------------|
     *                             |                                      |
     *
     *                                                buf_ptr_max
     *                          buffer                 (buf_ptr)       buf_end
     *                             +-----------------------+--------------+
     *                             |/ / / / / / / / / / / /|              |
     *  写入缓冲区：                | / / 待刷新数据  / /    |              |
     *                             |/ / / / / / / / / / / /|              |
     *                             +-----------------------+--------------+
     *                               由于向后定位，buf_ptr 可能
     *                               位于此处
     *
     *                            pos
     *               +-------------+----------------------------------------------+
     *  输出文件：    |             |                                              |
     *               +-------------+----------------------------------------------+
     *
     */
    unsigned char *buffer;  /**< 缓冲区的起始位置。 */
    int buffer_size;        /**< 最大缓冲区大小 */
    unsigned char *buf_ptr; /**< 缓冲区中的当前位置 */
    unsigned char *buf_end; /**< 数据的结束位置，可能小于
                                 buffer+buffer_size，如果读取函数返回的数据
                                 少于请求的数据，例如对于尚未接收到更多数据的流。 */
    void *opaque;           /**< 私有指针，传递给 read/write/seek/...
                                 函数。 */
    int (*read_packet)(void *opaque, uint8_t *buf, int buf_size);
#if FF_API_AVIO_WRITE_NONCONST
    int (*write_packet)(void *opaque, uint8_t *buf, int buf_size);
#else
    int (*write_packet)(void *opaque, const uint8_t *buf, int buf_size);
#endif
    int64_t (*seek)(void *opaque, int64_t offset, int whence);
    int64_t pos;            /**< 当前缓冲区在文件中的位置 */
    int eof_reached;        /**< 如果由于错误或到达文件末尾而无法读取则为 true */
    int error;              /**< 包含错误代码，如果没有发生错误则为 0 */
    int write_flag;         /**< 如果打开用于写入则为 true */
    int max_packet_size;
    int min_packet_size;    /**< 在刷新之前尝试缓冲至少这么多数据 */
    unsigned long checksum;
    unsigned char *checksum_ptr;
    unsigned long (*update_checksum)(unsigned long checksum, const uint8_t *buf, unsigned int size);
    /**
     * 暂停或恢复网络流协议的播放 - 例如 MMS。
     */
    int (*read_pause)(void *opaque, int pause);
    /**
     * 在指定的 stream_index 中定位到给定的时间戳。
     * 某些不支持定位到字节位置的网络流协议需要此功能。
     */
    int64_t (*read_seek)(void *opaque, int stream_index,
                         int64_t timestamp, int flags);
    /**
     * AVIO_SEEKABLE_ 标志的组合，如果流不可定位则为 0。
     */
    int seekable;

    /**
     * 如果可能，avio_read 和 avio_write 应该直接满足
     * 而不是通过缓冲区，avio_seek 将始终
     * 直接调用底层的 seek 函数。
     */
    int direct;

    /**
     * 以','分隔的允许协议列表。
     */
    const char *protocol_whitelist;

    /**
     * 以','分隔的禁止协议列表。
     */
    const char *protocol_blacklist;

    /**
     * 用于替代 write_packet 的回调。
     */
#if FF_API_AVIO_WRITE_NONCONST
    int (*write_data_type)(void *opaque, uint8_t *buf, int buf_size,
                           enum AVIODataMarkerType type, int64_t time);
#else
    int (*write_data_type)(void *opaque, const uint8_t *buf, int buf_size,
                           enum AVIODataMarkerType type, int64_t time);
#endif
    /**
     * 如果设置，不要为 AVIO_DATA_MARKER_BOUNDARY_POINT 单独调用 write_data_type，
     * 而是忽略它们并将其视为 AVIO_DATA_MARKER_UNKNOWN
     * （以避免从回调返回不必要的小数据块）。
     */
    int ignore_boundary_point;

    /**
     * 写入缓冲区中向后定位之前达到的最大位置，
     * 用于跟踪已写入的数据以便稍后刷新。
     */
    unsigned char *buf_ptr_max;

    /**
     * 此 AVIOContext 的已读字节数的只读统计。
     */
    int64_t bytes_read;

    /**
     * 此 AVIOContext 的已写字节数的只读统计。
     */
    int64_t bytes_written;
} AVIOContext;

/**
 * 返回将处理传入 URL 的协议的名称。
 *
 * 如果找不到处理给定 URL 的协议，则返回 NULL。
 *
 * @return 协议名称或 NULL。
 */
const char *avio_find_protocol_name(const char *url);

/**
 * 返回与 url 中资源的访问权限相对应的 AVIO_FLAG_* 访问标志，
 * 或者在失败的情况下返回对应于 AVERROR 代码的负值。
 * 返回的访问标志会被 flags 中的值掩码。
 *
 * @note 此函数本质上是不安全的，因为检查的资源可能会
 * 在一次调用到另一次调用之间改变其存在状态或权限状态。
 * 因此，除非你确定没有其他进程在访问被检查的资源，
 * 否则不应该信任返回的值。
 */
int avio_check(const char *url, int flags);

/**
 * 打开目录以供读取。
 *
 * @param s       目录读取上下文。必须传入一个指向 NULL 指针的指针。
 * @param url     要列出的目录。
 * @param options 包含协议私有选项的字典。返回时此参数将被销毁并替换为包含未找到选项的字典。可以为 NULL。
 * @return >=0 表示成功，负值表示错误。
 */
int avio_open_dir(AVIODirContext **s, const char *url, AVDictionary **options);

/**
 * 获取下一个目录条目。
 *
 * 返回的条目必须使用 avio_free_directory_entry() 释放。特别注意它可能比 AVIODirContext 存活更长时间。
 *
 * @param s         目录读取上下文。
 * @param[out] next 下一个条目，如果没有更多条目则为 NULL。
 * @return >=0 表示成功，负值表示错误。列表结束不被视为错误。
 */
int avio_read_dir(AVIODirContext *s, AVIODirEntry **next);

/**
 * 关闭目录。
 *
 * @note 使用 avio_read_dir() 创建的条目不会被删除，必须使用 avio_free_directory_entry() 释放。
 *
 * @param s         目录读取上下文。
 * @return >=0 表示成功，负值表示错误。
 */
int avio_close_dir(AVIODirContext **s);

/**
 * 释放由 avio_read_dir() 分配的条目。
 *
 * @param entry 要释放的条目。
 */
void avio_free_directory_entry(AVIODirEntry **entry);

/**
 * 为缓冲 I/O 分配并初始化一个 AVIOContext。之后必须使用 avio_context_free() 释放。
 *
 * @param buffer 用于通过 AVIOContext 进行输入/输出操作的内存块。
 *        缓冲区必须使用 av_malloc() 及其相关函数分配。
 *        它可能被 libavformat 释放并替换为新的缓冲区。
 *        AVIOContext.buffer 保存当前使用的缓冲区，
 *        之后必须使用 av_free() 释放。
 * @param buffer_size 缓冲区大小对性能非常重要。
 *        对于具有固定块大小的协议，应设置为此块大小。
 *        对于其他协议，典型大小是一个缓存页，例如 4kb。
 * @param write_flag 如果缓冲区应该可写则设置为 1，否则设置为 0。
 * @param opaque 指向用户特定数据的不透明指针。
 * @param read_packet  用于重新填充缓冲区的函数，可以为 NULL。
 *                     对于流协议，绝不能返回 0，而应该返回适当的 AVERROR 代码。
 * @param write_packet 用于写入缓冲区内容的函数，可以为 NULL。
 *        该函数不得更改输入缓冲区的内容。
 * @param seek 用于定位到指定字节位置的函数，可以为 NULL。
 *
 * @return 分配的 AVIOContext，失败时返回 NULL。
 */
AVIOContext *avio_alloc_context(
                  unsigned char *buffer,
                  int buffer_size,
                  int write_flag,
                  void *opaque,
                  int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
#if FF_API_AVIO_WRITE_NONCONST
                  int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
#else
                  int (*write_packet)(void *opaque, const uint8_t *buf, int buf_size),
#endif
                  int64_t (*seek)(void *opaque, int64_t offset, int whence));

/**
 * 释放提供的 IO 上下文及其相关的所有内容。
 *
 * @param s IO 上下文的双重指针。此函数将向 s 写入 NULL。
 */
void avio_context_free(AVIOContext **s);

void avio_w8(AVIOContext *s, int b);
void avio_write(AVIOContext *s, const unsigned char *buf, int size);
void avio_wl64(AVIOContext *s, uint64_t val);
void avio_wb64(AVIOContext *s, uint64_t val);
void avio_wl32(AVIOContext *s, unsigned int val);
void avio_wb32(AVIOContext *s, unsigned int val);
void avio_wl24(AVIOContext *s, unsigned int val);
void avio_wb24(AVIOContext *s, unsigned int val);
void avio_wl16(AVIOContext *s, unsigned int val);
void avio_wb16(AVIOContext *s, unsigned int val);

/**
 * 写入一个以 NULL 结尾的字符串。
 * @return 写入的字节数。
 */
int avio_put_str(AVIOContext *s, const char *str);

/**
 * 将 UTF-8 字符串转换为 UTF-16LE 并写入。
 * @param s AVIOContext
 * @param str 以 NULL 结尾的 UTF-8 字符串
 *
 * @return 写入的字节数。
 */
int avio_put_str16le(AVIOContext *s, const char *str);

/**
 * 将 UTF-8 字符串转换为 UTF-16BE 并写入。
 * @param s AVIOContext
 * @param str 以 NULL 结尾的 UTF-8 字符串
 *
 * @return 写入的字节数。
 */
int avio_put_str16be(AVIOContext *s, const char *str);

/**
 * 将写入的字节流标记为特定类型。
 *
 * 零长度范围将从输出中省略。
 *
 * @param s    AVIOContext
 * @param time 当前字节流位置对应的流时间（以 AV_TIME_BASE 单位），
 *             如果未知或不适用则为 AV_NOPTS_VALUE
 * @param type 从当前位置开始写入的数据类型
 */
void avio_write_marker(AVIOContext *s, int64_t time, enum AVIODataMarkerType type);

/**
 * 将此值与 seek 函数的 "whence" 参数进行或运算会导致它返回文件大小而不进行任何定位。
 * 支持此功能是可选的。如果不支持，则 seek 函数将返回 <0。
 */
#define AVSEEK_SIZE 0x10000

/**
 * 将此标志作为 "whence" 参数传递给 seek 函数会导致它通过任何方式（如重新打开和线性读取）
 * 或其他通常不合理的可能极其缓慢的方式进行定位。
 * seek 代码可能会忽略此标志。
 */
#define AVSEEK_FORCE 0x20000

/**
 * AVIOContext 的 fseek() 等效函数。
 * @return 新位置或 AVERROR。
 */
int64_t avio_seek(AVIOContext *s, int64_t offset, int whence);

/**
 * 向前跳过给定数量的字节
 * @return 新位置或 AVERROR。
 */
int64_t avio_skip(AVIOContext *s, int64_t offset);

/**
 * AVIOContext 的 ftell() 等效函数。
 * @return 位置或 AVERROR。
 */
static av_always_inline int64_t avio_tell(AVIOContext *s)
{
    return avio_seek(s, 0, SEEK_CUR);
}

/**
 * 获取文件大小。
 * @return 文件大小或 AVERROR
 */
int64_t avio_size(AVIOContext *s);

/**
 * 类似于 feof() 但在读取错误时也返回非零值。
 * @return 当且仅当到达文件末尾或发生读取错误时返回非零值。
 */
int avio_feof(AVIOContext *s);

/**
 * 使用 va_list 向上下文写入格式化字符串。
 * @return 写入的字节数，错误时返回 < 0。
 */
int avio_vprintf(AVIOContext *s, const char *fmt, va_list ap);

/**
 * 向上下文写入格式化字符串。
 * @return 写入的字节数，错误时返回 < 0。
 */
int avio_printf(AVIOContext *s, const char *fmt, ...) av_printf_format(2, 3);

/**
 * 向上下文写入以 NULL 结尾的字符串数组。
 * 通常你不需要直接使用这个函数，而是使用它的宏包装器 avio_print。
 */
void avio_print_string_array(AVIOContext *s, const char *strings[]);

/**
 * 向上下文写入字符串（const char *）。
 * 这是 avio_print_string_array 的一个便捷宏，它自动从可变参数列表创建字符串数组。
 * 对于简单的字符串连接，此函数比使用 avio_printf 性能更好，因为它不需要临时缓冲区。
 */
#define avio_print(s, ...) \
    avio_print_string_array(s, (const char*[]){__VA_ARGS__, NULL})

/**
 * 强制刷新缓冲数据。
 *
 * 对于写入流，强制立即将缓冲数据写入输出，而不是等待填满内部缓冲区。
 *
 * 对于读取流，丢弃所有当前缓冲的数据，并将报告的文件位置推进到底层流的位置。
 * 这不会读取新数据，也不会执行任何定位操作。
 */
void avio_flush(AVIOContext *s);

/**
 * 从 AVIOContext 读取 size 字节到 buf 中。
 * @return 读取的字节数或 AVERROR
 */
int avio_read(AVIOContext *s, unsigned char *buf, int size);

/**
 * 从 AVIOContext 读取 size 字节到 buf 中。与 avio_read() 不同，这允许读取少于请求的字节数。
 * 缺失的字节可以在下一次调用中读取。这总是尝试至少读取 1 个字节。
 * 在某些情况下用于减少延迟。
 * @return 读取的字节数或 AVERROR
 */
int avio_read_partial(AVIOContext *s, unsigned char *buf, int size);

/**
 * @name 从 AVIOContext 读取的函数
 * @{
 *
 * @note 如果是 EOF 则返回 0，所以如果需要 EOF 处理就不能使用它
 */
int          avio_r8  (AVIOContext *s);
unsigned int avio_rl16(AVIOContext *s);
unsigned int avio_rl24(AVIOContext *s);
unsigned int avio_rl32(AVIOContext *s);
uint64_t     avio_rl64(AVIOContext *s);
unsigned int avio_rb16(AVIOContext *s);
unsigned int avio_rb24(AVIOContext *s);
unsigned int avio_rb32(AVIOContext *s);
uint64_t     avio_rb64(AVIOContext *s);
/**
 * @}
 */

/**
 * 从 pb 读取字符串到 buf 中。当遇到 NULL 字符、已读取 maxlen 字节或无法从 pb 读取更多内容时，
 * 读取将终止。结果保证以 NULL 结尾，如果 buf 太小则会被截断。
 * 注意字符串不会以任何方式被解释或验证，对于多字节编码，它可能会在序列中间被截断。
 *
 * @return 读取的字节数（始终 <= maxlen）。
 * 如果在 EOF 或错误时结束读取，返回值将比实际读取的字节数多一个。
 */
int avio_get_str(AVIOContext *pb, int maxlen, char *buf, int buflen);

/**
 * 从 pb 读取 UTF-16 字符串并将其转换为 UTF-8。
 * 当遇到空字符或无效字符或已读取 maxlen 字节时，读取将终止。
 * @return 读取的字节数（始终 <= maxlen）
 */
int avio_get_str16le(AVIOContext *pb, int maxlen, char *buf, int buflen);
int avio_get_str16be(AVIOContext *pb, int maxlen, char *buf, int buflen);

/**
 * @name URL 打开模式
 * 传递给 avio_open 的 flags 参数必须是以下常量之一，
 * 可以与其他标志进行或运算。
 * @{
 */
#define AVIO_FLAG_READ  1                                      /**< 只读 */
#define AVIO_FLAG_WRITE 2                                      /**< 只写 */
#define AVIO_FLAG_READ_WRITE (AVIO_FLAG_READ|AVIO_FLAG_WRITE)  /**< 读写伪标志 */
/**
 * @}
 */

/**
 * 使用非阻塞模式。
 * 如果设置此标志，当操作无法立即执行时，
 * 上下文上的操作将返回 AVERROR(EAGAIN)。
 * 如果未设置此标志，上下文上的操作将永远不会返回 AVERROR(EAGAIN)。
 * 注意此标志不影响上下文的打开/连接。
 * 连接协议在必要时总是会阻塞(例如网络协议)，
 * 但永远不会挂起(例如在繁忙设备上)。
 * 警告：非阻塞协议仍在开发中；此标志可能会被静默忽略。
 */
#define AVIO_FLAG_NONBLOCK 8

/**
 * 使用直接模式。
 * avio_read 和 avio_write 应该尽可能直接满足请求
 * 而不是通过缓冲区，avio_seek 将始终
 * 直接调用底层的 seek 函数。
 */
#define AVIO_FLAG_DIRECT 0x8000

/**
 * 创建并初始化一个用于访问 url 指示的资源的 AVIOContext。
 * @note 当 url 指示的资源以读写模式打开时，
 * AVIOContext 只能用于写入。
 *
 * @param s 用于返回创建的 AVIOContext 的指针。
 * 如果失败，指向的值将被设置为 NULL。
 * @param url 要访问的资源
 * @param flags 控制如何打开 url 指示的资源的标志
 * @return >= 0 表示成功，负值对应于失败时的 AVERROR 代码
 */
int avio_open(AVIOContext **s, const char *url, int flags);

/**
 * 创建并初始化一个用于访问 url 指示的资源的 AVIOContext。
 * @note 当 url 指示的资源以读写模式打开时，
 * AVIOContext 只能用于写入。
 *
 * @param s 用于返回创建的 AVIOContext 的指针。
 * 如果失败，指向的值将被设置为 NULL。
 * @param url 要访问的资源
 * @param flags 控制如何打开 url 指示的资源的标志
 * @param int_cb 在协议级别使用的中断回调
 * @param options 填充有协议私有选项的字典。返回时
 * 此参数将被销毁并替换为包含未找到选项的字典。可以为 NULL。
 * @return >= 0 表示成功，负值对应于失败时的 AVERROR 代码
 */
int avio_open2(AVIOContext **s, const char *url, int flags,
               const AVIOInterruptCB *int_cb, AVDictionary **options);

/**
 * 关闭由 AVIOContext s 访问的资源并释放它。
 * 此函数只能用于通过 avio_open() 打开的 s。
 *
 * 在关闭资源之前会自动刷新内部缓冲区。
 *
 * @return 成功时返回 0，错误时返回 AVERROR < 0。
 * @see avio_closep
 */
int avio_close(AVIOContext *s);

/**
 * 关闭由 AVIOContext *s 访问的资源，释放它
 * 并将指向它的指针设置为 NULL。
 * 此函数只能用于通过 avio_open() 打开的 s。
 *
 * 在关闭资源之前会自动刷新内部缓冲区。
 *
 * @return 成功时返回 0，错误时返回 AVERROR < 0。
 * @see avio_close
 */
int avio_closep(AVIOContext **s);

/**
 * 打开一个仅写内存流。
 *
 * @param s 新的 IO 上下文
 * @return 如果没有错误则返回零。
 */
int avio_open_dyn_buf(AVIOContext **s);

/**
 * 返回已写入的大小和指向缓冲区的指针。
 * AVIOContext 流保持不变。
 * 不能释放缓冲区。
 * 不会向缓冲区添加填充。
 *
 * @param s IO 上下文
 * @param pbuffer 指向字节缓冲区的指针
 * @return 字节缓冲区的长度
 */
int avio_get_dyn_buf(AVIOContext *s, uint8_t **pbuffer);

/**
 * 返回已写入的大小和指向缓冲区的指针。缓冲区
 * 必须使用 av_free() 释放。
 * 会添加 AV_INPUT_BUFFER_PADDING_SIZE 的填充到缓冲区。
 *
 * @param s IO 上下文
 * @param pbuffer 指向字节缓冲区的指针
 * @return 字节缓冲区的长度
 */
int avio_close_dyn_buf(AVIOContext *s, uint8_t **pbuffer);

/**
 * 遍历可用协议的名称。
 *
 * @param opaque 表示当前协议的私有指针。
 *        在第一次迭代时必须是指向 NULL 的指针，
 *        并将通过连续调用 avio_enum_protocols 更新。
 * @param output 如果设置为 1，则遍历输出协议，
 *               否则遍历输入协议。
 *
 * @return 包含当前协议名称的静态字符串，如果没有更多协议则返回 NULL
 */
const char *avio_enum_protocols(void **opaque, int output);

/**
 * 通过可用协议的名称获取 AVClass。
 *
 * @return 输入协议名称的 AVClass 或 NULL
 */
const AVClass *avio_protocol_get_class(const char *name);

/**
 * 暂停和恢复播放 - 仅对使用网络流协议
 * (例如 MMS)有意义。
 *
 * @param h     从中调用 read_pause 函数指针的 IO 上下文
 * @param pause 1 表示暂停，0 表示恢复
 */
int     avio_pause(AVIOContext *h, int pause);

/**
 * 相对于某个组件流定位到给定的时间戳。
 * 仅对使用网络流协议(例如 MMS)有意义。
 *
 * @param h IO 上下文，从中调用 seek 函数指针
 * @param stream_index 时间戳相对的流索引。
 *        如果 stream_index 为(-1)，则时间戳应该是从演示开始的
 *        AV_TIME_BASE 单位。
 *        如果使用 stream_index >= 0 且协议不支持
 *        基于组件流的定位，调用将失败。
 * @param timestamp 以 AVStream.time_base 为单位的时间戳
 *        或如果没有指定流，则以 AV_TIME_BASE 为单位。
 * @param flags AVSEEK_FLAG_BACKWARD、AVSEEK_FLAG_BYTE
 *        和 AVSEEK_FLAG_ANY 的可选组合。协议可能会静默忽略
 *        AVSEEK_FLAG_BACKWARD 和 AVSEEK_FLAG_ANY，但如果使用
 *        AVSEEK_FLAG_BYTE 且不支持，将会失败。
 * @return 成功时返回 >= 0
 * @see AVInputFormat::read_seek
 */
int64_t avio_seek_time(AVIOContext *h, int stream_index,
                       int64_t timestamp, int flags);

/* 避免警告。由于会破坏 c++，无法包含头文件。 */
struct AVBPrint;

/**
 * 将 h 的内容读入打印缓冲区，最多读取 max_size 字节，或直到 EOF。
 *
 * @return 成功时返回 0(读取了 max_size 字节或达到 EOF)，
 * 否则返回负的错误代码
 */
int avio_read_to_bprint(AVIOContext *h, struct AVBPrint *pb, size_t max_size);

/**
 * 在服务器上下文上接受并分配客户端上下文。
 * @param  s 服务器上下文
 * @param  c 客户端上下文，必须未分配
 * @return   成功时返回 >= 0，失败时返回对应于
 *           AVERROR 的负值
 */
int avio_accept(AVIOContext *s, AVIOContext **c);

/**
 * 执行协议握手的一个步骤以接受新客户端。
 * 在使用 avio_accept() 返回的客户端作为读/写上下文之前，
 * 必须在其上调用此函数。
 * 它与 avio_accept() 分开是因为它可能会阻塞。
 * 握手的一个步骤由应用程序可能决定改变过程的位置定义。
 * 例如，在具有请求头和响应头的协议中，每个
 * 都可以构成一个步骤，因为应用程序可能使用请求中的参数
 * 来更改响应中的参数；或者请求的每个单独
 * 块都可以构成一个步骤。
 * 如果握手已经完成，avio_handshake() 不执行任何操作并
 * 立即返回 0。
 *
 * @param  c 要执行握手的客户端上下文
 * @return   0   表示完整且成功的握手
 *           > 0 表示握手已进行但尚未完成
 *           < 0 表示 AVERROR 代码
 */
int avio_handshake(AVIOContext *c);
#endif /* AVFORMAT_AVIO_H */
