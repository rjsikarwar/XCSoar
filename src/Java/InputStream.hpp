/*
 * Copyright (C) 2010-2012 Max Kellermann <max@duempel.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef XCSOAR_JAVA_INPUT_STREAM_HPP
#define XCSOAR_JAVA_INPUT_STREAM_HPP

#include <jni.h>
#include <assert.h>
#include <stddef.h>

namespace Java {
  /**
   * Wrapper for a java.io.InputStream object.
   */
  class InputStream {
    static jmethodID close_method, read_method;

  public:
    static void Initialise(JNIEnv *env);

    static void close(JNIEnv *env, jobject is) {
      assert(env != NULL);
      assert(is != NULL);
      assert(close_method != NULL);

      env->CallVoidMethod(is, close_method);
    }

    static int read(JNIEnv *env, jobject is, jbyteArray buffer) {
      assert(env != NULL);
      assert(is != NULL);
      assert(buffer != NULL);
      assert(read_method != NULL);

      return env->CallIntMethod(is, read_method, buffer);
    }
  };
}

#endif
