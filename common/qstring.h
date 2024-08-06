/**
 * @file qstring.h
 * @brief Header file for the qstring class.
 * 
 * This file contains the declaration of the qstring class, which provides flexible string handling using UT_string.
 * 
 * @note Ensure UT_string library is included and linked in your project.
 * 
 * @see UT_string
 * 
 * @author Arun A
 * @copyright Copyright (c) 2024 homenet25
 */

#ifndef qstring_h
#define qstring_h

#ifndef DEBUG_NEW
#include "./nvwa/debug_new.pch"
#endif

#include "./uthash/utstring.h"
#include <string>
#include <vector>
#include <cstdint>

/**
 * @class qstring
 * @brief A class for flexible string handling using UT_string.
 *
 * Manages strings and provides various constructors, methods for string manipulation,
 * and overloads for common operations. Utilizes the UT_string library for internal management.
 *
 * @note Ensure UT_string library is included and linked in your project.
 *
 * @see UT_string
 */
class qstring {
public:
    /**
     * @brief Default constructor.
     * Initializes an empty qstring object.
     */
    qstring() {
        utstring_new(ut_string);
    }

    /**
     * @brief Constructs from a C-style string.
     * @param str C-style string to initialize the qstring.
     */
    qstring(const char* str) {
        utstring_new(ut_string);
        if (str == nullptr) {
            return;
        }
        utstring_printf(ut_string, "%s", str);
    }

    /**
     * @brief Constructs from an integer.
     * @param n Integer value to initialize the qstring.
     */
    qstring(const int n) {
        utstring_new(ut_string);
        utstring_printf(ut_string, "%d", n);
    }

    /**
     * @brief Constructs from a long integer.
     * @param n Long integer value to initialize the qstring.
     */
    qstring(const long n) {
        utstring_new(ut_string);
        utstring_printf(ut_string, "%ld", n);
    }

    /**
     * @brief Constructs from a substring of a C-style string.
     * @param str C-style string.
     * @param len Length of the substring.
     */
    qstring(const char* str, int len) {
        utstring_new(ut_string);
        if (str == nullptr) {
            return;
        }
        utstring_printf(ut_string, "%.*s", len, str);
    }
    
    /**
     * @brief Constructs from a substring with size_t length.
     * @param str C-style string.
     * @param len Length of the substring.
     */
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
    
    /**
     * @brief Constructs from a binary buffer.
     * @param str Pointer to the binary data.
     * @param len Length of the binary data.
     */
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
    
    /**
     * @brief Copy constructor.
     * @param qstr qstring object to copy from.
     */
    qstring(const qstring& qstr) {
        utstring_new(ut_string);
        copy(qstr);
    }

    /**
     * @brief Static method to create a new qstring from a binary buffer.
     * @param str Pointer to the binary data.
     * @param len Length of the binary data.
     * @return Pointer to the newly created qstring.
     */
    static qstring* create(const uint8_t* str, size_t len) {
        if (str == nullptr) {
            return nullptr;
        }
        qstring* new_string = DEBUG_NEW qstring(str, len);
        return new_string;
    }

    /**
     * @brief Copies content from another qstring object.
     * @param qstr qstring object to copy from.
     */
    void copy(const qstring& qstr) {
        clear();
        utstring_printf(ut_string, "%.*s", (int)qstr.length(), qstr.c_str());
    }

    /**
     * @brief Copies content from a C-style string.
     * @param str C-style string to copy from.
     */
    void copy(const char* str) {
        if (str == nullptr) {
            return;
        }
        clear();
        utstring_printf(ut_string, "%s", str);
    }

    /**
     * @brief Appends formatted content without clearing existing data.
     * @param str C-style string.
     * @param len Length of the content.
     */
    void run_printf(const char* str, int len) {    // wont clear the data
        if (str == nullptr) {
            return;
        }
        utstring_printf(ut_string, "%.*s", len, str);
    }

    /**
     * @brief Copies binary data into the qstring object.
     * @param buf Pointer to the binary data.
     * @param len Length of the binary data.
     */
    void bin_copy(const uint8_t* buf, ssize_t len) {
        if (buf == nullptr) {
            return;
        }
        clear();
        utstring_bincpy(ut_string, buf, len);
    }

    /**
     * @brief Formats the qstring with variable arguments.
     * @param fmt Format string.
     * @param ... Variable arguments.
     */
    void format(const char* fmt, ...) {
        clear();
        va_list ap;
        va_start(ap, fmt);
        utstring_printf_va(ut_string, fmt, ap);
        va_end(ap);
    }

    /**
     * @brief Creates and returns a formatted qstring.
     * @param fmt Format string.
     * @param ... Variable arguments.
     * @return Formatted qstring.
     */
    static qstring format_string(const char* fmt, ...) {
        qstring new_str;
        va_list ap;
        va_start(ap, fmt);
        utstring_printf_va(new_str.ut_string, fmt, ap);
        va_end(ap);
        return new_str;
    }

    /**
     * @brief Finds the position of a substring.
     * @param start_pos Starting position for the search.
     * @param sub_str Substring to find.
     * @return Position of the substring or -1 if not found.
     */
    long find(long start_pos, const qstring& sub_str) const {
        return utstring_find(ut_string, start_pos, sub_str.c_str(), sub_str.length());
    }

