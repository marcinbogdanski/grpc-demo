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


using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;
using messages::FileRequest;
using messages::FileChunk;
using messages::FileServer;

// Boilerplate from grpc helloworld c++ tutorial
class FileServerClient {
 public:
  FileServerClient(std::shared_ptr<Channel> channel)
      : stub_(FileServer::NewStub(channel)) {}

  void GetFile(const std::string& local_path,
    const std::string& remote_path) {
    
    // Request a file at specified path
    FileRequest request;
    request.set_filepath(remote_path);

    // Get stream reader
    ClientContext context;
    std::unique_ptr<ClientReader<FileChunk> > reader(
      stub_->GetFile(&context, request));



    // Open local file, add ".part" suffix for incomplete download
    // Then remove ".part" when MD5 is verified after successful download
    std::string temp_local_path = local_path + ".part";
    std::ofstream os(temp_local_path, std::ios::binary);
    if(os.is_open()){

      FileChunk reply;
      unsigned long filesize;
      std::string filemd5;
      unsigned long bytesrecv = 0;

      bool first = true;
      while(reader->Read(&reply)){

        // Get header info, this is part of first chunk only
        if (first){
          first = false;
          filesize = reply.filesize();  // TODO: check if enough space on HDD
          filemd5 = reply.filemd5();
        }

        // std::cout << "received: "
        // << "data len: " << reply.data().length()
        // << "filesize: " << reply.filesize()
        // << "md5: " << reply.filemd5() << std::endl;

        bytesrecv += reply.data().length();

        // Write temp file
        os.write(reply.data().c_str(), reply.data().length());

      }
      os.close();

      if (bytesrecv == 0){
        std::cout << "Nothing received :(" << std::endl;
        std::remove(temp_local_path.c_str());
        return;
      }

      if (bytesrecv != filesize){
        std::cout << "Incomplete download :(" << std::endl;
        std::remove(temp_local_path.c_str());
        return;
      }

      // Check md5 on written file
      MD5 md5;
      char* tmp = md5.digestFile(temp_local_path.c_str());
      std::string md5str(tmp);

      // Check if MD5 ok, rename inbound file, print info
      if(filemd5 == md5str){
        // md5 check OK on inbound file
        std::rename(temp_local_path.c_str(), local_path.c_str());
        std::cout << "Got " << filesize << " bytes. Saved as "
                  << local_path << std::endl;
      }else{
        // md5 check failed
        std::cout << "MD5 check on inbound file failed :(" << std::endl;
      }

    }else{
      // Error if failed to open temp file
      std::cout << "Could not open file: " << local_path << " :(" << std::endl;
    }


  }

 private:
  std::unique_ptr<FileServer::Stub> stub_;
};



int main(int argc, char** argv) {

  // Get console parameters
  if (argc != 3){
    std::cout << "Usage: client REMOTE_PATH LOCAL_PATH" << std::endl;
    std::cout << "Server must be already running" << std::endl;
    return 1;
  }

  // Download single file
  FileServerClient greeter(grpc::CreateChannel(
      "localhost:50051", grpc::InsecureChannelCredentials()));
  std::string remote_path(argv[1]);
  std::string local_path(argv[2]);
  greeter.GetFile(local_path, remote_path);

  return 0;
}
