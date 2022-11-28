/**
 * @file LogStream.h
 * @brief
 *
 * @version 1.0
 * @author Tianen Lu (tianenlu957@gmail.com)
 * @date 2022-11
 */


#ifndef LOGSTREAM_H
#define LOGSTREAM_H

#include "Types.h"
#include "NonCopyable.h"
#include "StringPiece.h"

#include <cstring> // memcpy
#include <cassert> // assert


namespace Lux {
namespace detail {
    const int kSmallBuffer = 4000;
    const int kLargeBuffer = 4000 * 1000;

    template <int SIZE>
    class FixedBuffer : noncopyable {
    private:
        const char* end() const { return data_ + sizeof data_; }
        // Must be outline function for cookies.
        static void cookieStart();
        static void cookieEnd();

        void (*cookie_)();
        char data_[SIZE];
        char* cur_;

    public:
        FixedBuffer() : cur_(data_) { setCookie(cookieStart); }

        ~FixedBuffer() { setCookie(cookieEnd); }

        void append(const char* /*restrict*/ buf, size_t len) {
            // FIXME: append partially
            if (implicit_cast<size_t>(avail()) > len) {
                ::memcpy(cur_, buf, len);
                cur_ += len;
            }
        }

        const char* data() const { return data_; }
        int length() const { return static_cast<int>(cur_ - data_); }

        // write to data_ directly
        char* current() { return cur_; }
        int avail() const { return static_cast<int>(end() - cur_); }
        void add(size_t len) { cur_ += len; }

        void reset() { cur_ = data_; }
        void bzero() { memZero(data_, sizeof data_); }

        // for used by GDB
        const char* debugString();
        void setCookie(void (*cookie)()) { cookie_ = cookie; }
        // for used by unit test
        string toString() const { return string(data_, length()); }
        StringPiece toStringPiece() const {
            return StringPiece(data_, static_cast<size_t>(length()));
        }
    };
} // namespace detail


class LogStream : noncopyable {
public:
    using Buffer = detail::FixedBuffer<detail::kSmallBuffer>;

private:
    using self = LogStream;

    void staticCheck();

    template <typename T>
    void formatInteger(T);

    Buffer buffer_;

    static const int kMaxNumericSize = 48;

public:
    self& operator<<(bool v) {
        buffer_.append(v ? "1" : "0", 1);
        return *this;
    }

    self& operator<<(short);
    self& operator<<(unsigned short);
    self& operator<<(int);
    self& operator<<(unsigned int);
    self& operator<<(long);
    self& operator<<(unsigned long);
    self& operator<<(long long);
    self& operator<<(unsigned long long);

    self& operator<<(const void*);

    self& operator<<(float v) {
        *this << static_cast<double>(v);
        return *this;
    }
    self& operator<<(double);
    // self& operator<<(long double);

    self& operator<<(char v) {
        buffer_.append(&v, 1);
        return *this;
    }

    // self& operator<<(signed char);
    // self& operator<<(unsigned char);

    self& operator<<(const char* str) {
        if (str) {
            buffer_.append(str, strlen(str));
        } else {
            buffer_.append("(null)", 6);
        }
        return *this;
    }

    self& operator<<(const unsigned char* str) {
        return operator<<(reinterpret_cast<const char*>(str));
    }

    self& operator<<(const std::string& v) {
        buffer_.append(v.c_str(), v.size());
        return *this;
    }

    self& operator<<(const StringPiece& v) {
        buffer_.append(v.data(), static_cast<size_t>(v.size()));
        return *this;
    }

    self& operator<<(const Buffer& v) {
        *this << v.toStringPiece();
        return *this;
    }

    void append(const char* data, int len) { buffer_.append(data, static_cast<size_t>(len)); }
    const Buffer& buffer() const { return buffer_; }
    void resetBuffer() { buffer_.reset(); }
};


class Fmt /* : noncopyable */ {
private:
    char buf_[32];
    int length_;

public:
    template <typename T>
    Fmt(const char* fmt, T val);

    const char* data() const { return buf_; }
    int length() const { return length_; }
};

inline LogStream& operator<<(LogStream& s, const Fmt& fmt) {
    s.append(fmt.data(), fmt.length());
    return s;
}

// Format quantity n in SI units (k, M, G, T, P, E).
// The returned string is atmost 5 characters long.
// Requires n >= 0
std::string formatSI(int64_t n);

// Format quantity n in IEC (binary) units (Ki, Mi, Gi, Ti, Pi, Ei).
// The returned string is atmost 6 characters long.
// Requires n >= 0
std::string formatIEC(int64_t n);
} // namespace Lux

#endif // LOGSTREAM_H