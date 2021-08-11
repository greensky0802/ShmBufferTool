#include "shm_buffer.h"
#include "data.pb.h"

using namespace shm_buffer;
using namespace std::placeholders;

class DataCollector
{
    public:
        DataCollector():testBuffer(BufferConsumer2,6000)
        {
            cout << "DataCollector ctor" << endl;
        }

        ~DataCollector()
        {
            cout << "DataCollector dtor" << endl;
        }

        void Init()
        {
//                testBuffer.registCallback(&DataCollector::dataCallback,this);
            Fun fun = std::bind(&DataCollector::dataCallback,this,_1);//通过仿函数完成类成员函数的回调注册
            testBuffer.registCallback(fun);
        }

        void dataCallback(const DataPackage* dataPackage)
        {
//            DataCollector *dataCollector = (DataCollector*)context;

            auto msg_received = std::make_shared<data::DataTest>();
            msg_received->ParseFromArray(dataPackage->address,dataPackage->size);

            if(lastCommand != msg_received->command() - 1)
            {
                cout << "Got Command Error!!! " << getTime(getTimeStamp()) << " lastCommand: " << lastCommand << " Got command: " << msg_received->command() << endl;
            }

            lastCommand = msg_received->command();

//            cout << "dataCallback " << getTime(getTimeStamp()) << " source: " << msg_received->source() << " command: " << msg_received->command() << " param: " << msg_received->param() << " ret: " << msg_received->ret() << " size: " << dataPackage->size << endl;
        }


    private:

        shm_buffer::ShmBuffer testBuffer;

        int lastCommand;

};

int main()
{
    cout << "Hello BufferConsumer2!" << endl;

    DataCollector dataCollector;
    dataCollector.Init();

    while(1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return 0;
}
