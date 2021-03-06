/*
 * Copyright (c) 2017 Jean Guyomarc'h
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef __EOVIM_NVIM_H__
#define __EOVIM_NVIM_H__

#include "eovim/types.h"
#include "eovim/options.h"
#include "eovim/nvim_helper.h"
#include "eovim/gui.h"

#include <Eina.h>
#include <Ecore.h>
#include <msgpack.h>

#define NVIM_VERSION_MAJOR(Nvim) ((Nvim)->version.major)
#define NVIM_VERSION_MINOR(Nvim) ((Nvim)->version.minor)
#define NVIM_VERSION_PATCH(Nvim) ((Nvim)->version.patch)

#define NVIM_VERSION_EQ(Nvim, Major, Minor, Patch) \
   (NVIM_VERSION_MAJOR(Nvim) == (Major) && \
    NVIM_VERSION_MINOR(Nvim) == (Minor) && \
    NVIM_VERSION_PATCH(Nvim) == (Patch))

struct nvim
{
   s_gui gui;
   s_version version; /**< The neovim's version */
   s_config *config;
   const s_options *opts;

   Ecore_Exe *exe;
   Eina_List *requests;
   Eina_Hash *modes;
   Eina_Inlist *tabs;

   msgpack_unpacker unpacker;
   msgpack_sbuffer sbuffer;
   msgpack_packer packer;
   uint32_t request_id;

   void (*hl_group_decode)(s_nvim *, unsigned int, f_highlight_group_decode);

   Eina_UStrbuf *decode;
   Eina_Bool mouse_enabled;
   Eina_Bool true_colors;
};


Eina_Bool nvim_init(void);
void nvim_shutdown(void);
s_nvim *nvim_new(const s_options *opts, const char *const args[]);
void nvim_free(s_nvim *nvim);
uint32_t nvim_next_uid_get(s_nvim *nvim);
Eina_Bool nvim_api_response_dispatch(s_nvim *nvim, const s_request *req, const msgpack_object_array *args);
Eina_Bool nvim_mode_add(s_nvim *nvim, s_mode *mode);
const s_mode *nvim_named_mode_get(const s_nvim *nvim, Eina_Stringshare *name);
void nvim_mouse_enabled_set(s_nvim *nvim, Eina_Bool enable);
Eina_Bool nvim_mouse_enabled_get(const s_nvim *nvim);
void gui_mode_update(s_gui *gui, Eina_Stringshare *name);

#endif /* ! __EOVIM_NVIM_H__ */
