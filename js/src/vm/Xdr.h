/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sw=4 et tw=78:
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is SpiderMonkey string object code.
 *
 * The Initial Developer of the Original Code is
 * the Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef Xdr_h___
#define Xdr_h___

#include "jsapi.h"
#include "jsprvtd.h"
#include "jsnum.h"

namespace js {

/*
 * Bytecode version number. Increment the subtrahend whenever JS bytecode
 * changes incompatibly.
 *
 * This version number is XDR'd near the front of xdr bytecode and
 * aborts deserialization if there is a mismatch between the current
 * and saved versions. If deserialization fails, the data should be
 * invalidated if possible.
 */
static const uint32_t XDR_BYTECODE_VERSION = uint32_t(0xb973c0de - 114);

class XDRBuffer {
  public:
    XDRBuffer(JSContext *cx)
      : context(cx), base(NULL), cursor(NULL), limit(NULL) { }

    JSContext *cx() const {
        return context;
    }

    void *getData(uint32_t *lengthp) const {
        JS_ASSERT(size_t(cursor - base) <= size_t(UINT32_MAX));
        *lengthp = uint32_t(cursor - base);
        return base;
    }

    void setData(const void *data, uint32_t length) {
        base = static_cast<uint8_t *>(const_cast<void *>(data));
        cursor = base;
        limit = base + length;
    }

    const uint8_t *read(size_t n) {
        JS_ASSERT(n <= size_t(limit - cursor));
        uint8_t *ptr = cursor;
        cursor += n;
        return ptr;
    }

    const char *readCString() {
        char *ptr = reinterpret_cast<char *>(cursor);
        cursor = reinterpret_cast<uint8_t *>(strchr(ptr, '\0')) + 1;
        JS_ASSERT(base < cursor);
        JS_ASSERT(cursor <= limit);
        return ptr;
    }

    uint8_t *write(size_t n) {
        if (n > size_t(limit - cursor)) {
            if (!grow(n))
                return NULL;
        }
        uint8_t *ptr = cursor;
        cursor += n;
        return ptr;
    }

    static bool isUint32Overflow(size_t n) {
        return size_t(-1) > size_t(UINT32_MAX) && n > size_t(UINT32_MAX);
    }

    void freeBuffer();

  private:
    bool grow(size_t n);

    JSContext   *const context;
    uint8_t     *base;
    uint8_t     *cursor;
    uint8_t     *limit;
};

/* We use little-endian byteorder for all encoded data */

#if defined IS_LITTLE_ENDIAN

inline uint32_t
NormalizeByteOrder32(uint32_t x)
{
    return x;
}

inline uint16_t
NormalizeByteOrder16(uint16_t x)
{
    return x;
}

#elif defined IS_BIG_ENDIAN

inline uint32_t
NormalizeByteOrder32(uint32_t x)
{
    return (x >> 24) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000) | (x << 24);
}

inline uint16_t
NormalizeByteOrder16(uint16_t x)
{
    return (x >> 8) | (x << 8);
}

#else
#error "unknown byte order"
#endif

template <XDRMode mode>
class XDRState {
  public:
    XDRBuffer buf;

  protected:
    JSPrincipals *principals;
    JSPrincipals *originPrincipals;

    XDRState(JSContext *cx)
      : buf(cx), principals(NULL), originPrincipals(NULL) {
    }

  public:
    JSContext *cx() const {
        return buf.cx();
    }

    bool codeUint8(uint8_t *n) {
        if (mode == XDR_ENCODE) {
            uint8_t *ptr = buf.write(sizeof *n);
            if (!ptr)
                return false;
            *ptr = *n;
        } else {
            *n = *buf.read(sizeof *n);
        }
        return true;
    }

    bool codeUint16(uint16_t *n) {
        uint16_t tmp;
        if (mode == XDR_ENCODE) {
            uint8_t *ptr = buf.write(sizeof tmp);
            if (!ptr)
                return false;
            tmp = NormalizeByteOrder16(*n);
            memcpy(ptr, &tmp, sizeof tmp);
        } else {
            memcpy(&tmp, buf.read(sizeof tmp), sizeof tmp);
            *n = NormalizeByteOrder16(tmp);
        }
        return true;
    }

