#ifndef PROJECT_SHM_BUFFER_H
#define PROJECT_SHM_BUFFER_H

#include <iostream>
#include <thread>
#include <functional>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <atomic>


using namespace std;

#define QUEUE_SIZE 20


namespace shm_buffer
{
//    typedef void (*CallbackFun)(void* buffer,int size);
//    typedef std::function<void(const void*,const int)> Fun;

    //共享内存读写标志定义
    enum ShmAccessFlag
    {
        StateNULL,StateWriting,StateReady,StateReadDone,
        NUM_ShmAccessFlag
    };

    //共享内存访问对象定义
    enum BufferAccessObj
    {
        BufferConsumer1,BufferConsumer2,BufferConsumer3,BufferConsumer4,
        BufferProducer,
        NUM_BufferObj
    };

    typedef struct _DataPackage
    {
        void* address;
        int size;

    }DataPackage;

    typedef std::function<void(const DataPackage*)> Fun;

    typedef struct _ShareDataObj
    {
        unsigned char buffer[50];
        int buffer_size;
//        std::string string;
//        std::atomic_flag lock = ATOMIC_FLAG_INIT;
        std::atomic_int ready_flag[NUM_BufferObj-1];
//        int ready_read_flag[NUM_BufferObj-1];

    }ShareDataObj;

    typedef struct _ShareBuffer
    {
        ShareDataObj shareDataArray[QUEUE_SIZE];
        std::atomic_int current_ready_index;

    }ShareBuffer;

    std::time_t getTimeStamp()
    {
        std::chrono::time_point<std::chrono::system_clock,std::chrono::milliseconds> tp = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
        auto tmp=std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
        std::time_t timestamp = tmp.count();
        //std::time_t timestamp = std::chrono::system_clock::to_time_t(tp);
        return timestamp;
    }

    std::string getTime(int64_t timestamp)
    {
        int64_t milli = timestamp+ (int64_t)8*60*60*1000;//此处转化为东八区北京时间，如果是其它时区需要按需求修改
        auto mTime = std::chrono::milliseconds(milli);
        auto tp=std::chrono::time_point<std::chrono::system_clock,std::chrono::milliseconds>(mTime);
        auto tt = std::chrono::system_clock::to_time_t(tp);
        std::tm* now = std::gmtime(&tt);
        char str[100];
//        std::string time = " " + now->tm_year+1900 + "-" + now->tm_mon+1 + "-" + now->tm_mday + "_" + now->tm_hour + "_" + now->tm_min + "_" + now->tm_sec + "_" + milli%1000;
        sprintf(str,"%4d-%02d-%02d_%02d:%02d:%02d.%ld",now->tm_year+1900,now->tm_mon+1,now->tm_mday,now->tm_hour,now->tm_min,now->tm_sec,milli%1000);
        std::string time = str;
        return time;
    }

    class ShmBuffer
    {
        public:
            ShmBuffer(BufferAccessObj obj_name,int buffer_id)
            {
                cout << "ShmBuffer ctor obj_name: " << obj_name << " buffer_id: " << buffer_id << endl;
                access_obj_ = obj_name;
                buffer_id_ = buffer_id;

                shmid_buffer = shmget(buffer_id_, sizeof(ShareBuffer), 0666|IPC_CREAT);
                if(shmid_buffer < 0)
                {
                    cout << "Init shmid_buffer key " << shmid_buffer << " Error!!! " << endl;
                }
                else
                {
                    cout << "Init shmid_buffer key " << shmid_buffer << " Done " << endl;

                    shm_buffer = shmat(shmid_buffer, (void*)0, 0);

                    pShareBuffer = static_cast<ShareBuffer*>(shm_buffer);

                }

                if(access_obj_ == BufferProducer)
                {
                    initProducerBuffer();//生产者启动后对于缓冲区进行一次完整的置空初始化
                }
                else
                {
                    initConsumerBuffer();//消费者启动后对缓冲区中已读标志进行清除
                }
            }

            void initProducerBuffer()
            {
                pShareBuffer->current_ready_index.store(-1,std::memory_order_release);//-1表示回环缓冲区中未写入数据

                for(int i = 0; i < QUEUE_SIZE; i++)
                {
//                    pShareBuffer->shareDataArray[i].ready_read_flag[i] = StateNULL;
                    changeBufferIOStateALL(i,StateNULL);//生产者初始化时将全部ready_flag设置为StateNULL，消费者不能再次初始化
                }

                BufferWriteIndex = 0;//生产者当前写入位置, 生产者重启后固定从位置0开始写

                cout << "ShmBuffer Data Init Done!!!" << endl;
            }