    /**
     * @brief Replaces all occurrences of a substring.
     * @param sub_str Substring to replace.
     * @param str Replacement string.
     */
    void replace(const qstring& sub_str, const qstring& str) {
        if (length() == 0) {
            return;
        }
        std::vector<qstring> array;
        split(sub_str, array);
        if (array.size() == 0) {
            return;
        }
        bool is_last_char_match = (length() - sub_str.length()) == (unsigned long)find(length() - str.length(), sub_str);
        //        bool is_last_char_match = array[array.size()-1].length()>=str.length() && (length()-sub_str.length())==find(length()-str.length(), sub_str);
        clear();
        size_t itr = 0;
        for (auto s : array) {
            utstring_concat(ut_string, s.ut_string);
            if (itr < array.size() - 1 || is_last_char_match)
                utstring_concat(ut_string, str.ut_string);
            itr++;
        }
    }

    /**
     * @brief Splits the string into substrings.
     * @param sub_str Delimiter substring.
     * @param array Vector to hold the resulting substrings.
     * @param include_empty_string If true, includes empty substrings.
     */
    void split(const qstring& sub_str, std::vector<qstring>& array, bool include_empty_string = true) const {
        long start_pos = 0;
        long prev_start_pos = start_pos;
        bool found_atleast_one = false;
        while ((start_pos = find(prev_start_pos, sub_str)) >= 0) {
            qstring new_str;
            const char* src = c_str() + prev_start_pos;
            long len = start_pos - prev_start_pos;
            if (include_empty_string || len) {
                //                // Note : len==0 means empty string
                //                utstring_bincpy(new_str.ut_string, src, len==0 ? sub_str.length() : len);
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

    /**
     * @brief Destructor.
     * Frees the internal UT_string object.
     */
    ~qstring() {
        if (ut_string) {
            utstring_free(ut_string);
        }
    }

    /**
     * @brief Returns a C-style string representation.
     * @return C-style string.
     */
    const char* c_str() const {
        return utstring_body(ut_string);
    }

    /**
     * @brief Returns the length of the string.
     * @return Length of the string.
     */
    unsigned long length() const {
        return utstring_len(ut_string);
    }

    /**
     * @brief Checks if the string is empty.
     * @return True if empty, false otherwise.
     */
    bool isempty() const {
        return (utstring_len(ut_string) == 0) || compare("") == 0;
    }

    /**
     * @brief Clears the content of the qstring object.
     */
    void clear() {
        utstring_clear(ut_string);
    }

    /**
     * @brief Assigns content from another qstring.
     * @param qstr qstring object to assign from.
     */
    void operator=(const qstring& qstr) {
        copy(qstr);
    }

    /**
     * @brief Assigns content from a C-style string.
     * @param str C-style string to assign from.
     */
    void operator=(const char* str) {
        copy(str);
    }

    /**
     * @brief Compares for equality with another qstring.
     * @param str qstring object to compare with.
     * @return True if equal, false otherwise.
     */
    bool operator==(const qstring& str) {
        const char* b_str = str.c_str();
        const char* a_str = c_str();
        long len = str.length();
        int res = strncmp(a_str, b_str, len);
        return res == 0;
    }
    
    /**
     * @brief Compares for inequality with another qstring.
     * @param str qstring object to compare with.
     * @return True if not equal, false otherwise.
     */
    bool operator!=(const qstring& str) {
        const char* b_str = str.c_str();
        const char* a_str = c_str();
        long len = str.length();
        int res = strncmp(a_str, b_str, len);
        return res != 0;
    }

    /**
     * @brief Compares if this qstring is less than another.
     * @param other qstring object to compare with.
     * @return True if less, false otherwise.
     */
    bool operator<(const qstring& other) const {
        return strcmp(utstring_body(ut_string), utstring_body(other.get_utstring())) < 0;
    }

    /**
     * @brief Compares with another qstring.
     * @param str qstring object to compare with.
     * @return 0 if equal, 1 otherwise.
     */
    int compare(const qstring& str) const {
        qstring tmp(*this);
        return tmp == str ? 0 : 1;
    }

    /**
     * @brief Concatenates and returns a new qstring.
     * @param qstr qstring object to concatenate.
     * @return New concatenated qstring.
     */
    qstring operator+(const qstring& qstr) {
        qstring new_str(*this);
        utstring_concat(new_str.ut_string, qstr.get_utstring());
        return new_str;
    }

    /**
     * @brief Appends another qstring.
     * @param qstr qstring object to append.
     */
    void operator+=(const qstring& qstr) {
        utstring_concat(ut_string, qstr.get_utstring());
    }

    /**
     * @brief Appends a C-style string.
     * @param str C-style string to append.
     */
    void operator+=(const char* str) {
        qstring tmp(str);
        utstring_concat(ut_string, tmp.get_utstring());
    }

    /**
     * @brief Returns the internal UT_string object.
     * @return Pointer to the internal UT_string.
     */
    const UT_string* get_utstring() const {
        return ut_string;
    }

private:
    UT_string* ut_string = nullptr; /**< Internal string representation managed by UT_string. */
};
#endif /* qstring_h */
