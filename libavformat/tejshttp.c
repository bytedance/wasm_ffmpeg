// Copyright (c) 2025 [ByteDance Ltd. and/or its affiliates.]
// SPDX-License-Identifier：GNU Lesser General Public License v2.1 or later

#include "libavutil/opt.h"
#include "libavformat/url.h"

// #include "TEJSNetwork/TEJSNetwork_C.h"

#include <emscripten.h>


static JSHttpContext *tejs_http_ctx = NULL;
void register_tejs_http(JSHttpContext* ctx){
   assert(tejs_http_ctx == NULL);
   tejs_http_ctx = ctx;
}

typedef struct TEJSHttpContext
{
   void * http_ctx; // cpp impl
} TEJSHttpContext;

static int te_js_http_open(URLContext *h, const char *uri, int flags, AVDictionary **options)
{
   // emscripten_log(EM_LOG_INFO | EM_LOG_CONSOLE,  "te_js_http_open: %s\n", uri);
   if(tejs_http_ctx && tejs_http_ctx->tejs_http_init) {
      void * http_ctx = tejs_http_ctx->tejs_http_init(uri);

      TEJSHttpContext * ctx = h->priv_data;
      ctx->http_ctx = http_ctx;
      
      return 0;
   }
   return AVERROR(EINVAL);
}

static int te_js_http_read(URLContext *h, uint8_t *buf, int size) 
{
   TEJSHttpContext * ctx = h->priv_data;
   if(tejs_http_ctx && tejs_http_ctx->tejs_http_read) {
      return tejs_http_ctx->tejs_http_read(ctx->http_ctx, buf, size);
   }
   return AVERROR(errno);
}

static int te_js_http_close(URLContext *h)
{
   TEJSHttpContext * ctx = h->priv_data;

   if(ctx->http_ctx != NULL){
      if(tejs_http_ctx && tejs_http_ctx->tejs_http_uninit) {
         tejs_http_ctx->tejs_http_uninit(ctx->http_ctx);
      }
      ctx->http_ctx = NULL;
   }
   return 0;
}

static int64_t te_js_http_seek(URLContext *h, int64_t off, int whence)
{
   // emscripten_log(EM_LOG_INFO | EM_LOG_CONSOLE,  "te_js_http_seek\n");
   return 0;
}



/* private protocals 
 * implement by user, concat huangtao.coder@bytedance.com for more details
 */
const URLProtocol ff_tejshttp_protocol = {
   .name                = "tejshttp",
   .url_open2           = te_js_http_open,
   .url_read            = te_js_http_read,
   .url_close           = te_js_http_close,
   .url_seek            = te_js_http_seek,
   .flags               = URL_PROTOCOL_FLAG_NETWORK,
   .priv_data_size = sizeof(TEJSHttpContext)
};
