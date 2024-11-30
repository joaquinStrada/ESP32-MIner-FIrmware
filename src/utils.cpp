#include <Arduino.h>

uint8_t hex(char ch)
{
    uint8_t r = (ch > 57) ? (ch - 55) : (ch - 48);
    return r & 0x0F;
}

int toByteArray(const char *in, size_t in_size, uint8_t *out)
{
    int count = 0;
    if (in_size % 2) {
        while (*in && out) {
            *out = hex(*in++);
            if (!*in)
                return count;
            *out = (*out << 4) | hex(*in++);
            *out++;
            count++;
        }
        return count;
    } else {
        while (*in && out) {
            *out++ = (hex(*in++) << 4) | hex(*in++);
            count++;
        }
        return count;
    }
}

void reverseArray(uint8_t * array, size_t b_offset, size_t b_size)
{
    for (size_t j = b_offset; j < b_offset + (b_size / 2); j++)
    {
        uint8_t buff = array[j];
        array[j] = array[2 * b_offset + b_size - j - 1];
        array[2 * b_offset + b_size - j - 1] = buff;
    }
}

void showData(String title, uint8_t * data, size_t init, size_t finish)
{
    Serial.print(title);

    for (size_t i = init; i < finish; i++)
    {
        Serial.printf("%02x ", data[i]);
    }
    
    Serial.println();
}

bool checkValid(unsigned char* hash, unsigned char* target) {
  bool valid = true;
  for(uint8_t i=31; i>=0; i--) {
    if(hash[i] > target[i]) {
      valid = false;
      break;
    } else if (hash[i] < target[i]) {
      valid = true;
      break;
    }
  }
  if (valid) {
    Serial.print("\tvalid : ");
    for (size_t i = 0; i < 32; i++)
        Serial.printf("%02x ", hash[i]);
    Serial.println();
  }
  return valid;
}

float getRSSIasQuality(float rssi)
{
    float quality;

    if (rssi <= -100)
    {
        quality = 0;
    } else if (rssi >= -50) {
        quality = 100;
    } else {
        quality = 2 * (rssi + 100);
    }

    return quality;
}