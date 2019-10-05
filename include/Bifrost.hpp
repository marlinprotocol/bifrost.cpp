#ifndef MARLIN_BIFROST_BIFROST_HPP
#define MARLIN_BIFROST_BIFROST_HPP

#include <marlin/net/udp/UdpTransport.hpp>
#include <marlin/stream/StreamTransportHelper.hpp>
#include <marlin/pubsub/PubSubNode.hpp>
#include <marlin/beacon/DiscoveryClient.hpp>
#include <marlin/lpf/LpfTransportFactory.hpp>
#include <cryptopp/blake2.h>

#define PUBSUB_PROTOCOL_NUMBER 0x10000000

using namespace marlin;
using namespace marlin::net;
using namespace marlin::beacon;
using namespace marlin::pubsub;
using namespace marlin::lpf;

// template<typename Delegate>
// using ClientTransportType = LpfTransport<Delegate, TcpTransport, 8>;

template<typename Delegate>
using ClientTransportType = UdpTransport<Delegate>;

class Bifrost {
private:
	typedef
		marlin::stream::StreamTransportHelper<
			marlin::pubsub::PubSubNode<Bifrost>,
			net::UdpTransport
		> StreamTransportHelper;

public:
	marlin::beacon::DiscoveryClient<Bifrost> *b;
	marlin::pubsub::PubSubNode<Bifrost> *ps;
	ClientTransportType<Bifrost> *lpft = nullptr;

	std::vector<std::tuple<uint32_t, uint16_t, uint16_t>> get_protocols() {
		return {};
	}

	void new_peer(
		net::SocketAddress const &addr,
		uint32_t protocol,
		uint16_t
	) {
		if(protocol == PUBSUB_PROTOCOL_NUMBER) {
			ps->subscribe(addr);

			// TODO: Better design
			ps->add_subscriber(addr);
		}
	}

	std::vector<std::string> channels = {"eth"};

	void did_unsubscribe(
		marlin::pubsub::PubSubNode<Bifrost> &,
		std::string channel
	) {
		SPDLOG_INFO("Did unsubscribe: {}", channel);
	}

	void did_subscribe(
		marlin::pubsub::PubSubNode<Bifrost> &,
		std::string channel
	) {
		SPDLOG_INFO("Did subscribe: {}", channel);
	}

	void did_recv_message(
		marlin::pubsub::PubSubNode<Bifrost> &,
		std::unique_ptr<char[]> &&message,
		uint64_t size,
		std::string &channel,
		uint64_t message_id
	) {
		SPDLOG_INFO(
			"Received message {} on channel {}",
			message_id,
			channel
		);

		if(lpft) {
			lpft->send(marlin::net::Buffer(message.release(), size));
		}
	}

	void did_recv_packet(ClientTransportType<Bifrost> &transport, Buffer &&message) {
		CryptoPP::BLAKE2b blake2b((uint)8);
		blake2b.Update((uint8_t *)message.data(), message.size());
		uint64_t message_id;
		blake2b.TruncatedFinal((uint8_t *)&message_id, 8);
		SPDLOG_INFO(
			"Transport {{ Src: {}, Dst: {} }}: Did recv message {}: {} bytes",
			transport.src_addr.to_string(),
			transport.dst_addr.to_string(),
			message_id,
			message.size()
		);

		ps->send_message_on_channel(
			channels[0],
			message_id,
			message.data(),
			message.size()
		);
	}

	void did_send_packet(ClientTransportType<Bifrost> &transport, Buffer &&message) {
		SPDLOG_INFO(
			"Transport {{ Src: {}, Dst: {} }}: Did send message: {} bytes",
			transport.src_addr.to_string(),
			transport.dst_addr.to_string(),
			message.size()
		);
	}

	void did_close(ClientTransportType<Bifrost> &) {
		lpft = nullptr;
	}

	void did_dial(ClientTransportType<Bifrost> &transport) {
		(void)transport;
		// transport.send(Buffer(new char[10], 10));
	}

	bool should_accept(SocketAddress const &) {
		return true;
	}

	void did_create_transport(ClientTransportType<Bifrost> &transport) {
		lpft  = &transport;
		transport.setup(this);
	}

	void manage_subscribers(
		std::string channel,
		typename marlin::pubsub::PubSubNode<Bifrost>::TransportSet& transport_set,
		typename marlin::pubsub::PubSubNode<Bifrost>::TransportSet& potential_transport_set) {

			// move some of the subscribers to potential subscribers if oversubscribed
			if (transport_set.size() >= DefaultMaxSubscriptions) {
				// insert churn algorithm here. need to find a better algorithm to give old bad performers a chance gain. Pick randomly from potential peers?
				// send message to removed and added peers

				marlin::pubsub::PubSubNode<Bifrost>::BaseTransport* toReplaceTransport = StreamTransportHelper::find_max_rtt_transport(transport_set);

				marlin::pubsub::PubSubNode<Bifrost>::BaseTransport* toReplaceWithTransport = StreamTransportHelper::find_random_rtt_transport(potential_transport_set);

				if (toReplaceTransport != nullptr &&
					toReplaceWithTransport != nullptr) {

					SPDLOG_INFO("Moving address: {} from subscribers to potential subscribers list on channel: {} ",
						toReplaceTransport->dst_addr.to_string(),
						channel);

					ps->remove_subscriber_from_channel(channel, *toReplaceTransport);
					ps->add_subscriber_to_potential_channel(channel, *toReplaceTransport);

					SPDLOG_INFO("Moving address: {} from potential subscribers to subscribers list on channel: {} ",
						toReplaceWithTransport->dst_addr.to_string(),
						channel);

					ps->remove_subscriber_from_potential_channel(channel, *toReplaceWithTransport);
					ps->add_subscriber_to_channel(channel, *toReplaceWithTransport);
				}
			}

			for (auto* pot_transport : potential_transport_set) {
				SPDLOG_INFO("Potential Subscriber: {}  rtt: {} on channel {}", pot_transport->dst_addr.to_string(), pot_transport->get_rtt(), channel);
			}

			for (auto* transport : transport_set) {
				SPDLOG_INFO("Subscriber: {}  rtt: {} on channel {}", transport->dst_addr.to_string(),  transport->get_rtt(), channel);
			}
	}
};

#endif // MARLIN_BIFROST_BIFROST_HPP
