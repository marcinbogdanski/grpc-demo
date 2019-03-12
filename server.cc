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
#include <fstream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include "messages.grpc.pb.h"


using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using messages::FileRequest;
using messages::FileChunk;
using messages::FileServer;

// Logic and data behind the server's behavior.
class FileServerServiceImpl final : public FileServer::Service {
  Status GetFile(ServerContext* context, const FileRequest* request,
                  ServerWriter<FileChunk>* writer) override {
    
    FileChunk reply;

    std::cout << "Requested file: " << request->filepath() << std::endl;

    std::ifstream is(request->filepath(), std::ios::binary);
    if(is.is_open()){
      std::cout << "File opened successfuly" << std::endl;

      is.seekg(0, is.end);
      int filesize = is.tellg();
      is.seekg(0, is.beg);

      char* buffer = new char [filesize];

      while(is.tellg() != -1){  // -1 on failure

        is.read(buffer, 1024*1024);

        std::cout << "gcount() " << is.gcount() << std::endl;
        std::cout << "tellg() " << is.tellg() << std::endl;

        if(is){
          std::cout << "all read ok" << std::endl;
        }else{
          std::cout << "error reading" << std::endl;
        }

      }

      is.close();
      delete[] buffer;


    }else{
      std::cout << "Could not open file: " << request->filepath() << std::endl;
    }

    std::string prefix("Hello ");
    for (int i = 0; i < 4; i++){
      reply.set_filemd5(prefix + request->filepath());
      reply.set_data("ala ma kota", 5);
      writer->Write(reply);
    }

    return Status::OK;
  }
};

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  FileServerServiceImpl service;

  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {
  RunServer();

  return 0;
}
