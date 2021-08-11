#include "shm_buffer.h"
#include "data.pb.h"

using namespace shm_buffer;

int lastCommand;

void dataCallback(const DataPackage* dataPackage)
{
    auto msg_received = std::make_shared<data::DataTest>();
    msg_received->ParseFromArray(dataPackage->address,dataPackage->size);

    if(lastCommand != msg_received->command() - 1)
    {
        cout << "Got Command Error!!! " << getTime(getTimeStamp()) << " lastCommand: " << lastCommand << " Got command: " << msg_received->command() << endl;
    }

    lastCommand = msg_received->command();

    cout << "dataCallback " << getTime(getTimeStamp()) << " source: " << msg_received->source() << " command: " << msg_received->command() << " param: " << msg_received->param() << " ret: " << msg_received->ret() << " size: " << dataPackage->size << endl;
}


int main()
{
    cout << "Hello BufferConsumer3!" << endl;

    ShmBuffer testBuffer(BufferConsumer3,6000);
    testBuffer.registCallback(dataCallback);


    while(1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return 0;
}
