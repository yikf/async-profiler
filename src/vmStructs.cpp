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

#include <string.h>
#include "vmStructs.h"
#include "library.h"


#define DEFINE_VM_OFFSET(NAME, STRUCT, FIELD) \
    int VMStructs::NAME = -1;

#define DEFINE_VM_STATIC(TYPE, NAME, STRUCT, FIELD) \
    TYPE VMStructs::NAME;

FOR_ALL_VM_OFFSETS(DEFINE_VM_OFFSET)
FOR_ALL_VM_STATICS(DEFINE_VM_STATIC)


static uintptr_t readSymbol(NativeLibrary* lib, const char* symbol_name) {
    const void* symbol = lib->findSymbol(symbol_name);
    if (symbol == NULL) {
        // Avoid JVM crash in case of missing symbols
        return 0;
    }
    return *(uintptr_t*)symbol;
}

bool VMStructs::init(NativeLibrary* libjvm) {
    if (available()) {
        return true;
    }

    uintptr_t entry = readSymbol(libjvm, "gHotSpotVMStructs");
    uintptr_t stride = readSymbol(libjvm, "gHotSpotVMStructEntryArrayStride");
    uintptr_t type_offset = readSymbol(libjvm, "gHotSpotVMStructEntryTypeNameOffset");
    uintptr_t field_offset = readSymbol(libjvm, "gHotSpotVMStructEntryFieldNameOffset");
    uintptr_t offset_offset = readSymbol(libjvm, "gHotSpotVMStructEntryOffsetOffset");
    uintptr_t address_offset = readSymbol(libjvm, "gHotSpotVMStructEntryAddressOffset");

    if (entry == 0 || stride == 0) {
        return false;
    }

    while (true) {
        const char* type = *(const char**)(entry + type_offset);
        const char* field = *(const char**)(entry + field_offset);
        if (type == NULL || field == NULL) {
            return available();
        }

#define PARSE_VM_OFFSET(NAME, STRUCT, FIELD) \
        else if (strcmp(type, #STRUCT) == 0 && strcmp(field, #FIELD) == 0) { \
            NAME = *(int*)(entry + offset_offset); \
        }

#define PARSE_VM_STATIC(TYPE, NAME, STRUCT, FIELD) \
        else if (strcmp(type, #STRUCT) == 0 && strcmp(field, #FIELD) == 0) { \
            NAME = **(TYPE**)(entry + address_offset); \
        }

        FOR_ALL_VM_OFFSETS(PARSE_VM_OFFSET)
        FOR_ALL_VM_STATICS(PARSE_VM_STATIC)

        entry += stride;
    }
}

#define TEST_VM_OFFSET(NAME, STRUCT, FIELD) \
    if (NAME < 0) return false;

bool VMStructs::available() {
    FOR_ALL_VM_OFFSETS(TEST_VM_OFFSET)
    return true;
}

#define PRINT_VM_OFFSET(NAME, STRUCT, FIELD) \
    printf(#STRUCT "::" #FIELD " = %d\n", NAME);

#define PRINT_VM_STATIC(TYPE, NAME, STRUCT, FIELD) \
    printf(#STRUCT "::" #FIELD " = 0x%lx\n", (intptr_t)NAME);

void VMStructs::print() {
    FOR_ALL_VM_OFFSETS(PRINT_VM_OFFSET)
    FOR_ALL_VM_STATICS(PRINT_VM_STATIC)
}
