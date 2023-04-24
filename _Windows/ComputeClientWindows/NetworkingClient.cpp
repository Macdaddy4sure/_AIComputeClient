/*
    Copyright(c) 2021 Tyler Crockett | Macdaddy4sure.com

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissionsand
    limitations under the License.
*/

#include "ComputeClient.h"

using namespace std;

enum { max_length = 1024 };

class client
{
public:
    client(boost::asio::io_service& io_service,
        boost::asio::ssl::context& context,
        boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
        : socket_(io_service, context)
    {
        socket_.set_verify_mode(boost::asio::ssl::verify_peer);
        socket_.set_verify_callback(
            boost::bind(&client::verify_certificate, this, _1, _2));

        boost::asio::async_connect(socket_.lowest_layer(), endpoint_iterator,
            boost::bind(&client::handle_connect, this,
                boost::asio::placeholders::error));
    }

    bool verify_certificate(bool preverified,
        boost::asio::ssl::verify_context& ctx)
    {
        // The verify callback can be used to check whether the certificate that is
        // being presented is valid for the peer. For example, RFC 2818 describes
        // the steps involved in doing this for HTTPS. Consult the OpenSSL
        // documentation for more details. Note that the callback is called once
        // for each certificate in the certificate chain, starting from the root
        // certificate authority.

        // In this example we will simply print the certificate's subject name.
        char subject_name[256];
        X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
        X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
        std::cout << "Verifying " << subject_name << "\n";

        return preverified;
    }

    void handle_connect(const boost::system::error_code& error)
    {
        if (!error)
        {
            socket_.async_handshake(boost::asio::ssl::stream_base::client,
                boost::bind(&client::handle_handshake, this,
                    boost::asio::placeholders::error));
        }
        else
        {
            std::cout << "Connect failed: " << error.message() << "\n";
        }
    }

    void handle_handshake(const boost::system::error_code& error)
    {
        if (!error)
        {
            std::cout << "Enter message: ";
            std::cin.getline(request_, max_length);
            size_t request_length = strlen(request_);

            boost::asio::async_write(socket_,
                boost::asio::buffer(request_, request_length),
                boost::bind(&client::handle_write, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        }
        else
        {
            std::cout << "Handshake failed: " << error.message() << "\n";
        }
    }

    void handle_write(const boost::system::error_code& error,
        size_t bytes_transferred)
    {
        if (!error)
        {
            boost::asio::async_read(socket_,
                boost::asio::buffer(reply_, bytes_transferred),
                boost::bind(&client::handle_read, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        }
        else
        {
            std::cout << "Write failed: " << error.message() << "\n";
        }
    }

    void handle_read(const boost::system::error_code& error,
        size_t bytes_transferred)
    {
        if (!error)
        {
            std::cout << "Reply: ";
            std::cout.write(reply_, bytes_transferred);
            std::cout << "\n";
        }
        else
        {
            std::cout << "Read failed: " << error.message() << "\n";
        }
    }

private:
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket_;
    char request_[max_length];
    char reply_[max_length];
};

void NetworkClient(string hostname, string port)
{
    try
    {
        boost::asio::io_service io_service;

        boost::asio::ip::tcp::resolver resolver(io_service);
        boost::asio::ip::tcp::resolver::query query(hostname, port);
        boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);

        boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
        ctx.load_verify_file("ca.pem");

        client c(io_service, ctx, iterator);

        io_service.run();
    }
    catch (exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}