            void initConsumerBuffer()
            {
                if(pShareBuffer->current_ready_index.load(std::memory_order_acquire) != -1)//说明消费者重启前，生产者已经写入了数据
                {
                    int current_ready_index_temp = pShareBuffer->current_ready_index.load(std::memory_order_acquire);
#if 0

                    int current_write_index = current_ready_index_temp + 1;//当前正在写的位置是current_ready+1

                    if(current_write_index >= QUEUE_SIZE)
                    {
                        current_write_index = 0;
                    }

                    for(int i = current_write_index; i < QUEUE_SIZE; i++)
                    {
//                        if(pShareBuffer->shareDataArray[i].ready_flag[access_obj_].load(std::memory_order_acquire) == StateReadDone)
//                        {
//                            pShareBuffer->shareDataArray[i].ready_flag[access_obj_].store(StateReady,std::memory_order_release);
//                        }
                        pShareBuffer->shareDataArray[i].ready_flag[access_obj_].store(StateNULL,std::memory_order_release);//将从current_write_index开始之后的全部标志改为StateNULL，因为此时存储的数据均是生产者上一轮写入的，需要丢弃
                    }
#endif
                    for(int i = 0; i < QUEUE_SIZE; i++)
                    {
                        if(i == current_ready_index_temp)
                        {
                            continue;
                        }
                        else
                        {
                            pShareBuffer->shareDataArray[i].ready_flag[access_obj_].store(StateNULL,std::memory_order_release);//将current_ready_index之外的全部标志改为StateNULL
                        }
                    }

                    BufferReadIndex = current_ready_index_temp;//设置消费者读取生产者最新写入数据
//                    BufferReadIndex = 0;
                    cout << "initConsumerBuffer BufferReadIndex: " << BufferReadIndex << endl;
                }
                else
                {
                    BufferReadIndex = 0;//此处说明生产者还未写入数据，则消费者从0开始读取
                }
            }



//添加了回环缓冲功能的producer不需要判定buffer是否可写，有需要时直接写入
//            bool isReadyToWrite()
//            {
//                if((pShareBuffer->shareDataArray[BufferWriteIndex].ready_flag[0].load(std::memory_order_acquire) == StateReadDone) || (pShareBuffer->ready_flag.load(std::memory_order_acquire) == StateNULL))
//                {
//                    return true;
//                }
//                else
//                {
//                    return false;
//                }
//            }

