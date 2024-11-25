#ifndef __UTILS_H__
#define __UTILS_H__

    #include <Arduino.h>

    uint8_t hex(char ch);

    int toByteArray(const char *in, size_t in_size, uint8_t *out);

    void reverseArray(uint8_t * array, size_t b_offset, size_t b_size);

    void showData(String title, uint8_t * data, size_t init, size_t finish);

    bool checkValid(unsigned char* hash, unsigned char* target);

#endif // __UTILS_H__