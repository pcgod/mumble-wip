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

#ifndef _SIMPLERPC_H
#define _SIMPLERPC_H

#include "murmur_pch.h"

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread.hpp>

namespace simplerpc {

class Connection;

class SimpleRpc {
  public:
	SimpleRpc::SimpleRpc(boost::asio::io_service *io_service, unsigned short port);

	void RemoveConnection(boost::shared_ptr<Connection> connection);
	void HandleAccept(boost::shared_ptr<Connection> connection, const boost::system::error_code &error);

	static void init();
	static void shutdown();

  private:
	boost::asio::io_service &io_service_;
	boost::asio::ip::tcp::acceptor acceptor_;
	std::vector< boost::shared_ptr<Connection> > connections_;
	boost::shared_mutex connections_lock_;
};

class Connection : public boost::enable_shared_from_this<Connection> {
  public:
	Connection(boost::asio::io_service *io_service, SimpleRpc *simplerpc);

	void HandleRead(const boost::system::error_code &error, size_t /*bytes_transferred*/);
	void Run();
	boost::asio::ip::tcp::socket &Socket();

  private:
	enum State {
		kNew
	};

	State state_;
	SimpleRpc *simplerpc_;
	boost::asio::ip::tcp::socket socket_;
	boost::asio::io_service *io_service_;
	boost::asio::streambuf buffer_;
	boost::asio::strand write_strand_;
};

}

#endif  // _SIMPLERPC_H
