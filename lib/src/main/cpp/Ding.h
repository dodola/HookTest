//
// Created by didi on 2017/10/30.
//

#ifndef PROFILER_DING_H
#define PROFILER_DING_H


#include <jni.h>
#include <stdlib.h>

extern "C" {
#include <stdint.h>
#include <sys/mman.h>
}

#include <cstdio>
#include <string>
#include "aarch32/constants-aarch32.h"
#include "aarch32/instructions-aarch32.h"
#include "aarch32/macro-assembler-aarch32.h"
#include "aarch32/disasm-aarch32.h"

using namespace vixl;
using namespace vixl::aarch32;

#include <fb/include/fb/fbjni.h>

using namespace facebook::jni;

extern alias_ref<jclass> nativeEngineClass;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved);

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *vm, void *reserved);


class ExecutableMemory {
public:
    ExecutableMemory(const byte *code_start, size_t size)
            : size_(size),
              buffer_(reinterpret_cast<byte *>(mmap(NULL,
                                                    size,
                                                    PROT_READ | PROT_WRITE | PROT_EXEC,
                                                    MAP_SHARED | MAP_ANONYMOUS,
                                                    -1,
                                                    0))) {
        VIXL_ASSERT(reinterpret_cast<intptr_t>(buffer_) != -1);
        memcpy(buffer_, code_start, size_);
        __builtin___clear_cache((char *) (buffer_), (char *) (buffer_ + size_));
    }

//    ~ExecutableMemory() { munmap(buffer_, size_); }

    template<typename T>
    T GetEntryPoint(const Label &entry_point, InstructionSet isa) const {
        int32_t location = entry_point.GetLocation();
        if (isa == T32) location += 1;
        return GetOffsetAddress<T>(location);
    }

protected:
    template<typename T>
    T GetOffsetAddress(int32_t offset) const {
        VIXL_ASSERT((offset >= 0) && (static_cast<size_t>(offset) <= size_));
        T function_address;
        byte *buffer_address = buffer_ + offset;
        memcpy(&function_address, &buffer_address, sizeof(T));
        return function_address;
    }

private:
    size_t size_;
    byte *buffer_;
};

class androidbuf : public std::streambuf {
public:
    enum {
        bufsize = 128
    }; // ... or some other suitable buffer size
    androidbuf() { this->setp(buffer, buffer + bufsize - 1); }

private:
    int overflow(int c) {
        if (c == traits_type::eof()) {
            *this->pptr() = traits_type::to_char_type(c);
            this->sbumpc();
        }
        return this->sync() ? traits_type::eof() : traits_type::not_eof(c);
    }

    int sync() {
        int rc = 0;
        if (this->pbase() != this->pptr()) {
            __android_log_print(ANDROID_LOG_ERROR,
                                "Native",
                                "%s",
                                std::string(this->pbase(),
                                            this->pptr() - this->pbase()).c_str());
            rc = 0;
            this->setp(buffer, buffer + bufsize - 1);
        }
        return rc;
    }

    char buffer[bufsize];
};


struct HookInfo {
    jobject reflectedMethod;
    jobject additionalInfo;
//    void* originalMethod;
//    mirror::ArtMethod* originalMethod;
//    const char *shorty;
};

typedef struct _RegisterContext {
    uint32_t dummy_0;
    uint32_t dummy_1;

    union {
        uint32_t r[13];
        struct {
            uint32_t r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12;
        } regs;
    } general;

    uint32_t lr;
} RegisterContext;




#endif //PROFILER_DING_H
