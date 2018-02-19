#include "strawpoll.hpp"
#include <uWS.h>

#include <iostream> // DEBUG

void sendResponse(uWS::WebSocket<uWS::SERVER>* ws, FlatBufferRef buffer)
{
  ws->send(reinterpret_cast<const char*>(buffer.data), buffer.size, uWS::OpCode::BINARY);
}

void broadcastResponse(uWS::Hub& h, FlatBufferRef buffer)
{
  h.getDefaultGroup<uWS::SERVER>().broadcast(reinterpret_cast<const char*>(buffer.data), buffer.size, uWS::OpCode::BINARY);
}

int main(int argc, char* argv[])
{
  const int port = (argc > 1) ? std::atoi(argv[1]) : 3003;

  std::cout << "Sever starting up...\n";

  PollData<VoteGuardNoCheck<EmptyAddress>> poll_data{};

  uWS::Hub h;

  h.onMessage([&h, &poll_data](uWS::WebSocket<uWS::SERVER>* ws, char* message, size_t length, uWS::OpCode) {
    const bool ok = flatbuffers::Verifier(reinterpret_cast<const uint8_t*>(message), length)
                                .VerifyBuffer<Strawpoll::Request>(nullptr);
    if (!ok) {
      sendResponse(ws, poll_data.error_responses.invalid_message);
      return;
    }

    const auto request = flatbuffers::GetRoot<Strawpoll::Request>(message);
    switch (request->type()) {
      case Strawpoll::RequestType_Poll:
        sendResponse(ws, poll_data.poll_response());
        break;
      case Strawpoll::RequestType_Result:
        poll_data.register_vote(request->vote(), {},
                                [&ws](FlatBufferRef br) { sendResponse(ws, br); },
                                [&h] (FlatBufferRef br) { broadcastResponse(h, br); }
        );
        break;
      default:
        sendResponse(ws, poll_data.error_responses.invalid_type);
    }
  });

  // connect to port and exit if blocked
  if (!h.listen(port)) return 1;
  std::cout << "Listening on port "<< port <<"\n";

  h.run();
}
