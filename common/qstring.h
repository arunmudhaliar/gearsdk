//
//  qstring.h
//  common
//
//  Created by Arun A on 21/11/23.
//

#ifndef qstring_h
#define qstring_h

#ifndef DEBUG_NEW
#include "./nvwa/debug_new.pch"
#endif

#include "./uthash/utstring.h"
#include <string>
#include <vector>
#include <cstdint>

class qstring {
public:
    qstring() {
        utstring_new(ut_string);
    }

    qstring(const char* str) {
        utstring_new(ut_string);
        if (str == nullptr) {
            return;
        }
        utstring_printf(ut_string, "%s", str);
    }

    qstring(const int n) {
        utstring_new(ut_string);
        utstring_printf(ut_string, "%d", n);
    }
    
    qstring(const long n) {
        utstring_new(ut_string);
        utstring_printf(ut_string, "%ld", n);
    }

    qstring(const char* str, int len) {
        utstring_new(ut_string);
        if (str == nullptr) {
            return;
        }
        utstring_printf(ut_string, "%.*s", len, str);
    }
    qstring(const char* str, size_t len) {
        utstring_new(ut_string);
        if (str == nullptr) {
            return;
        }
        utstring_printf(ut_string, "%.*s", (int)len, str);
    }
    //    qstring(const uint8_t* str, int len) {
    //        utstring_new(ut_string);
    //        utstring_printf(ut_string, "%.*s", len, str);
    //    }
    qstring(const uint8_t* str, size_t len) {
        utstring_new(ut_string);
        if (str == nullptr) {
            return;
        }
        utstring_printf(ut_string, "%.*s", (int)len, str);
    }
    //    qstring(const uint8_t* str, unsigned long len) {
    //        utstring_new(ut_string);
    //        utstring_printf(ut_string, "%.*s", (int)len, str);
    //    }
    qstring(const qstring& qstr) {
        utstring_new(ut_string);
        copy(qstr);
    }

    static qstring* create(const uint8_t* str, size_t len) {
        if (str == nullptr) {
            return nullptr;
        }
        qstring* new_string = DEBUG_NEW qstring(str, len);
        return new_string;
    }

    void copy(const qstring& qstr) {
        clear();
        utstring_printf(ut_string, "%.*s", (int)qstr.length(), qstr.c_str());
    }

    void copy(const char* str) {
        if (str == nullptr) {
            return;
        }
        clear();
        utstring_printf(ut_string, "%s", str);
    }

    void format(const char* fmt, ...) {
        clear();
        va_list ap;
        va_start(ap, fmt);
        utstring_printf_va(ut_string, fmt, ap);
        va_end(ap);
    }

    static qstring format_string(const char* fmt, ...) {
        qstring new_str;
        va_list ap;
        va_start(ap, fmt);
        utstring_printf_va(new_str.ut_string, fmt, ap);
        va_end(ap);
        return new_str;
    }

    long find(long start_pos, const qstring& sub_str) const {
        return utstring_find(ut_string, start_pos, sub_str.c_str(), sub_str.length());
    }

    void replace(const qstring& sub_str, const qstring& str) {
        if (length()==0) {
            return;
        }
        std::vector<qstring> array;
        split(sub_str, array);
        bool is_last_char_match = (sub_str.length()==1 && (length()-1)==find(length()-1, sub_str));
        if (array.size()==0) {
            return;
        }
        clear();
        int itr = 0;
        for (auto s : array) {
            utstring_concat(ut_string, s.ut_string);
            if (itr<array.size()-1 || is_last_char_match)
                utstring_concat(ut_string, str.ut_string);
            itr++;
        }
    }

    void split(const qstring& sub_str, std::vector<qstring>& array, bool include_empty_string = true) const {
        long start_pos = 0;
        long prev_start_pos = start_pos;
        bool found_atleast_one = false;
        while ((start_pos = find(prev_start_pos, sub_str)) >= 0) {
            qstring new_str;
            const char* src = c_str() + prev_start_pos;
            long len = start_pos - prev_start_pos;
            if (include_empty_string || len) {
                utstring_bincpy(new_str.ut_string, src, len);
                array.push_back(new_str);
            }
            prev_start_pos = start_pos + sub_str.length();
            found_atleast_one = true;
        }

        // reminder
        long len = length() - prev_start_pos;
        if (len && found_atleast_one) {
            qstring new_str;
            const char* src = c_str() + prev_start_pos;
            utstring_bincpy(new_str.ut_string, src, len);
            array.push_back(new_str);
        }
    }

    ~qstring() {
        if (ut_string) {
            utstring_free(ut_string);
        }
    }

    const char* c_str() const {
        return utstring_body(ut_string);
    }

    unsigned long length() const {
        return utstring_len(ut_string);
    }

    bool isempty() const {
        return (utstring_len(ut_string) == 0) || compare("") == 0;
    }

    void clear() {
        utstring_clear(ut_string);
    }

    void operator=(const qstring& qstr) {
        copy(qstr);
    }

    void operator=(const char* str) {
        copy(str);
    }

    bool operator==(const qstring& str) {
        const char* b_str = str.c_str();
        const char* a_str = c_str();
        long len = str.length();
        int res = strncmp(a_str, b_str, len);
        return res == 0;
    }

    int compare(const qstring& str) const {
        qstring tmp(*this);
        return tmp == str ? 0 : 1;
    }

    qstring operator+(const qstring& qstr) {
        qstring new_str(*this);
        utstring_concat(new_str.ut_string, qstr.get_utstring());
        return new_str;
    }

    void operator+=(const qstring& qstr) {
        utstring_concat(ut_string, qstr.get_utstring());
    }

    void operator+=(const char* str) {
        qstring tmp(str);
        utstring_concat(ut_string, tmp.get_utstring());
    }

    const UT_string* get_utstring() const {
        return ut_string;
    }

private:
    UT_string* ut_string = nullptr;
};
#endif /* qstring_h */
