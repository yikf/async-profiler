/*
 * Copyright 2016 Andrei Pangin
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

#include <stdlib.h>
#include <string.h>
#include "library.h"


void Library::expand() {
    Symbol* old_symbols = _symbols;
    Symbol* new_symbols = new Symbol[_capacity * 2];
    memcpy(new_symbols, old_symbols, _capacity * sizeof(Symbol));
    _capacity *= 2;
    _symbols = new_symbols;
    delete[] old_symbols;
}

void Library::add(const void* start, int length, jmethodID method) {
    if (_count >= _capacity) {
        expand();
    }

    _symbols[_count]._start = start;
    _symbols[_count]._end = (const char*)start + length;
    _symbols[_count]._method = method;
    _count++;
}

void Library::remove(const void* start, jmethodID method) {
    for (int i = 0; i < _count; i++) {
        if (_symbols[i]._start == start && _symbols[i]._method == method) {
            _symbols[i]._method = NULL;
            return;
        }
    }
}

jmethodID Library::find(const void* address) {
    for (int i = 0; i < _count; i++) {
        if (address >= _symbols[i]._start && address < _symbols[i]._end) {
            return _symbols[i]._method;
        }
    }
    return NULL;
}


NativeLibrary::NativeLibrary(const char* name, const void* min_address, const void* max_address) {
    _name = strdup(name);
    _min_address = min_address;
    _max_address = max_address;
}

NativeLibrary::~NativeLibrary() {
    for (int i = 0; i < _count; i++) {
        free(_symbols[i]._method);
    }
    free(_name);
}

void NativeLibrary::add(const void* start, int length, const char* name) {
    Library::add(start, length, (jmethodID)strdup(name));
}

void NativeLibrary::sort() {
    if (_count == 0) return;

    qsort(_symbols, _count, sizeof(Symbol), Symbol::comparator);

    if (_min_address == NULL) _min_address = _symbols[0]._start;
    if (_max_address == NULL) _max_address = _symbols[_count - 1]._end;
}

const char* NativeLibrary::binarySearch(const void* address) {
    int low = 0;
    int high = _count - 1;

    while (low <= high) {
        int mid = (unsigned int)(low + high) >> 1;
        if (_symbols[mid]._end <= address) {
            low = mid + 1;
        } else if (_symbols[mid]._start > address) {
            high = mid - 1;
        } else {
            return (const char*)_symbols[mid]._method;
        }
    }

    // Symbols with zero size can be valid functions: e.g. ASM entry points or kernel code
    if (low > 0 && _symbols[low - 1]._start == _symbols[low - 1]._end) {
        return (const char*)_symbols[low - 1]._method;
    }
    return _name;
}

const void* NativeLibrary::findSymbol(const char* prefix) {
    int prefix_len = strlen(prefix);
    for (int i = 0; i < _count; i++) {
        const char* symbol_name = (const char*)_symbols[i]._method;
        if (symbol_name != NULL && strncmp(symbol_name, prefix, prefix_len) == 0) {
            return _symbols[i]._start;
        }
    }
    return NULL;
}
