#ifndef CHAT_MESSAGE_H
#define CHAT_MESSAGE_H


#include <cstdio>
#include <cstdlib>
#include <cstring>

class chat_message
{
public:
  enum { header_length = 4 };
  enum { max_body_length = 512 };

  chat_message() : body_length_(0){}
  const char* data() const {return data_;}
  char* data() {return data_;  }
  size_t length() const {return header_length + body_length_;}
  const char* body() const {return data_ + header_length;}
  char* body() {return data_ + header_length;}
  size_t body_length() const {return body_length_;}
  void body_length(size_t length);
  bool decode_header();
  void encode_header();

private:
  char data_[header_length + max_body_length];
  size_t body_length_;
};

#endif // CHAT_MESSAGE_H
