/*
 * Copyright 2017 Andrei Pangin
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include "vmStructs.h"


const int entry_frame_call_wrapper_offset = -6;
const int interpreter_frame_sender_sp_offset = -1;


class StackWalker {
  private:
    uintptr_t _pc;
    uintptr_t _sp;
    uintptr_t _fp;

    void set(uintptr_t pc, uintptr_t sp, uintptr_t fp) {
        _pc = pc;
        _sp = sp;
        _fp = fp;
    }

    uintptr_t& at(int offset) {
        return ((uintptr_t*)_fp)[offset];
    }

    void walk() {
        if (_pc == StubRoutines::callStubReturnAddress()) {
            walkEntryFrame();
        } else if (Interpreter::contains(_pc)) {
            walkInterpreterFrame();
        } else {
            CodeBlob* cb = CodeCache::findBlob(_pc);
            if (cb != NULL) {
                walkCompiledFrame(cb);
            } else {
                walkNativeFrame();
            }
        }
    }

    void walkEntryFrame() {
        JavaCallWrapper* wrapper = (JavaCallWrapper*)&at(entry_frame_call_wrapper_offset);
        JavaFrameAnchor* anchor = wrapper->anchor();
        uintptr_t prev_pc = anchor->lastJavaPC();
        if (prev_pc == 0) {
            prev_pc = ((uintptr_t*)anchor->lastJavaSP())[-1];
        }
        set(prev_pc, anchor->lastJavaSP(), anchor->lastJavaFP());
    }

    void walkInterpreterFrame() {
        uintptr_t prev_fp = at(0);
        uintptr_t prev_pc = at(1);
        uintptr_t prev_sp = at(interpreter_frame_sender_sp_offset);
        set(prev_pc, prev_sp, prev_fp);
    }

    void walkCompiledFrame(CodeBlob* cb) {
        uintptr_t prev_sp = _sp + cb->frameSize() * sizeof(uintptr_t);
        uintptr_t prev_fp = ((uintptr_t*)prev_sp)[-2];
        uintptr_t prev_pc = ((uintptr_t*)prev_sp)[-1];
        set(prev_pc, prev_sp, prev_fp);
    }

    void walkNativeFrame() {
        uintptr_t prev_fp = at(0);
        uintptr_t prev_pc = at(1);
        uintptr_t prev_sp = (uintptr_t)&at(2);
        set(prev_pc, prev_sp, prev_fp);
    }

  public:
    StackWalker(uintptr_t pc, uintptr_t sp, uintptr_t fp) : _pc(pc), _sp(sp), _fp(fp) {
    }
};
