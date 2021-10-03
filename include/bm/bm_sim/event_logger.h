/* Copyright 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

//! @file event_logger.h

#ifndef BM_BM_SIM_EVENT_LOGGER_H_
#define BM_BM_SIM_EVENT_LOGGER_H_

#include <bm/config.h>

#include <string>
#include <memory>

#include "device_id.h"
#include "phv_forward.h"
#include "transport.h"

namespace bm {

class Packet;

// Forward declarations of P4 object classes. This is ugly, but:
// 1) I don't have to worry about circular dependencies
// 2) If I decide to switch from id to name for msgs, I won't have to modify the
// EventLogger interface
class Parser;
class Deparser;
class MatchTableAbstract;
class MatchEntry;
class ActionFn;
struct ActionData;
class Conditional;
class Checksum;
class Pipeline;

using entry_handle_t = uint32_t;

//! Signals significant packet event by publishing a message on a
//! transport. This is meant to be used with the nanomsg PUB/SUB transport.
//! Other processes can subscribe to the channel to monitor the activity of the
//! switch (e.g. for logging). So far, we are mostly using this for the
//! end-to-end testing of target switch implementations. Note that depending on
//! the transport, some messages can be lost (the nanomsg PUB/SUB transport does
//! not guarantee delivery to all subscribers, and message drops will happen if
//! a subscriber is slower than the producer).
//!
//! Most messages are generated by the bmv2 code, but the target is for now
//! responsible of generated "packet in" and "packet out" messages (when a
//! packet is received / transmitted). Obviously, this is optional and you do
//! not have to do it if you are not interested in using the event logger.
class EventLogger {
 public:
  explicit EventLogger(std::unique_ptr<TransportIface> transport,
                       device_id_t device_id = 0)
      : transport_instance(std::move(transport)), device_id(device_id) { }

  // we need the ingress / egress ports, but they are part of the Packet
  //! Signal that a packet was received by the switch
  void packet_in(const Packet &packet);
  //! Signal that a packet was transmitted by the switch
  void packet_out(const Packet &packet);

  void parser_start(const Packet &packet, const Parser &parser);
  void parser_done(const Packet &packet, const Parser &parser);
  void parser_extract(const Packet &packet, header_id_t header);

  void deparser_start(const Packet &packet, const Deparser &deparser);
  void deparser_done(const Packet &packet, const Deparser &deparser);
  void deparser_emit(const Packet &packet, header_id_t header);

  void checksum_update(const Packet &packet, const Checksum &checksum);

  void pipeline_start(const Packet &packet, const Pipeline &pipeline);
  void pipeline_done(const Packet &packet, const Pipeline &pipeline);

  void condition_eval(const Packet &packet,
                      const Conditional &cond, bool result);
  void table_hit(const Packet &packet,
                 const MatchTableAbstract &table, entry_handle_t handle);
  void table_miss(const Packet &packet, const MatchTableAbstract &table);

  void action_execute(const Packet &packet,
                      const ActionFn &action_fn, const ActionData &action_data);

  void config_change();

  static EventLogger *get() {
    static EventLogger event_logger(TransportIface::make_dummy());
    return &event_logger;
  }

  static void init(std::unique_ptr<TransportIface> transport,
                   device_id_t device_id = 0) {
    get()->transport_instance = std::move(transport);
    get()->device_id = device_id;
  }

 private:
  std::unique_ptr<TransportIface> transport_instance{nullptr};
  device_id_t device_id{};
};

}  // namespace bm

//! Log an event with the event logger.
//! For example:
//! @code
//! BMELOG(packet_in, packet);
//! // packet processing
//! BMELOG(packet_out, packet);
//! @endcode
#ifdef BM_ELOG_ON
#define BMELOG(fn, ...) bm::EventLogger::get()->fn(__VA_ARGS__)
#else
#define BMELOG(fn, ...)
#endif

#endif  // BM_BM_SIM_EVENT_LOGGER_H_
