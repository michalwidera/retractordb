#include "qry.hpp"

#include <chrono>
#include <iostream>
#include <thread>

#include <boost/json.hpp>

#include "constants.hpp"

namespace {
namespace json = boost::json;
using boost::property_tree::ptree;

json::object event(std::string_view type) { return {{"version", 1}, {"event", type}}; }

void emit(const json::object &value) { std::cout << json::serialize(value) << '\n' << std::flush; }

json::object schemaEvent(const ptree &pt) {
  auto result      = event("schema");
  result["stream"] = pt.get<std::string>("db.stream");
  result["delta"]  = pt.get<std::string>("db.duration");
  result["query"]  = pt.get<std::string>("db.processed_line");
  json::array fields;
  for (const auto &[key, field] : pt.get_child("db.field")) {
    const auto name = field.get_value<std::string>();
    fields.push_back(json::object{{"name", name},
                                  {"type", pt.get<std::string>("db.field_type." + name)},
                                  {"count", pt.get<int>("db.field_count." + name)}});
  }
  result["fields"] = std::move(fields);
  return result;
}
}  // namespace

int qry::jsonCommand(const std::string &command, const std::string &input, int limit, int idleTimeoutMs) {
  auto fail = [](std::string_view code, std::string_view message) {
    auto result       = event("error");
    result["code"]    = code;
    result["message"] = message;
    emit(result);
    return 1;
  };
  auto verdict = [&](selectResult result) {
    const char *code = "server_no_response";
    switch (result) {
      case selectResult::streamNotFound:
        code = "stream_not_found";
        break;
      case selectResult::noActivePlan:
        code = "no_active_plan";
        break;
      case selectResult::serverStopping:
        code = "server_stopping";
        break;
      default:
        break;
    }
    return fail(code, toString(result));
  };
  try {
    if (command == "hello") {
      if (hello() != 0) return fail("server_no_response", "Server did not answer hello");
      emit(event("pong"));
      return 0;
    }
    if (command == "dir") {
      const auto pt = netClient("get", "");
      if (const auto error = pt.get_optional<std::string>("error.response")) return fail("server_no_response", *error);
      auto result = event("streams");
      json::array streams;
      if (const auto nodes = pt.get_child_optional("db.stream")) {
        for (const auto &[key, stream] : *nodes)
          streams.push_back(
              json::object{{"name", stream.get_value<std::string>()}, {"delta", stream.get<std::string>("duration")}});
      } else if (pt.get<std::string>("db", "") != constants::kNoActivePlanReply) {
        return fail("server_stopping", "Server did not provide a stream list");
      }
      result["streams"] = std::move(streams);
      emit(result);
      return 0;
    }
    const auto detail = detailNode(input);
    if (!detail) return verdict(detail.error());
    auto schema = schemaEvent(*detail);
    if (command == "detail") {
      emit(schema);
      return 0;
    }
    const auto response = netClient("show", input);
    if (const auto error = response.get_optional<std::string>("error.response")) return fail("client_queue_missing", *error);
    // Destruktor zatrzymuje producenta takze przy wyjatku serializacji lub IPC.
    std::jthread producer([this](const std::stop_token &stop) {
      std::stop_callback stopProducer(stop, [this] { transport_->done = true; });
      transport_->producer();
    });
    emit(schema);
    auto lastData = std::chrono::steady_clock::now();
    int received  = 0;
    std::string reason;
    while (reason.empty()) {
      ptree row;
      if (transport_->popQueue(row)) {
        const auto stream = row.get<std::string>("stream");
        if (stream == constants::Reserved_id_oob) {
          reason = "server_stopped_or_reloaded";
          break;
        }
        if (stream != input) return fail("protocol_error", "Unexpected stream");
        auto record      = event("record");
        record["stream"] = stream;
        json::array values;
        const auto nullmap = row.get<std::string>("nullmap");
        for (int i = 0; i < row.get<int>("count"); ++i) {
          if (Formatter::isNullAt(nullmap, i))
            values.push_back(nullptr);
          else
            values.push_back(json::value(row.get<std::string>(std::to_string(i))));
        }
        record["values"] = std::move(values);
        emit(record);
        lastData = std::chrono::steady_clock::now();
        if (limit > 0 && ++received == limit) reason = "limit";
      } else {
        if (transport_->done)
          return fail(transport_->responseQueueMissing ? "client_queue_missing" : "disconnected", "IPC receiver stopped");
        if (idleTimeoutMs > 0 && std::chrono::steady_clock::now() - lastData >= std::chrono::milliseconds(idleTimeoutMs))
          return fail("idle_timeout", "No stream data within the idle timeout");
        std::this_thread::sleep_for(ipc::kQueuePollInterval);
      }
    }
    auto end      = event("end");
    end["reason"] = reason;
    emit(end);
    return 0;
  } catch (const std::exception &error) {
    return fail("communication_error", error.what());
  }
}
