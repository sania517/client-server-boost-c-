#ifndef CLIENT_H
#define CLIENT_H

#include <cstdlib>
#include <deque>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include "chat_message.h"

using boost::asio::ip::tcp;
typedef std::deque<chat_message> chat_message_queue;

class chat_client
{
public:
  chat_client(boost::asio::io_service& io_service,
      std::string host, std::string port);


  char const *name(){return nickName_;}
  int nameSize(){return lengthName_;}

  void write(const chat_message& msg)
  {
    io_service_.post(boost::bind(&chat_client::do_write, this, msg));
  }

  void close()
  {
    io_service_.post(boost::bind(&chat_client::do_close, this));
  }

private:
  void handle_connect(const boost::system::error_code& error,
      tcp::resolver::iterator endpoint_iterator);
  void handle_read_header(const boost::system::error_code& error);
  void handle_read_body(const boost::system::error_code& error);
  void do_write(chat_message msg);
  void handle_write(const boost::system::error_code& error);

  void do_close(){socket_.close();}

private:
  boost::asio::io_service& io_service_;
  tcp::socket socket_;
  chat_message read_msg_;
  chat_message_queue write_msgs_;
  char nickName_[16];
  int lengthName_ = 0;
public:
void start_write_client();
};




#endif // CLIENT_H
