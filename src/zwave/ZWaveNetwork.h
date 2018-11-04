#pragma once

#include <set>
#include <string>
#include <vector>

#include <Poco/SharedPtr.h>
#include <Poco/Timespan.h>

#include "zwave/ZWaveNode.h"

namespace BeeeOn {

/**
 * @brief ZWaveNetwork is an interface to a real Z-Wave network.
 *
 * It provides just high-level operations:
 *
 * - start and cancel of the inclusion process
 * - start and cancel of the node removal process
 * - access to high-level events by polling
 */
class ZWaveNetwork {
public:
	typedef Poco::SharedPtr<ZWaveNetwork> Ptr;

	/**
	 * @brief Representation of events reported by the ZWaveNetwork
	 * implementation via the call pollEvent().
	 */
	class PollEvent {
	public:
		enum Type {
			/**
			 * @brief Dummy, nothing happens. It might come
			 * when interrupted for some reason (termination).
			 * It can be just a spurious wakeup.
			 */
			EVENT_NONE = 0,

			/**
			 * @brief A new Z-Wave node has been detected.
			 * There might be incomplete information about it.
			 * Use method PollEvent::node() to access it.
			 */
			EVENT_NEW_NODE = 1,

			/**
			 * @brief A Z-Wave node's information has been
			 * updated. Use method PollEvent::node() to access it.
			 */
			EVENT_UPDATE_NODE = 2,

			/**
			 * @brief A Z-Wave node has been removed from the Z-Wave
			 * network. Use method PollEvent::node() to access it.
			 */
			EVENT_REMOVE_NODE = 3,

			/**
			 * @brief Received data from a Z-Wave node. Use method
			 * PollEvent::value() to access it.
			 */
			EVENT_VALUE = 4,

			/**
			 * @brief Z-Wave inclusion process has started.
			 */
			EVENT_INCLUSION_START = 5,

			/**
			 * @brief Z-Wave inclusion process has stopped.
			 */
			EVENT_INCLUSION_DONE = 7,

			/**
			 * @brief Z-Wave node removal process has started.
			 */
			EVENT_REMOVE_NODE_START = 8,

			/**
			 * @brief Z-Wave node removal process has stopped.
			 */
			EVENT_REMOVE_NODE_DONE = 9,

			/**
			 * @brief All available Z-Wave nodes have been queried.
			 */
			EVENT_READY = 10,
		};

		PollEvent();
		~PollEvent();

		static PollEvent createNewNode(const ZWaveNode &node);
		static PollEvent createUpdateNode(const ZWaveNode &node);
		static PollEvent createRemoveNode(const ZWaveNode &node);
		static PollEvent createValue(const ZWaveNode::Value &value);
		static PollEvent createInclusionStart();
		static PollEvent createInclusionDone();
		static PollEvent createRemoveNodeStart();
		static PollEvent createRemoveNodeDone();
		static PollEvent createReady();

		bool isNone() const
		{
			return type() == EVENT_NONE;
		}

		Type type() const;
		const ZWaveNode &node() const;
		const ZWaveNode::Value &value() const;

		std::string toString() const;

	protected:
		PollEvent(Type type);
		PollEvent(Type type, const ZWaveNode &node);
		PollEvent(const ZWaveNode::Value &value);

	private:
		Type m_type;
		Poco::SharedPtr<ZWaveNode> m_node;
		Poco::SharedPtr<ZWaveNode::Value> m_value;
	};

	virtual ~ZWaveNetwork();

	/**
	 * @brief Poll for new events in the ZWaveNetwork.
	 *
	 * The call is blocking or non-blocking based on
	 * the given timeout.
	 */
	virtual PollEvent pollEvent(
		const Poco::Timespan &timeout) = 0;

	/**
	 * @brief Starts the Z-Wave network node inclusion process.
	 *
	 * The call is non-blocking.
	 */
	virtual void startInclusion() = 0;

	/**
	 * @brief Cancel inclusion if it is running.
	 */
	virtual void cancelInclusion() = 0;

	/**
	 * @brief Start node removal process in the Z-Wave network.
	 *
	 * The call is blocking.
	 */
	virtual void startRemoveNode() = 0;

	/**
	 * @brief Cancel remove node if it is running.
	 */
	virtual void cancelRemoveNode() = 0;

	/**
	 * @brief Interrupt any blocking calls currently in progress.
	 */
	virtual void interrupt() = 0;

	/**
	 * @brief Post the given value into the Z-Wave network. There is no implicit
	 * feedback about the result status.
	 *
	 * The method might throw Poco::NotImplementedException in case of setting
	 * unsupported values or if not supported by the backend.
	 */
	virtual void postValue(const ZWaveNode::Value&) = 0;

};

}
