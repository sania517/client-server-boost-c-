#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <set>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include "chat_message.h"
#include "client.h"


using boost::asio::ip::tcp;

//----------------------------------------------------------------------

typedef std::deque<chat_message> chat_message_queue;

//----------------------------------------------------------------------

class chat_participant
{
public:
  virtual ~chat_participant() {}
  virtual void deliver(const chat_message& msg) = 0;
};

typedef boost::shared_ptr<chat_participant> chat_participant_ptr;

//----------------------------------------------------------------------

class chat_room
{
public:
  void join(chat_participant_ptr participant)
  {
    participants_.insert(participant);
    std::for_each(recent_msgs_.begin(), recent_msgs_.end(),
        boost::bind(&chat_participant::deliver, participant, _1));
  }

  void leave(chat_participant_ptr participant)
  {
    participants_.erase(participant);
  }

  void deliver(const chat_message& msg)
  {
    recent_msgs_.push_back(msg);
    while (recent_msgs_.size() > max_recent_msgs)
      recent_msgs_.pop_front();

    std::for_each(participants_.begin(), participants_.end(),
        boost::bind(&chat_participant::deliver, _1, boost::ref(msg)));
  }

private:
  std::set<chat_participant_ptr> participants_;
  enum { max_recent_msgs = 100 };
  chat_message_queue recent_msgs_;
};

//----------------------------------------------------------------------

class chat_session
  : public chat_participant,
    public boost::enable_shared_from_this<chat_session>
{
public:
  chat_session(boost::asio::io_service& io_service, chat_room& room)
    : socket_(io_service),
      room_(room)
  {
  }

  tcp::socket& socket()
  {
    return socket_;
  }

  void start()
  {
    room_.join(shared_from_this());
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.data(), chat_message::header_length),
        boost::bind(
          &chat_session::handle_read_header, shared_from_this(),
          boost::asio::placeholders::error));
  }

  void deliver(const chat_message& msg)
  {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress)
    {
      boost::asio::async_write(socket_,
          boost::asio::buffer(write_msgs_.front().data(),
            write_msgs_.front().length()),
          boost::bind(&chat_session::handle_write, shared_from_this(),
            boost::asio::placeholders::error));
    }
  }

  void handle_read_header(const boost::system::error_code& error)
  {
    if (!error && read_msg_.decode_header())
    {
      boost::asio::async_read(socket_,
          boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
          boost::bind(&chat_session::handle_read_body, shared_from_this(),
            boost::asio::placeholders::error));
    }
    else
    {
      room_.leave(shared_from_this());

    }
  }

  void handle_read_body(const boost::system::error_code& error)
  {
    if (!error)
    {
      room_.deliver(read_msg_);
      boost::asio::async_read(socket_,
          boost::asio::buffer(read_msg_.data(), chat_message::header_length),
          boost::bind(&chat_session::handle_read_header, shared_from_this(),
            boost::asio::placeholders::error));
    }
    else
    {
      room_.leave(shared_from_this());
    }
  }

  void handle_write(const boost::system::error_code& error)
  {
    if (!error)
    {
      write_msgs_.pop_front();
      if (!write_msgs_.empty())
      {
        boost::asio::async_write(socket_,
            boost::asio::buffer(write_msgs_.front().data(),
              write_msgs_.front().length()),
            boost::bind(&chat_session::handle_write, shared_from_this(),
              boost::asio::placeholders::error));
      }
    }
    else
    {
      room_.leave(shared_from_this());
    }
  }

private:
  tcp::socket socket_;
  chat_room& room_;
  chat_message read_msg_;
  chat_message_queue write_msgs_;
};

typedef boost::shared_ptr<chat_session> chat_session_ptr;

//----------------------------------------------------------------------

class chat_server
{
public:
  chat_server(boost::asio::io_service& io_service,
      const tcp::endpoint& endpoint)
    : io_service_(io_service),
      acceptor_(io_service, endpoint)
  {
    chat_session_ptr new_session(new chat_session(io_service_, room_));
    acceptor_.async_accept(new_session->socket(),
        boost::bind(&chat_server::handle_accept, this, new_session,
          boost::asio::placeholders::error));
  }

  void handle_accept(chat_session_ptr session,
      const boost::system::error_code& error)
  {
    if (!error)
    {
      session->start();
      chat_session_ptr new_session(new chat_session(io_service_, room_));
      acceptor_.async_accept(new_session->socket(),
          boost::bind(&chat_server::handle_accept, this, new_session,
            boost::asio::placeholders::error));
      std:: cout <<"conected new client\n";
    }
  }

private:
  boost::asio::io_service& io_service_;
  tcp::acceptor acceptor_;
  chat_room room_;
};

