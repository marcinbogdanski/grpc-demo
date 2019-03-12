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


/*
 * This is heavily based on GRPC helloworld tutorial
 */

#include <iostream>
#include <fstream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include "messages.grpc.pb.h"

// Copied from:
// https://bobobobo.wordpress.com/2010/10/17/md5-c-implementation/
// and modified as per comment on LP64
#include "md5.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using messages::FileRequest;
using messages::FileChunk;
using messages::FileServer;

inline bool check_file_exists (const std::string& name) {
    std::ifstream f(name.c_str());
    return f.good();
}

// Boilerplate form helloworld grpc tutorial
class FileServerServiceImpl final : public FileServer::Service {
  Status GetFile(ServerContext* context, const FileRequest* request,
                  ServerWriter<FileChunk>* writer) override {

    // How big chunks to send over network
    const int BUFF_SIZE = 1024*1024; // 1MiB

    // Print stuff
    if(check_file_exists(request->filepath())){
      std::cout << "Requested file: " << request->filepath() << std::endl;
    }else{
      std::cout << "File " << request->filepath()
      << " does not exist :(" << std::endl;
      return Status::CANCELLED;
    }

    // Calculate MD5 sum on selected file
    MD5 md5;
    char* tmp = md5.digestFile(request->filepath().c_str());
    std::string md5str(tmp);
    // std::cout << md5str << std::endl;
    
    // Open file
    std::ifstream is(request->filepath(), std::ios::binary);
    if(is.is_open()){

      // Get file size
      is.seekg(0, is.end);
      int filesize = is.tellg();
      is.seekg(0, is.beg);

      // Read-buffer
      char* buffer = new char [BUFF_SIZE];

      FileChunk reply;
      reply.set_filemd5(md5str);     // MD5 sum,  whole file
      reply.set_filesize(filesize);  // size in bytes, whole file

      bool first = true;
      while(is.tellg() != -1){  // -1 on failure

        is.read(buffer, BUFF_SIZE);

        //std::cout << "gcount() " << is.gcount() << std::endl;
        //std::cout << "tellg() " << is.tellg() << std::endl;

        reply.set_data(buffer, is.gcount());
        writer->Write(reply);

        // if(is){
        //   std::cout << "all read ok" << std::endl;
        // }else{
        //   std::cout << "error reading" << std::endl;
        // }

        if (first){
          first = false;
          reply.clear_filemd5();   // no need to waste bandwith
          reply.clear_filesize();  // sending this over again (?)
        }

      }

      is.close();
      delete[] buffer;


    }else{
      std::cout << "Could not open file: " << request->filepath() << std::endl;
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
