#include "Bifrost.hpp"

#include <cxxopts.hpp>
#include <string>

using namespace marlin::net;
using namespace marlin::beacon;
using namespace marlin::pubsub;
using namespace marlin::lpf;

// template<typename ListenDelegate, typename TransportDelegate>
// using ClientTransportFactoryType = LpfTransportFactory<ListenDelegate, TransportDelegate, TcpTransportFactory, TcpTransport, 8>;

template<typename ListenDelegate, typename TransportDelegate>
using ClientTransportFactoryType = UdpTransportFactory<ListenDelegate, TransportDelegate>;

int main(int argc, char **argv) {
	cxxopts::Options options("Bifrost", "Gateway to the Marlin Network");
	options.add_options()
		("b,beacon", "Address of a beacon", cxxopts::value<std::string>())
		("c,channel", "Channel", cxxopts::value<std::string>())
		;

	auto args = options.parse(argc, argv);

	Bifrost bifrost;

	bifrost.channels = {args["channel"].as<std::string>()};

	PubSubNode<Bifrost> ps(SocketAddress::from_string("0.0.0.0:8000"));
	ps.delegate = &bifrost;
	bifrost.ps = &ps;

	DiscoveryClient<Bifrost> b(SocketAddress::from_string("0.0.0.0:8002"));
	b.delegate = &bifrost;
	bifrost.b = &b;

	b.start_discovery(SocketAddress::from_string(args["beacon"].as<std::string>()));

	ClientTransportFactoryType<Bifrost, Bifrost> f;
	f.bind(SocketAddress::loopback_ipv4(9000));
	f.listen(bifrost);

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
