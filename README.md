This is heavily hacked __helloworld__ example from [GRCP tutorials](https://grpc.io/docs/quickstart/cpp.html).

Functinality:
* __insecure__: gives unauthenticated access to your hard drive!
* download a file from server
* check MD5 sum after download
* tested only on loopback interface and Ubuntu 18.04

TODO:
* handle network interrupts (?)

Run server:
```
./server
```

In another terminal:
```
./client /path/to/remote/file /path/to/local/file
```

With a bit of luck it should download file specified