            bool isReadyToRead()
            {
//                cout << "current_ready_index: " << pShareBuffer->current_ready_index << " BufferReadIndex: " << BufferReadIndex << " access_obj_: " << (int)access_obj_ << " ready_flag: "<< pShareBuffer->shareDataArray[BufferReadIndex].ready_flag[access_obj_].load(std::memory_order_acquire) << endl;
//                cout << "current_ready_index: " << pShareBuffer->current_ready_index << " BufferReadIndex: " << BufferReadIndex << " access_obj_: " << (int)access_obj_ << " ready_read_flag: "<< pShareBuffer->shareDataArray[BufferReadIndex].ready_read_flag[access_obj_] << endl;
                if(pShareBuffer->current_ready_index.load(std::memory_order_acquire) != -1)
                {
//                    cout << "before current_ready_index: " << pShareBuffer->current_ready_index << " BufferReadIndex: " << BufferReadIndex << endl;

//                    if(pShareBuffer->current_ready_index > BufferReadIndex)//当前写入index > 当前已读Index 极端情况 current 9  read 0
//                    {
//                        if(pShareBuffer->current_ready_index - BufferReadIndex >= QUEUE_SIZE - 1)//说明消费者马上就要跟不上生产者了的节奏了
//                        {
//                            BufferReadIndex = pShareBuffer->current_ready_index;//强制要求消费者直接读取最新写入数据
//                        }
//                        else
//                        {
//                            //说明消费者还可以跟得上生产者，继续读取BufferReadIndex处数据
//                        }
//                    }
//                    else//当前写入index < 当前已读Index 极端情况 current 0  read 9
//                    {
//                        if(BufferReadIndex - pShareBuffer->current_ready_index >= QUEUE_SIZE - 1)//说明消费者马上就要跟不上生产者了的节奏了
//                        {
//                            BufferReadIndex = pShareBuffer->current_ready_index;//强制要求消费者直接读取最新写入数据
//                        }
//                        else
//                        {
//                            //说明消费者还可以跟得上生产者，继续读取BufferReadIndex处数据
//                        }
//                    }

//                    cout << "after current_ready_index: " << pShareBuffer->current_ready_index << " BufferReadIndex: " << BufferReadIndex << endl;

//                    if(pShareBuffer->shareDataArray[BufferReadIndex].ready_flag[access_obj_].load(std::memory_order_acquire) == StateNULL)
//                    {
//                        BufferReadIndex = pShareBuffer->current_ready_index;//当在此处判定状态为StateNULL时，说明生产者刚进行了重启，需要将BufferReadIndex设置为最新的current_ready_index
////                        return true;
//                    }

                    if(pShareBuffer->shareDataArray[BufferReadIndex].ready_flag[access_obj_].load(std::memory_order_acquire) == StateReady)
//                    if(pShareBuffer->shareDataArray[BufferReadIndex].ready_read_flag[access_obj_] == StateReady)
                    {
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    BufferReadIndex = 0;//说明此时生产者还未产生数据，BufferReadIndex设置为0
                    return false;
                }
            }

            void* getBufferAddress()
            {
                return pShareBuffer->shareDataArray[BufferReadIndex].buffer;
            }

//            void setBufferSize(int size)
//            {
//                pShareBuffer->buffer_size = size;
//            }

            int getBufferSize()
            {
                return pShareBuffer->shareDataArray[BufferReadIndex].buffer_size;
            }

            void writeData(const void* buffer,const int size)
            {

                changeBufferIOStateALL(BufferWriteIndex,StateWriting);

                memcpy(pShareBuffer->shareDataArray[BufferWriteIndex].buffer,buffer,size);
                pShareBuffer->shareDataArray[BufferWriteIndex].buffer_size = size;
                pShareBuffer->current_ready_index.store(BufferWriteIndex,std::memory_order_release);

                changeBufferIOStateALL(BufferWriteIndex,StateReady);

                if(BufferWriteIndex >= QUEUE_SIZE - 1)
                {
                    BufferWriteIndex = 0;
                }
                else
                {
                    BufferWriteIndex++;
                }
            }

            void readData(void* buffer)
            {
//                cout << "size: " << pShareBuffer->shareDataArray[BufferReadIndex].buffer_size << endl;
//                cout << "buffer[22] " << (int)pShareBuffer->shareDataArray[BufferReadIndex].buffer[22] << endl;
//                cout << "buffer: " << buffer << endl;
                memcpy(buffer,pShareBuffer->shareDataArray[BufferReadIndex].buffer,pShareBuffer->shareDataArray[BufferReadIndex].buffer_size);
//                size = pShareBuffer->shareDataArray[BufferReadIndex].buffer_size;

//                changeBufferIOState(BufferReadIndex,StateReadDone);//注意只能将当前access_obj_对应的ready_flag设置为StateReadDone
//                pShareBuffer->shareDataArray[BufferReadIndex].ready_read_flag[access_obj_] = StateReadDone;
                pShareBuffer->shareDataArray[BufferReadIndex].ready_flag[access_obj_].store(StateReadDone,std::memory_order_release);

                if(BufferReadIndex >= QUEUE_SIZE - 1)
                {
                    BufferReadIndex = 0;
                }
                else
                {
                    BufferReadIndex++;
                }
            }


            void registCallback(Fun callback)
            {
                pCallback = callback;
//                context_ = context;
                this->listenThread().detach();
            }

        private:

            void changeBufferIOStateALL(int index,ShmAccessFlag state)
            {
                for(int i = 0; i < NUM_BufferObj - 1; i++)
                {
                    pShareBuffer->shareDataArray[index].ready_flag[i].store(state,std::memory_order_release);
//                    pShareBuffer->shareDataArray[index].ready_read_flag[i] = state;
                }
            }

            void checkBufferReadytoRead()
            {
                while(1)
                {
//                    cout << getTime(getTimeStamp()) << endl;

                    if(isReadyToRead())
                    {
//                        cout << "Buffer is ready to Read!!!" << getTime(getTimeStamp()) << endl;
//                        pCallback(pShareBuffer->buffer,pShareBuffer->buffer_size,this);
                        DataPackage dataPackage;
                        dataPackage.address = pShareBuffer->shareDataArray[BufferReadIndex].buffer;
                        dataPackage.size = pShareBuffer->shareDataArray[BufferReadIndex].buffer_size;
//                        pCallback(pShareBuffer->shareDataArray[BufferReadIndex].buffer,pShareBuffer->shareDataArray[BufferReadIndex].buffer_size);
                        pCallback(&dataPackage);
//                        changeBufferIOState(BufferReadIndex,StateReadDone);
                        pShareBuffer->shareDataArray[BufferReadIndex].ready_flag[access_obj_].store(StateReadDone,std::memory_order_release);
//                        pShareBuffer->shareDataArray[BufferReadIndex].ready_read_flag[access_obj_] = StateReadDone;

                        if(BufferReadIndex >= QUEUE_SIZE - 1)
                        {
                            BufferReadIndex = 0;
                        }
                        else
                        {
                            BufferReadIndex++;
                        }
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }

            std::thread listenThread()
            {
                return std::thread(&ShmBuffer::checkBufferReadytoRead,this);
            }

            BufferAccessObj access_obj_;
            int buffer_id_;

            int shmid_buffer;
            void *shm_buffer;

            ShareBuffer *pShareBuffer;

            Fun pCallback;

            void* context_;

            int BufferWriteIndex,BufferReadIndex;
    };



}
#endif //PROJECT_SHM_BUFFER_H
