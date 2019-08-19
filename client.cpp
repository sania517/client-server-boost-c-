#include "client.h"



  chat_client::chat_client(boost::asio::io_service& io_service,
      std::string host, std::string port)
    : io_service_(io_service),
      socket_(io_service)
  {
      char str[512];
      std::cout << "Enter you nickname (max. 15 character): " << '\n';
      std::cin.getline(str,511);
      strncpy(nickName_,str,15);
      nickName_[15] = '\0';

      tcp::resolver resolver(io_service_);
      tcp::resolver::query query(host, port);
      tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

      tcp::endpoint endpoint = *endpoint_iterator;
      socket_.async_connect(endpoint,
          boost::bind(&chat_client::handle_connect, this,
            boost::asio::placeholders::error, ++endpoint_iterator));
      lengthName_ = strlen(nickName_);
  }


  void chat_client::handle_connect(const boost::system::error_code& error,
      tcp::resolver::iterator endpoint_iterator)
  {
    if (!error)
    {
      boost::asio::async_read(socket_,
          boost::asio::buffer(read_msg_.data(), chat_message::header_length),
          boost::bind(&chat_client::handle_read_header, this,
            boost::asio::placeholders::error));
    }
    else if (endpoint_iterator != tcp::resolver::iterator())
    {
      socket_.close();
      tcp::endpoint endpoint = *endpoint_iterator;
      socket_.async_connect(endpoint,
          boost::bind(&chat_client::handle_connect, this,
            boost::asio::placeholders::error, ++endpoint_iterator));
    }
  }

  void chat_client::handle_read_header(const boost::system::error_code& error)
  {
    if (!error && read_msg_.decode_header())
    {
      boost::asio::async_read(socket_,
          boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
          boost::bind(&chat_client::handle_read_body, this,
            boost::asio::placeholders::error));
    }
    else
    {
      do_close();
    }
  }

  void chat_client::handle_read_body(const boost::system::error_code& error)
  {
    if (!error)
    {
      std::cout.write(read_msg_.body(), read_msg_.body_length());
      std::cout << "\n";
      boost::asio::async_read(socket_,
          boost::asio::buffer(read_msg_.data(), chat_message::header_length),
          boost::bind(&chat_client::handle_read_header, this,
            boost::asio::placeholders::error));
    }
    else
    {
      do_close();
    }
  }

  void chat_client::do_write(chat_message msg)
  {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress)
    {
      boost::asio::async_write(socket_,
          boost::asio::buffer(write_msgs_.front().data(),
            write_msgs_.front().length()),
          boost::bind(&chat_client::handle_write, this,
            boost::asio::placeholders::error));
    }
  }

  void chat_client::handle_write(const boost::system::error_code& error)
  {
    if (!error)
    {
      write_msgs_.pop_front();
      if (!write_msgs_.empty())
      {
        boost::asio::async_write(socket_,
            boost::asio::buffer(write_msgs_.front().data(),
              write_msgs_.front().length()),
            boost::bind(&chat_client::handle_write, this,
              boost::asio::placeholders::error));
      }
    }
    else
    {
      do_close();
    }
  }


void chat_client::start_write_client()
{
    std::thread t(boost::bind(&boost::asio::io_service::run, &io_service_));

    char line[chat_message::max_body_length + 1 - 16];
    std::cout << "Started" << ":\n";
    while (std::cin.getline(line, chat_message::max_body_length + 1 -16))
    {
        char body[chat_message::max_body_length + 1]{};

      using namespace std; // For strlen and memcpy.

      chat_message msg;
      //-------------------
      strcat(body,nickName_);
      body[strlen(nickName_)]= ':';
      strcat (body,line);
      msg.body_length(strlen(body));
      memcpy(msg.body(), body, msg.body_length());

      msg.encode_header();
      this->write(msg);
    }

    this->close();
    t.join();

}


