#ifndef AVCODEC_MEDIACODEC_H
#define AVCODEC_MEDIACODEC_H

#include "libavcodec/avcodec.h"

/**
 * 该结构体持有一个 android/view/Surface 对象的引用,
 * 该对象将被解码器用作输出。
 *
 */
typedef struct AVMediaCodecContext {

    /**
     * android/view/Surface 对象引用。
     */
    void *surface;

} AVMediaCodecContext;

/**
 * 分配并初始化一个 MediaCodec 上下文。
 *
 * 当使用 MediaCodec 解码完成后,调用者必须使用 av_mediacodec_default_free
 * 释放 MediaCodec 上下文。
 *
 * @return 成功时返回指向新分配的 AVMediaCodecContext 的指针,失败时返回 NULL
 */
AVMediaCodecContext *av_mediacodec_alloc_context(void);

/**
 * 设置 MediaCodec 上下文的便捷函数。
 *
 * @param avctx 编解码器上下文
 * @param ctx 要初始化的 MediaCodec 上下文
 * @param surface android/view/Surface 的引用
 * @return 成功返回 0,否则返回 < 0
 */
int av_mediacodec_default_init(AVCodecContext *avctx, AVMediaCodecContext *ctx, void *surface);

/**
 * 必须调用此函数来释放通过 av_mediacodec_default_init() 初始化的
 * MediaCodec 上下文。
 *
 * @param avctx 编解码器上下文
 */
void av_mediacodec_default_free(AVCodecContext *avctx);

/**
 * 表示要渲染的 MediaCodec 缓冲区的不透明结构。
 */
typedef struct MediaCodecBuffer AVMediaCodecBuffer;

/**
 * 释放 MediaCodec 缓冲区并将其渲染到与解码器关联的表面。此函数对给定缓冲区
 * 应该只调用一次,一旦释放,底层缓冲区返回到编解码器,因此后续调用此函数将
 * 不会产生任何效果。
 *
 * @param buffer 要渲染的缓冲区
 * @param render 1 表示释放并将缓冲区渲染到表面,0 表示丢弃缓冲区
 * @return 成功返回 0,否则返回 < 0
 */
int av_mediacodec_release_buffer(AVMediaCodecBuffer *buffer, int render);

/**
 * 释放 MediaCodec 缓冲区并在给定时间将其渲染到与解码器关联的表面。时间戳必须
 * 在当前 `java/lang/System#nanoTime()` (在 Android 上使用 `CLOCK_MONOTONIC` 
 * 实现)的一秒内。更多详细信息请参见 Android MediaCodec 文档中的
 * [`android/media/MediaCodec#releaseOutputBuffer(int,long)`][0]。
 *
 * @param buffer 要渲染的缓冲区
 * @param time 渲染缓冲区的纳秒时间戳
 * @return 成功返回 0,否则返回 < 0
 *
 * [0]: https://developer.android.com/reference/android/media/MediaCodec#releaseOutputBuffer(int,%20long)
 */
int av_mediacodec_render_buffer_at_time(AVMediaCodecBuffer *buffer, int64_t time);

#endif /* AVCODEC_MEDIACODEC_H */
