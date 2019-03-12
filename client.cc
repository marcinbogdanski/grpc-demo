/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include "messages.grpc.pb.h"


using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;
using messages::FileRequest;
using messages::FileChunk;
using messages::FileServer;

class FileServerClient {
 public:
  FileServerClient(std::shared_ptr<Channel> channel)
      : stub_(FileServer::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  std::string GetFile(const std::string& path) {
    // Data we are sending to the server.
    FileRequest request;
    request.set_filepath(path);

    // Container for the data we expect from the server.
    FileChunk reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    std::unique_ptr<ClientReader<FileChunk> > reader(
      stub_->GetFile(&context, request));

    while(reader->Read(&reply)){
      std::cout << reply.filemd5() << std::endl;
      std::cout << reply.data().length() << std::endl;
    }





    /*
    // The actual RPC.
    Status status = stub_->GetFile(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      return reply.filemd5();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
    */
  }

 private:
  std::unique_ptr<FileServer::Stub> stub_;
};

int main(int argc, char** argv) {
  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint (in this case,
  // localhost at port 50051). We indicate that the channel isn't authenticated
  // (use of InsecureChannelCredentials()).
  FileServerClient greeter(grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials()));
  std::string path("/home/user/dronex-ai_2018.09.06.zip");
  std::string reply = greeter.GetFile(path);
  std::cout << "FileServer received: " << reply << std::endl;

  return 0;
}
