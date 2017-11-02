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

#ifndef _STACKWALKER_H
#define _STACKWALKER_H

#include <stdint.h>
#include "vmStructs.h"


const int entry_frame_call_wrapper_offset = -6;
const int interpreter_frame_sender_sp_offset = -1;
const int interpreter_frame_method_offset = -3;


class StackWalker {
  private:
    uintptr_t _pc;
    uintptr_t _fp;
    uintptr_t _sp;
    uintptr_t _unextended_sp;

    void set(uintptr_t pc, uintptr_t fp, uintptr_t sp, uintptr_t unextended_sp) {
        _pc = pc;
        _fp = fp;
        _sp = sp;
        _unextended_sp = unextended_sp;
    }

    uintptr_t& at(int offset) {
        return ((uintptr_t*)_fp)[offset];
    }

    void walkEntryFrame() {
        JavaCallWrapper* wrapper = (JavaCallWrapper*)&at(entry_frame_call_wrapper_offset);
        JavaFrameAnchor* anchor = wrapper->anchor();
        uintptr_t prev_fp = anchor->lastJavaFP();
        uintptr_t prev_sp = anchor->lastJavaSP();
        uintptr_t prev_pc = anchor->lastJavaPC();
        if (prev_pc == 0) {
            prev_pc = ((uintptr_t*)prev_sp)[-1];
        }
        set(prev_pc, prev_fp, prev_sp, prev_sp);
    }

    void walkInterpreterFrame() {
        VMMethod* method = (VMMethod*)at(interpreter_frame_method_offset);
        VMSymbol* className = method->holder()->name();
        VMSymbol* methodName = method->name();
        printf("  method = %s.%s\n", className->body(), methodName->body());

        uintptr_t prev_fp = at(0);
        uintptr_t prev_pc = at(1);
        uintptr_t prev_sp = (uintptr_t)&at(2);
        uintptr_t unextended_sp = at(interpreter_frame_sender_sp_offset);
        set(prev_pc, prev_fp, prev_sp, unextended_sp);
    }

    void walkCompiledFrame(CodeBlob* cb) {
        printf("  name = %s, frameSize = %d\n", cb->name(), cb->frameSize());
        if (cb->isNMethod()) {
            VMMethod* method = cb->method();
            VMSymbol* className = method->holder()->name();
            VMSymbol* methodName = method->name();
            printf("  method = %s.%s\n", className->body(), methodName->body());
        }

        uintptr_t prev_sp = _unextended_sp + cb->frameSize() * sizeof(void*);
        uintptr_t prev_fp = ((uintptr_t*)prev_sp)[-2];
        uintptr_t prev_pc = ((uintptr_t*)prev_sp)[-1];
        set(prev_pc, prev_fp, prev_sp, prev_sp);
    }

    void walkNativeFrame() {
        uintptr_t prev_fp = at(0);
        uintptr_t prev_pc = at(1);
        uintptr_t prev_sp = (uintptr_t)&at(2);
        set(prev_pc, prev_fp, prev_sp, prev_sp);
    }

  public:
    StackWalker(uintptr_t pc, uintptr_t fp, uintptr_t sp) : _pc(pc), _fp(fp), _sp(sp), _unextended_sp(sp) {
    }

    uintptr_t walk() {
        printf("[pc = %p, fp = %p, sp = %p, usp = %p]  ", (char*)_pc, (char*)_fp, (char*)_sp, (char*)_unextended_sp);
        if (_pc == StubRoutines::callStubReturnAddress()) {
            printf("EntryFrame\n");
            walkEntryFrame();
        } else if (Interpreter::contains(_pc)) {
            printf("InterpreterFrame\n");
            walkInterpreterFrame();
        } else {
            CodeBlob* cb = CodeCache::findBlob(_pc);
            if (cb != NULL) {
                printf("CompiledFrame\n");
                walkCompiledFrame(cb);
            } else {
                printf("NativeFrame\n");
                walkNativeFrame();
            }
        }
        return _sp;
    }
};

#endif // _STACKWALKER_H
