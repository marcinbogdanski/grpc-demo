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


// Copied from:
// https://bobobobo.wordpress.com/2010/10/17/md5-c-implementation/
// and modified as per comment on LP64
#include "md5.h"


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
  std::string GetFile(const std::string& local_path,
    const std::string& remote_path) {
    
    // Data we are sending to the server.
    FileRequest request;
    request.set_filepath(remote_path);



    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    std::unique_ptr<ClientReader<FileChunk> > reader(
      stub_->GetFile(&context, request));

    std::string temp_local_path = local_path + ".part";
    std::ofstream os(temp_local_path, std::ios::binary);
    if(os.is_open()){

      FileChunk reply;

      int filesize;
      std::string filemd5;

      bool first = true;
      while(reader->Read(&reply)){

        if (first){
          first = false;
          filesize = reply.filesize();  // TODO: check if enough space on HDD
          filemd5 = reply.filemd5();
        }

        std::cout << "received: "
        << "data len: " << reply.data().length()
        << "filesize: " << reply.filesize()
        << "md5: " << reply.filemd5() << std::endl;

        os.write(reply.data().c_str(), reply.data().length());

      }

      os.close();

      // Check md5 on written file
      MD5 md5;
      char* tmp = md5.digestFile(temp_local_path.c_str());
      std::string md5str(tmp);
      std::cout << md5str << std::endl;
      std::cout << filemd5 << std::endl;

      if(filemd5 == md5str){
        // md5 check OK on inbound file
        std::rename(temp_local_path.c_str(), local_path.c_str());
        std::cout << "Download completed" << std::endl;
      }else{
        // md5 check failed
        std::cout << "MD5 check on inbound file failed." << std::endl;
      }


    }else{
      std::cout << "Could not open file: " << local_path << std::endl;
    }


    // while(reader->Read(&reply)){
    //   std::cout << reply.filemd5() << std::endl;
    //   std::cout << reply.data().length() << std::endl;
    // }





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
  std::string local_path("temp.zip");
  std::string remote_path("/home/user/dronex-ai_2018.09.06.zip");
  std::string reply = greeter.GetFile(local_path, remote_path);
  std::cout << "FileServer received: " << reply << std::endl;

  return 0;
}