    bool codeUint32(uint32_t *n) {
        uint32_t tmp;
        if (mode == XDR_ENCODE) {
            uint8_t *ptr = buf.write(sizeof tmp);
            if (!ptr)
                return false;
            tmp = NormalizeByteOrder32(*n);
            memcpy(ptr, &tmp, sizeof tmp);
        } else {
            memcpy(&tmp, buf.read(sizeof tmp), sizeof tmp);
            *n = NormalizeByteOrder32(tmp);
        }
        return true;
    }

    bool codeDouble(double *dp) {
        jsdpun tmp;
        if (mode == XDR_ENCODE) {
            uint8_t *ptr = buf.write(sizeof tmp);
            if (!ptr)
                return false;
            tmp.d = *dp;
            tmp.s.lo = NormalizeByteOrder32(tmp.s.lo);
            tmp.s.hi = NormalizeByteOrder32(tmp.s.hi);
            memcpy(ptr, &tmp.s.lo, sizeof tmp.s.lo);
            memcpy(ptr + sizeof tmp.s.lo, &tmp.s.hi, sizeof tmp.s.hi);
        } else {
            const uint8_t *ptr = buf.read(sizeof tmp);
            memcpy(&tmp.s.lo, ptr, sizeof tmp.s.lo);
            memcpy(&tmp.s.hi, ptr + sizeof tmp.s.lo, sizeof tmp.s.hi);
            tmp.s.lo = NormalizeByteOrder32(tmp.s.lo);
            tmp.s.hi = NormalizeByteOrder32(tmp.s.hi);
            *dp = tmp.d;
        }
        return true;
    }

    bool codeBytes(void *bytes, size_t len) {
        if (mode == XDR_ENCODE) {
            uint8_t *ptr = buf.write(len);
            if (!ptr)
                return false;
            memcpy(ptr, bytes, len);
        } else {
            memcpy(bytes, buf.read(len), len);
        }
        return true;
    }

    /*
     * During encoding the string is written into the buffer together with its
     * terminating '\0'. During decoding the method returns a pointer into the
     * decoding buffer and the caller must copy the string if it will outlive
     * the decoding buffer.
     */
    bool codeCString(const char **sp) {
        if (mode == XDR_ENCODE) {
            size_t n = strlen(*sp) + 1;
            uint8_t *ptr = buf.write(n);
            if (!ptr)
                return false;
            memcpy(ptr, *sp, n);
        } else {
            *sp = buf.readCString();
        }
        return true;
    }

    bool codeChars(jschar *chars, size_t nchars);
    bool codeString(JSString **strp);

    bool codeFunction(JSObject **objp);
    bool codeScript(JSScript **scriptp);

    void initScriptPrincipals(JSScript *script) {
        JS_ASSERT(mode == XDR_DECODE);
        
        /* The origin principals must be normalized at this point. */
        JS_ASSERT_IF(principals, originPrincipals);
        JS_ASSERT(!script->principals);
        JS_ASSERT(!script->originPrincipals);
        if (principals) {
            script->principals = principals;
            JS_HoldPrincipals(principals);
        }
        if (originPrincipals) {
            script->originPrincipals = originPrincipals;
            JS_HoldPrincipals(originPrincipals);
        }
    }
};

class XDREncoder : public XDRState<XDR_ENCODE> {
  public:
    XDREncoder(JSContext *cx)
      : XDRState<XDR_ENCODE>(cx) {
    }

    ~XDREncoder() {
        buf.freeBuffer();
    }

    const void *getData(uint32_t *lengthp) const {
        return buf.getData(lengthp);
    }

    void *forgetData(uint32_t *lengthp) {
        void *data = buf.getData(lengthp);
        buf.setData(NULL, 0);
        return data;
    }
};

class XDRDecoder : public XDRState<XDR_DECODE> {
  public:
    XDRDecoder(JSContext *cx, const void *data, uint32_t length,
               JSPrincipals *principals, JSPrincipals *originPrincipals);

};

} /* namespace js */

#endif /* Xdr_h___ */
