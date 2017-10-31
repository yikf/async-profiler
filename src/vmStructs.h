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

#ifndef _VMSTRUCTS_H
#define _VMSTRUCTS_H

#include <stdint.h>
#include "library.h"


#define FOR_ALL_VM_STRUCTS(off, var) \
    off(int,        _klass_name_offset,        "Klass", "_name") \
    off(int,        _symbol_length_offset,     "Symbol", "_length") \
    off(int,        _symbol_body_offset,       "Symbol", "_body") \
    off(int,        _anchor_sp_offset,         "JavaFrameAnchor", "_last_Java_sp") \
    off(int,        _anchor_pc_offset,         "JavaFrameAnchor", "_last_Java_pc") \
    off(int,        _anchor_fp_offset,         "JavaFrameAnchor", "_last_Java_fp") \
    off(int,        _wrapper_anchor_offset,    "JavaCallWrapper", "_anchor") \
    off(int,        _stub_buffer_offset,       "StubQueue", "_stub_buffer") \
    off(int,        _stub_buffer_limit_offset, "StubQueue", "_buffer_limit") \
    off(int,        _heap_memory_offset,       "CodeHeap", "_memory") \
    off(int,        _heap_segmap_offset,       "CodeHeap", "_segmap") \
    off(int,        _heap_segment_size_offset, "CodeHeap", "_log2_segment_size") \
    off(int,        _vs_low_boundary_offset,   "VirtualSpace", "_low_boundary") \
    off(int,        _vs_high_boundary_offset,  "VirtualSpace", "_high_boundary") \
    off(int,        _vs_low_offset,            "VirtualSpace", "_low") \
    off(int,        _vs_high_offset,           "VirtualSpace", "_high") \
    off(int,        _heap_block_used_offset,   "HeapBlock::Header", "_used") \
    off(int,        _cb_name_offset,           "CodeBlob", "_name") \
    off(int,        _cb_size_offset,           "CodeBlob", "_size") \
    off(int,        _cb_frame_size_offset,     "CodeBlob", "_frame_size") \
    var(int,        _class_klass_offset,       "java_lang_Class", "_klass_offset") \
    var(uintptr_t,  _call_stub_return_address, "StubRoutines", "_call_stub_return_address") \
    var(StubQueue*, _interpreter_code,         "AbstractInterpreter", "_code") \
    var(CodeHeap*,  _code_cache_heap,          "CodeCache", "_heap") \

#define DECLARE_VM_STRUCT(TYPE, NAME, STRUCT, FIELD) \
    static TYPE NAME;


class StubQueue;
class CodeHeap;

class VMStructs {
  protected:
    FOR_ALL_VM_STRUCTS(DECLARE_VM_STRUCT, DECLARE_VM_STRUCT)

    const char* at(int offset) {
        return (const char*)this + offset;
    }

  public:
    static bool init(NativeLibrary* libjvm);

    static bool available() {
        return _klass_name_offset >= 0
            && _symbol_length_offset >= 0
            && _symbol_body_offset >= 0
            && _class_klass_offset >= 0;
    }
};


class VMSymbol : VMStructs {
  public:
    unsigned short length() {
        return *(unsigned short*) at(_symbol_length_offset);
    }

    const char* body() {
        return at(_symbol_body_offset);
    }
};

class VMKlass : VMStructs {
  public:
    VMSymbol* name() {
        return *(VMSymbol**) at(_klass_name_offset);
    }
};

class java_lang_Class : VMStructs {
  public:
    VMKlass* klass() {
        return *(VMKlass**) at(_class_klass_offset);
    }
};

class JavaFrameAnchor : VMStructs {
  public:
    uintptr_t lastJavaSP() {
        return *(uintptr_t*) at(_anchor_sp_offset);
    }

    uintptr_t lastJavaPC() {
        return *(uintptr_t*) at(_anchor_pc_offset);
    }

    uintptr_t lastJavaFP() {
        return *(uintptr_t*) at(_anchor_fp_offset);
    }
};

class JavaCallWrapper : VMStructs {
  public:
    JavaFrameAnchor* anchor() {
        return (JavaFrameAnchor*) at(_wrapper_anchor_offset);
    }
};

class StubRoutines : VMStructs {
  public:
    static uintptr_t callStubReturnAddress() {
        return _call_stub_return_address;
    }
};

class StubQueue : VMStructs {
  public:
    uintptr_t buffer() {
        return *(uintptr_t*) at(_stub_buffer_offset);
    }

    int buffer_limit() {
        return *(int*) at(_stub_buffer_limit_offset);
    }
};

class Interpreter : VMStructs {
  public:
    static bool contains(uintptr_t pc) {
        uintptr_t code_start = _interpreter_code->buffer();
        uintptr_t code_end = code_start + _interpreter_code->buffer_limit();
        return code_start <= pc && pc < code_end;
    }
};

class VirtualSpace : VMStructs {
  public:
    const char* low_boundary() {
        return *(const char**) at(_vs_low_boundary_offset);
    }

    const char* high_boundary() {
        return *(const char**) at(_vs_high_boundary_offset);
    }

    const char* low() {
        return *(const char**) at(_vs_low_offset);
    }

    const char* high() {
        return *(const char**) at(_vs_high_offset);
    }

    bool contains(const char* addr) {
        return low_boundary() <= addr && addr < high();
    }
};

class CodeBlob : VMStructs {
  public:
    const char* name() {
        return *(const char**) at(_cb_name_offset);
    }

    int size() {
        return *(int*) at(_cb_size_offset);
    }

    int frameSize() {
        return *(int*) at(_cb_frame_size_offset);
    }

    bool contains(const char* pc) {
        return at(0) <= pc && pc < at(size());
    }
};

class CodeHeap : VMStructs {
  private:
    size_t addrToIndex(const char* pc) {
        return (pc - memory()->low()) >> log2SegmentSize();
    }

    const char* indexToAddr(size_t index) {
        return memory()->low() + (index << log2SegmentSize());
    }

  public:
    VirtualSpace* memory() {
        return (VirtualSpace*) at(_heap_memory_offset);
    }

    VirtualSpace* segmap() {
        return (VirtualSpace*) at(_heap_segmap_offset);
    }

    int log2SegmentSize() {
        return *(int*) at(_heap_segment_size_offset);
    }

    CodeBlob* findBlob(const char* pc) {
        if (!memory()->contains(pc)) {
            return NULL;
        }

        const char* b = segmap()->low();
        size_t i = addrToIndex(pc);
        if (b[i] == 0xff) {
            return NULL;
        }

        while (b[i] > 0) {
            i -= (int)b[i];
        }

        const char* block = indexToAddr(i);
        if (*(bool*)(block + _heap_block_used_offset)) {
            return NULL;
        }
        return (CodeBlob*)block + 2;
    }
};

class CodeCache : VMStructs {
  public:
    static CodeBlob* findBlob(uintptr_t pc) {
        return findBlob((const char*)pc);
    }

    static CodeBlob* findBlob(const char* pc) {
        CodeBlob* cb = _code_cache_heap->findBlob(pc);
        if (cb != NULL && cb->contains(pc)) {
            return cb;
        }
        return NULL;
    }
};

#endif // _VMSTRUCTS_H
