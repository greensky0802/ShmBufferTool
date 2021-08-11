#include "shm_buffer.h"
#include "data.pb.h"
#include <malloc.h>

using namespace shm_buffer;

int main()
{
    cout << "Hello BufferProducer!" << endl;

    ShmBuffer testBuffer(BufferProducer,6000);



    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    for(int i = 0; i < 20; i++)
    {
        int count = 0;

        while(1)
        {
            auto msg = std::make_shared<data::DataTest>();
            msg->set_source("GREENSKY");
            msg->set_command(count);
            msg->set_param(3.1415*count);
            msg->set_ret(1);
            int size = msg->ByteSizeLong();

            void *buffer = malloc(size);
            msg->SerializeToArray(buffer,size);
            testBuffer.writeData(buffer,size);

            cout << "Data write count: " << count++ << " " << getTime(getTimeStamp()) << endl;

            free(buffer);

            if(count >= 20000)
                break;

            free(buffer);

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    return 0;
}
