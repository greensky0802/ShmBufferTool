#include "shm_buffer.h"
#include "data.pb.h"
#include <malloc.h>

using namespace shm_buffer;

int main()
{
    cout << "Hello BufferConsumer1!" << endl;

    ShmBuffer testBuffer(BufferConsumer1,6000);

    int lastCommand = -1;

    while(1)
    {
        while(testBuffer.isReadyToRead())
        {
            int size = testBuffer.getBufferSize();
            void *buffer = malloc(size);
            testBuffer.readData(buffer);
            auto msg_received = std::make_shared<data::DataTest>();
            msg_received->ParseFromArray(buffer,size);
//            cout << "msg_received " << getTime(getTimeStamp()) << " source: " << msg_received->source() << " command: " << msg_received->command() << " param: " << msg_received->param() << " ret: " << msg_received->ret() << " size: " << testBuffer.getBufferSize() << endl;

            if(lastCommand != msg_received->command() - 1)
            {
                cout << "Got Command Error!!! " << getTime(getTimeStamp()) << " lastCommand: " << lastCommand << " Got Command: " << msg_received->command() << endl;
            }

            lastCommand = msg_received->command();

            free(buffer);
        }
//        cout << "Waiting... " << getTime(getTimeStamp()) << endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    return 0;
}