typedef boost::shared_ptr<chat_server> chat_server_ptr;
typedef std::list<chat_server_ptr> chat_server_list;

//----------------------------------------------------------------------
int choice(){

    while(true){
        std::cout << "What would you like to launch? Make corect choice:\n\n";
        std::cout << "S - Enter character S if you need launch Server chat\n";
        std::cout << "C - Enter character C if you need launch Client chat\n";
        std::cout << "Q - Enter character Q for quit chat sesion\n";
        std::string tempStr{};
        std::cin >> tempStr;
        if (tempStr.size() > 1 || tempStr.size() == 0){
            std::cout << "Incorect choice! Repite choice.\n";
            continue;
        }
        switch (tempStr.data()[0]) {
        case ('C'):
        case ('c'):
            std::cout << "C\n";
            return 0;
        case ('S'):
        case ('s'):
            std::cout << "S\n";
            return 1;
        case ('Q'):
        case ('q'):
            std::cout << "Q\n";
            return -1;
        default:
            std::cout << "Incorect character! Repite choice.\n";
            continue;
        }
    }

}

bool parse(std::vector<std::string> &query, int choice){
    std::string temp_str{};
    if (choice == 0){
        std::cout << "your choice Client chat.\n"
                  << "Usage: chat_client <host> <port>\n";

    } else if (choice == 1){
        std::cout << "your choice Server chat.\n"
                  << "Usage: chat_server <port> [<port> ...]\n";

    }

        std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
        std::getline(std::cin, temp_str);
        int count{0};
        int start_pos{0};
        int end_pos{0};
        for(int i{0}; i < temp_str.size(); ++i){
            if(temp_str[i] == ' ' && start_pos == end_pos){
                start_pos++; end_pos++;
            } else if(temp_str[i] == ' ' && start_pos != end_pos){
                count++;
                char ch[80]{};
                temp_str.copy(ch, end_pos-start_pos, start_pos);
                query.emplace_back(ch);
                start_pos = ++end_pos;

            } else if(temp_str[i] != ' '){
                end_pos++;
            }
        } if (start_pos != end_pos){
            count++;
            char ch[80]{};
            temp_str.copy(ch,end_pos-start_pos, (start_pos));
            query.emplace_back(ch);
        }

//        for (auto var : query) {
//            std::cout << "query: " << var << '\n';
//        }

        if(choice == 0 && query.size() > 2){
            query.erase(query.begin()+2, query.end());
            std::cout << "query.size(): " << query.size() << '\n';
            return true;
        }    else if(choice == 0 && query.size() == 2){
            return true;
        } else if (choice == 0 && query.size() < 2){
            return false;
        } else if (choice == 1 && query.size() > 0){
            return true;
        }else {
            return false;
        }




}


int main(int argc, char* argv[])
{
    try
    {
    // !std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
    //проверить отбасывание обязательно
    //std::ios_base::sync_with_stdio(false); // for cin.readsome working!!!

    int ch = choice();
    if (ch == -1)
        return 2;

    std::vector<std::string> query;
    while (!parse(query,ch)){
     std::cout << "incorect data of conection point\n"
               << "Usage: for chat_client  <host> <port>\n"
               << "Usage: chat_server <port> [<port> ...]\n";
    }



    boost::asio::io_service io_service;

    std::string host_client = "127.0.0.1";
    std::string port_client;
    chat_server_list servers;

    if (ch == 1){
        port_client = query[0];

//        chat_server_list servers;
        for (int i = 0; i < query.size(); ++i)
        {
         using namespace std; // For atoi.
        tcp::endpoint endpoint(tcp::v4(), atoi(query[i].data()));
         chat_server_ptr server(new chat_server(io_service, endpoint));
         servers.push_back(server);
        }
    } else {
        host_client = query[0];
        port_client = query[1];
    }

//        chat_server_list servers;
//        for (int i = 0; i < query.size(); ++i)
//        {
//         using namespace std; // For atoi.
//        tcp::endpoint endpoint(tcp::v4(), atoi(query[i].data()));
//         chat_server_ptr server(new chat_server(io_service, endpoint));
//         servers.push_back(server);
//        }

        chat_client CC(io_service, host_client, port_client);
        std::thread tt(boost::bind(&chat_client::start_write_client, &CC));




    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
