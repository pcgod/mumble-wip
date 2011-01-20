/* Copyright (c) 2011 Benjamin Jemlich <pcgod@users.sourceforge.net>

   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.
   - Neither the name of the Mumble Developers nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "SimpleRpc.h"

#include <iostream>

#include "../../../json_spirit/json_spirit/json_spirit_reader_template.h"
#include "../../../json_spirit/json_spirit/json_spirit_writer_template.h"

namespace {

boost::asio::io_service *s_io_service = NULL;
simplerpc::SimpleRpc *s_srpc = NULL;

}

namespace simplerpc {

void SimpleRpc::init() {
	s_io_service = new boost::asio::io_service();
	s_srpc = new SimpleRpc(s_io_service, 6666);

	int32_t thread_count = boost::thread::hardware_concurrency();
	boost::thread_group thread_pool;

	if (thread_count == 0) {
		thread_count = 1;
	}

	std::cout << "Threads: " << thread_count << std::endl;

	for (int i = 0; i < thread_count; ++i) {
		thread_pool.create_thread(boost::bind(&boost::asio::io_service::run,
			s_io_service));
	}
	//thread_pool.join_all();
}

void SimpleRpc::shutdown() {
}

SimpleRpc::SimpleRpc(boost::asio::io_service *io_service, unsigned short port)
	: io_service_(*io_service),
	  acceptor_(*io_service,
		boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)) {
	boost::shared_ptr<Connection> connection = boost::make_shared<Connection>(&io_service_, this);
	acceptor_.async_accept(connection->Socket(),
		boost::bind(&SimpleRpc::HandleAccept, this, connection,
			boost::asio::placeholders::error));

}

void SimpleRpc::RemoveConnection(boost::shared_ptr<Connection> connection) {
	std::vector< boost::shared_ptr<Connection> >::iterator it;
	boost::shared_lock<boost::shared_mutex> l(connections_lock_);
	it = std::find(connections_.begin(), connections_.end(), connection);

	if (it == connections_.end()) {
		assert(false);
		return;
	}

	std::cout << "removing connection" << std::endl;

	boost::upgrade_lock<boost::shared_mutex> l2(connections_lock_);
	connections_.erase(it);

	std::cout << "Connections left: " << connections_.size() << std::endl;
}

void SimpleRpc::HandleAccept(boost::shared_ptr<Connection> connection, const boost::system::error_code &error) {
	if (error) {
		std::cout << "Accept error: " << error.message() << std::endl;
		return;
	}

	{
		boost::unique_lock<boost::shared_mutex> l(connections_lock_);
		connections_.push_back(connection);
	}

	connection->Run();

	connection = boost::make_shared<Connection>(&io_service_, this);
	acceptor_.async_accept(connection->Socket(),
		boost::bind(&SimpleRpc::HandleAccept, this, connection,
			boost::asio::placeholders::error));
}

Connection::Connection(boost::asio::io_service *io_service, SimpleRpc *simplerpc)
	: simplerpc_(simplerpc),
	  socket_(*io_service),
	  io_service_(io_service),
	  write_strand_(*io_service) {

}

boost::asio::ip::tcp::socket &Connection::Socket() {
	return socket_;
}

void Connection::Run() {
	boost::asio::async_read(socket_, buffer_, boost::asio::transfer_at_least(6),
							boost::bind(&Connection::HandleRead,
										shared_from_this(),
										boost::asio::placeholders::error,
										boost::asio::placeholders::bytes_transferred));
}

void Connection::HandleRead(const boost::system::error_code &error, size_t /*bytes_transferred*/) {
	if (error) {
		simplerpc_->RemoveConnection(shared_from_this());
		return;
	}

	boost::asio::async_read(socket_, buffer_, boost::asio::transfer_at_least(6),
							boost::bind(&Connection::HandleRead,
										shared_from_this(),
										boost::asio::placeholders::error,
										boost::asio::placeholders::bytes_transferred));
}

}
