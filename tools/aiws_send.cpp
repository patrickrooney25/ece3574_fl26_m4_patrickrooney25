#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

//client- a CLI utility

namespace {

//resolve default socket location
std::string get_default_socket_path(){
    const char* user= std::getenv("USER");
    if(user!=nullptr&&*user!='\0'){
        return "/tmp/aiws-" +std::string(user) +".sock";
    }
    return "/tmp/aiws-"+std::to_string(getuid()) + ".sock";
}

//loops over write to ensure all bytes are sent
bool send_all (int fd, const void* data, size_t length){
    const auto* ptr = static_cast<const char*>(data);
    size_t remaining =length;
    while(remaining>0){
        ssize_t written = write(fd, ptr, remaining);
        if(written <=0){
            return false;
        }
        ptr+=written;
        remaining-=static_cast<size_t>(written);
    }
    return true;
}

//loops over bytes
bool read_all(int fd, void* data, size_t length){
    auto* ptr = static_cast<char*>(data);
    size_t remaining=length;
    while(remaining>0){
        ssize_t bytes_read = read(fd, ptr, remaining);
        if(bytes_read <=0){
            return false;
        }
        ptr+=bytes_read;
        remaining-=static_cast<size_t>(bytes_read);
    }
    return true;
}

} //namepsace

int main(int argc, char* argv[]){
    std::string socket_path =get_default_socket_path();
    std::string file_path;

    //parse arguments
    for(int i=1; i<argc; i++){
        std::string arg=argv[i];
        if (arg=="--socket"){
            if(i+1>=argc){
                std::cerr << "Error: --socket requires a path argument\n";
                return 1; //invalid argument
            }
            socket_path = argv[++i];
        }
        else if(arg=="add"){
            if(i+1>=argc){
                std::cerr << "Error: add requires a FILE argument\n";
                return 1; //missing file
            }
            file_path = argv[++i];
        }
        else{
            std::cerr << "Error: unknown argument: " << arg << "\n";
            return 1; //unkown option
        }
    }
    if(file_path.empty()){
        std::cerr << "Error: no file specified. \n";
        return 1; //rejected doc for being empty
    }

    //normalize path for compatability
    std::string abs_path;
    try{
        abs_path = std::filesystem::absolute(file_path).lexically_normal().string();
    } catch(...){
        abs_path = file_path;
    }


    //create posix unix domain streaming socket
    int fd= socket(AF_UNIX, SOCK_STREAM, 0);
    if(fd<0){
        return 2; //failed socket creation
    }

    sockaddr_un addr{};
    addr.sun_family=AF_UNIX;
    if(socket_path.size()>=sizeof(addr.sun_path)){
        std::cerr << "Error: socket path too long: " << socket_path << "\n";
        close(fd);
        return 2; //connection error
    }
    std::strncpy(addr.sun_path, socket_path.c_str(), sizeof(addr.sun_path) -1);

    //connect within 2 seconds
    const auto connect_start = std::chrono::steady_clock::now();
    bool connected=false;
    while(true){
        if(connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr))==0){
            connected =true;
            break;
        }
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - connect_start);
        if(elapsed.count()>=2000){
            break;
        }
        usleep(50000);//50ms for retrying
    }
    if(!connected){
        close(fd);
        return 2; //unreachable server or timeout
    }

    //5 second timeout
    struct timeval tv{};
    tv.tv_sec= 5;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    //send 4 byte network endian path length with string
    uint32_t path_len=htonl(static_cast<uint32_t>(abs_path.size()));
    if(!send_all(fd, &path_len, sizeof(path_len))||!send_all(fd, abs_path.data(), abs_path.size())){
        close(fd);
        return 2;
    }

    //wire protocol to read 1 byte status
    uint8_t status_byte=0;
    if(!read_all(fd, &status_byte, sizeof(status_byte))){
        close(fd);
        return 2;
    }
    //wire protocol to read 4 byte reason
    uint32_t reason_len_net=0;
    if(!read_all(fd, &reason_len_net, sizeof(reason_len_net))){
        close(fd);
        return 2;
    }
    uint32_t reason_len = ntohl(reason_len_net);

    //read error string explanation
    std::string reason;
    if(reason_len>0){
        reason.resize(reason_len);
        if(!read_all(fd, &reason[0], reason_len)){
            close(fd);
            return 2;
        }
    }
    close(fd);

    //read rejection
    if(status_byte==1){
        if(!reason.empty()){
            std::cerr << reason << "\n";
        }
        else{
            std::cerr << "File rejected by application.\n";
        }
        return 1;
    }
    return 0;//file accepted
}